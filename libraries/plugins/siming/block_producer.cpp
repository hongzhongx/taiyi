#include <plugins/siming/block_producer.hpp>

#include <protocol/base.hpp>
#include <protocol/config.hpp>
#include <protocol/version.hpp>

#include <chain/database_exceptions.hpp>
#include <chain/db_with.hpp>
#include <chain/siming_objects.hpp>

#include <fc/macros.hpp>

namespace taiyi { namespace plugins { namespace siming {

chain::signed_block block_producer::generate_block(fc::time_point_sec when, const chain::account_name_type& siming_owner, const fc::ecc::private_key& block_signing_private_key, uint32_t skip)
{
    chain::signed_block result;
    taiyi::chain::detail::with_skip_flags(_db, skip, [&]() {
        try
        {
            result = _generate_block( when, siming_owner, block_signing_private_key );
        }
        FC_CAPTURE_AND_RETHROW( (siming_owner) )
    });
    return result;
}

chain::signed_block block_producer::_generate_block(fc::time_point_sec when, const chain::account_name_type& siming_owner, const fc::ecc::private_key& block_signing_private_key)
{
    uint32_t skip = _db.get_node_properties().skip_flags;
    uint32_t slot_num = _db.get_slot_at_time( when );
    FC_ASSERT( slot_num > 0 );
    string scheduled_siming = _db.get_scheduled_siming( slot_num );
    FC_ASSERT( scheduled_siming == siming_owner );
    
    const auto& siming_obj = _db.get_siming( siming_owner );
    
    if( !(skip & chain::database::skip_siming_signature) )
        FC_ASSERT( siming_obj.signing_key == block_signing_private_key.get_public_key() );
    
    chain::signed_block pending_block;
    
    pending_block.previous = _db.head_block_id();
    pending_block.timestamp = when;
    pending_block.siming = siming_owner;
    
    adjust_hardfork_version_vote( _db.get_siming( siming_owner ), pending_block );
    
    apply_pending_transactions( siming_owner, when, pending_block );
    
    // We have temporarily broken the invariant that
    // _pending_tx_session is the result of applying _pending_tx, as
    // _pending_tx now consists of the set of postponed transactions.
    // However, the push_block() call below will re-create the
    // _pending_tx_session.
    
    if( !(skip & chain::database::skip_siming_signature) )
        pending_block.sign( block_signing_private_key, fc::ecc::bip_0062);
    
    // TODO:  Move this to _push_block() so session is restored.
    if( !(skip & chain::database::skip_block_size_check) )
    {
        FC_ASSERT( fc::raw::pack_size(pending_block) <= TAIYI_MAX_BLOCK_SIZE );
    }
    
    _db.push_block( pending_block, skip );
    
    return pending_block;
}

void block_producer::adjust_hardfork_version_vote(const chain::siming_object& siming, chain::signed_block& pending_block)
{
    using namespace taiyi::protocol;

    if( siming.running_version != TAIYI_BLOCKCHAIN_VERSION )
        pending_block.extensions.insert( block_header_extensions( TAIYI_BLOCKCHAIN_VERSION ) );

    const auto& hfp = _db.get_hardfork_property_object();
    const auto& hf_versions = _db.get_hardfork_versions();

    if( hfp.current_hardfork_version < TAIYI_BLOCKCHAIN_VERSION // Binary is newer hardfork than has been applied
       && ( siming.hardfork_version_vote != hf_versions.versions[ hfp.last_hardfork + 1 ] || siming.hardfork_time_vote != hf_versions.times[ hfp.last_hardfork + 1 ] ) ) // Siming vote does not match binary configuration
    {
        // Make vote match binary configuration
        pending_block.extensions.insert( block_header_extensions( hardfork_version_vote( hf_versions.versions[ hfp.last_hardfork + 1 ], hf_versions.times[ hfp.last_hardfork + 1 ] ) ) );
    }
    else if( hfp.current_hardfork_version == TAIYI_BLOCKCHAIN_VERSION // Binary does not know of a new hardfork
            && siming.hardfork_version_vote > TAIYI_BLOCKCHAIN_VERSION ) // Voting for hardfork in the future, that we do not know of...
    {
        // Make vote match binary configuration. This is vote to not apply the new hardfork.
        pending_block.extensions.insert( block_header_extensions( hardfork_version_vote( hf_versions.versions[ hfp.last_hardfork ], hf_versions.times[ hfp.last_hardfork ] ) ) );
    }
}

void block_producer::apply_pending_transactions(const chain::account_name_type& siming_owner, fc::time_point_sec when, chain::signed_block& pending_block)
{
    // The 4 is for the max size of the transaction vector length
    size_t total_block_size = fc::raw::pack_size( pending_block ) + 4;
    const auto& gpo = _db.get_dynamic_global_properties();
    uint64_t maximum_block_size = gpo.maximum_block_size; //TAIYI_MAX_BLOCK_SIZE;
    uint64_t maximum_transaction_partition_size = maximum_block_size;

    //
    // The following code throws away existing pending_tx_session and
    // rebuilds it by re-applying pending transactions.
    //
    // This rebuild is necessary because pending transactions' validity
    // and semantics may have changed since they were received, because
    // time-based semantics are evaluated based on the current block
    // time.  These changes can only be reflected in the database when
    // the value of the "when" variable is known, which means we need to
    // re-apply pending transactions in this method.
    //
    _db.pending_transaction_session().reset();
    _db.pending_transaction_session() = _db.start_undo_session();

    /// modify current siming so transaction evaluators can know who included the transaction
    _db.modify(_db.get_dynamic_global_properties(), [&]( chain::dynamic_global_property_object& dgp ) {
        dgp.current_siming = siming_owner;
    });

    uint64_t postponed_tx_count = 0;
    // pop pending state (reset to head block state)
    for( const chain::signed_transaction& tx : _db._pending_tx )
    {
        // Only include transactions that have not expired yet for currently generating block,
        // this should clear problem transactions and allow block production to continue

        if( postponed_tx_count > TAIYI_BLOCK_GENERATION_POSTPONED_TX_LIMIT )
            break;

        if( tx.expiration < when )
            continue;

        uint64_t new_total_size = total_block_size + fc::raw::pack_size( tx );

        // postpone transaction if it would make block too big
        if( new_total_size >= maximum_transaction_partition_size )
        {
            postponed_tx_count++;
            continue;
        }

        try
        {
            _db.set_producing( true ); //这使得在op执行或者响应的时候可以获得“出块验证”的标识

            auto temp_session = _db.start_undo_session();
            _db.apply_transaction( tx, _db.get_node_properties().skip_flags );
            temp_session.squash();

            _db.set_producing( false );

            total_block_size = new_total_size;
            pending_block.transactions.push_back( tx ); //出块节点将通过验证的交易打包进新块
        }
        catch ( const fc::exception& e )
        {
            _db.set_producing( false );
            // Do nothing, transaction will not be re-applied
            //wlog( "Transaction was not processed while generating block due to ${e}", ("e", e) );
            //wlog( "The transaction was ${t}", ("t", tx) );
        }
    }
    if( postponed_tx_count > 0 )
    {
        wlog( "Postponed ${n} transactions due to block size limit", ("n", _db._pending_tx.size() - pending_block.transactions.size()) );
    }

    //由于是为了出块在当前状态上验证了交易，因此要回滚状态到之前的链头部
    _db.pending_transaction_session().reset();

    pending_block.transaction_merkle_root = pending_block.calculate_merkle_root();
}

} } } // taiyi::plugins::siming

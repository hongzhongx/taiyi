#include <chain/taiyi_fwd.hpp>

#include <chain/taiyi_evaluator.hpp>
#include <chain/database.hpp>
#include <chain/custom_operation_interpreter.hpp>
#include <chain/taiyi_objects.hpp>
#include <chain/account_object.hpp>
#include <chain/siming_objects.hpp>
#include <chain/block_summary_object.hpp>

#include <fc/macros.hpp>

FC_TODO( "After we vendor fc, also vendor diff_match_patch and fix these warnings" )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wsign-compare"
#   pragma GCC diagnostic push
#   if !defined( __clang__ )
#       pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#   endif
#   include <diff_match_patch.h>
#   pragma GCC diagnostic pop
#   pragma GCC diagnostic pop
#   include <boost/locale/encoding_utf.hpp>

using boost::locale::conv::utf_to_utf;

std::wstring utf8_to_wstring(const std::string& str)
{
    return utf_to_utf<wchar_t>(str.c_str(), str.c_str() + str.size());
}

std::string wstring_to_utf8(const std::wstring& str)
{
    return utf_to_utf<char>(str.c_str(), str.c_str() + str.size());
}

#include <fc/uint128.hpp>
#include <fc/utf8.hpp>

#include <limits>

namespace taiyi { namespace chain {

    using fc::uint128_t;

    inline void validate_permlink_0_1( const string& permlink )
    {
        FC_ASSERT( permlink.size() > TAIYI_MIN_PERMLINK_LENGTH && permlink.size() < TAIYI_MAX_PERMLINK_LENGTH, "Permlink is not a valid size." );
        
        for( const auto& c : permlink )
        {
            switch( c )
            {
                case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
                case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
                case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': case '0':
                case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                case '-':
                    break;
                default:
                    FC_ASSERT( false, "Invalid permlink character: ${s}", ("s", std::string() + c ) );
            }
        }
    }

    template< bool force_canon >
    void copy_legacy_chain_properties( chain_properties& dest, const legacy_chain_properties& src )
    {
        dest.account_creation_fee = src.account_creation_fee.to_asset< force_canon >();
        dest.maximum_block_size = src.maximum_block_size;
    }

    operation_result siming_update_evaluator::do_apply( const siming_update_operation& o )
    {
        _db.get_account( o.owner ); // verify owner exists
        
        FC_ASSERT( o.props.account_creation_fee.symbol.is_canon() );
        FC_ASSERT( o.props.account_creation_fee.amount <= TAIYI_MAX_ACCOUNT_CREATION_FEE, "account_creation_fee greater than maximum account creation fee" );
        
        FC_ASSERT( o.props.maximum_block_size <= TAIYI_SOFT_MAX_BLOCK_SIZE, "Max block size cannot be more than 2MiB" );
        
        const auto& by_siming_name_idx = _db.get_index< siming_index >().indices().get< by_name >();
        auto wit_itr = by_siming_name_idx.find( o.owner );
        if( wit_itr != by_siming_name_idx.end() )
        {
            _db.modify( *wit_itr, [&]( siming_object& w ) {
                w.url = o.url;
                w.signing_key       = o.block_signing_key;
                copy_legacy_chain_properties< false >( w.props, o.props );
            });
        }
        else
        {
            _db.create< siming_object >( [&]( siming_object& w ) {
                w.owner             = o.owner;
                w.url = o.url;
                w.signing_key       = o.block_signing_key;
                w.created           = _db.head_block_time();
                copy_legacy_chain_properties< false >( w.props, o.props );
            });
        }
        
        return void_result();
    }
    
    struct siming_properties_change_flags
    {
        uint32_t account_creation_changed       : 1;
        uint32_t max_block_changed              : 1;
        uint32_t key_changed                    : 1;
        uint32_t url_changed                    : 1;
    };

    operation_result siming_set_properties_evaluator::do_apply( const siming_set_properties_operation& o )
    {
        const auto& siming = _db.get< siming_object, by_name >( o.owner ); // verifies siming exists;
        
        // Capture old properties. This allows only updating the object once.
        chain_properties  props;
        public_key_type   signing_key;
        string            url;
        
        siming_properties_change_flags flags;
        
        auto itr = o.props.find( "key" );
        
        // This existence of 'key' is checked in siming_set_properties_operation::validate
        fc::raw::unpack_from_vector( itr->second, signing_key );
        FC_ASSERT( signing_key == siming.signing_key, "'key' does not match siming signing key.", ("key", signing_key)("signing_key", siming.signing_key) );
        
        itr = o.props.find( "account_creation_fee" );
        flags.account_creation_changed = itr != o.props.end();
        if( flags.account_creation_changed )
        {
            fc::raw::unpack_from_vector( itr->second, props.account_creation_fee );
            FC_ASSERT( props.account_creation_fee.amount <= TAIYI_MAX_ACCOUNT_CREATION_FEE, "account_creation_fee greater than maximum account creation fee" );
        }
        
        itr = o.props.find( "maximum_block_size" );
        flags.max_block_changed = itr != o.props.end();
        if( flags.max_block_changed )
        {
            fc::raw::unpack_from_vector( itr->second, props.maximum_block_size );
        }
                
        itr = o.props.find( "new_signing_key" );
        flags.key_changed = itr != o.props.end();
        if( flags.key_changed )
        {
            fc::raw::unpack_from_vector( itr->second, signing_key );
        }
        
        itr = o.props.find( "url" );
        flags.url_changed = itr != o.props.end();
        if( flags.url_changed )
        {
            fc::raw::unpack_from_vector< std::string >( itr->second, url );
        }
        
        _db.modify( siming, [&]( siming_object& w ) {
            if( flags.account_creation_changed )
                w.props.account_creation_fee = props.account_creation_fee;
            
            if( flags.max_block_changed )
                w.props.maximum_block_size = props.maximum_block_size;
                        
            if( flags.key_changed )
                w.signing_key = signing_key;
            
            if( flags.url_changed )
                w.url = url;
        });
        
        return void_result();
    }

    void verify_authority_accounts_exist(const database& db, const authority& auth, const account_name_type& auth_account, authority::classification auth_class)
    {
        for( const std::pair< account_name_type, weight_type >& aw : auth.account_auths )
        {
            const account_object* a = db.find_account( aw.first );
            FC_ASSERT( a != nullptr, "New ${ac} authority on account ${aa} references non-existing account ${aref}", ("aref", aw.first)("ac", auth_class)("aa", auth_account) );
        }
    }

    void initialize_account_object( account_object& acc, const account_name_type& name, const public_key_type& key, const dynamic_global_property_object& props, const account_name_type& recovery_account, uint32_t hardfork )
    {
        acc.name = name;
        acc.memo_key = key;
        acc.created = props.time;
        
        if( recovery_account != TAIYI_TEMP_ACCOUNT )
            acc.recovery_account = recovery_account;
    }

    operation_result account_create_evaluator::do_apply( const account_create_operation& o )
    {
        const auto& creator = _db.get_account( o.creator );
        const auto& props = _db.get_dynamic_global_properties();
        const siming_schedule_object& wso = _db.get_siming_schedule_object();
        
        FC_ASSERT( o.fee <= asset( TAIYI_MAX_ACCOUNT_CREATION_FEE, YANG_SYMBOL ), "Account creation fee cannot be too large" );
        FC_ASSERT( o.fee == wso.median_props.account_creation_fee, "Must pay the exact account creation fee. paid: ${p} fee: ${f}", ("p", o.fee) ("f", wso.median_props.account_creation_fee) );
        
        validate_auth_size( o.owner );
        validate_auth_size( o.active );
        validate_auth_size( o.posting );
        
        verify_authority_accounts_exist( _db, o.owner, o.new_account_name, authority::owner );
        verify_authority_accounts_exist( _db, o.active, o.new_account_name, authority::active );
        verify_authority_accounts_exist( _db, o.posting, o.new_account_name, authority::posting );
        
        _db.adjust_balance( creator, -o.fee );
        
        const auto& new_account = _db.create< account_object >( [&]( account_object& acc ) {
            initialize_account_object( acc, o.new_account_name, o.memo_key, props, o.creator, _db.get_hardfork() );
        });
        
#ifndef IS_LOW_MEM
        _db.create< account_metadata_object >( [&]( account_metadata_object& meta ) {
            meta.account = new_account.id;
            meta.json_metadata = o.json_metadata;
        });
#endif
        
        _db.create< account_authority_object >( [&]( account_authority_object& auth ) {
            auth.account = o.new_account_name;
            auth.owner = o.owner;
            auth.active = o.active;
            auth.posting = o.posting;
            auth.last_owner_update = fc::time_point_sec::min();
        });
        
        if( o.fee.amount > 0 )
            _db.create_qi( new_account, o.fee );
        
        return void_result();
    }

    operation_result account_update_evaluator::do_apply( const account_update_operation& o )
    {
        FC_ASSERT( o.account != TAIYI_TEMP_ACCOUNT, "Cannot update temp account." );
        
        if( o.posting )
            o.posting->validate();
        
        const auto& account = _db.get_account( o.account );
        const auto& account_auth = _db.get< account_authority_object, by_account >( o.account );
        
        if( o.owner )
            validate_auth_size( *o.owner );
        if( o.active )
            validate_auth_size( *o.active );
        if( o.posting )
            validate_auth_size( *o.posting );
        
        if( o.owner )
        {
#ifndef IS_TEST_NET
            FC_ASSERT( _db.head_block_time() - account_auth.last_owner_update > TAIYI_OWNER_UPDATE_LIMIT, "Owner authority can only be updated once an hour." );
#endif
            
            verify_authority_accounts_exist( _db, *o.owner, o.account, authority::owner );
            _db.update_owner_authority( account, *o.owner );
        }
        
        if( o.active )
            verify_authority_accounts_exist( _db, *o.active, o.account, authority::active );
        
        if( o.posting)
            verify_authority_accounts_exist( _db, *o.posting, o.account, authority::posting );
        
        _db.modify( account, [&]( account_object& acc ) {
            if( o.memo_key != public_key_type() )
                acc.memo_key = o.memo_key;
            
            acc.last_account_update = _db.head_block_time();
        });
        
#ifndef IS_LOW_MEM
        if( o.json_metadata.size() > 0 )
        {
            _db.modify( _db.get< account_metadata_object, by_account >( account.id ), [&]( account_metadata_object& meta ) {
                meta.json_metadata = o.json_metadata;
            });
        }
#endif
        
        if( o.active || o.posting )
        {
            _db.modify( account_auth, [&]( account_authority_object& auth) {
                if( o.active )  auth.active  = *o.active;
                if( o.posting ) auth.posting = *o.posting;
            });
        }
        
        return void_result();
    }

    operation_result transfer_evaluator::do_apply( const transfer_operation& o )
    {
        FC_ASSERT( o.amount.symbol == YANG_SYMBOL || o.to != TAIYI_TREASURY_ACCOUNT, "Can only transfer YANG to ${s}", ("s", TAIYI_TREASURY_ACCOUNT) );
        
        _db.adjust_balance( o.from, -o.amount );
        _db.adjust_balance( o.to, o.amount );
        
        return void_result();
    }

    operation_result transfer_to_qi_evaluator::do_apply( const transfer_to_qi_operation& o )
    {
        const auto& from_account = _db.get_account(o.from);
        const auto& to_account = o.to.size() ? _db.get_account(o.to) : from_account;
        
        FC_ASSERT( o.to != TAIYI_TREASURY_ACCOUNT, "Can only transfer YANG to ${s}", ("s", TAIYI_TREASURY_ACCOUNT) );
        
        _db.adjust_balance( from_account, -o.amount );
        _db.create_qi( to_account, o.amount );
        
        return void_result();
    }

    operation_result withdraw_qi_evaluator::do_apply( const withdraw_qi_operation& o )
    {
        FC_ASSERT( o.qi_shares.amount >= 0, "Cannot withdraw negative Qi. account: ${account}, qi:${qi}", ("account", o.account)("qi", o.qi_shares) );
        
        const auto& account = _db.get_account( o.account );
        FC_ASSERT( account.qi_shares >= asset( 0, QI_SYMBOL ), "Account does not have sufficient Taiyi Qi for withdraw." );
        FC_ASSERT( account.qi_shares - account.delegated_qi_shares >= o.qi_shares, "Account does not have sufficient Taiyi Qi for withdraw." );
        
        if( o.qi_shares.amount == 0 )
        {
            FC_ASSERT( account.qi_withdraw_rate.amount != 0, "This operation would not change the qi withdraw rate." );
            _db.modify( account, [&]( account_object& a ) {
                a.qi_withdraw_rate = asset( 0, QI_SYMBOL );
                a.next_qi_withdrawal_time = time_point_sec::maximum();
                a.to_withdraw = 0;
                a.withdrawn = 0;
            });
        }
        else
        {
            int qi_withdraw_intervals = TAIYI_QI_WITHDRAW_INTERVALS;
            
            _db.modify( account, [&]( account_object& a ) {
                auto new_qi_withdraw_rate = asset( o.qi_shares.amount / qi_withdraw_intervals, QI_SYMBOL );
                
                if( new_qi_withdraw_rate.amount == 0 )
                    new_qi_withdraw_rate.amount = 1;
                
                if( new_qi_withdraw_rate.amount * qi_withdraw_intervals < o.qi_shares.amount )
                    new_qi_withdraw_rate.amount += 1;
                
                FC_ASSERT( account.qi_withdraw_rate  != new_qi_withdraw_rate, "This operation would not change the qi withdraw rate." );
                
                a.qi_withdraw_rate = new_qi_withdraw_rate;
                a.next_qi_withdrawal_time = _db.head_block_time() + fc::seconds(TAIYI_QI_WITHDRAW_INTERVAL_SECONDS);
                a.to_withdraw = o.qi_shares.amount;
                a.withdrawn = 0;
            });
        }
        
        return void_result();
    }

    operation_result set_withdraw_qi_route_evaluator::do_apply( const set_withdraw_qi_route_operation& o )
    { try {
        const auto& from_account = _db.get_account( o.from_account );
        const auto& to_account = _db.get_account( o.to_account );
        const auto& wd_idx = _db.get_index< withdraw_qi_route_index >().indices().get< by_withdraw_route >();
        auto itr = wd_idx.find( boost::make_tuple( from_account.name, to_account.name ) );
        
        FC_ASSERT( o.to_account != TAIYI_TREASURY_ACCOUNT, "Cannot withdraw qi to ${s}", ("s", TAIYI_TREASURY_ACCOUNT) );
        
        if( itr == wd_idx.end() )
        {
            FC_ASSERT( o.percent != 0, "Cannot create a 0% destination." );
            FC_ASSERT( from_account.withdraw_routes < TAIYI_MAX_WITHDRAW_ROUTES, "Account already has the maximum number of routes." );
            
            _db.create< withdraw_qi_route_object >( [&]( withdraw_qi_route_object& wvdo ) {
                wvdo.from_account = from_account.name;
                wvdo.to_account = to_account.name;
                wvdo.percent = o.percent;
                wvdo.auto_vest = o.auto_vest;
            });
            
            _db.modify( from_account, [&]( account_object& a ) {
                a.withdraw_routes++;
            });
        }
        else if( o.percent == 0 )
        {
            _db.remove( *itr );
            
            _db.modify( from_account, [&]( account_object& a ) {
                a.withdraw_routes--;
            });
        }
        else
        {
            _db.modify( *itr, [&]( withdraw_qi_route_object& wvdo ) {
                wvdo.from_account = from_account.name;
                wvdo.to_account = to_account.name;
                wvdo.percent = o.percent;
                wvdo.auto_vest = o.auto_vest;
            });
        }
        
        itr = wd_idx.upper_bound( boost::make_tuple( from_account.name, account_name_type() ) );
        uint16_t total_percent = 0;
        
        while( itr != wd_idx.end() && itr->from_account == from_account.name )
        {
            total_percent += itr->percent;
            ++itr;
        }
        
        FC_ASSERT( total_percent <= TAIYI_100_PERCENT, "More than 100% of qi withdrawals allocated to destinations." );
        
        return void_result();
    } FC_CAPTURE_AND_RETHROW() }

    operation_result account_siming_proxy_evaluator::do_apply( const account_siming_proxy_operation& o )
    {
        const auto& account = _db.get_account( o.account );
        FC_ASSERT( account.proxy != o.proxy, "Proxy must change." );
        FC_ASSERT( account.can_adore, "Account has declined the ability to adore and cannot proxy adores." );
        
        /// remove all current adores
        std::array<share_type, TAIYI_MAX_PROXY_RECURSION_DEPTH+1> delta;
        delta[0] = -account.qi_shares.amount;
        for( int i = 0; i < TAIYI_MAX_PROXY_RECURSION_DEPTH; ++i )
            delta[i+1] = -account.proxied_vsf_adores[i];
        _db.adjust_proxied_siming_adores( account, delta );
        
        if( o.proxy.size() ) {
            const auto& new_proxy = _db.get_account( o.proxy );
            flat_set<account_id_type> proxy_chain( { account.id, new_proxy.id } );
            proxy_chain.reserve( TAIYI_MAX_PROXY_RECURSION_DEPTH + 1 );
            
            /// check for proxy loops and fail to update the proxy if it would create a loop
            auto cprox = &new_proxy;
            while( cprox->proxy.size() != 0 ) {
                const auto next_proxy = _db.get_account( cprox->proxy );
                FC_ASSERT( proxy_chain.insert( next_proxy.id ).second, "This proxy would create a proxy loop." );
                cprox = &next_proxy;
                FC_ASSERT( proxy_chain.size() <= TAIYI_MAX_PROXY_RECURSION_DEPTH, "Proxy chain is too long." );
            }
            
            /// clear all individual adore records
            _db.clear_siming_adores( account );
            
            _db.modify( account, [&]( account_object& a ) {
                a.proxy = o.proxy;
            });
            
            /// add all new adores
            for( int i = 0; i <= TAIYI_MAX_PROXY_RECURSION_DEPTH; ++i )
                delta[i] = -delta[i];
            _db.adjust_proxied_siming_adores( account, delta );
        }
        else { /// we are clearing the proxy which means we simply update the account
            _db.modify( account, [&]( account_object& a ) {
                a.proxy = o.proxy;
            });
        }
        
        return void_result();
    }

    operation_result account_siming_adore_evaluator::do_apply( const account_siming_adore_operation& o )
    {
        const auto& adorer = _db.get_account( o.account );
        FC_ASSERT( adorer.proxy.size() == 0, "A proxy is currently set, please clear the proxy before adoring for a siming." );
        
        if( o.approve )
            FC_ASSERT( adorer.can_adore, "Account has declined its adoring rights." );
        
        const auto& siming = _db.get_siming( o.siming );
        
        const auto& by_account_siming_idx = _db.get_index< siming_adore_index >().indices().get< by_account_siming >();
        auto itr = by_account_siming_idx.find( boost::make_tuple( adorer.name, siming.owner ) );
        
        if( itr == by_account_siming_idx.end() ) {
            FC_ASSERT( o.approve, "Vote doesn't exist, user must indicate a desire to approve siming." );
            FC_ASSERT( adorer.simings_adored_for < TAIYI_MAX_ACCOUNT_SIMING_ADORES, "Account has adored for too many simings." );
            
            _db.create<siming_adore_object>( [&]( siming_adore_object& v ) {
                v.siming = siming.owner;
                v.account = adorer.name;
            });
            
            _db.adjust_siming_adore( siming, adorer.siming_adore_weight() );
            
            _db.modify( adorer, [&]( account_object& a ) {
                a.simings_adored_for++;
            });
        }
        else {
            FC_ASSERT( !o.approve, "Vote currently exists, user must indicate a desire to reject siming." );
            
            _db.adjust_siming_adore( siming, -adorer.siming_adore_weight() );
            _db.modify( adorer, [&]( account_object& a ) {
                a.simings_adored_for--;
            });
            _db.remove( *itr );
        }
        
        return void_result();
    }

    operation_result custom_evaluator::do_apply( const custom_operation& o )
    {
        database& d = db();
        
        if( d.is_producing() )
            FC_ASSERT( o.data.size() <= TAIYI_CUSTOM_OP_DATA_MAX_LENGTH, "Operation data must be less than ${bytes} bytes.", ("bytes", TAIYI_CUSTOM_OP_DATA_MAX_LENGTH) );
        
        FC_ASSERT( o.required_auths.size() <= TAIYI_MAX_AUTHORITY_MEMBERSHIP, "Authority membership exceeded. Max: ${max} Current: ${n}", ("max", TAIYI_MAX_AUTHORITY_MEMBERSHIP)("n", o.required_auths.size()) );
        
        return void_result();
    }

    operation_result custom_json_evaluator::do_apply( const custom_json_operation& o )
    {
        database& d = db();
        
        if( d.is_producing() )
            FC_ASSERT( o.json.length() <= TAIYI_CUSTOM_OP_DATA_MAX_LENGTH, "Operation JSON must be less than ${bytes} bytes.", ("bytes", TAIYI_CUSTOM_OP_DATA_MAX_LENGTH) );
        
        size_t num_auths = o.required_auths.size() + o.required_posting_auths.size();
        FC_ASSERT( num_auths <= TAIYI_MAX_AUTHORITY_MEMBERSHIP, "Authority membership exceeded. Max: ${max} Current: ${n}", ("max", TAIYI_MAX_AUTHORITY_MEMBERSHIP)("n", num_auths) );
        
        std::shared_ptr< custom_operation_interpreter > eval = d.get_custom_json_evaluator( o.id );
        if( !eval )
            return void_result();
        
        try
        {
            eval->apply( o );
        }
        catch( const fc::exception& e )
        {
            if( d.is_producing() )
                throw e;
        }
        catch(...)
        {
            elog( "Unexpected exception applying custom json evaluator." );
        }
        
        return void_result();
    }

    operation_result request_account_recovery_evaluator::do_apply( const request_account_recovery_operation& o )
    {
        const auto& account_to_recover = _db.get_account( o.account_to_recover );
        
        if ( account_to_recover.recovery_account.length() )   // Make sure recovery matches expected recovery account
        {
            FC_ASSERT( account_to_recover.recovery_account == o.recovery_account, "Cannot recover an account that does not have you as there recovery partner." );
            if( o.recovery_account == TAIYI_TEMP_ACCOUNT )
                wlog( "Recovery by temp account" );
        }
        else                                                  // Empty string recovery account defaults to top siming
            FC_ASSERT( _db.get_index< siming_index >().indices().get< by_adore_name >().begin()->owner == o.recovery_account, "Top siming must recover an account with no recovery partner." );
        
        const auto& recovery_request_idx = _db.get_index< account_recovery_request_index >().indices().get< by_account >();
        auto request = recovery_request_idx.find( o.account_to_recover );
        
        if( request == recovery_request_idx.end() ) // New Request
        {
            FC_ASSERT( !o.new_owner_authority.is_impossible(), "Cannot recover using an impossible authority." );
            FC_ASSERT( o.new_owner_authority.weight_threshold, "Cannot recover using an open authority." );
            
            validate_auth_size( o.new_owner_authority );
            
            // Check accounts in the new authority exist
            for( auto& a : o.new_owner_authority.account_auths )
            {
                _db.get_account( a.first );
            }
            
            _db.create< account_recovery_request_object >( [&]( account_recovery_request_object& req ) {
                req.account_to_recover = o.account_to_recover;
                req.new_owner_authority = o.new_owner_authority;
                req.expires = _db.head_block_time() + TAIYI_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD;
            });
        }
        else if( o.new_owner_authority.weight_threshold == 0 ) // Cancel Request if authority is open
        {
            _db.remove( *request );
        }
        else // Change Request
        {
            FC_ASSERT( !o.new_owner_authority.is_impossible(), "Cannot recover using an impossible authority." );
            
            // Check accounts in the new authority exist
            for( auto& a : o.new_owner_authority.account_auths )
            {
                _db.get_account( a.first );
            }
            
            _db.modify( *request, [&]( account_recovery_request_object& req ) {
                req.new_owner_authority = o.new_owner_authority;
                req.expires = _db.head_block_time() + TAIYI_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD;
            });
        }
        
        return void_result();
    }
    
    operation_result recover_account_evaluator::do_apply( const recover_account_operation& o )
    {
        const auto& account = _db.get_account( o.account_to_recover );
        
        FC_ASSERT( _db.head_block_time() - account.last_account_recovery > TAIYI_OWNER_UPDATE_LIMIT, "Owner authority can only be updated once an hour." );
        
        const auto& recovery_request_idx = _db.get_index< account_recovery_request_index >().indices().get< by_account >();
        auto request = recovery_request_idx.find( o.account_to_recover );
        
        FC_ASSERT( request != recovery_request_idx.end(), "There are no active recovery requests for this account." );
        FC_ASSERT( request->new_owner_authority == o.new_owner_authority, "New owner authority does not match recovery request." );
        
        const auto& recent_auth_idx = _db.get_index< owner_authority_history_index >().indices().get< by_account >();
        auto hist = recent_auth_idx.lower_bound( o.account_to_recover );
        bool found = false;
        
        while( hist != recent_auth_idx.end() && hist->account == o.account_to_recover && !found )
        {
            found = hist->previous_owner_authority == o.recent_owner_authority;
            if( found ) break;
            ++hist;
        }
        
        FC_ASSERT( found, "Recent authority not found in authority history." );
        
        _db.remove( *request ); // Remove first, update_owner_authority may invalidate iterator
        _db.update_owner_authority( account, o.new_owner_authority );
        _db.modify( account, [&]( account_object& a ) {
            a.last_account_recovery = _db.head_block_time();
        });
        
        return void_result();
    }

    operation_result change_recovery_account_evaluator::do_apply( const change_recovery_account_operation& o )
    {
        _db.get_account( o.new_recovery_account ); // Simply validate account exists
        const auto& account_to_recover = _db.get_account( o.account_to_recover );
        
        const auto& change_recovery_idx = _db.get_index< change_recovery_account_request_index >().indices().get< by_account >();
        auto request = change_recovery_idx.find( o.account_to_recover );
        
        if( request == change_recovery_idx.end() ) // New request
        {
            _db.create< change_recovery_account_request_object >( [&]( change_recovery_account_request_object& req ) {
                req.account_to_recover = o.account_to_recover;
                req.recovery_account = o.new_recovery_account;
                req.effective_on = _db.head_block_time() + TAIYI_OWNER_AUTH_RECOVERY_PERIOD;
            });
        }
        else if( account_to_recover.recovery_account != o.new_recovery_account ) // Change existing request
        {
            _db.modify( *request, [&]( change_recovery_account_request_object& req ) {
                req.recovery_account = o.new_recovery_account;
                req.effective_on = _db.head_block_time() + TAIYI_OWNER_AUTH_RECOVERY_PERIOD;
            });
        }
        else // Request exists and changing back to current recovery account
        {
            _db.remove( *request );
        }
        
        return void_result();
    }
    
    operation_result decline_adoring_rights_evaluator::do_apply( const decline_adoring_rights_operation& o )
    {
        const auto& account = _db.get_account( o.account );
        const auto& request_idx = _db.get_index< decline_adoring_rights_request_index >().indices().get< by_account >();
        auto itr = request_idx.find( account.name );
        
        if( o.decline )
        {
            FC_ASSERT( itr == request_idx.end(), "Cannot create new request because one already exists." );
            
            _db.create< decline_adoring_rights_request_object >( [&]( decline_adoring_rights_request_object& req ) {
                req.account = account.name;
                req.effective_date = _db.head_block_time() + TAIYI_OWNER_AUTH_RECOVERY_PERIOD;
            });
        }
        else
        {
            FC_ASSERT( itr != request_idx.end(), "Cannot cancel the request because it does not exist." );
            _db.remove( *itr );
        }
        
        return void_result();
    }

    operation_result claim_reward_balance_evaluator::do_apply( const claim_reward_balance_operation& op )
    {
        const auto& acnt = _db.get_account( op.account );
        
        FC_ASSERT( op.reward_yang <= acnt.reward_yang_balance, "Cannot claim that much YANG. Claim: ${c} Actual: ${a}", ("c", op.reward_yang)("a", acnt.reward_yang_balance) );
        FC_ASSERT( op.reward_qi <= acnt.reward_qi_balance, "Cannot claim that much QI. Claim: ${c} Actual: ${a}", ("c", op.reward_qi)("a", acnt.reward_qi_balance) );
        
        asset reward_qi_yang_to_move = asset( 0, YANG_SYMBOL );
        if( op.reward_qi == acnt.reward_qi_balance )
            reward_qi_yang_to_move = acnt.reward_qi_yang;
        else
            reward_qi_yang_to_move = asset(((uint128_t(op.reward_qi.amount.value) * uint128_t(acnt.reward_qi_yang.amount.value)) / uint128_t(acnt.reward_qi_balance.amount.value)).to_uint64(), YANG_SYMBOL);
        
        _db.adjust_reward_balance( acnt, -op.reward_yang );
        _db.adjust_balance( acnt, op.reward_yang );
        
        _db.modify( acnt, [&]( account_object& a ) {            
            a.qi_shares += op.reward_qi;
            a.reward_qi_balance -= op.reward_qi;
            a.reward_qi_yang -= reward_qi_yang_to_move;
        });
        
        _db.modify( _db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo ) {
            gpo.total_qi_shares += op.reward_qi;
            gpo.total_qi_fund_yang += reward_qi_yang_to_move;
            
            gpo.pending_rewarded_qi_shares -= op.reward_qi;
            gpo.pending_rewarded_qi_yang -= reward_qi_yang_to_move;
        });
        
        _db.adjust_proxied_siming_adores( acnt, op.reward_qi.amount );
        
        return void_result();
    }
    
    template< typename T >
    int64_t get_effective_qi_shares( const T& account )
    {
        int64_t effective_qi_shares = account.qi_shares.amount.value              // base qi shares
            + account.received_qi_shares.amount.value    // incoming delegations
            - account.delegated_qi_shares.amount.value;  // outgoing delegations
        
        // If there is a power down occuring, also reduce effective qi shares by this week's power down amount
        if( account.next_qi_withdrawal_time != fc::time_point_sec::maximum() )
        {
            effective_qi_shares -= std::min(account.qi_withdraw_rate.amount.value,                  // Weekly amount
                                            account.to_withdraw.value - account.withdrawn.value);   // Or remainder
        }
        
        return effective_qi_shares;
    }

    operation_result delegate_qi_shares_evaluator::do_apply( const delegate_qi_shares_operation& op )
    {
        const auto& delegator = _db.get_account( op.delegator );
        const auto& delegatee = _db.get_account( op.delegatee );
        auto delegation = _db.find< qi_delegation_object, by_delegation >( boost::make_tuple( op.delegator, op.delegatee ) );
        
        const auto& gpo = _db.get_dynamic_global_properties();
        
        asset available_shares = asset( 0, QI_SYMBOL );
        
        auto max_mana = get_effective_qi_shares( delegator );
                
        // Assume delegated QI are used first when consuming mana. You cannot delegate received qi shares
        available_shares.amount = max_mana - delegator.received_qi_shares.amount;
        
        if( delegator.next_qi_withdrawal_time < fc::time_point_sec::maximum() && (delegator.to_withdraw - delegator.withdrawn) > delegator.qi_withdraw_rate.amount )
        {
            /*
             current adoring mana does not include the current week's power down:
             
             std::min(
                account.qi_withdraw_rate.amount.value,           // Weekly amount
                account.to_withdraw.value - account.withdrawn.value   // Or remainder
             );
             
             But an account cannot delegate **any** QI that they are powering down.
             The remaining withdrawal needs to be added in but then the current week is double counted.
             */
            
            auto weekly_withdraw = asset( std::min(delegator.qi_withdraw_rate.amount.value,         // Weekly amount
                                         delegator.to_withdraw.value - delegator.withdrawn.value),  // Or remainder
                                         QI_SYMBOL );
            
            available_shares += weekly_withdraw - asset( delegator.to_withdraw - delegator.withdrawn, QI_SYMBOL );
        }
        
        const auto& wso = _db.get_siming_schedule_object();
        
        auto min_delegation = asset( wso.median_props.account_creation_fee.amount / 3, YANG_SYMBOL ) * gpo.get_qi_share_price();
        auto min_update = asset( wso.median_props.account_creation_fee.amount / 30, YANG_SYMBOL ) * gpo.get_qi_share_price();
        
        // If delegation doesn't exist, create it
        if( delegation == nullptr )
        {
            FC_ASSERT( available_shares >= op.qi_shares, "Account ${acc} does not have enough mana to delegate. required: ${r} available: ${a}", ("acc", op.delegator)("r", op.qi_shares)("a", available_shares) );
            FC_ASSERT( op.qi_shares >= min_delegation, "Account must delegate a minimum of ${v}", ("v", min_delegation) );
            
            _db.create< qi_delegation_object >( [&]( qi_delegation_object& obj ) {
                obj.delegator = op.delegator;
                obj.delegatee = op.delegatee;
                obj.qi_shares = op.qi_shares;
                obj.min_delegation_time = _db.head_block_time();
            });
            
            _db.modify( delegator, [&]( account_object& a ) {
                a.delegated_qi_shares += op.qi_shares;
            });
            
            _db.modify( delegatee, [&]( account_object& a ) {
                a.received_qi_shares += op.qi_shares;
            });
        }
        // Else if the delegation is increasing
        else if( op.qi_shares >= delegation->qi_shares )
        {
            auto delta = op.qi_shares - delegation->qi_shares;
            
            FC_ASSERT( delta >= min_update, "Taiyi Qi increase is not enough of a difference. min_update: ${min}", ("min", min_update) );
            FC_ASSERT( available_shares >= delta, "Account ${acc} does not have enough mana to delegate. required: ${r} available: ${a}", ("acc", op.delegator)("r", delta)("a", available_shares) );
            
            _db.modify( delegator, [&]( account_object& a ) {
                a.delegated_qi_shares += delta;
            });
            
            _db.modify( delegatee, [&]( account_object& a ) {
                a.received_qi_shares += delta;
            });
            
            _db.modify( *delegation, [&]( qi_delegation_object& obj ) {
                obj.qi_shares = op.qi_shares;
            });
        }
        // Else the delegation is decreasing
        else /* delegation->qi_shares > op.qi_shares */
        {
            auto delta = delegation->qi_shares - op.qi_shares;
            
            if( op.qi_shares.amount > 0 )
            {
                FC_ASSERT( delta >= min_update, "Taiyi Qi decrease is not enough of a difference. min_update: ${min}", ("min", min_update) );
                FC_ASSERT( op.qi_shares >= min_delegation, "Delegation must be removed or leave minimum delegation amount of ${v}", ("v", min_delegation) );
            }
            else
            {
                FC_ASSERT( delegation->qi_shares.amount > 0, "Delegation would set qi_shares to zero, but it is already zero");
            }
            
            _db.create< qi_delegation_expiration_object >( [&]( qi_delegation_expiration_object& obj ) {
                obj.delegator = op.delegator;
                obj.qi_shares = delta;
                obj.expiration = std::max( _db.head_block_time() + gpo.delegation_return_period, delegation->min_delegation_time );
            });
            
            _db.modify( delegatee, [&]( account_object& a ) {
                a.received_qi_shares -= delta;
            });
            
            if( op.qi_shares.amount > 0 )
            {
                _db.modify( *delegation, [&]( qi_delegation_object& obj ) {
                    obj.qi_shares = op.qi_shares;
                });
            }
            else
            {
                _db.remove( *delegation );
            }
        }
        
        return void_result();
    }

} } // taiyi::chain


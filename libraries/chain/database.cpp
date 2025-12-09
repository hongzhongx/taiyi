#include <chain/taiyi_fwd.hpp>

#include <protocol/taiyi_operations.hpp>

#include <chain/block_summary_object.hpp>
#include <chain/custom_operation_interpreter.hpp>
#include <chain/database.hpp>
#include <chain/database_exceptions.hpp>
#include <chain/db_with.hpp>
#include <chain/evaluator_registry.hpp>
#include <chain/global_property_object.hpp>
#include <chain/asset_objects/asset_objects.hpp>
#include <chain/taiyi_evaluator.hpp>
#include <chain/taiyi_objects.hpp>
#include <chain/tiandao_property_object.hpp>
#include <chain/account_object.hpp>
#include <chain/transaction_object.hpp>
#include <chain/contract_objects.hpp>
#include <chain/nfa_objects.hpp>
#include <chain/siming_objects.hpp>
#include <chain/siming_schedule.hpp>
#include <chain/proposal_processor.hpp>

#include <chain/util/uint256.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/uint128.hpp>

#include <fc/container/deque.hpp>

#include <fc/io/fstream.hpp>

#include <boost/scope_exit.hpp>

#include <rocksdb/perf_context.h>

#include <iostream>

#include <cstdint>
#include <deque>
#include <fstream>
#include <functional>

namespace taiyi { namespace chain {

    class database_impl
    {
    public:
        database_impl( database& self );
        
        database&                                       _self;
        evaluator_registry< operation >                 _evaluator_registry;
    };

    database_impl::database_impl( database& self )
        : _self(self), _evaluator_registry(self)
    {}
    
    database::database()
        : _my( new database_impl(*this) )
    {}

    database::~database()
    {
        clear_pending();
    }

    void database::open( const open_args& args )
    { try {
        chainbase::database::open( args.state_storage_dir, args.chainbase_flags, args.database_cfg );
        
        initialize_indexes();
        initialize_evaluators();
        
        if( !find< dynamic_global_property_object >() ) {
            with_write_lock( [&]() {
                init_genesis( args.initial_supply, args.initial_qi_supply );
            });
        }
        
        _xinsu_mark_nfa_symbol_id = get<nfa_symbol_object, by_symbol>(TAIYI_NFA_SYMBOL_NAME_XINSU_MARK).id;
        
        _benchmark_dumper.set_enabled( args.benchmark_is_enabled );
        
        assert( args.data_dir.is_absolute() );
        chainbase::bfs::create_directories( args.data_dir );
        _block_log.open( args.data_dir / "block_log" );
        
        auto log_head = _block_log.head();
        
        // Rewind all undo state. This should return us to the state at the last irreversible block.
        with_write_lock( [&]() {
            if( args.chainbase_flags & chainbase::skip_env_check )
            {
                set_revision( head_block_num() );
            }
            else
            {
                FC_ASSERT( revision() == head_block_num(), "Chainbase revision does not match head block num.",
                          ("rev", revision())("head_block", head_block_num()) );
                if (args.do_validate_invariants)
                    validate_invariants();
            }
        });
        
        if( head_block_num() )
        {
            auto head_block = _block_log.read_block_by_num( head_block_num() );
            // This assertion should be caught and a reindex should occur
            FC_ASSERT( head_block.valid() && head_block->id() == head_block_id(), "Chain state does not match block log. Please reindex blockchain." );
            
            _fork_db.start_block( *head_block );
        }
        
        with_read_lock( [&]() {
            init_hardforks(); // Writes to local state, but reads from db
        });
        
        if (args.benchmark.first)
        {
            args.benchmark.second(0, get_abstract_index_cntr());
            auto last_block_num = _block_log.head()->block_num();
            args.benchmark.second(last_block_num, get_abstract_index_cntr());
        }
        
        _proposal_remove_threshold = args.proposal_remove_threshold;
                
    } FC_CAPTURE_LOG_AND_RETHROW( (args.data_dir)(args.state_storage_dir) ) }

    void reindex_set_index_helper( database& db, mira::index_type type, const boost::filesystem::path& p, const boost::any& cfg, std::vector< std::string > indices )
    {
        index_delegate_map delegates;
        
        if ( indices.size() > 0 )
        {
            for ( auto& index_name : indices )
            {
                if ( db.has_index_delegate( index_name ) )
                    delegates[ index_name ] = db.get_index_delegate( index_name );
                else
                    wlog( "Encountered an unknown index name '${name}'.", ("name", index_name) );
            }
        }
        else
        {
            delegates = db.index_delegates();
        }
        
        std::string type_str = type == mira::index_type::mira ? "mira" : "bmic";
        for ( auto const& delegate : delegates )
        {
            ilog( "Converting index '${name}' to ${type} type.", ("name", delegate.first)("type", type_str) );
            delegate.second.set_index_type( db, type, p, cfg );
        }
    }
    
    uint32_t database::reindex( const open_args& args )
    {
        reindex_notification note( args );
        
        BOOST_SCOPE_EXIT(this_, &note) {
            TAIYI_TRY_NOTIFY(this_->_post_reindex_signal, note);
        } BOOST_SCOPE_EXIT_END
        
        try
        {
            ilog( "Reindexing Blockchain" );
            initialize_indexes();
            
            wipe( args.data_dir, args.state_storage_dir, false );
            open( args );
            
            TAIYI_TRY_NOTIFY(_pre_reindex_signal, note);
            
            if( args.replay_in_memory )
            {
                ilog( "Configuring replay to use memory..." );
                reindex_set_index_helper( *this, mira::index_type::bmic, args.state_storage_dir, args.database_cfg, args.replay_memory_indices );
            }
            
            _fork_db.reset();    // override effect of _fork_db.start_block() call in open()
            
            auto start = fc::time_point::now();
            TAIYI_ASSERT( _block_log.head(), block_log_exception, "No blocks in block log. Cannot reindex an empty chain." );
            
            ilog( "Replaying blocks..." );
            
            uint64_t skip_flags =
                skip_siming_signature |
                skip_transaction_signatures |
                skip_transaction_dupe_check |
                skip_tapos_check |
                skip_merkle_check |
                skip_siming_schedule_check |
                skip_authority_check |
                skip_validate | /// no need to validate operations
                (args.do_validate_invariants ? 0 : skip_validate_invariants) |
                skip_block_log;
            
            with_write_lock( [&]() {
                _block_log.set_locking( false );
                auto itr = _block_log.read_block( 0 );
                auto last_block_num = _block_log.head()->block_num();
                if( args.stop_replay_at > 0 && args.stop_replay_at < last_block_num )
                    last_block_num = args.stop_replay_at;
                if( args.benchmark.first > 0 )
                {
                    args.benchmark.second( 0, get_abstract_index_cntr() );
                }
                
                while( itr.first.block_num() != last_block_num )
                {
                    auto cur_block_num = itr.first.block_num();
                    if( cur_block_num % 100000 == 0 )
                    {
                        std::cerr << "   " << double( cur_block_num ) * 100  / last_block_num << "%   " << cur_block_num << " of " << last_block_num << "   (" <<
                        get_cache_size()  << " objects cached using " << (get_cache_usage() >> 20) << "M" << ")\n";
                        
                        //rocksdb::SetPerfLevel(rocksdb::kEnableCount);
                        //rocksdb::get_perf_context()->Reset();
                    }
                    apply_block( itr.first, skip_flags );
                    
                    if( cur_block_num % 100000 == 0 )
                    {
                        //std::cout << rocksdb::get_perf_context()->ToString() << std::endl;
                        if( cur_block_num % 1000000 == 0 )
                        {
                            dump_lb_call_counts();
                        }
                    }
                    
                    if( (args.benchmark.first > 0) && (cur_block_num % args.benchmark.first == 0) )
                        args.benchmark.second( cur_block_num, get_abstract_index_cntr() );
                    itr = _block_log.read_block( itr.second );
                }
                
                apply_block( itr.first, skip_flags );
                note.last_block_number = itr.first.block_num();
                
                if( (args.benchmark.first > 0) && (note.last_block_number % args.benchmark.first == 0) )
                    args.benchmark.second( note.last_block_number, get_abstract_index_cntr() );
                set_revision( head_block_num() );
                _block_log.set_locking( true );
                
                //get_index< account_index >().indices().print_stats();
            });
            
            if( _block_log.head()->block_num() )
                _fork_db.start_block( *_block_log.head() );
            
            if( args.replay_in_memory )
            {
                ilog( "Migrating state to disk..." );
                reindex_set_index_helper( *this, mira::index_type::mira, args.state_storage_dir, args.database_cfg, args.replay_memory_indices );
            }
            
            auto end = fc::time_point::now();
            ilog( "Done reindexing, elapsed time: ${t} sec", ("t",double((end-start).count())/1000000.0 ) );
            
            note.reindex_success = true;
            
            return note.last_block_number;
        }
        FC_CAPTURE_AND_RETHROW( (args.data_dir)(args.state_storage_dir) )
    }

    void database::wipe( const fc::path& data_dir, const fc::path& state_storage_dir, bool include_blocks)
    {
        close();
        chainbase::database::wipe( state_storage_dir );
        if( include_blocks )
        {
            fc::remove_all( data_dir / "block_log" );
            fc::remove_all( data_dir / "block_log.index" );
        }
    }

    void database::close(bool rewind)
    { try {
        // Since pop_block() will move tx's in the popped blocks into pending,
        // we have to clear_pending() after we're done popping to get a clean
        // DB state (issue #336).
        clear_pending();
        
        undo_all();
        
        chainbase::database::flush();
        chainbase::database::close();
        
        _block_log.close();
        
        _fork_db.reset();
    } FC_CAPTURE_AND_RETHROW() }
    
    bool database::is_known_block( const block_id_type& id )const
    { try {
        return fetch_block_by_id( id ).valid();
    } FC_CAPTURE_AND_RETHROW() }

    /**
     * Only return true *if* the transaction has not expired or been invalidated. If this
     * method is called with a VERY old transaction we will return false, they should
     * query things by blocks if they are that old.
     */
    bool database::is_known_transaction( const transaction_id_type& id )const
    { try {
        const auto& trx_idx = get_index<transaction_index>().indices().get<by_trx_id>();
        return trx_idx.find( id ) != trx_idx.end();
    } FC_CAPTURE_AND_RETHROW() }
    
    block_id_type database::find_block_id_for_num( uint32_t block_num )const
    { try {
        if( block_num == 0 )
            return block_id_type();
        
        // Reversible blocks are *usually* in the TAPOS buffer.  Since this
        // is the fastest check, we do it first.
        block_summary_id_type bsid = block_num & 0xFFFF;
        const block_summary_object* bs = find< block_summary_object, by_id >( bsid );
        if( bs != nullptr )
        {
            if( protocol::block_header::num_from_id(bs->block_id) == block_num )
                return bs->block_id;
        }
        
        // Next we query the block log.   Irreversible blocks are here.
        auto b = _block_log.read_block_by_num( block_num );
        if( b.valid() )
            return b->id();
        
        // Finally we query the fork DB.
        shared_ptr< fork_item > fitem = _fork_db.fetch_block_on_main_branch_by_number( block_num );
        if( fitem )
            return fitem->id;
        
        return block_id_type();
    } FC_CAPTURE_AND_RETHROW( (block_num) ) }
    
    block_id_type database::get_block_id_for_num( uint32_t block_num )const
    {
        block_id_type bid = find_block_id_for_num( block_num );
        FC_ASSERT( bid != block_id_type() );
        return bid;
    }
    
    optional<signed_block> database::fetch_block_by_id( const block_id_type& id )const
    { try {
        auto b = _fork_db.fetch_block( id );
        if( !b )
        {
            auto tmp = _block_log.read_block_by_num( protocol::block_header::num_from_id( id ) );
            if( tmp && tmp->id() == id )
                return tmp;
            
            tmp.reset();
            return tmp;
        }
        
        return b->data;
    } FC_CAPTURE_AND_RETHROW() }
    
    optional<signed_block> database::fetch_block_by_number( uint32_t block_num )const
    { try {
        optional< signed_block > b;
        shared_ptr< fork_item > fitem = _fork_db.fetch_block_on_main_branch_by_number( block_num );
        
        if( fitem )
            b = fitem->data;
        else
            b = _block_log.read_block_by_num( block_num );
        
        return b;
    } FC_LOG_AND_RETHROW() }
    
    const signed_transaction database::get_recent_transaction( const transaction_id_type& trx_id ) const
    { try {
        const auto& index = get_index<transaction_index>().indices().get<by_trx_id>();
        auto itr = index.find(trx_id);
        FC_ASSERT(itr != index.end());
        signed_transaction trx;
        fc::raw::unpack_from_buffer( itr->packed_trx, trx );
        return trx;;
    } FC_CAPTURE_AND_RETHROW() }
    
    std::vector< block_id_type > database::get_block_ids_on_fork( block_id_type head_of_fork ) const
    { try {
        pair<fork_database::branch_type, fork_database::branch_type> branches = _fork_db.fetch_branch_from(head_block_id(), head_of_fork);
        if( !((branches.first.back()->previous_id() == branches.second.back()->previous_id())) )
        {
            edump( (head_of_fork)
                  (head_block_id())
                  (branches.first.size())
                  (branches.second.size()) );
            assert(branches.first.back()->previous_id() == branches.second.back()->previous_id());
        }
        std::vector< block_id_type > result;
        for( const item_ptr& fork_block : branches.second )
            result.emplace_back(fork_block->id);
        result.emplace_back(branches.first.back()->previous_id());
        return result;
    } FC_CAPTURE_AND_RETHROW() }

    chain_id_type database::get_chain_id() const
    {
        return _taiyi_chain_id;
    }
    
    void database::set_chain_id( const chain_id_type& chain_id )
    {
        _taiyi_chain_id = chain_id;
        idump( (_taiyi_chain_id) );
    }
    
    void database::foreach_block(std::function<bool(const signed_block_header&, const signed_block&)> processor) const
    {
        if(!_block_log.head())
            return;
        
        auto itr = _block_log.read_block( 0 );
        auto last_block_num = _block_log.head()->block_num();
        signed_block_header previousBlockHeader = itr.first;
        while( itr.first.block_num() != last_block_num )
        {
            const signed_block& b = itr.first;
            if(processor(previousBlockHeader, b) == false)
                return;
            
            previousBlockHeader = b;
            itr = _block_log.read_block( itr.second );
        }
        
        processor(previousBlockHeader, itr.first);
    }

    void database::foreach_tx(std::function<bool(const signed_block_header&, const signed_block&, const signed_transaction&, uint32_t)> processor) const
    {
        foreach_block([&processor](const signed_block_header& prevBlockHeader, const signed_block& block) -> bool {
            uint32_t txInBlock = 0;
            for( const auto& trx : block.transactions )
            {
                if(processor(prevBlockHeader, block, trx, txInBlock) == false)
                    return false;
                ++txInBlock;
            }
            
            return true;
        } );
    }

    void database::foreach_operation(std::function<bool(const signed_block_header&,const signed_block&, const signed_transaction&, uint32_t, const operation&, uint16_t)> processor) const
    {
        foreach_tx([&processor](const signed_block_header& prevBlockHeader, const signed_block& block, const signed_transaction& tx, uint32_t txInBlock) -> bool {
            uint16_t opInTx = 0;
            for(const auto& op : tx.operations)
            {
                if(processor(prevBlockHeader, block, tx, txInBlock, op, opInTx) == false)
                    return false;
                ++opInTx;
            }
            return true;
        } );
    }

    const siming_object& database::get_siming( const account_name_type& name ) const
    { try {
        return get< siming_object, by_name >( name );
    } FC_CAPTURE_AND_RETHROW( (name) ) }
    
    const siming_object* database::find_siming( const account_name_type& name ) const
    {
        return find< siming_object, by_name >( name );
    }
    
    const account_object& database::get_account( const account_name_type& name )const
    { try {
        return get< account_object, by_name >( name );
    } FC_CAPTURE_AND_RETHROW( (name) ) }
    
    const account_object* database::find_account( const account_name_type& name )const
    {
        return find< account_object, by_name >( name );
    }

    const dynamic_global_property_object& database::get_dynamic_global_properties() const
    { try {
        return get< dynamic_global_property_object >();
    } FC_CAPTURE_AND_RETHROW() }
    
    const node_property_object& database::get_node_properties() const
    {
        return _node_property_object;
    }
    
    const siming_schedule_object& database::get_siming_schedule_object()const
    { try {
        return get< siming_schedule_object >();
    } FC_CAPTURE_AND_RETHROW() }
    
    const hardfork_property_object& database::get_hardfork_property_object()const
    { try {
        return get< hardfork_property_object >();
    } FC_CAPTURE_AND_RETHROW() }
    
    uint32_t database::siming_participation_rate()const
    {
        const dynamic_global_property_object& dpo = get_dynamic_global_properties();
        return uint64_t(TAIYI_100_PERCENT) * dpo.recent_slots_filled.popcount() / 128;
    }
    
    void database::add_checkpoints( const flat_map< uint32_t, block_id_type >& checkpts )
    {
        for( const auto& i : checkpts )
            _checkpoints[i.first] = i.second;
    }

    bool database::before_last_checkpoint()const
    {
        return (_checkpoints.size() > 0) && (_checkpoints.rbegin()->first >= head_block_num());
    }
    
    /**
     * Push block "may fail" in which case every partial change is unwound.  After
     * push block is successful the block is appended to the chain database on disk.
     *
     * @return true if we switched forks as a result of this push.
     */
    bool database::push_block(const signed_block& new_block, uint32_t skip)
    {
        //fc::time_point begin_time = fc::time_point::now();
        
        auto block_num = new_block.block_num();
        if( _checkpoints.size() && _checkpoints.rbegin()->second != block_id_type() )
        {
            auto itr = _checkpoints.find( block_num );
            if( itr != _checkpoints.end() )
                FC_ASSERT( new_block.id() == itr->second, "Block did not match checkpoint", ("checkpoint",*itr)("block_id",new_block.id()) );
            
            if( _checkpoints.rbegin()->first >= block_num )
                skip = skip_siming_signature
                | skip_transaction_signatures
                | skip_transaction_dupe_check
                //| skip_fork_db Fork db cannot be skipped or else blocks will not be written out to block log
                | skip_block_size_check
                | skip_tapos_check
                | skip_authority_check
                //| skip_merkle_check While blockchain is being downloaded, txs need to be validated against block headers
                | skip_undo_history_check
                | skip_siming_schedule_check
                | skip_validate
                | skip_validate_invariants
                ;
        }
        
        bool result;
        detail::with_skip_flags( *this, skip, [&]() {
            detail::without_pending_transactions( *this, std::move(_pending_tx), [&]() {
                try
                {
                    result = _push_block(new_block);
                }
                FC_CAPTURE_AND_RETHROW( (new_block) )                
            });
        });

        //fc::time_point end_time = fc::time_point::now();
        //fc::microseconds dt = end_time - begin_time;
        //if( ( new_block.block_num() % 10000 ) == 0 )
        //   ilog( "push_block ${b} took ${t} microseconds", ("b", new_block.block_num())("t", dt.count()) );
        return result;
    }
    
    void database::_maybe_warn_multiple_production( uint32_t height )const
    {
        auto blocks = _fork_db.fetch_block_by_number( height );
        if( blocks.size() > 1 )
        {
            vector< std::pair< account_name_type, fc::time_point_sec > > siming_time_pairs;
            for( const auto& b : blocks )
            {
                siming_time_pairs.push_back( std::make_pair( b->data.siming, b->data.timestamp ) );
            }
            
            ilog( "Encountered block num collision at block ${n} due to a fork, simings are: ${w}", ("n", height)("w", siming_time_pairs) );
        }
    }

    bool database::_push_block(const signed_block& new_block)
    { try {
        uint32_t skip = get_node_properties().skip_flags;
        //uint32_t skip_undo_db = skip & skip_undo_block;
        
        if( !(skip&skip_fork_db) )
        {
            shared_ptr<fork_item> new_head = _fork_db.push_block(new_block);
            _maybe_warn_multiple_production( new_head->num );
            
            //If the head block from the longest chain does not build off of the current head, we need to switch forks.
            if( new_head->data.previous != head_block_id() )
            {
                //If the newly pushed block is the same height as head, we get head back in new_head
                //Only switch forks if new_head is actually higher than head
                if( new_head->data.block_num() > head_block_num() )
                {
                    wlog( "Switching to fork: ${id}", ("id",new_head->data.id()) );
                    auto branches = _fork_db.fetch_branch_from(new_head->data.id(), head_block_id());
                    
                    // pop blocks until we hit the forked block
                    while( head_block_id() != branches.second.back()->data.previous )
                        pop_block();
                    
                    // push all blocks on the new fork
                    for( auto ritr = branches.first.rbegin(); ritr != branches.first.rend(); ++ritr )
                    {
                        ilog( "pushing blocks from fork ${n} ${id}", ("n",(*ritr)->data.block_num())("id",(*ritr)->data.id()) );
                        optional<fc::exception> except;
                        try
                        {
                            _fork_db.set_head( *ritr );
                            auto session = start_undo_session();
                            apply_block( (*ritr)->data, skip );
                            session.push();
                        }
                        catch ( const fc::exception& e ) { except = e; }
                        if( except )
                        {
                            wlog( "exception thrown while switching forks ${e}", ("e",except->to_detail_string() ) );
                            // remove the rest of branches.first from the fork_db, those blocks are invalid
                            while( ritr != branches.first.rend() )
                            {
                                _fork_db.remove( (*ritr)->data.id() );
                                ++ritr;
                            }
                            
                            // pop all blocks from the bad fork
                            while( head_block_id() != branches.second.back()->data.previous )
                                pop_block();
                            
                            // restore all blocks from the good fork
                            for( auto ritr = branches.second.rbegin(); ritr != branches.second.rend(); ++ritr )
                            {
                                _fork_db.set_head( *ritr );
                                auto session = start_undo_session();
                                apply_block( (*ritr)->data, skip );
                                session.push();
                            }
                            throw *except;
                        }
                    }
                    return true;
                }
                else
                    return false;
            }
        }
        
        try
        {
            auto session = start_undo_session();
            apply_block(new_block, skip);
            session.push();
        }
        catch( const fc::exception& e )
        {
            elog("Failed to push new block:\n${e}", ("e", e.to_detail_string()));
            _fork_db.remove(new_block.id());
            throw;
        }
        
        return false;
    } FC_CAPTURE_AND_RETHROW() }
    
    /**
     * Attempts to push the transaction into the pending queue
     *
     * When called to push a locally generated transaction, set the skip_block_size_check bit on the skip argument. This
     * will allow the transaction to be pushed even if it causes the pending block size to exceed the maximum block size.
     * Although the transaction will probably not propagate further now, as the peers are likely to have their pending
     * queues full as well, it will be kept in the queue to be propagated later when a new block flushes out the pending
     * queues.
     */
    void database::push_transaction( const signed_transaction& trx, uint32_t skip )
    { try {
        FC_ASSERT( fc::raw::pack_size(trx) <= (get_dynamic_global_properties().maximum_block_size - 256) );
        detail::with_skip_flags( *this, skip, [&]() {
            _push_transaction( trx );
        });
    } FC_CAPTURE_AND_RETHROW( (trx) ) }
    
    // 这里是所有收到的外来广播或者自己产生的新交易，在当前状态上验证执行后，加入到pending队列中
    void database::_push_transaction( const signed_transaction& trx )
    {
        // If this is the first transaction pushed after applying a block, start a new undo session.
        // This allows us to quickly rewind to the clean state of the head block, in case a new block arrives.
        if( !_pending_tx_session.valid() )
            _pending_tx_session = start_undo_session();
        
        // Create a temporary undo session as a child of _pending_tx_session.
        // The temporary session will be discarded by the destructor if
        // _apply_transaction fails.  If we make it to merge(), we
        // apply the changes.
        
        auto temp_session = start_undo_session();
        _apply_transaction( trx );
        _pending_tx.push_back( trx );
        
        notify_changed_objects();
        // The transaction applied successfully. Merge its changes into the pending block session.
        temp_session.squash();
    }
    
    /**
     * Removes the most recent block from the database and
     * undoes any changes it made.
     */
    void database::pop_block()
    { try {
        _pending_tx_session.reset();
        auto head_id = head_block_id();
        
        /// save the head block so we can recover its transactions
        optional<signed_block> head_block = fetch_block_by_id( head_id );
        TAIYI_ASSERT( head_block.valid(), pop_empty_chain, "there are no blocks to pop" );
        
        _fork_db.pop_block();
        undo();
        
        _popped_tx.insert( _popped_tx.begin(), head_block->transactions.begin(), head_block->transactions.end() );
        
    } FC_CAPTURE_AND_RETHROW() }
    
    void database::clear_pending()
    { try {
        assert( (_pending_tx.size() == 0) || _pending_tx_session.valid() );
        _pending_tx.clear();
        _pending_tx_session.reset();
    } FC_CAPTURE_AND_RETHROW() }
    
    void database::push_virtual_operation( const operation& op )
    {
        FC_ASSERT( is_virtual_operation( op ) );
        operation_notification note = create_operation_notification( op );
        ++_current_virtual_op;
        note.virtual_op = _current_virtual_op;
        notify_pre_apply_operation( note );
        notify_post_apply_operation( note );
    }
    
    void database::pre_push_virtual_operation( const operation& op )
    {
        FC_ASSERT( is_virtual_operation( op ) );
        operation_notification note = create_operation_notification( op );
        ++_current_virtual_op;
        note.virtual_op = _current_virtual_op;
        notify_pre_apply_operation( note );
    }
    
    void database::post_push_virtual_operation( const operation& op )
    {
        FC_ASSERT( is_virtual_operation( op ) );
        operation_notification note = create_operation_notification( op );
        note.virtual_op = _current_virtual_op;
        notify_post_apply_operation( note );
    }
    
    void database::notify_pre_apply_operation( const operation_notification& note )
    {
        TAIYI_TRY_NOTIFY( _pre_apply_operation_signal, note )
    }
    
    void database::notify_post_apply_operation( const operation_notification& note )
    {
        TAIYI_TRY_NOTIFY( _post_apply_operation_signal, note )
    }
    
    void database::notify_pre_apply_block( const block_notification& note )
    {
        TAIYI_TRY_NOTIFY( _pre_apply_block_signal, note )
    }
    
    void database::notify_irreversible_block( uint32_t block_num )
    {
        TAIYI_TRY_NOTIFY( _on_irreversible_block, block_num )
    }
    
    void database::notify_post_apply_block( const block_notification& note )
    {
        TAIYI_TRY_NOTIFY( _post_apply_block_signal, note )
    }
    
    void database::notify_pre_apply_transaction( const transaction_notification& note )
    {
        TAIYI_TRY_NOTIFY( _pre_apply_transaction_signal, note )
    }
    
    void database::notify_post_apply_transaction( const transaction_notification& note )
    {
        TAIYI_TRY_NOTIFY( _post_apply_transaction_signal, note )
    }
    
    account_name_type database::get_scheduled_siming( uint32_t slot_num )const
    {
        const dynamic_global_property_object& dpo = get_dynamic_global_properties();
        const siming_schedule_object& wso = get_siming_schedule_object();
        uint64_t current_aslot = dpo.current_aslot + slot_num;
        return wso.current_shuffled_simings[ current_aslot % wso.num_scheduled_simings ];
    }
    
    fc::time_point_sec database::get_slot_time(uint32_t slot_num)const
    {
        if( slot_num == 0 )
            return fc::time_point_sec();
        
        auto interval = TAIYI_BLOCK_INTERVAL;
        const dynamic_global_property_object& dpo = get_dynamic_global_properties();
        
        if( head_block_num() == 0 )
        {
            // n.b. first block is at genesis_time plus one block interval
            fc::time_point_sec genesis_time = dpo.time;
            return genesis_time + slot_num * interval;
        }
        
        int64_t head_block_abs_slot = head_block_time().sec_since_epoch() / interval;
        fc::time_point_sec head_slot_time( head_block_abs_slot * interval );
        
        // "slot 0" is head_slot_time
        // "slot 1" is head_slot_time,
        //   plus maint interval if head block is a maint block
        //   plus block interval if head block is not a maint block
        return head_slot_time + (slot_num * interval);
    }
    
    uint32_t database::get_slot_at_time(fc::time_point_sec when)const
    {
        fc::time_point_sec first_slot_time = get_slot_time( 1 );
        if( when < first_slot_time )
            return 0;
        return (when - first_slot_time).to_seconds() / TAIYI_BLOCK_INTERVAL + 1;
    }

    // Create qi, then a caller-supplied callback after determining how many shares to create, but before
    // we modify the database.
    // This allows us to implement virtual op pre-notifications in the Before function.
    template< typename Before >
    asset create_qi2( database& db, const account_object& to_account, asset liquid, bool to_reward_balance, Before&& before_qi_callback )
    { try {
                
        FC_ASSERT( liquid.symbol == YANG_SYMBOL );
        const auto& cprops = db.get_dynamic_global_properties();
        
        // Calculate new qi from provided liquid using share price.
        asset new_qi = liquid * TAIYI_QI_SHARE_PRICE;
        before_qi_callback( new_qi );
        
        // Add new qi to owner's balance.
        if( to_reward_balance )
            db.adjust_reward_balance( to_account, liquid, new_qi );
        else
            db.adjust_balance( to_account, new_qi );
        
        // Update global qi pool numbers.
        db.modify( cprops, [&]( dynamic_global_property_object& props ) {
            if( to_reward_balance )
                props.pending_rewarded_qi += new_qi;
            else
                props.total_qi += new_qi;
        } );
        
        // Update siming adoring numbers.
        if( !to_reward_balance )
            db.adjust_proxied_siming_adores( to_account, new_qi.amount );
        
        return new_qi;
        
    } FC_CAPTURE_AND_RETHROW( (to_account.name)(liquid) ) }
    
    /**
     * @param to_account - the account to receive the new qi shares
     * @param liquid     - YANG to be converted to qi shares
     */
    asset database::create_qi( const account_object& to_account, asset liquid, bool to_reward_balance )
    {
        return create_qi2( *this, to_account, liquid, to_reward_balance, []( asset qi_created ) {} );
    }
    
    void database::adjust_proxied_siming_adores( const account_object& a, const std::array< share_type, TAIYI_MAX_PROXY_RECURSION_DEPTH+1 >& delta, int depth )
    {
        if( a.proxy != TAIYI_PROXY_TO_SELF_ACCOUNT )
        {
            /// nested proxies are not supported, adore will not propagate
            if( depth >= TAIYI_MAX_PROXY_RECURSION_DEPTH )
                return;
            
            const auto& proxy = get_account( a.proxy );
            modify( proxy, [&]( account_object& a ) {
                for( int i = TAIYI_MAX_PROXY_RECURSION_DEPTH - depth - 1; i >= 0; --i )
                {
                    a.proxied_vsf_adores[i+depth] += delta[i];
                }
            } );
            
            adjust_proxied_siming_adores( proxy, delta, depth + 1 );
        }
        else
        {
            share_type total_delta = 0;
            for( int i = TAIYI_MAX_PROXY_RECURSION_DEPTH - depth; i >= 0; --i )
                total_delta += delta[i];
            adjust_siming_adores( a, total_delta );
        }
    }
    
    void database::adjust_proxied_siming_adores( const account_object& a, share_type delta, int depth )
    {
        if( a.proxy != TAIYI_PROXY_TO_SELF_ACCOUNT )
        {
            /// nested proxies are not supported, adore will not propagate
            if( depth >= TAIYI_MAX_PROXY_RECURSION_DEPTH )
                return;
            
            const auto& proxy = get_account( a.proxy );
            modify( proxy, [&]( account_object& a ) {
                a.proxied_vsf_adores[depth] += delta;
            } );
            
            adjust_proxied_siming_adores( proxy, delta, depth + 1 );
        }
        else
        {
            adjust_siming_adores( a, delta );
        }
    }
    
    void database::adjust_siming_adores( const account_object& a, share_type delta )
    {
        const auto& vidx = get_index< siming_adore_index >().indices().get< by_account_siming >();
        auto itr = vidx.lower_bound( boost::make_tuple( a.name, account_name_type() ) );
        while( itr != vidx.end() && itr->account == a.name )
        {
            adjust_siming_adore( get< siming_object, by_name >(itr->siming), delta );
            ++itr;
        }
    }
    
    void database::adjust_siming_adore( const siming_object& siming, share_type delta )
    {
        const siming_schedule_object& wso = get_siming_schedule_object();
        modify( siming, [&]( siming_object& w ) {
            auto delta_pos = w.adores.value * (wso.current_virtual_time - w.virtual_last_update);
            w.virtual_position += delta_pos;
            
            w.virtual_last_update = wso.current_virtual_time;
            w.adores += delta;
            FC_ASSERT( w.adores <= get_dynamic_global_properties().total_qi.amount, "", ("w.adores", w.adores)("props",get_dynamic_global_properties().total_qi) );
            
            w.virtual_scheduled_time = w.virtual_last_update + (TAIYI_VIRTUAL_SCHEDULE_LAP_LENGTH - w.virtual_position)/(w.adores.value+1);
            
            /** simings with a low number of adores could overflow the time field and end up with a scheduled time in the past */
            if( w.virtual_scheduled_time < wso.current_virtual_time )
                w.virtual_scheduled_time = fc::uint128::max_value();
        } );
    }
    
    void database::clear_siming_adores( const account_object& a )
    {
        const auto& vidx = get_index< siming_adore_index >().indices().get<by_account_siming>();
        auto itr = vidx.lower_bound( boost::make_tuple( a.name, account_name_type() ) );
        while( itr != vidx.end() && itr->account == a.name )
        {
            const auto& current = *itr;
            ++itr;
            remove(current);
        }
        
        modify( a, [&](account_object& acc ) {
            acc.simings_adored_for = 0;
        });
    }
    
    void database::update_owner_authority( const account_object& account, const authority& owner_authority )
    {
        if( head_block_num() >= TAIYI_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM )
        {
            create< owner_authority_history_object >( [&]( owner_authority_history_object& hist ) {
                hist.account = account.name;
                hist.previous_owner_authority = get< account_authority_object, by_account >( account.name ).owner;
                hist.last_valid_time = head_block_time();
            });
        }
        
        modify( get< account_authority_object, by_account >( account.name ), [&]( account_authority_object& auth ) {
            auth.owner = owner_authority;
            auth.last_owner_update = head_block_time();
        });
    }
    
    void database::process_qi_withdrawals()
    {
        const auto& widx = get_index< account_index, by_next_qi_withdrawal_time >();
        const auto& didx = get_index< withdraw_qi_route_index, by_withdraw_route >();
        const auto& cprops = get_dynamic_global_properties();
        
        auto current = widx.begin();
        while( current != widx.end() && current->next_qi_withdrawal_time <= head_block_time() )
        {
            const auto& from_account = *current;
            ++current;
            
            /**
             *  Let T = total tokens in qi fund
             *  Let V = total qi shares
             *  Let v = total qi shares being cashed out
             *
             *  The user may withdraw  vT / V tokens
             */
            share_type to_withdraw;
            if ( from_account.to_withdraw - from_account.withdrawn < from_account.qi_withdraw_rate.amount )
                to_withdraw = std::min( from_account.qi.amount, from_account.to_withdraw % from_account.qi_withdraw_rate.amount ).value;
            else
                to_withdraw = std::min( from_account.qi.amount, from_account.qi_withdraw_rate.amount ).value;
            
            share_type qi_deposited_as_yang = 0;
            share_type qi_deposited_as_qi = 0;
            asset total_yang_converted = asset( 0, YANG_SYMBOL );
            
            // Do two passes, the first for qi, the second for yang. Try to maintain as much accuracy for qi as possible.
            for( auto itr = didx.upper_bound( boost::make_tuple( from_account.name, account_name_type() ) );
                itr != didx.end() && itr->from_account == from_account.name;
                ++itr )
            {
                if( itr->auto_vest )
                {
                    share_type to_deposit = ( ( fc::uint128_t ( to_withdraw.value ) * itr->percent ) / TAIYI_100_PERCENT ).to_uint64();
                    qi_deposited_as_qi += to_deposit;
                    
                    if( to_deposit > 0 )
                    {
                        const auto& to_account = get< account_object, by_name >( itr->to_account );
                        
                        operation vop = fill_qi_withdraw_operation( from_account.name, to_account.name, asset( to_deposit, QI_SYMBOL ), asset( to_deposit, QI_SYMBOL ) );
                        
                        pre_push_virtual_operation( vop );
                        
                        modify( to_account, [&]( account_object& a ) {
                            a.qi.amount += to_deposit;
                        });
                        adjust_proxied_siming_adores( to_account, to_deposit );
                        
                        post_push_virtual_operation( vop );
                    }
                }
            }
            
            for( auto itr = didx.upper_bound( boost::make_tuple( from_account.name, account_name_type() ) );
                itr != didx.end() && itr->from_account == from_account.name;
                ++itr )
            {
                if( !itr->auto_vest )
                {
                    const auto& to_account = get< account_object, by_name >( itr->to_account );
                    
                    share_type to_deposit = ( ( fc::uint128_t ( to_withdraw.value ) * itr->percent ) / TAIYI_100_PERCENT ).to_uint64();
                    qi_deposited_as_yang += to_deposit;
                    auto converted_yang = asset( to_deposit, QI_SYMBOL ) * TAIYI_QI_SHARE_PRICE;
                    total_yang_converted += converted_yang;
                    
                    if( to_deposit > 0 )
                    {
                        operation vop = fill_qi_withdraw_operation( from_account.name, to_account.name, asset( to_deposit, QI_SYMBOL), converted_yang );
                        
                        pre_push_virtual_operation( vop );
                        
                        modify( to_account, [&]( account_object& a ) {
                            a.balance += converted_yang;
                        });
                        
                        modify( cprops, [&]( dynamic_global_property_object& o ) {
                            o.total_qi.amount -= to_deposit;
                        });
                        
                        post_push_virtual_operation( vop );
                    }
                }
            }
            
            share_type to_convert = to_withdraw - qi_deposited_as_yang - qi_deposited_as_qi;
            FC_ASSERT( to_convert >= 0, "Deposited more qi than were supposed to be withdrawn" );
            
            auto converted_yang = asset( to_convert, QI_SYMBOL ) * TAIYI_QI_SHARE_PRICE;
            operation vop = fill_qi_withdraw_operation( from_account.name, from_account.name, asset( to_convert, QI_SYMBOL ), converted_yang );
            pre_push_virtual_operation( vop );
            
            modify( from_account, [&]( account_object& a ) {
                a.qi.amount -= to_withdraw;
                a.balance += converted_yang;
                a.withdrawn += to_withdraw;
                
                if( a.withdrawn >= a.to_withdraw || a.qi.amount == 0 )
                {
                    a.qi_withdraw_rate.amount = 0;
                    a.next_qi_withdrawal_time = fc::time_point_sec::maximum();
                }
                else
                {
                    a.next_qi_withdrawal_time += fc::seconds( TAIYI_QI_WITHDRAW_INTERVAL_SECONDS );
                }
            });
            
            modify( cprops, [&]( dynamic_global_property_object& o ) {
                o.total_qi.amount -= to_convert;
            });
            
            if( to_withdraw > 0 )
                adjust_proxied_siming_adores( from_account, -to_withdraw );
            
            post_push_virtual_operation( vop );
        }
    }
    /**
     *  This method pays out qi and reward shares every block.
     */
    void database::process_funds()
    {
        const auto& props = get_dynamic_global_properties();
        const auto& wso = get_siming_schedule_object();
        
        /**
         * At block 0 have a 10% instantaneous inflation rate, decreasing to 1% at a rate of 0.01%
         * every 233.6k blocks. This narrowing will take approximately 20 years and will complete on block 210,240,000
         */
        int64_t start_inflation_rate = int64_t( TAIYI_INFLATION_RATE_START_PERCENT );
        int64_t inflation_rate_adjustment = int64_t( head_block_num() / TAIYI_INFLATION_NARROWING_PERIOD );
        int64_t inflation_rate_floor = int64_t( TAIYI_INFLATION_RATE_STOP_PERCENT );
        
        // below subtraction cannot underflow int64_t because inflation_rate_adjustment is <2^32
        int64_t current_inflation_rate = std::max( start_inflation_rate - inflation_rate_adjustment, inflation_rate_floor );
        
        // 年化率转为块化率来计算每个块的新增阳寿
        share_type new_yang = ( props.current_supply.amount * current_inflation_rate ) / ( int64_t( TAIYI_100_PERCENT ) * int64_t( TAIYI_BLOCKS_PER_YEAR ) );
        
        share_type content_reward_yang = ( new_yang * props.content_reward_yang_percent ) / TAIYI_100_PERCENT;
        content_reward_yang = std::max(content_reward_yang, share_type(TAIYI_MIN_REWARD_FUND));
        
        share_type content_reward_qi_fund = ( new_yang * props.content_reward_qi_fund_percent ) / TAIYI_100_PERCENT;
        content_reward_qi_fund = std::max(content_reward_qi_fund, share_type(TAIYI_MIN_REWARD_FUND));

        std::tie(content_reward_yang, content_reward_qi_fund) = pay_reward_funds( content_reward_yang, content_reward_qi_fund );

        auto siming_reward = new_yang - content_reward_yang - content_reward_qi_fund;
        siming_reward = std::max(siming_reward, share_type(TAIYI_MIN_REWARD_FUND));

        new_yang = content_reward_yang + content_reward_qi_fund + siming_reward;
        
        asset last_siming_production_reward;
        const auto& csiming = get_siming( props.current_siming );
        // pay siming in qi shares
        const auto& siming_account = get_account( csiming.owner );
        if( props.head_block_number >= TAIYI_START_ADORING_BLOCK || (siming_account.qi.amount.value == 0) )
        {
            operation vop = producer_reward_operation( csiming.owner, asset( 0, QI_SYMBOL ) );
            create_qi2( *this, siming_account, asset( siming_reward, YANG_SYMBOL ), false, [&]( const asset& qi ) {
                last_siming_production_reward = qi;
                vop.get< producer_reward_operation >().reward = qi;
                pre_push_virtual_operation( vop );
            } );
            post_push_virtual_operation( vop );
            last_siming_production_reward = vop.get< producer_reward_operation >().reward;
        }
        else
        {
            operation vop = producer_reward_operation( csiming.owner, asset( siming_reward, YANG_SYMBOL ) );
            pre_push_virtual_operation( vop );
            modify( siming_account, [&]( account_object& a ) {
                a.balance += asset( siming_reward, YANG_SYMBOL );
            } );
            post_push_virtual_operation( vop );
            last_siming_production_reward = vop.get< producer_reward_operation >().reward;
        }
        
        modify( props, [&]( dynamic_global_property_object& p ) {
            p.current_supply += asset( new_yang, YANG_SYMBOL );
            p.last_siming_production_reward = last_siming_production_reward.symbol == QI_SYMBOL ? last_siming_production_reward : (last_siming_production_reward * TAIYI_QI_SHARE_PRICE);
        });
    }
    
    std::tuple<share_type, share_type> database::pay_reward_funds( share_type reward_yang, share_type reward_qi_fund )
    {
        auto now = head_block_time();
        auto gpo = get_dynamic_global_properties();
        const auto& reward_idx = get_index< reward_fund_index, by_id >();
        share_type used_rewards_yang = 0;
        share_type used_rewards_qi_fund = 0;
        
        for( auto itr = reward_idx.begin(); itr != reward_idx.end(); ++itr )
        {
            // reward is a per block reward and the percents are 16-bit. This should never overflow
            auto ryang = ( reward_yang * itr->percent_content_rewards ) / TAIYI_100_PERCENT;
            ryang = std::max(ryang, share_type(TAIYI_MIN_REWARD_FUND));
            
            auto rqiyang = ( reward_qi_fund * itr->percent_content_rewards ) / TAIYI_100_PERCENT;
            rqiyang = std::max(rqiyang, share_type(TAIYI_MIN_REWARD_FUND));
            asset reward_qi = asset( rqiyang, YANG_SYMBOL ) * TAIYI_QI_SHARE_PRICE;;
            
            modify( *itr, [&]( reward_fund_object& rfo ) {
                rfo.reward_balance += asset( ryang, YANG_SYMBOL );
                rfo.reward_qi_balance += reward_qi;
                rfo.last_update = now;
            });
            
            modify( gpo, [&]( dynamic_global_property_object& o ) {
                o.pending_rewarded_qi += reward_qi;
            });
            
            used_rewards_yang += ryang;
            used_rewards_qi_fund += rqiyang;
            
            // Sanity check to ensure we aren't printing more YANG than has been allocated through inflation
            FC_ASSERT( used_rewards_yang <= reward_yang );
            FC_ASSERT( used_rewards_qi_fund <= reward_qi_fund );
        }
        
        return std::make_tuple(used_rewards_yang, used_rewards_qi_fund);
    }
    
    void database::account_recovery_processing()
    {
        // Clear expired recovery requests
        const auto& rec_req_idx = get_index< account_recovery_request_index >().indices().get< by_expiration >();
        auto rec_req = rec_req_idx.begin();
        
        while( rec_req != rec_req_idx.end() && rec_req->expires <= head_block_time() )
        {
            remove( *rec_req );
            rec_req = rec_req_idx.begin();
        }
        
        // Clear invalid historical authorities
        const auto& hist_idx = get_index< owner_authority_history_index >().indices(); //by id
        auto hist = hist_idx.begin();
        
        while( hist != hist_idx.end() && time_point_sec( hist->last_valid_time + TAIYI_OWNER_AUTH_RECOVERY_PERIOD ) < head_block_time() )
        {
            remove( *hist );
            hist = hist_idx.begin();
        }
        
        // Apply effective recovery_account changes
        const auto& change_req_idx = get_index< change_recovery_account_request_index >().indices().get< by_effective_date >();
        auto change_req = change_req_idx.begin();
        
        while( change_req != change_req_idx.end() && change_req->effective_on <= head_block_time() )
        {
            modify( get_account( change_req->account_to_recover ), [&]( account_object& a ) {
                a.recovery_account = change_req->recovery_account;
            });
            
            remove( *change_req );
            change_req = change_req_idx.begin();
        }
    }
    
    void database::process_decline_adoring_rights()
    {
        const auto& request_idx = get_index< decline_adoring_rights_request_index >().indices().get< by_effective_date >();
        auto itr = request_idx.begin();
        
        while( itr != request_idx.end() && itr->effective_date <= head_block_time() )
        {
            const auto& account = get< account_object, by_name >( itr->account );
            
            /// remove all current adores
            std::array<share_type, TAIYI_MAX_PROXY_RECURSION_DEPTH+1> delta;
            delta[0] = -account.qi.amount;
            for( int i = 0; i < TAIYI_MAX_PROXY_RECURSION_DEPTH; ++i )
                delta[i+1] = -account.proxied_vsf_adores[i];
            adjust_proxied_siming_adores( account, delta );
            
            clear_siming_adores( account );
            
            modify( account, [&]( account_object& a ) {
                a.can_adore = false;
                a.proxy = TAIYI_PROXY_TO_SELF_ACCOUNT;
            });
            
            remove( *itr );
            itr = request_idx.begin();
        }
    }
    
    time_point_sec database::head_block_time()const
    {
        return get_dynamic_global_properties().time;
    }
    
    uint32_t database::head_block_num()const
    {
        return get_dynamic_global_properties().head_block_number;
    }
    
    block_id_type database::head_block_id()const
    {
        return get_dynamic_global_properties().head_block_id;
    }
    
    node_property_object& database::node_properties()
    {
        return _node_property_object;
    }
    
    uint32_t database::last_non_undoable_block_num() const
    {
        return get_dynamic_global_properties().last_irreversible_block_num;
    }
    
    void database::initialize_evaluators()
    {
        _my->_evaluator_registry.register_evaluator< transfer_evaluator                       >();
        _my->_evaluator_registry.register_evaluator< transfer_to_qi_evaluator                 >();
        _my->_evaluator_registry.register_evaluator< withdraw_qi_evaluator                    >();
        _my->_evaluator_registry.register_evaluator< set_withdraw_qi_route_evaluator          >();
        _my->_evaluator_registry.register_evaluator< account_create_evaluator                 >();
        _my->_evaluator_registry.register_evaluator< account_update_evaluator                 >();
        _my->_evaluator_registry.register_evaluator< siming_update_evaluator                  >();
        _my->_evaluator_registry.register_evaluator< account_siming_adore_evaluator           >();
        _my->_evaluator_registry.register_evaluator< account_siming_proxy_evaluator           >();
        _my->_evaluator_registry.register_evaluator< custom_evaluator                         >();
        _my->_evaluator_registry.register_evaluator< custom_json_evaluator                    >();
        _my->_evaluator_registry.register_evaluator< request_account_recovery_evaluator       >();
        _my->_evaluator_registry.register_evaluator< recover_account_evaluator                >();
        _my->_evaluator_registry.register_evaluator< change_recovery_account_evaluator        >();
        _my->_evaluator_registry.register_evaluator< decline_adoring_rights_evaluator         >();
        _my->_evaluator_registry.register_evaluator< claim_reward_balance_evaluator           >();
        _my->_evaluator_registry.register_evaluator< delegate_qi_evaluator                    >();
        _my->_evaluator_registry.register_evaluator< siming_set_properties_evaluator          >();
        
        _my->_evaluator_registry.register_evaluator< create_contract_evaluator                >();
        _my->_evaluator_registry.register_evaluator< revise_contract_evaluator                >();
        _my->_evaluator_registry.register_evaluator< release_contract_evaluator               >();
        _my->_evaluator_registry.register_evaluator< call_contract_function_evaluator         >();
        
        _my->_evaluator_registry.register_evaluator< action_nfa_evaluator                     >();
    }
    
    void database::register_custom_operation_interpreter( std::shared_ptr< custom_operation_interpreter > interpreter )
    {
        FC_ASSERT( interpreter );
        bool inserted = _custom_operation_interpreters.emplace( interpreter->get_custom_id(), interpreter ).second;
        // This assert triggering means we're mis-configured (multiple registrations of custom JSON evaluator for same ID)
        FC_ASSERT( inserted );
    }
    
    std::shared_ptr< custom_operation_interpreter > database::get_custom_json_evaluator( const custom_id_type& id )
    {
        auto it = _custom_operation_interpreters.find( id );
        if( it != _custom_operation_interpreters.end() )
            return it->second;
        return std::shared_ptr< custom_operation_interpreter >();
    }
    
    void initialize_core_indexes( database& db );
    
    void database::initialize_indexes()
    {
        initialize_core_indexes( *this );
        _plugin_index_signal();
    }
    
    void g_init_tiandao_property_object( tiandao_property_object& tiandao );
    void database::init_genesis(uint64_t init_supply, uint64_t init_qi_supply)
    { try {
        
        struct auth_inhibitor
        {
            auth_inhibitor(database& db) : db(db), old_flags(db.node_properties().skip_flags) { db.node_properties().skip_flags |= skip_authority_check; }
            ~auth_inhibitor() { db.node_properties().skip_flags = old_flags; }
        private:
            database& db;
            uint32_t old_flags;
        } inhibitor(*this);
        
        // Create blockchain accounts
        
        create< account_object >( [&]( account_object& a ) {
            a.name = TAIYI_NULL_ACCOUNT;
        } );
        create< account_authority_object >( [&]( account_authority_object& auth ) {
            auth.account = TAIYI_NULL_ACCOUNT;
            auth.owner.weight_threshold = 1;
            auth.active.weight_threshold = 1;
            auth.posting.weight_threshold = 1;
        });
        
        create< account_object >( [&]( account_object& a ) {
            a.name = TAIYI_DAO_ACCOUNT;
            a.recovery_account = TAIYI_DAO_ACCOUNT;
        } );
        create< account_authority_object >( [&]( account_authority_object& auth ) {
            auth.account = TAIYI_DAO_ACCOUNT;
            auth.owner.weight_threshold = 1;
            auth.active.weight_threshold = 1;
            auth.posting.weight_threshold = 1;
        });
        
        create< account_object >( [&]( account_object& a ) {
            a.name = TAIYI_DANUO_ACCOUNT;
            a.recovery_account = TAIYI_DANUO_ACCOUNT;
        } );
        create< account_authority_object >( [&]( account_authority_object& auth ) {
            auth.account = TAIYI_DANUO_ACCOUNT;
            auth.owner.weight_threshold = 1;
            auth.active.weight_threshold = 1;
            auth.posting.weight_threshold = 1;
        });
        
        create< account_object >( [&]( account_object& a ) {
            a.name = TAIYI_TEMP_ACCOUNT;
        } );
        create< account_authority_object >( [&]( account_authority_object& auth ) {
            auth.account = TAIYI_TEMP_ACCOUNT;
            auth.owner.weight_threshold = 0;
            auth.active.weight_threshold = 0;
            auth.posting.weight_threshold = 1;
        });
        
        public_key_type init_public_key(TAIYI_INIT_PUBLIC_KEY);
        for( int i = 0; i < TAIYI_NUM_INIT_SIMINGS; ++i )
        {
            auto acc_rf = create< account_object >( [&]( account_object& a ) {
                a.name = TAIYI_INIT_SIMING_NAME + ( i ? fc::to_string( i ) : std::string() );
                a.memo_key = init_public_key;
                a.balance  = asset( i ? 0 : init_supply, YANG_SYMBOL );
                a.qi  = asset( i ? 0 : init_qi_supply, QI_SYMBOL );
            } );
            
            create< account_authority_object >( [&]( account_authority_object& auth ) {
                auth.account = TAIYI_INIT_SIMING_NAME + ( i ? fc::to_string( i ) : std::string() );
                auth.owner.add_authority( init_public_key, 1 );
                auth.owner.weight_threshold = 1;
                auth.active  = auth.owner;
                auth.posting = auth.active;
            });
            
            create< siming_object >( [&]( siming_object& w ) {
                w.owner         = TAIYI_INIT_SIMING_NAME + ( i ? fc::to_string(i) : std::string() );
                w.signing_key   = init_public_key;
                w.schedule      = siming_object::miner;
            } );
        }
        
        create< dynamic_global_property_object >( [&]( dynamic_global_property_object& p ) {
            p.current_siming = TAIYI_INIT_SIMING_NAME;
            p.time = TAIYI_GENESIS_TIME;
            p.recent_slots_filled = fc::uint128::max_value();
            p.participation_count = 128;
            p.total_qi = asset( init_qi_supply, QI_SYMBOL );
            p.current_supply = asset( init_supply, YANG_SYMBOL ) + p.total_qi * TAIYI_QI_SHARE_PRICE;
            p.maximum_block_size = TAIYI_MAX_BLOCK_SIZE;
            p.next_maintenance_time = TAIYI_GENESIS_TIME;
            p.xinsu_count = 1;
        } );
        
        create< tiandao_property_object >( [&]( tiandao_property_object& p ) {
            p.cruelty = 0;
            p.enjoyment = 0;
            p.decay = 0;
            p.falsity = 0;
            
            p.v_years = 0;
            p.v_months = 0;
            p.v_days = 0;
            p.v_timeonday = 0;
            p.v_times = 0;
            
            p.next_npc_born_time = TAIYI_GENESIS_TIME;
            g_init_tiandao_property_object(p);
        } );
        
        for( int i = 0; i < 0x10000; i++ )
            create< block_summary_object >( [&]( block_summary_object& ) {});
        
        create< hardfork_property_object >( [&](hardfork_property_object& hpo ) {
            hpo.processed_hardforks.push_back( TAIYI_GENESIS_TIME );
        } );
        
        // Create siming scheduler
        create< siming_schedule_object >( [&]( siming_schedule_object& wso ) {
            wso.current_shuffled_simings[0] = TAIYI_INIT_SIMING_NAME;
        } );
        
        auto content_rf = create< reward_fund_object >( [&]( reward_fund_object& rfo ) {
            rfo.name = TAIYI_CULTIVATION_REWARD_FUND_NAME;
            rfo.last_update = head_block_time();
            rfo.percent_content_rewards = TAIYI_100_PERCENT;
        });
        // As a shortcut in payout processing, we use the id as an array index.
        // The IDs must be assigned this way. The assertion is a dummy check to ensure this happens.
        FC_ASSERT( content_rf.id._id == 0 );
        
        // Create basic contracts such as THE first contract baseENV
        create_basic_contract_objects();
        
        // Create basic contracts such as THE default actor symbol
        create_basic_nfa_symbol_objects();
        create_basic_nfa_objects();

    } FC_CAPTURE_AND_RETHROW() }
    
    void database::validate_transaction( const signed_transaction& trx )
    {
        database::with_write_lock( [&]() {
            auto session = start_undo_session();
            _apply_transaction( trx );
            session.undo();
        });
    }

    void database::notify_changed_objects()
    { try {
    /*
        vector< chainbase::generic_id > ids;
        get_changed_ids( ids );
        TAIYI_TRY_NOTIFY( changed_objects, ids )
    */

    /*
        if( _undo_db.enabled() )
        {
            const auto& head_undo = _undo_db.head();
            vector<object_id_type> changed_ids;  changed_ids.reserve(head_undo.old_values.size());
            for( const auto& item : head_undo.old_values ) changed_ids.push_back(item.first);
            for( const auto& item : head_undo.new_ids ) changed_ids.push_back(item);
            vector<const object*> removed;
            removed.reserve( head_undo.removed.size() );
            for( const auto& item : head_undo.removed )
            {
                changed_ids.push_back( item.first );
                removed.emplace_back( item.second.get() );
            }
            TAIYI_TRY_NOTIFY( changed_objects, changed_ids )
        }
    */
    } FC_CAPTURE_AND_RETHROW() }

    void database::set_flush_interval( uint32_t flush_blocks )
    {
        _flush_blocks = flush_blocks;
        _next_flush_block = 0;
    }
    
    //////////////////// private methods ////////////////////
    
    void database::apply_block( const signed_block& next_block, uint32_t skip )
    { try {
        //fc::time_point begin_time = fc::time_point::now();
        
        detail::with_skip_flags( *this, skip, [&]() {
            _apply_block( next_block );
        } );
        
        try
        {
            // check invariants
            if( /*is_producing() ||*/ !( skip & skip_validate_invariants ) )
                validate_invariants();
        }
        FC_CAPTURE_AND_RETHROW( (next_block) );

        auto block_num = next_block.block_num();

        //fc::time_point end_time = fc::time_point::now();
        //fc::microseconds dt = end_time - begin_time;
        if( _flush_blocks != 0 )
        {
            if( _next_flush_block == 0 )
            {
                uint32_t lep = block_num + 1 + _flush_blocks * 9 / 10;
                uint32_t rep = block_num + 1 + _flush_blocks;
                
                // use time_point::now() as RNG source to pick block randomly between lep and rep
                uint32_t span = rep - lep;
                uint32_t x = lep;
                if( span > 0 )
                {
                    uint64_t now = uint64_t( fc::time_point::now().time_since_epoch().count() );
                    x += now % span;
                }
                _next_flush_block = x;
                //ilog( "Next flush scheduled at block ${b}", ("b", x) );
            }
            
            if( _next_flush_block == block_num )
            {
                _next_flush_block = 0;
                //ilog( "Flushing database state at block ${b}", ("b", block_num) );
                chainbase::database::flush();
            }
        }
        
    } FC_CAPTURE_AND_RETHROW( (next_block) ) }
    
    void database::_apply_block( const signed_block& next_block )
    { try {
        block_notification note( next_block );
        notify_pre_apply_block( note );
        
        const uint32_t next_block_num = note.block_num;
        
        BOOST_SCOPE_EXIT( this_ ) {
            this_->_currently_processing_block_id.reset();
        } BOOST_SCOPE_EXIT_END
        _currently_processing_block_id = note.block_id;
        
        uint32_t skip = get_node_properties().skip_flags;
        
        _current_block_num    = next_block_num;
        _current_trx_in_block = 0;
        _current_virtual_op   = 0;
        
        if( BOOST_UNLIKELY( next_block_num == 1 ) )
        {
            // For every existing before the head_block_time (genesis time), apply the hardfork
            // This allows the test net to launch with past hardforks and apply the next harfork when running
            
            uint32_t n;
            for( n=0; n<TAIYI_NUM_HARDFORKS; n++ )
            {
                if( _hardfork_versions.times[n+1] > next_block.timestamp )
                    break;
            }
            
            if( n > 0 )
            {
                ilog( "Processing ${n} genesis hardforks", ("n", n) );
                set_hardfork( n, true );
                
                const hardfork_property_object& hardfork_state = get_hardfork_property_object();
                FC_ASSERT( hardfork_state.current_hardfork_version == _hardfork_versions.versions[n], "Unexpected genesis hardfork state" );
                
                const auto& siming_idx = get_index<siming_index>().indices().get<by_id>();
                vector<siming_id_type> wit_ids_to_update;
                for( auto it=siming_idx.begin(); it!=siming_idx.end(); ++it )
                    wit_ids_to_update.push_back(it->id);
                
                for( siming_id_type wit_id : wit_ids_to_update )
                {
                    modify( get( wit_id ), [&]( siming_object& wit ) {
                        wit.running_version = _hardfork_versions.versions[n];
                        wit.hardfork_version_vote = _hardfork_versions.versions[n];
                        wit.hardfork_time_vote = _hardfork_versions.times[n];
                    } );
                }
            }
        }
        
        if( !( skip & skip_merkle_check ) )
        {
            auto merkle_root = next_block.calculate_merkle_root();
            
            try
            {
                FC_ASSERT( next_block.transaction_merkle_root == merkle_root, "Merkle check failed", ("next_block.transaction_merkle_root",next_block.transaction_merkle_root)("calc",merkle_root)("next_block",next_block)("id",next_block.id()) );
            }
            catch( const fc::assert_exception& e )
            {
                throw e;
            }
        }
        
        const siming_object& signing_siming = validate_block_header(skip, next_block);
        
        const auto& gprops = get_dynamic_global_properties();
        auto block_size = fc::raw::pack_size( next_block );
        FC_ASSERT( block_size <= gprops.maximum_block_size, "Block Size is too Big", ("next_block_num",next_block_num)("block_size", block_size)("max",gprops.maximum_block_size) );
        if( block_size < TAIYI_MIN_BLOCK_SIZE )
            elog( "Block size is too small", ("next_block_num",next_block_num)("block_size", block_size)("min",TAIYI_MIN_BLOCK_SIZE));
        
        /// modify current siming so transaction evaluators can know who included the transaction,
        modify( gprops, [&]( dynamic_global_property_object& dgp ){
            dgp.current_siming = next_block.siming;
        });
        
        /// parse siming version reporting
        process_header_extensions( next_block );
        
        const auto& siming = get_siming( next_block.siming );
        const auto& hardfork_state = get_hardfork_property_object();
        FC_ASSERT( siming.running_version >= hardfork_state.current_hardfork_version,
                  "Block produced by siming that is not running current hardfork",
                  ("siming",siming)("next_block.siming",next_block.siming)("hardfork_state", hardfork_state)
                  );
        
        for( const auto& trx : next_block.transactions )
        {
            /* We do not need to push the undo state for each transaction
             * because they either all apply and are valid or the
             * entire block fails to apply.  We only need an "undo" state
             * for transactions when validating broadcast transactions or
             * when building a block.
             * 这里是对收到的块（包括自己出的块）中的交易，进行状态修改和应用的确切地方
             * 注意，这里不需要对状态设置回滚保护，因为整个块在应用中任何一个环节出问题，
             * 最外层的回滚保护机制会动作。
             * 然而，为了不影响当前节点的状态，对广播来的交易（自己收交易请求api也走的
             * 广播）的验证，以及在出块时候对打包交易的验证，都是需要专门的回滚操作的。
             */
            apply_transaction( trx, skip );
            ++_current_trx_in_block;
        }
        
        _current_trx_in_block = -1;
        _current_op_in_trx = 0;
        _current_virtual_op = 0;
        
        update_global_dynamic_data(next_block);
        update_signing_siming(signing_siming, next_block);
        
        update_last_irreversible_block();
        
        create_block_summary(next_block);
        clear_expired_transactions();
        clear_expired_delegations();
        
        update_siming_schedule(*this);
                
        process_funds();
        process_qi_withdrawals();
        
        account_recovery_processing();
        process_decline_adoring_rights();

        process_proposals(note);

        process_tiandao();
        process_nfa_tick();
        process_actor_tick();
        
        process_cultivations();

        process_hardforks();
        
        // notify observers that the block has been applied
        notify_post_apply_block(note);
        
        notify_changed_objects();
        
        // This moves newly irreversible blocks from the fork db to the block log
        // and commits irreversible state to the database. This should always be the
        // last call of applying a block because it is the only thing that is not
        // reversible.
        migrate_irreversible_state();
        
        trim_cache();
        
    } FC_CAPTURE_LOG_AND_RETHROW( (next_block.block_num()) ) }

    struct process_header_visitor
    {
        process_header_visitor( const std::string& siming, database& db ) :
        _siming( siming ), _db( db ) {}
        
        typedef void result_type;
        
        const std::string& _siming;
        database& _db;
        
        void operator()( const void_t& obj ) const {}  //Nothing to do.
        
        void operator()( const version& reported_version ) const
        {
            const auto& signing_siming = _db.get_siming( _siming );
            //idump( (next_block.siming)(signing_siming.running_version)(reported_version) );
            if( reported_version != signing_siming.running_version )
            {
                _db.modify( signing_siming, [&]( siming_object& wo ) {
                    wo.running_version = reported_version;
                });
            }
        }
        
        void operator()( const hardfork_version_vote& hfv ) const
        {
            const auto& signing_siming = _db.get_siming( _siming );
            //idump( (next_block.siming)(signing_siming.running_version)(hfv) );
            
            if( hfv.hf_version != signing_siming.hardfork_version_vote || hfv.hf_time != signing_siming.hardfork_time_vote ) {
                _db.modify( signing_siming, [&]( siming_object& wo ) {
                    wo.hardfork_version_vote = hfv.hf_version;
                    wo.hardfork_time_vote = hfv.hf_time;
                });
            }
        }
    };

    void database::process_header_extensions( const signed_block& next_block)
    {
        process_header_visitor _v( next_block.siming, *this );
        for( const auto& e : next_block.extensions )
            e.visit( _v );
    }
    
    void database::apply_transaction(const signed_transaction& trx, uint32_t skip)
    {
        detail::with_skip_flags( *this, skip, [&]() {
            _apply_transaction(trx);
        });
    }
    
    void database::_apply_transaction(const signed_transaction& trx)
    { try {
        transaction_notification note(trx);
        _current_trx_id = note.transaction_id;
        _current_trx = &trx;
        const transaction_id_type& trx_id = note.transaction_id;
        _current_virtual_op = 0;
        
        uint32_t skip = get_node_properties().skip_flags;
        
        if( !(skip&skip_validate) )   /* issue #505 explains why this skip_flag is disabled */
            trx.validate();
        
        auto& trx_idx = get_index<transaction_index>();
        const chain_id_type& chain_id = get_chain_id();
        // idump((trx_id)(skip&skip_transaction_dupe_check));
        FC_ASSERT( (skip & skip_transaction_dupe_check) ||
                  trx_idx.indices().get<by_trx_id>().find(trx_id) == trx_idx.indices().get<by_trx_id>().end(),
                  "Duplicate transaction check failed", ("trx_ix", trx_id) );
        
        if( !(skip & (skip_transaction_signatures | skip_authority_check) ) )
        {
            auto get_active  = [&]( const string& name ) { return authority( get< account_authority_object, by_account >( name ).active ); };
            auto get_owner   = [&]( const string& name ) { return authority( get< account_authority_object, by_account >( name ).owner );  };
            auto get_posting = [&]( const string& name ) { return authority( get< account_authority_object, by_account >( name ).posting );  };
            
            try
            {
                trx.verify_authority( chain_id, get_active, get_owner, get_posting, TAIYI_MAX_SIG_CHECK_DEPTH,
                                     TAIYI_MAX_AUTHORITY_MEMBERSHIP, TAIYI_MAX_SIG_CHECK_ACCOUNTS, fc::ecc::bip_0062);
            }
            catch( protocol::tx_missing_active_auth& e )
            {
                throw e;
            }
        }
        
        //Skip all manner of expiration and TaPoS checking if we're on block 1; It's impossible that the transaction is
        //expired, and TaPoS makes no sense as no blocks exist.
        if( BOOST_LIKELY(head_block_num() > 0) )
        {
            if( !(skip & skip_tapos_check) )
            {
                const auto& tapos_block_summary = get< block_summary_object >( trx.ref_block_num );
                //Verify TaPoS block summary has correct ID prefix, and that this block's time is not past the expiration
                TAIYI_ASSERT( trx.ref_block_prefix == tapos_block_summary.block_id._hash[1], transaction_tapos_exception,
                             "", ("trx.ref_block_prefix", trx.ref_block_prefix)
                             ("tapos_block_summary",tapos_block_summary.block_id._hash[1]));
            }
            
            fc::time_point_sec now = head_block_time();
            
            TAIYI_ASSERT( trx.expiration <= now + fc::seconds(TAIYI_MAX_TIME_UNTIL_EXPIRATION), transaction_expiration_exception,
                         "", ("trx.expiration",trx.expiration)("now",now)("max_til_exp",TAIYI_MAX_TIME_UNTIL_EXPIRATION));
            // Simple solution to pending trx bug when now == trx.expiration
            TAIYI_ASSERT( now < trx.expiration, transaction_expiration_exception, "", ("now",now)("trx.exp",trx.expiration) );
        }
        
        //Insert transaction into unique transactions database.
        const transaction_object* pto = nullptr;
        if( !(skip & skip_transaction_dupe_check) )
        {
            pto = &create<transaction_object>([&](transaction_object& transaction) {
                transaction.trx_id = trx_id;
                transaction.expiration = trx.expiration;
                fc::raw::pack_to_buffer( transaction.packed_trx, trx );
            });
        }
        
        notify_pre_apply_transaction( note );
        
        vector<operation_result> operation_results;
        operation_results.reserve(trx.operations.size());
        
        //Finally process the operations
        _current_op_in_trx = 0;
        for( const auto& op : trx.operations )
        {
            try {
                auto op_result = apply_operation(op);
                operation_results.emplace_back(op_result);
                
                ++_current_op_in_trx;
            } FC_CAPTURE_AND_RETHROW( (op) );
        }
        _current_trx_id = transaction_id_type();
        _current_trx = 0;
        
        if(pto) {
            modify(*pto, [&](transaction_object& transaction) {
                transaction.operation_results = std::move(operation_results);
            });
        }
        
        notify_post_apply_transaction( note );
        
    } FC_CAPTURE_AND_RETHROW( (trx) ) }
    
    operation_result database::apply_operation(const operation& op)
    {
        operation_notification note = create_operation_notification( op );
        notify_pre_apply_operation( note );
        
        if( _benchmark_dumper.is_enabled() )
            _benchmark_dumper.begin();
        
        auto result = _my->_evaluator_registry.get_evaluator( op ).apply( op );
        
        if( _benchmark_dumper.is_enabled() )
            _benchmark_dumper.end< true/*APPLY_CONTEXT*/ >( _my->_evaluator_registry.get_evaluator( op ).get_name( op ) );
        
        notify_post_apply_operation( note );
        
        return result;
    }
    
    template <typename TFunction> struct fcall {};
    
    template <typename TResult, typename... TArgs>
    struct fcall<TResult(TArgs...)>
    {
        using TNotification = std::function<TResult(TArgs...)>;
        
        fcall() = default;
        fcall(const TNotification& func, util::advanced_benchmark_dumper& dumper,
              const abstract_plugin& plugin, const std::string& item_name)
            : _func(func), _benchmark_dumper(dumper)
        {
            _name = plugin.get_name() + item_name;
        }
        
        void operator () (TArgs&&... args)
        {
            if (_benchmark_dumper.is_enabled())
                _benchmark_dumper.begin();
            
            _func(std::forward<TArgs>(args)...);
            
            if (_benchmark_dumper.is_enabled())
                _benchmark_dumper.end(_name);
        }
        
    private:
        TNotification                    _func;
        util::advanced_benchmark_dumper& _benchmark_dumper;
        std::string                      _name;
    };

    template <typename TResult, typename... TArgs>
    struct fcall<std::function<TResult(TArgs...)>>
        : public fcall<TResult(TArgs...)>
    {
        typedef fcall<TResult(TArgs...)> TBase;
        using TBase::TBase;
    };
    
    template <typename TSignal, typename TNotification>
    boost::signals2::connection database::connect_impl( TSignal& signal, const TNotification& func, const abstract_plugin& plugin, int32_t group, const std::string& item_name )
    {
        fcall<TNotification> fcall_wrapper(func,_benchmark_dumper,plugin,item_name);
        
        return signal.connect(group, fcall_wrapper);
    }
    
    template< bool IS_PRE_OPERATION >
    boost::signals2::connection database::any_apply_operation_handler_impl( const apply_operation_handler_t& func, const abstract_plugin& plugin, int32_t group )
    {
        auto complex_func = [this, func, &plugin]( const operation_notification& o )
        {
            std::string name;
            
            if (_benchmark_dumper.is_enabled())
            {
                if( _my->_evaluator_registry.is_evaluator( o.op ) )
                    name = _benchmark_dumper.generate_desc< IS_PRE_OPERATION >( plugin.get_name(), _my->_evaluator_registry.get_evaluator( o.op ).get_name( o.op ) );
                else
                    name = util::advanced_benchmark_dumper::get_virtual_operation_name();
                
                _benchmark_dumper.begin();
            }
            
            func( o );
            
            if (_benchmark_dumper.is_enabled())
                _benchmark_dumper.end( name );
        };
        
        if( IS_PRE_OPERATION )
            return _pre_apply_operation_signal.connect(group, complex_func);
        else
            return _post_apply_operation_signal.connect(group, complex_func);
    }

    boost::signals2::connection database::add_pre_apply_operation_handler( const apply_operation_handler_t& func, const abstract_plugin& plugin, int32_t group )
    {
        return any_apply_operation_handler_impl< true/*IS_PRE_OPERATION*/ >( func, plugin, group );
    }

    boost::signals2::connection database::add_post_apply_operation_handler( const apply_operation_handler_t& func, const abstract_plugin& plugin, int32_t group )
    {
        return any_apply_operation_handler_impl< false/*IS_PRE_OPERATION*/ >( func, plugin, group );
    }
    
    boost::signals2::connection database::add_pre_apply_transaction_handler( const apply_transaction_handler_t& func, const abstract_plugin& plugin, int32_t group )
    {
        return connect_impl(_pre_apply_transaction_signal, func, plugin, group, "->transaction");
    }
    
    boost::signals2::connection database::add_post_apply_transaction_handler( const apply_transaction_handler_t& func, const abstract_plugin& plugin, int32_t group )
    {
        return connect_impl(_post_apply_transaction_signal, func, plugin, group, "<-transaction");
    }
    
    boost::signals2::connection database::add_pre_apply_block_handler( const apply_block_handler_t& func, const abstract_plugin& plugin, int32_t group )
    {
        return connect_impl(_pre_apply_block_signal, func, plugin, group, "->block");
    }

    boost::signals2::connection database::add_post_apply_block_handler( const apply_block_handler_t& func, const abstract_plugin& plugin, int32_t group )
    {
        return connect_impl(_post_apply_block_signal, func, plugin, group, "<-block");
    }
    
    boost::signals2::connection database::add_irreversible_block_handler( const irreversible_block_handler_t& func, const abstract_plugin& plugin, int32_t group )
    {
        return connect_impl(_on_irreversible_block, func, plugin, group, "<-irreversible");
    }
    
    boost::signals2::connection database::add_pre_reindex_handler(const reindex_handler_t& func, const abstract_plugin& plugin, int32_t group )
    {
        return connect_impl(_pre_reindex_signal, func, plugin, group, "->reindex");
    }
    
    boost::signals2::connection database::add_post_reindex_handler(const reindex_handler_t& func, const abstract_plugin& plugin, int32_t group )
    {
        return connect_impl(_post_reindex_signal, func, plugin, group, "<-reindex");
    }

    const siming_object& database::validate_block_header( uint32_t skip, const signed_block& next_block )const
    { try {
        FC_ASSERT( head_block_id() == next_block.previous, "", ("head_block_id",head_block_id())("next.prev",next_block.previous) );
        FC_ASSERT( head_block_time() < next_block.timestamp, "", ("head_block_time",head_block_time())("next",next_block.timestamp)("blocknum",next_block.block_num()) );
        const siming_object& siming = get_siming( next_block.siming );
        
        if( !(skip&skip_siming_signature) )
            FC_ASSERT( next_block.validate_signee( siming.signing_key, fc::ecc::bip_0062 ) );
        
        if( !(skip&skip_siming_schedule_check) )
        {
            uint32_t slot_num = get_slot_at_time( next_block.timestamp );
            FC_ASSERT( slot_num > 0 );
            
            string scheduled_siming = get_scheduled_siming( slot_num );
            FC_ASSERT( siming.owner == scheduled_siming, "Siming produced block at wrong time",
                      ("block siming",next_block.siming)("scheduled",scheduled_siming)("slot_num",slot_num) );
        }
        
        return siming;
    } FC_CAPTURE_AND_RETHROW() }
    
    void database::create_block_summary(const signed_block& next_block)
    { try {
        block_summary_id_type sid( next_block.block_num() & 0xffff );
        modify( get< block_summary_object >( sid ), [&](block_summary_object& p) {
            p.block_id = next_block.id();
        });
    } FC_CAPTURE_AND_RETHROW() }
    
    void database::update_global_dynamic_data( const signed_block& b )
    { try {
        const dynamic_global_property_object& _dgp = get_dynamic_global_properties();
        
        uint32_t missed_blocks = 0;
        if( head_block_time() != fc::time_point_sec() )
        {
            missed_blocks = get_slot_at_time( b.timestamp );
            assert( missed_blocks != 0 );
            missed_blocks--;
            for( uint32_t i = 0; i < missed_blocks; ++i )
            {
                const auto& siming_missed = get_siming( get_scheduled_siming( i + 1 ) );
                if(  siming_missed.owner != b.siming )
                {
                    modify( siming_missed, [&]( siming_object& w ) {
                        w.total_missed++;
                        if( head_block_num() - w.last_confirmed_block_num  > TAIYI_BLOCKS_PER_DAY )
                        {
                            w.signing_key = public_key_type();
                            push_virtual_operation( shutdown_siming_operation( w.owner ) );
                        }
                    } );
                }
            }
        }
        
        // dynamic global properties updating
        modify( _dgp, [&]( dynamic_global_property_object& dgp ) {
            // This is constant time assuming 100% participation. It is O(B) otherwise (B = Num blocks between update)
            for( uint32_t i = 0; i < missed_blocks + 1; i++ )
            {
                dgp.participation_count -= dgp.recent_slots_filled.hi & 0x8000000000000000ULL ? 1 : 0;
                dgp.recent_slots_filled = ( dgp.recent_slots_filled << 1 ) + ( i == 0 ? 1 : 0 );
                dgp.participation_count += ( i == 0 ? 1 : 0 );
            }
            
            dgp.head_block_number = b.block_num();
            // Following FC_ASSERT should never fail, as _currently_processing_block_id is always set by caller
            FC_ASSERT( _currently_processing_block_id.valid() );
            dgp.head_block_id = *_currently_processing_block_id;
            dgp.time = b.timestamp;
            dgp.current_aslot += missed_blocks+1;
        } );
        
        if( !(get_node_properties().skip_flags & skip_undo_history_check) )
        {
            TAIYI_ASSERT( _dgp.head_block_number - _dgp.last_irreversible_block_num  < TAIYI_MAX_UNDO_HISTORY, undo_database_exception, "The database does not have enough undo history to support a blockchain with so many missed blocks. Please add a checkpoint if you would like to continue applying blocks beyond this point.", ("last_irreversible_block_num",_dgp.last_irreversible_block_num)("head", _dgp.head_block_number)("max_undo",TAIYI_MAX_UNDO_HISTORY) );
        }
    } FC_CAPTURE_AND_RETHROW() }
        
    void database::update_signing_siming(const siming_object& signing_siming, const signed_block& new_block)
    { try {
        const dynamic_global_property_object& dpo = get_dynamic_global_properties();
        uint64_t new_block_aslot = dpo.current_aslot + get_slot_at_time( new_block.timestamp );
        
        modify( signing_siming, [&]( siming_object& _wit ) {
            _wit.last_aslot = new_block_aslot;
            _wit.last_confirmed_block_num = new_block.block_num();
        } );
    } FC_CAPTURE_AND_RETHROW() }

    void database::update_last_irreversible_block()
    { try {
        const dynamic_global_property_object& dpo = get_dynamic_global_properties();
        auto old_last_irreversible = dpo.last_irreversible_block_num;
        
        /**
         * Prior to adoring taking over, we must be more conservative...
         */
        if( head_block_num() < TAIYI_START_ADORING_BLOCK )
        {
            modify( dpo, [&]( dynamic_global_property_object& _dpo ) {
                if ( head_block_num() > TAIYI_MAX_SIMINGS )
                    _dpo.last_irreversible_block_num = head_block_num() - TAIYI_MAX_SIMINGS;
            } );
        }
        else
        {
            const siming_schedule_object& wso = get_siming_schedule_object();
            
            vector< const siming_object* > wit_objs;
            wit_objs.reserve( wso.num_scheduled_simings );
            for( int i = 0; i < wso.num_scheduled_simings; i++ )
                wit_objs.push_back( &get_siming( wso.current_shuffled_simings[i] ) );
            
            static_assert( TAIYI_IRREVERSIBLE_THRESHOLD > 0, "irreversible threshold must be nonzero" );
            
            // 1 1 1 2 2 2 2 2 2 2 -> 2     .7*10 = 7
            // 1 1 1 1 1 1 1 2 2 2 -> 1
            // 3 3 3 3 3 3 3 3 3 3 -> 3
            
            size_t offset = ((TAIYI_100_PERCENT - TAIYI_IRREVERSIBLE_THRESHOLD) * wit_objs.size() / TAIYI_100_PERCENT);
            
            std::nth_element( wit_objs.begin(), wit_objs.begin() + offset, wit_objs.end(),
                             []( const siming_object* a, const siming_object* b ) {
                return a->last_confirmed_block_num < b->last_confirmed_block_num;
            } );
            
            uint32_t new_last_irreversible_block_num = wit_objs[offset]->last_confirmed_block_num;
            
            if( new_last_irreversible_block_num > dpo.last_irreversible_block_num )
            {
                modify( dpo, [&]( dynamic_global_property_object& _dpo ) {
                    _dpo.last_irreversible_block_num = new_last_irreversible_block_num;
                } );
            }
        }
        
        for( uint32_t i = old_last_irreversible; i <= dpo.last_irreversible_block_num; ++i )
        {
            notify_irreversible_block( i );
        }
    } FC_CAPTURE_AND_RETHROW() }
    
    // This method should happen atomically. We cannot prevent unclean shutdown in the middle
    // of the call, but all side effects happen at the end to minize the chance that state
    // invariants will be violated.
    void database::migrate_irreversible_state()
    { try {
        const dynamic_global_property_object& dpo = get_dynamic_global_properties();
        
        auto fork_head = _fork_db.head();
        if( fork_head )
        {
            FC_ASSERT( fork_head->num == dpo.head_block_number, "Fork Head: ${f} Chain Head: ${c}", ("f",fork_head->num)("c", dpo.head_block_number) );
        }
        
        if( !( get_node_properties().skip_flags & skip_block_log ) )
        {
            // output to block log based on new last irreverisible block num
            const auto& tmp_head = _block_log.head();
            uint64_t log_head_num = 0;
            vector< item_ptr > blocks_to_write;
            
            if( tmp_head )
                log_head_num = tmp_head->block_num();
            
            if( log_head_num < dpo.last_irreversible_block_num )
            {
                // Check for all blocks that we want to write out to the block log but don't write any
                // unless we are certain they all exist in the fork db
                while( log_head_num < dpo.last_irreversible_block_num )
                {
                    item_ptr block_ptr = _fork_db.fetch_block_on_main_branch_by_number( log_head_num+1 );
                    FC_ASSERT( block_ptr, "Current fork in the fork database does not contain the last_irreversible_block" );
                    blocks_to_write.push_back( block_ptr );
                    log_head_num++;
                }
                
                for( auto block_itr = blocks_to_write.begin(); block_itr != blocks_to_write.end(); ++block_itr )
                {
                    _block_log.append( block_itr->get()->data );
                }
                
                _block_log.flush();
            }
        }
        
        // This deletes blocks from the fork db
        _fork_db.set_max_size( dpo.head_block_number - dpo.last_irreversible_block_num + 1 );
        
        // This deletes undo state
        commit( dpo.last_irreversible_block_num );
    } FC_CAPTURE_AND_RETHROW() }
    
    void database::clear_expired_transactions()
    {
        //Look for expired transactions in the deduplication list, and remove them.
        //Transactions must have expired by at least two forking windows in order to be removed.
        auto& transaction_idx = get_index< transaction_index >();
        const auto& dedupe_index = transaction_idx.indices().get< by_expiration >();
        while( ( !dedupe_index.empty() ) && ( head_block_time() > dedupe_index.begin()->expiration ) )
            remove( *dedupe_index.begin() );
    }
    
    void database::clear_expired_delegations()
    {
        auto now = head_block_time();
        const auto& delegations_by_exp = get_index< qi_delegation_expiration_index, by_expiration >();
        auto itr = delegations_by_exp.begin();
        const auto& gpo = get_dynamic_global_properties();
        
        while( itr != delegations_by_exp.end() && itr->expiration < now )
        {
            operation vop = return_qi_delegation_operation( itr->delegator, itr->qi );
            try{
                pre_push_virtual_operation( vop );
                
                modify( get_account( itr->delegator ), [&]( account_object& a ) {
                    a.delegated_qi -= itr->qi;
                });
                
                post_push_virtual_operation( vop );
                
                remove( *itr );
                itr = delegations_by_exp.begin();
            } FC_CAPTURE_AND_RETHROW( (vop) )
        }
    }
    
    template< typename asset_balance_object_type, class balance_operator_type >
    void database::adjust_asset_balance( const account_name_type& name, const asset& delta, bool check_account, balance_operator_type balance_operator )
    {
        asset_symbol_type liquid_symbol = delta.symbol.is_qi() ? delta.symbol.get_paired_symbol() : delta.symbol;
        const asset_balance_object_type* bo = find< asset_balance_object_type, by_owner_liquid_symbol >( boost::make_tuple( name, liquid_symbol ) );
        
        if( bo == nullptr )
        {
            // No balance object related to the FA means '0' balance. Check delta to avoid creation of negative balance.
            FC_ASSERT( delta.amount.value >= 0, "Insufficient FA ${a} funds", ("a", delta.symbol) );
            // No need to create object with '0' balance (see comment above).
            if( delta.amount.value == 0 )
                return;
            
            if( check_account )
                get_account( name );
            
            create< asset_balance_object_type >( [&]( asset_balance_object_type& asset_balance ) {
                asset_balance.clear_balance( liquid_symbol );
                asset_balance.owner = name;
                balance_operator.add_to_balance( asset_balance );
                asset_balance.validate();
            } );
        }
        else
        {
            bool is_all_zero = false;
            int64_t result = balance_operator.get_combined_balance( bo, &is_all_zero );
            // Check result to avoid negative balance storing.
            FC_ASSERT( result >= 0, "Insufficient FA ${a} funds", ( "a", delta.symbol ) );
            
            // Exit if whole balance becomes zero.
            if( is_all_zero )
            {
                // Zero balance is the same as non object balance at all.
                // Remove balance object if both liquid and qi balances are zero.
                remove( *bo );
            }
            else
            {
                modify( *bo, [&]( asset_balance_object_type& asset_balance ) {
                    balance_operator.add_to_balance( asset_balance );
                } );
            }
        }
    }
    
    void database::modify_balance( const account_object& a, const asset& delta, bool check_balance )
    {
        modify( a, [&]( account_object& acnt ) {
            switch( delta.symbol.asset_num )
            {
                case TAIYI_ASSET_NUM_YANG:
                    acnt.balance += delta;
                    if( check_balance )
                        FC_ASSERT( acnt.balance.amount.value >= 0, "Insufficient YANG funds" );
                    break;
                case TAIYI_ASSET_NUM_QI:
                    acnt.qi += delta;
                    if( check_balance )
                        FC_ASSERT( acnt.qi.amount.value >= 0, "Insufficient QI funds" );
                    break;
                default:
                    FC_ASSERT( false, "invalid symbol" );
            }
        } );
    }

    void database::modify_reward_balance( const account_object& a, const asset& value_delta, const asset& share_delta, const asset& feigang_delta, bool check_balance )
    {
        modify( a, [&]( account_object& acnt ) {
            switch( value_delta.symbol.asset_num )
            {
                case TAIYI_ASSET_NUM_YANG:
                {
                    if( share_delta.amount.value == 0 )
                    {
                        acnt.reward_yang_balance += value_delta;
                        if( check_balance )
                            FC_ASSERT( acnt.reward_yang_balance.amount.value >= 0, "Insufficient reward YANG funds" );
                    }
                    else
                    {
                        acnt.reward_qi_balance += share_delta;
                        if( check_balance )
                            FC_ASSERT( acnt.reward_qi_balance.amount.value >= 0, "Insufficient reward QI funds" );
                    }

                    acnt.reward_feigang_balance += feigang_delta;
                    if( check_balance )
                        FC_ASSERT( acnt.reward_feigang_balance.amount.value >= 0, "Insufficient reward QI (Feigang) funds" );
                    
                    break;
                }
                default:
                    FC_ASSERT( false, "invalid value symbol" );
            }
        });
    }

    void database::set_index_delegate( const std::string& n, index_delegate&& d )
    {
        _index_delegate_map[ n ] = std::move( d );
    }
    
    const index_delegate& database::get_index_delegate( const std::string& n )
    {
        return _index_delegate_map.at( n );
    }
    
    bool database::has_index_delegate( const std::string& n )
    {
        return _index_delegate_map.find( n ) != _index_delegate_map.end();
    }
    
    const index_delegate_map& database::index_delegates()
    {
        return _index_delegate_map;
    }
    
    struct asset_regular_balance_operator
    {
        asset_regular_balance_operator( const asset& delta ) : delta(delta), is_qi(delta.symbol.is_qi()) {}
        
        void add_to_balance( account_regular_balance_object& bo )
        {
            if( is_qi )
                bo.qi += delta;
            else
                bo.liquid += delta;
        }
        int64_t get_combined_balance( const account_regular_balance_object* bo, bool* is_all_zero )
        {
            asset result = is_qi ? bo->qi + delta : bo->liquid + delta;
            *is_all_zero = result.amount.value == 0 && (is_qi ? bo->liquid.amount.value : bo->qi.amount.value) == 0;
            return result.amount.value;
        }
        
        asset delta;
        bool  is_qi;
    };

    struct asset_reward_balance_operator
    {
        asset_reward_balance_operator( const asset& liquid_delta, const asset& share_delta )
        : liquid_delta(liquid_delta), share_delta(share_delta), is_qi(share_delta.amount.value != 0)
        {
            FC_ASSERT( liquid_delta.symbol.is_qi() == false && share_delta.symbol.is_qi() );
        }
        
        void add_to_balance( account_rewards_balance_object& bo )
        {
            if( is_qi )
                bo.pending_qi += share_delta;
            else
                bo.pending_liquid += liquid_delta;
        }
        int64_t get_combined_balance( const account_rewards_balance_object* bo, bool* is_all_zero )
        {
            asset result = is_qi ? (bo->pending_qi * TAIYI_QI_SHARE_PRICE + liquid_delta) : (bo->pending_liquid + liquid_delta);
            *is_all_zero = result.amount.value == 0 && (is_qi ? bo->pending_liquid.amount.value : bo->pending_qi.amount.value) == 0;
            return result.amount.value;
        }
        
        asset liquid_delta;
        asset share_delta;
        bool  is_qi;
    };
    
    void database::adjust_balance( const account_object& a, const asset& delta )
    {
        if ( delta.amount < 0 )
        {
            asset available = get_balance( a, delta.symbol );
            FC_ASSERT( available >= -delta,
                      "Account ${acc} does not have sufficient funds for balance adjustment. Required: ${r}, Available: ${a}",
                      ("acc", a.name)("r", delta)("a", available) );
        }
        
        if( delta.symbol.space() == asset_symbol_type::fai_space
           || delta.symbol.asset_num == TAIYI_ASSET_NUM_GOLD
           || delta.symbol.asset_num == TAIYI_ASSET_NUM_FOOD
           || delta.symbol.asset_num == TAIYI_ASSET_NUM_WOOD
           || delta.symbol.asset_num == TAIYI_ASSET_NUM_FABRIC
           || delta.symbol.asset_num == TAIYI_ASSET_NUM_HERB )
        {
            // No account object modification for asset balance, hence separate handling here.
            asset_regular_balance_operator balance_operator( delta );
            adjust_asset_balance< account_regular_balance_object >( a.name, delta, false, balance_operator );
        }
        else
            modify_balance( a, delta, true );
    }

    void database::adjust_balance( const account_name_type& name, const asset& delta )
    {
        if ( delta.amount < 0 )
        {
            asset available = get_balance( name, delta.symbol );
            FC_ASSERT( available >= -delta,
                      "Account ${acc} does not have sufficient funds for balance adjustment. Required: ${r}, Available: ${a}",
                      ("acc", name)("r", delta)("a", available) );
        }
        
        if( delta.symbol.space() == asset_symbol_type::fai_space
           || delta.symbol.asset_num == TAIYI_ASSET_NUM_GOLD
           || delta.symbol.asset_num == TAIYI_ASSET_NUM_FOOD
           || delta.symbol.asset_num == TAIYI_ASSET_NUM_WOOD
           || delta.symbol.asset_num == TAIYI_ASSET_NUM_FABRIC
           || delta.symbol.asset_num == TAIYI_ASSET_NUM_HERB )
        {
            // No account object modification for asset balance, hence separate handling here.
            asset_regular_balance_operator balance_operator( delta );
            adjust_asset_balance< account_regular_balance_object >( name, delta, true, balance_operator );
        }
        else
            modify_balance( get_account( name ), delta, true );
    }
    
    void database::adjust_reward_balance( const account_object& a, const asset& liquid_delta, const asset& share_delta, const asset& feigang_delta )
    {
        FC_ASSERT( liquid_delta.symbol.is_qi() == false && share_delta.symbol.is_qi() && feigang_delta.symbol.is_qi() );
        
        // No account object modification for asset balance, hence separate handling here.
        if( liquid_delta.symbol.space() == asset_symbol_type::fai_space
           || liquid_delta.symbol.asset_num == TAIYI_ASSET_NUM_GOLD
           || liquid_delta.symbol.asset_num == TAIYI_ASSET_NUM_FOOD
           || liquid_delta.symbol.asset_num == TAIYI_ASSET_NUM_WOOD
           || liquid_delta.symbol.asset_num == TAIYI_ASSET_NUM_FABRIC
           || liquid_delta.symbol.asset_num == TAIYI_ASSET_NUM_HERB )
        {
            FC_ASSERT(feigang_delta.amount.value == 0, "not support feigang");
            asset_reward_balance_operator balance_operator( liquid_delta, share_delta );
            adjust_asset_balance< account_rewards_balance_object >( a.name, liquid_delta, false, balance_operator );
        }
        else
            modify_reward_balance(a, liquid_delta, share_delta, feigang_delta, true);
    }
    
    void database::adjust_reward_balance( const account_name_type& name, const asset& liquid_delta, const asset& share_delta, const asset& feigang_delta )
    {
        FC_ASSERT( liquid_delta.symbol.is_qi() == false && share_delta.symbol.is_qi() && feigang_delta.symbol.is_qi() );
        
        // No account object modification for asset balance, hence separate handling here.
        if( liquid_delta.symbol.space() == asset_symbol_type::fai_space
           || liquid_delta.symbol.asset_num == TAIYI_ASSET_NUM_GOLD
           || liquid_delta.symbol.asset_num == TAIYI_ASSET_NUM_FOOD
           || liquid_delta.symbol.asset_num == TAIYI_ASSET_NUM_WOOD
           || liquid_delta.symbol.asset_num == TAIYI_ASSET_NUM_FABRIC
           || liquid_delta.symbol.asset_num == TAIYI_ASSET_NUM_HERB )
        {
            FC_ASSERT(feigang_delta.amount.value == 0, "not support feigang");
            asset_reward_balance_operator balance_operator( liquid_delta, share_delta );
            adjust_asset_balance< account_rewards_balance_object >( name, liquid_delta, true, balance_operator );
        }
        else
            modify_reward_balance(get_account( name ), liquid_delta, share_delta, feigang_delta, true);
    }
        
    asset database::get_balance( const account_object& a, asset_symbol_type symbol )const
    {
        switch( symbol.asset_num )
        {
            case TAIYI_ASSET_NUM_YANG:
                return a.balance;
            case TAIYI_ASSET_NUM_QI:
                return a.qi;
            default:
            {
                FC_ASSERT( symbol.asset_num == TAIYI_ASSET_NUM_GOLD ||
                          symbol.asset_num == TAIYI_ASSET_NUM_FOOD ||
                          symbol.asset_num == TAIYI_ASSET_NUM_WOOD ||
                          symbol.asset_num == TAIYI_ASSET_NUM_FABRIC ||
                          symbol.asset_num == TAIYI_ASSET_NUM_HERB ||
                          symbol.space() == asset_symbol_type::fai_space,
                          "Invalid symbol: ${s}", ("s", symbol) );
                auto key = boost::make_tuple( a.name, symbol.is_qi() ? symbol.get_paired_symbol() : symbol );
                const account_regular_balance_object* arbo = find< account_regular_balance_object, by_owner_liquid_symbol >( key );
                if( arbo == nullptr )
                {
                    return asset(0, symbol);
                }
                else
                {
                    return symbol.is_qi() ? arbo->qi : arbo->liquid;
                }
            }
        }
    }
    
    asset database::get_balance( const account_name_type& name, asset_symbol_type symbol )const
    {
        return get_balance( get_account( name ), symbol );
    }
    
    void database::init_hardforks()
    {
        _hardfork_versions.times[ 0 ] = fc::time_point_sec( TAIYI_GENESIS_TIME );
        _hardfork_versions.versions[ 0 ] = hardfork_version( 0, 0 );
        
        const auto& hardforks = get_hardfork_property_object();
        FC_ASSERT( hardforks.last_hardfork <= TAIYI_NUM_HARDFORKS, "Chain knows of more hardforks than configuration", ("hardforks.last_hardfork",hardforks.last_hardfork)("TAIYI_NUM_HARDFORKS",TAIYI_NUM_HARDFORKS) );
        FC_ASSERT( _hardfork_versions.versions[ hardforks.last_hardfork ] <= TAIYI_BLOCKCHAIN_VERSION, "Blockchain version is older than last applied hardfork" );
        FC_ASSERT( TAIYI_BLOCKCHAIN_HARDFORK_VERSION >= TAIYI_BLOCKCHAIN_VERSION );
        FC_ASSERT( TAIYI_BLOCKCHAIN_HARDFORK_VERSION == _hardfork_versions.versions[ TAIYI_NUM_HARDFORKS ] );
    }
    
    void database::process_hardforks()
    { try {
        // If there are upcoming hardforks and the next one is later, do nothing
        const auto& hardforks = get_hardfork_property_object();
        while( _hardfork_versions.versions[ hardforks.last_hardfork ] < hardforks.next_hardfork && hardforks.next_hardfork_time <= head_block_time() )
        {
            if( hardforks.last_hardfork < TAIYI_NUM_HARDFORKS ) {
                apply_hardfork( hardforks.last_hardfork + 1 );
            }
            else
                throw unknown_hardfork_exception();
        }
    } FC_CAPTURE_AND_RETHROW() }
    
    bool database::has_hardfork( uint32_t hardfork )const
    {
        return get_hardfork_property_object().processed_hardforks.size() > hardfork;
    }
    
    uint32_t database::get_hardfork()const
    {
        return get_hardfork_property_object().processed_hardforks.size() - 1;
    }
    
    void database::set_hardfork( uint32_t hardfork, bool apply_now )
    {
        auto const& hardforks = get_hardfork_property_object();
        
        for( uint32_t i = hardforks.last_hardfork + 1; i <= hardfork && i <= TAIYI_NUM_HARDFORKS; i++ )
        {
            modify( hardforks, [&]( hardfork_property_object& hpo ) {
                hpo.next_hardfork = _hardfork_versions.versions[i];
                hpo.next_hardfork_time = head_block_time();
            } );
            
            if( apply_now )
                apply_hardfork( i );
        }
    }
    
    void database::apply_hardfork( uint32_t hardfork )
    {
        if( _log_hardforks )
            elog( "HARDFORK ${hf} at block ${b}", ("hf", hardfork)("b", head_block_num()) );
        
        operation hardfork_vop = hardfork_operation( hardfork );
        pre_push_virtual_operation( hardfork_vop );
        
        switch( hardfork )
        {
            default:
                break;
        }
        
        modify( get_hardfork_property_object(), [&]( hardfork_property_object& hfp ) {
            FC_ASSERT( hardfork == hfp.last_hardfork + 1, "Hardfork being applied out of order",  ("hardfork",hardfork)("hfp.last_hardfork",hfp.last_hardfork) );
            FC_ASSERT( hfp.processed_hardforks.size() == hardfork, "Hardfork being applied out of order" );
            hfp.processed_hardforks.push_back( _hardfork_versions.times[ hardfork ] );
            hfp.last_hardfork = hardfork;
            hfp.current_hardfork_version = _hardfork_versions.versions[ hardfork ];
            FC_ASSERT( hfp.processed_hardforks[ hfp.last_hardfork ] == _hardfork_versions.times[ hfp.last_hardfork ], "Hardfork processing failed sanity check..." );
        } );
        
        post_push_virtual_operation( hardfork_vop );
    }

    /**
     * Verifies all supply invariantes check out
     */
    void database::validate_invariants()const
    { try {
        asset total_supply = asset(0, YANG_SYMBOL);
        share_type total_vsf_adores = share_type( 0 );
        
        asset total_gold = asset(0, GOLD_SYMBOL);
        asset total_food = asset(0, FOOD_SYMBOL);
        asset total_wood = asset(0, WOOD_SYMBOL);
        asset total_fabric = asset(0, FABRIC_SYMBOL);
        asset total_herb = asset(0, HERB_SYMBOL);

        auto gpo = get_dynamic_global_properties();
        
        // verify no siming has too many adores
        const auto& siming_idx = get_index< siming_index >().indices();
        for( auto itr = siming_idx.begin(); itr != siming_idx.end(); ++itr )
            FC_ASSERT(itr->adores <= gpo.total_qi.amount, "核对司命收到信仰总值的合理性失败");
        
        asset total_account_qi = asset(0, QI_SYMBOL);
        asset total_reward_qi = asset(0, QI_SYMBOL);
        asset total_reward_feigang = asset(0, QI_SYMBOL);
        const auto& account_idx = get_index< account_index, by_name >();
        for( auto itr = account_idx.begin(); itr != account_idx.end(); ++itr )
        {
            total_supply += itr->balance;
            total_supply += itr->reward_yang_balance;
            
            total_account_qi += itr->qi;
            total_reward_qi += itr->reward_qi_balance;
            total_reward_feigang += itr->reward_feigang_balance;
            
            total_gold += get_balance(*itr, GOLD_SYMBOL);
            total_food += get_balance(*itr, FOOD_SYMBOL);
            total_wood += get_balance(*itr, WOOD_SYMBOL);
            total_fabric += get_balance(*itr, FABRIC_SYMBOL);
            total_herb += get_balance(*itr, HERB_SYMBOL);
            
            total_vsf_adores += ( itr->proxy == TAIYI_PROXY_TO_SELF_ACCOUNT ?
                itr->siming_adore_weight() :
                ( TAIYI_MAX_PROXY_RECURSION_DEPTH > 0 ?
                    itr->proxied_vsf_adores[TAIYI_MAX_PROXY_RECURSION_DEPTH - 1] :
                    itr->qi.amount
                )
            );
        }

        FC_ASSERT(total_account_qi.amount == total_vsf_adores, "核对由所有账号产生的信仰总值失败", ("total_account_qi", total_account_qi)("total_vsf_adores", total_vsf_adores));
        FC_ASSERT(gpo.pending_rewarded_feigang == total_reward_feigang, "核对所有账号的未领取非罡失败", ("gpo.pending_rewarded_feigang", gpo.pending_rewarded_feigang)("total_reward_feigang", total_reward_feigang));

        asset total_nfa_qi = asset(0, QI_SYMBOL);
        asset total_cultivation_qi = asset(0, QI_SYMBOL);
        const auto& nfa_idx = get_index< nfa_index, by_id >();
        for( auto itr = nfa_idx.begin(); itr != nfa_idx.end(); ++itr)
        {
            total_supply += get_nfa_balance(*itr, YANG_SYMBOL);
            total_nfa_qi += itr->qi;
            
            total_gold += get_nfa_balance(*itr, GOLD_SYMBOL);
            total_food += get_nfa_balance(*itr, FOOD_SYMBOL);
            total_wood += get_nfa_balance(*itr, WOOD_SYMBOL);
            total_fabric += get_nfa_balance(*itr, FABRIC_SYMBOL);
            total_herb += get_nfa_balance(*itr, HERB_SYMBOL);

            total_cultivation_qi.amount += itr->cultivation_value;
        }
        
        const auto& nfa_material_idx = get_index< nfa_material_index, by_id >();
        for( auto itr = nfa_material_idx.begin(); itr != nfa_material_idx.end(); ++itr)
        {
            total_gold += itr->gold;
            total_food += itr->food;
            total_wood += itr->wood;
            total_fabric += itr->fabric;
            total_herb += itr->herb;
        }

        FC_ASSERT(gpo.pending_cultivation_qi == total_cultivation_qi, "核对参与修真的真气总量失败", ("gpo.pending_cultivation_qi", gpo.pending_cultivation_qi)("total_cultivation_qi", total_cultivation_qi));

        const auto& reward_idx = get_index< reward_fund_index, by_id >();
        for( auto itr = reward_idx.begin(); itr != reward_idx.end(); ++itr ) {
            total_supply += itr->reward_balance;
            total_reward_qi += itr->reward_qi_balance;
        }

        FC_ASSERT(gpo.pending_rewarded_qi == total_reward_qi, "核对修真奖励池和账号未领取修真奖励真气失败", ("gpo.pending_rewarded_qi", gpo.pending_rewarded_qi)("total_reward_qi", total_reward_qi));

        FC_ASSERT(gpo.total_qi == (total_account_qi + total_nfa_qi), "核对自由真气总量失败", ("gpo.total_qi", gpo.total_qi)("total_account_qi", total_account_qi)("total_nfa_qi", total_nfa_qi));
        
        //统计所有非阳寿相关的等价真气，避免最后换算阳寿时候的整型误差
        asset total_qi = total_account_qi + total_reward_feigang + total_nfa_qi + total_cultivation_qi + total_reward_qi;
        //换算物质到等价真气
        total_qi += total_gold * TAIYI_GOLD_QI_PRICE;
        total_qi += total_food * TAIYI_FOOD_QI_PRICE;
        total_qi += total_wood * TAIYI_WOOD_QI_PRICE;
        total_qi += total_fabric * TAIYI_FABRIC_QI_PRICE;
        total_qi += total_herb * TAIYI_HERB_QI_PRICE;

        //统计以阳寿表示的全局总量
        total_supply += total_qi * TAIYI_QI_SHARE_PRICE;
        FC_ASSERT(gpo.current_supply == total_supply, "核对系统总阳寿量失败", ("gpo.current_supply", gpo.current_supply)("total_supply", total_supply));
        
        //核对系统总物质量
        FC_ASSERT(gpo.total_gold == total_gold, "核对系统总金石含量失败", ("gpo.total_gold", gpo.total_gold)("total_gold", total_gold));
        FC_ASSERT(gpo.total_food == total_food, "核对系统总食物含量失败", ("gpo.total_food", gpo.total_food)("total_food", total_food));
        FC_ASSERT(gpo.total_wood == total_wood, "核对系统总木材含量失败", ("gpo.total_wood", gpo.total_wood)("total_wood", total_wood));
        FC_ASSERT(gpo.total_fabric == total_fabric, "核对系统总织物含量失败", ("gpo.total_fabric", gpo.total_fabric)("total_fabric", total_fabric));
        FC_ASSERT(gpo.total_herb == total_herb, "核对系统总药材含量失败", ("gpo.total_herb", gpo.total_herb)("total_herb", total_herb));
        
    } FC_CAPTURE_LOG_AND_RETHROW( (head_block_num()) ); }

    optional< chainbase::database::session >& database::pending_transaction_session()
    {
        return _pending_tx_session;
    }
    
    void database::process_proposals(const block_notification& note)
    {
        proposal_processor ps(*this);
        ps.run(note);
    }
        
} } //taiyi::chain

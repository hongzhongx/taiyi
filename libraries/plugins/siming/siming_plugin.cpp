#include <plugins/siming/siming_plugin.hpp>
#include <plugins/siming/siming_plugin_objects.hpp>

#include <chain/database_exceptions.hpp>
#include <chain/account_object.hpp>
#include <chain/siming_objects.hpp>
#include <chain/index.hpp>
#include <chain/util/impacted.hpp>

#include <utilities/key_conversion.hpp>
#include <utilities/plugin_utilities.hpp>

#include <fc/io/json.hpp>
#include <fc/macros.hpp>
#include <fc/smart_ref_impl.hpp>

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <iostream>

#define DISTANCE_CALC_PRECISION (10000)
#define BLOCK_PRODUCING_LAG_TIME (750)
#define BLOCK_PRODUCTION_LOOP_SLEEP_TIME (200000)


namespace taiyi { namespace plugins { namespace siming {

    using namespace taiyi::chain;

    using std::string;
    using std::vector;
    
    namespace bpo = boost::program_options;


    void new_chain_banner( const chain::database& db )
    {
        std::cerr << "\n"
            "********************************\n"
            "*                              *\n"
            "*   ------- NEW CHAIN ------   *\n"
            "*   -   Welcome to Taiyi!  -   *\n"
            "*   ------------------------   *\n"
            "*                              *\n"
            "********************************\n"
        "\n";
        return;
    }

    namespace detail
    {
        class siming_plugin_impl
        {
        public:
            siming_plugin_impl( boost::asio::io_service& io ) :
                _timer(io),
                _chain_plugin( appbase::app().get_plugin< taiyi::plugins::chain::chain_plugin >() ),
                _db( appbase::app().get_plugin< taiyi::plugins::chain::chain_plugin >().db() ),
                _block_producer( std::make_shared< siming::block_producer >( _db ) )
            {}

            void on_post_apply_block( const chain::block_notification& note );
            void on_pre_apply_operation( const chain::operation_notification& note );
            void on_post_apply_operation( const chain::operation_notification& note );
            
            void schedule_production_loop();
            block_production_condition::block_production_condition_enum block_production_loop();
            block_production_condition::block_production_condition_enum maybe_produce_block(fc::mutable_variant_object& capture);
            
            bool     _production_enabled                = false;
            uint32_t _required_siming_participation     = 33 * TAIYI_1_PERCENT;
            uint32_t _production_skip_flags             = chain::database::skip_nothing;
            
            std::map< taiyi::protocol::public_key_type, fc::ecc::private_key > _private_keys;
            std::set< taiyi::protocol::account_name_type >                     _simings;
            boost::asio::deadline_timer                                        _timer;

            plugins::chain::chain_plugin& _chain_plugin;
            chain::database&              _db;
            boost::signals2::connection   _post_apply_block_conn;
            boost::signals2::connection   _pre_apply_operation_conn;
            boost::signals2::connection   _post_apply_operation_conn;
            
            std::shared_ptr< siming::block_producer >                         _block_producer;
        };

        void check_memo( const string& memo, const chain::account_object& account, const account_authority_object& auth )
        {
            vector< public_key_type > keys;
            
            try
            {
                // Check if memo is a private key
                keys.push_back( fc::ecc::extended_private_key::from_base58( memo ).get_public_key() );
            }
            catch( fc::parse_error_exception& ) {}
            catch( fc::assert_exception& ) {}
            
            // Get possible keys if memo was an account password
            string owner_seed = account.name + "owner" + memo;
            auto owner_secret = fc::sha256::hash( owner_seed.c_str(), owner_seed.size() );
            keys.push_back( fc::ecc::private_key::regenerate( owner_secret ).get_public_key() );
            
            string active_seed = account.name + "active" + memo;
            auto active_secret = fc::sha256::hash( active_seed.c_str(), active_seed.size() );
            keys.push_back( fc::ecc::private_key::regenerate( active_secret ).get_public_key() );
            
            string posting_seed = account.name + "posting" + memo;
            auto posting_secret = fc::sha256::hash( posting_seed.c_str(), posting_seed.size() );
            keys.push_back( fc::ecc::private_key::regenerate( posting_secret ).get_public_key() );
            
            // Check keys against public keys in authorites
            for( auto& key_weight_pair : auth.owner.key_auths )
            {
                for( auto& key : keys )
                    TAIYI_ASSERT( key_weight_pair.first != key, plugin_exception,
                                 "Detected private owner key in memo field. You should change your owner keys." );
            }
            
            for( auto& key_weight_pair : auth.active.key_auths )
            {
                for( auto& key : keys )
                    TAIYI_ASSERT( key_weight_pair.first != key,  plugin_exception,
                                 "Detected private active key in memo field. You should change your active keys." );
            }
            
            for( auto& key_weight_pair : auth.posting.key_auths )
            {
                for( auto& key : keys )
                    TAIYI_ASSERT( key_weight_pair.first != key,  plugin_exception,
                                 "Detected private posting key in memo field. You should change your posting keys." );
            }
            
            const auto& memo_key = account.memo_key;
            for( auto& key : keys )
                TAIYI_ASSERT( memo_key != key,  plugin_exception,
                             "Detected private memo key in memo field. You should change your memo key." );
        }

        struct operation_visitor
        {
            operation_visitor( const chain::database& db ) : _db( db ) {}
            
            const chain::database& _db;
            
            typedef void result_type;
            
            template< typename T >
            void operator()( const T& )const {}
            
            void operator()( const transfer_operation& o )const
            {
                if( o.memo.length() > 0 )
                    check_memo( o.memo, _db.get< chain::account_object, chain::by_name >( o.from ), _db.get< account_authority_object, chain::by_account >( o.from ) );
            }
        };

        void siming_plugin_impl::on_pre_apply_operation( const chain::operation_notification& note )
        {
            if( _db.is_producing() )
                note.op.visit( operation_visitor( _db ) );
        }

        void siming_plugin_impl::on_post_apply_operation( const chain::operation_notification& note )
        {
            switch( note.op.which() )
            {
                case operation::tag< custom_operation >::value:
                case operation::tag< custom_json_operation >::value:
                    if( _db.is_producing() )
                    {
                        flat_set< account_name_type > impacted;
                        chain::operation_get_impacted_accounts( note.op, impacted );
                        
                        for( const account_name_type& account : impacted )
                        {
                            // Possible alternative implementation:  Don't call find(), simply catch
                            // the exception thrown by db.create() when violating uniqueness (std::logic_error).
                            //
                            // This alternative implementation isn't "idiomatic" (i.e. AFAICT no existing
                            // code uses this approach).  However, it may improve performance.
                            
                            const siming_custom_op_object* coo = _db.find< siming_custom_op_object, by_account >( account );
                            if( !coo )
                            {
                                _db.create< siming_custom_op_object >( [&]( siming_custom_op_object& o ) {
                                    o.account = account;
                                    o.count = 1;
                                });
                            }
                            else
                            {
                                TAIYI_ASSERT( coo->count < SIMING_CUSTOM_OP_BLOCK_LIMIT, plugin_exception,
                                             "Account ${a} already submitted ${n} custom json operation(s) this block.",
                                             ("a", account)("n", SIMING_CUSTOM_OP_BLOCK_LIMIT) );
                                
                                _db.modify( *coo, [&]( siming_custom_op_object& o ) {
                                    o.count++;
                                });
                            }
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        void siming_plugin_impl::on_post_apply_block( const block_notification& note )
        {
            const auto& idx = _db.get_index< siming_custom_op_index >().indices().get< by_id >();
            while( true )
            {
                auto it = idx.begin();
                if( it == idx.end() )
                    break;
                _db.remove( *it );
            }
        }
        
        void siming_plugin_impl::schedule_production_loop() {
            // Sleep for 200ms, before checking the block production
            fc::time_point now = fc::time_point::now();
            int64_t time_to_sleep = BLOCK_PRODUCTION_LOOP_SLEEP_TIME - (now.time_since_epoch().count() % BLOCK_PRODUCTION_LOOP_SLEEP_TIME);
            if (time_to_sleep < 50000) // we must sleep for at least 50ms
                time_to_sleep += BLOCK_PRODUCTION_LOOP_SLEEP_TIME;
            
            _timer.expires_from_now( boost::posix_time::microseconds( time_to_sleep ) );
            _timer.async_wait( boost::bind( &siming_plugin_impl::block_production_loop, this ) );
        }

        block_production_condition::block_production_condition_enum siming_plugin_impl::block_production_loop()
        {
            if( fc::time_point::now() < fc::time_point(TAIYI_GENESIS_TIME) )
            {
                wlog( "waiting until genesis time to produce block: ${t}", ("t",TAIYI_GENESIS_TIME) );
                schedule_production_loop();
                return block_production_condition::wait_for_genesis;
            }
            
            block_production_condition::block_production_condition_enum result;
            fc::mutable_variant_object capture;
            try
            {
                result = maybe_produce_block(capture);
            }
            catch( const fc::canceled_exception& )
            {
                //We're trying to exit. Go ahead and let this one out.
                throw;
            }
            catch( const chain::unknown_hardfork_exception& e )
            {
                // Hit a hardfork that the current node know nothing about, stop production and inform user
                elog( "${e}\nNode may be out of date...", ("e", e.to_detail_string()) );
                throw;
            }
            catch( const fc::exception& e )
            {
                elog("Got exception while generating block:\n${e}", ("e", e.to_detail_string()));
                result = block_production_condition::exception_producing_block;
            }
            
            switch(result)
            {
                case block_production_condition::produced:
                    ilog("Generated block #${n} with timestamp ${t} at time ${c}", (capture));
                    break;
                case block_production_condition::not_synced:
                    ilog("Not producing block because production is disabled until we receive a recent block (see: --enable-stale-production)");
                    break;
                case block_production_condition::not_my_turn:
                    //          ilog("Not producing block because it isn't my turn");
                    break;
                case block_production_condition::not_time_yet:
                    //          ilog("Not producing block because slot has not yet arrived");
                    break;
                case block_production_condition::no_private_key:
                    ilog("Not producing block because I don't have the private key for ${scheduled_key}", (capture) );
                    break;
                case block_production_condition::low_participation:
                    elog("Not producing block because node appears to be on a minority fork with only ${pct}% siming participation", (capture) );
                    break;
                case block_production_condition::lag:
                    elog("Not producing block because node didn't wake up within ${t}ms of the slot time.", ("t", BLOCK_PRODUCING_LAG_TIME));
                    break;
                case block_production_condition::consecutive:
                    elog("Not producing block because the last block was generated by the same siming.\nThis node is probably disconnected from the network so block production has been disabled.\nDisable this check with --allow-consecutive option.");
                    break;
                case block_production_condition::exception_producing_block:
                    elog( "exception producing block" );
                    break;
                case block_production_condition::wait_for_genesis:
                    break;
            }
            
            schedule_production_loop();
            return result;
        }

        block_production_condition::block_production_condition_enum siming_plugin_impl::maybe_produce_block(fc::mutable_variant_object& capture)
        {
            fc::time_point now_fine = fc::time_point::now();
            fc::time_point_sec now = now_fine + fc::microseconds( 500000 );
            
            // If the next block production opportunity is in the present or future, we're synced.
            if( !_production_enabled )
            {
                if( _db.get_slot_time(1) >= now )
                    _production_enabled = true;
                else
                    return block_production_condition::not_synced;
            }
            
            // is anyone scheduled to produce now or one second in the future?
            uint32_t slot = _db.get_slot_at_time( now );
            if( slot == 0 )
            {
                capture("next_time", _db.get_slot_time(1));
                return block_production_condition::not_time_yet;
            }
            
            //
            // this assert should not fail, because now <= db.head_block_time()
            // should have resulted in slot == 0.
            //
            // if this assert triggers, there is a serious bug in get_slot_at_time()
            // which would result in allowing a later block to have a timestamp
            // less than or equal to the previous block
            //
            assert( now > _db.head_block_time() );
            
            chain::account_name_type scheduled_siming = _db.get_scheduled_siming( slot );
            // we must control the siming scheduled to produce the next block.
            if( _simings.find( scheduled_siming ) == _simings.end() )
            {
                capture("scheduled_siming", scheduled_siming);
                return block_production_condition::not_my_turn;
            }
            
            fc::time_point_sec scheduled_time = _db.get_slot_time( slot );
            chain::public_key_type scheduled_key = _db.get< chain::siming_object, chain::by_name >(scheduled_siming).signing_key;
            auto private_key_itr = _private_keys.find( scheduled_key );
            
            if( private_key_itr == _private_keys.end() )
            {
                capture("scheduled_siming", scheduled_siming);
                capture("scheduled_key", scheduled_key);
                return block_production_condition::no_private_key;
            }
            
            uint32_t prate = _db.siming_participation_rate();
            if( prate < _required_siming_participation )
            {
                capture("pct", uint32_t(100*uint64_t(prate) / TAIYI_1_PERCENT));
                return block_production_condition::low_participation;
            }
            
            if( llabs((scheduled_time - now).count()) > fc::milliseconds( BLOCK_PRODUCING_LAG_TIME ).count() )
            {
                capture("scheduled_time", scheduled_time)("now", now);
                return block_production_condition::lag;
            }
            
            auto block = _chain_plugin.generate_block(scheduled_time, scheduled_siming, private_key_itr->second, _production_skip_flags);
            capture("n", block.block_num())("t", block.timestamp)("c", now);
            
            appbase::app().get_plugin< taiyi::plugins::p2p::p2p_plugin >().broadcast_block( block );
            return block_production_condition::produced;
        }

    } // detail
    
    siming_plugin::siming_plugin() {}
    siming_plugin::~siming_plugin() {}

    void siming_plugin::set_program_options(boost::program_options::options_description& cli, boost::program_options::options_description& cfg)
    {
        cfg.add_options()
            ("enable-stale-production", bpo::value<bool>()->default_value( false ), "Enable block production, even if the chain is stale.")
            ("required-participation", bpo::value< uint32_t >()->default_value( 33 ), "Percent of simings (0-99) that must be participating in order to produce blocks")
            ("siming,w", bpo::value<vector<string>>()->composing()->multitoken(), "name of siming controlled by this node (e.g. initsiming )" )
            ("private-key", bpo::value<vector<string>>()->composing()->multitoken(), "WIF PRIVATE KEY to be used by one or more simings or miners" )
        ;
        cli.add_options()
            ("enable-stale-production", bpo::bool_switch()->default_value( false ), "Enable block production, even if the chain is stale.")
        ;
    }

    void siming_plugin::plugin_initialize(const boost::program_options::variables_map& options)
    { try {
        ilog( "Initializing siming plugin" );
        my = std::make_unique< detail::siming_plugin_impl >( appbase::app().get_io_service() );
        
        my->_chain_plugin.register_block_generator( get_name(), my->_block_producer );
        
        TAIYI_LOAD_VALUE_SET( options, "siming", my->_simings, taiyi::protocol::account_name_type )
        
        if( options.count("private-key") )
        {
            const std::vector<std::string> keys = options["private-key"].as<std::vector<std::string>>();
            for (const std::string& wif_key : keys )
            {
                fc::optional<fc::ecc::private_key> private_key = taiyi::utilities::wif_to_key(wif_key);
                FC_ASSERT( private_key.valid(), "unable to parse private key" );
                my->_private_keys[private_key->get_public_key()] = *private_key;
            }
        }
        
        my->_production_enabled = options.at( "enable-stale-production" ).as< bool >();
                
        if( options.count( "required-participation" ) )
        {
            my->_required_siming_participation = TAIYI_1_PERCENT * options.at( "required-participation" ).as< uint32_t >();
        }
        
        my->_post_apply_block_conn = my->_db.add_post_apply_block_handler([&](const chain::block_notification& note) { my->on_post_apply_block( note ); }, *this, 0);
        my->_pre_apply_operation_conn = my->_db.add_pre_apply_operation_handler([&](const chain::operation_notification& note) { my->on_pre_apply_operation( note ); }, *this, 0);
        my->_post_apply_operation_conn = my->_db.add_pre_apply_operation_handler([&](const chain::operation_notification& note) { my->on_post_apply_operation( note ); }, *this, 0);
        
        if( my->_simings.size() && my->_private_keys.size() )
            my->_chain_plugin.set_write_lock_hold_time( -1 );
        
        TAIYI_ADD_PLUGIN_INDEX(my->_db, siming_custom_op_index);
    } FC_LOG_AND_RETHROW() }
    
    void siming_plugin::plugin_startup()
    { try {
        ilog("siming plugin:  plugin_startup() begin" );
        chain::database& d = appbase::app().get_plugin< taiyi::plugins::chain::chain_plugin >().db();
        
        if( !my->_simings.empty() )
        {
            ilog( "Launching block production for ${n} simings.", ("n", my->_simings.size()) );
            appbase::app().get_plugin< taiyi::plugins::p2p::p2p_plugin >().set_block_production( true );
            if( my->_production_enabled )
            {
                if( d.head_block_num() == 0 )
                    new_chain_banner( d );
                my->_production_skip_flags |= chain::database::skip_undo_history_check;
            }
            my->schedule_production_loop();
        }
        else
            elog("No simings configured! Please add siming IDs and private keys to configuration.");
        
        ilog("siming plugin:  plugin_startup() end");
    } FC_CAPTURE_AND_RETHROW() }
    
    void siming_plugin::plugin_shutdown()
    {
        try
        {
            chain::util::disconnect_signal( my->_post_apply_block_conn );
            chain::util::disconnect_signal( my->_pre_apply_operation_conn );
            chain::util::disconnect_signal( my->_post_apply_operation_conn );
            
            my->_timer.cancel();
        }
        catch(fc::exception& e)
        {
            edump( (e.to_detail_string()) );
        }
    }

} } } // taiyi::plugins::siming

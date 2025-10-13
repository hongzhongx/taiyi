#include <plugins/baiyujing_api/baiyujing_api.hpp>
#include <plugins/baiyujing_api/baiyujing_api_plugin.hpp>

#include <plugins/database_api/database_api_plugin.hpp>
#include <plugins/block_api/block_api_plugin.hpp>
#include <plugins/account_history_api/account_history_api_plugin.hpp>
#include <plugins/account_by_key_api/account_by_key_api_plugin.hpp>
#include <plugins/network_broadcast_api/network_broadcast_api_plugin.hpp>

#include <utilities/git_revision.hpp>

#include <chain/util/uint256.hpp>

#include <chain/lua_context.hpp>
#include <chain/contract_worker.hpp>

#include <fc/git_revision.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/thread/future.hpp>
#include <boost/thread/lock_guard.hpp>

#define CHECK_ARG_SIZE( s ) \
    FC_ASSERT( args.size() == s, "Expected #s argument(s), was ${n}", ("n", args.size()) );

namespace taiyi { namespace plugins { namespace baiyujing_api {

    namespace detail
    {
        typedef std::function< void( const broadcast_transaction_synchronous_return& ) > confirmation_callback;
        
        class baiyujing_api_impl
        {
        public:
            baiyujing_api_impl() : _chain( appbase::app().get_plugin< taiyi::plugins::chain::chain_plugin >() ), _db( _chain.db() )
            {
                _on_post_apply_block_conn = _db.add_post_apply_block_handler([&]( const block_notification& note ) { on_post_apply_block( note.block ); }, appbase::app().get_plugin<taiyi::plugins::baiyujing_api::baiyujing_api_plugin>(), 0);
            }

            DECLARE_API_IMPL(
                (get_version)
                (get_state)
                (get_active_simings)
                (get_block_header)
                (get_block)
                (get_ops_in_block)
                (get_config)
                (get_dynamic_global_properties)
                (get_chain_properties)
                (get_siming_schedule)
                (get_hardfork_version)
                (get_next_scheduled_hardfork)
                (get_reward_fund)
                (get_key_references)
                (get_accounts)
                (lookup_account_names)
                (lookup_accounts)
                (get_account_count)
                (get_owner_history)
                (get_recovery_request)
                (get_withdraw_routes)
                (get_qi_delegations)
                (get_expiring_qi_delegations)
                (get_simings)
                (get_siming_by_account)
                (get_simings_by_adore)
                (lookup_siming_accounts)
                (get_siming_count)
                (get_transaction_hex)
                (get_transaction)
                (get_transaction_results)
                (get_required_signatures)
                (get_potential_signatures)
                (verify_authority)
                (verify_account_authority)
                (get_account_history)
                (broadcast_transaction)
                (broadcast_transaction_synchronous)
                (broadcast_block)
                (get_account_resources)

                (find_nfa_symbol)
                (find_nfa_symbol_by_contract)
                (find_nfa)
                (find_nfas)
                (list_nfas)
                (get_nfa_history)
                (get_nfa_action_info)
                (eval_nfa_action)
                (eval_nfa_action_with_string_args)
                             
                (find_actor)
                (find_actors)
                (list_actors)
                (get_actor_history)
                (list_actors_below_health)
                (find_actor_talent_rules)
                (list_actors_on_zone)

                (get_tiandao_properties)
                (find_zones)
                (find_zones_by_name)
                (list_zones)
                (list_zones_by_type)
                (list_to_zones_by_from)
                (list_from_zones_by_to)
                (find_way_to_zone)

                (list_relations_from_actor)
                (list_relations_to_actor)
                (get_relation_from_to_actor)
                (get_actor_connections)
                (list_actor_groups)
                (find_actor_group)
                (list_actor_friends)
                (get_actor_needs)
                (list_actor_mating_targets_by_zone)
                (stat_people_by_zone)
                (stat_people_by_base)

                (get_contract_source_code)
            )
            
            void on_post_apply_block( const signed_block& b );
            
            taiyi::plugins::chain::chain_plugin&                              _chain;
            chain::database&                                                  _db;
            
            std::shared_ptr< database_api::database_api >                     _database_api;
            std::shared_ptr< block_api::block_api >                           _block_api;
            std::shared_ptr< account_history::account_history_api >           _account_history_api;
            std::shared_ptr< account_by_key::account_by_key_api >             _account_by_key_api;
            std::shared_ptr< network_broadcast_api::network_broadcast_api >   _network_broadcast_api;
            
            p2p::p2p_plugin*                                                  _p2p = nullptr;
            map< transaction_id_type, confirmation_callback >                 _callbacks;
            map< time_point_sec, vector< transaction_id_type > >              _callback_expirations;
            boost::signals2::connection                                       _on_post_apply_block_conn;
            
            boost::mutex                                                      _mtx;
        };

        DEFINE_API_IMPL( baiyujing_api_impl, get_version )
        {
            CHECK_ARG_SIZE( 0 )
            return get_version_return(std::string( TAIYI_BLOCKCHAIN_VERSION ), std::string( taiyi::utilities::git_revision_sha ), std::string( fc::git_revision_sha ));
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_state )
        {
            CHECK_ARG_SIZE( 1 )
            string path = args[0].as< string >();
            
            state _state;
            _state.props         = get_dynamic_global_properties( {} );
            _state.current_route = path;
            
            try
            {
                if( path.size() && path[0] == '/' )
                    path = path.substr(1); /// remove '/' from front
                
                if( !path.size() )
                    path = "trending";
                
                set<string> accounts;
                
                vector<string> part; part.reserve(4);
                boost::split( part, path, boost::is_any_of("/") );
                part.resize(std::max( part.size(), size_t(4) ) ); // at least 4
                
                auto tag = fc::to_lower( part[1] );
                
                if( part[0].size() && part[0][0] == '@' ) {
                    auto acnt = part[0].substr(1);
                    _state.accounts[acnt] = extended_account( database_api::api_account_object( _db.get_account( acnt ), _db ) );
                    
                    auto& eacnt = _state.accounts[acnt];
                    if( part[1] == "transfers" )
                    {
                        if( _account_history_api )
                        {
                            legacy_operation l_op;
                            legacy_operation_conversion_visitor visitor( l_op );
                            auto history = _account_history_api->get_account_history( { acnt, uint64_t(-1), 1000 } ).history;
                            for( auto& item : history )
                            {
                                switch( item.second.op.which() ) {
                                    case operation::tag<transfer_to_qi_operation>::value:
                                    case operation::tag<withdraw_qi_operation>::value:
                                    case operation::tag<transfer_operation>::value:
                                    case operation::tag<claim_reward_balance_operation>::value:
                                        if( item.second.op.visit( visitor ) )
                                        {
                                            eacnt.transfer_history.emplace( item.first, api_operation_object( item.second, visitor.l_op ) );
                                        }
                                        break;
                                    case operation::tag<account_siming_adore_operation>::value:
                                    case operation::tag<account_siming_proxy_operation>::value:
                                        break;
                                    case operation::tag<account_create_operation>::value:
                                    case operation::tag<account_update_operation>::value:
                                    case operation::tag<siming_update_operation>::value:
                                    case operation::tag<custom_operation>::value:
                                    case operation::tag<producer_reward_operation>::value:
                                    default:
                                        if( item.second.op.visit( visitor ) )
                                        {
                                            eacnt.other_history.emplace( item.first, api_operation_object( item.second, visitor.l_op ) );
                                        }
                                }
                            }
                        }
                    }
                }
                else if( part[0] == "simings" || part[0] == "~simings")
                {
                    auto wits = get_simings_by_adore( (vector< fc::variant >){ fc::variant(""), fc::variant(100) } );
                    for( const auto& w : wits )
                    {
                        _state.simings[w.owner] = w;
                    }
                }
                else {
                    elog( "What... no matches" );
                }
                
                for( const auto& a : accounts )
                {
                    _state.accounts.erase("");
                    _state.accounts[a] = extended_account( database_api::api_account_object( _db.get_account( a ), _db ) );
                }
                
                _state.siming_schedule = _database_api->get_siming_schedule( {} );
                
            }
            catch ( const fc::exception& e )
            {
                _state.error = e.to_detail_string();
            }
            
            return _state;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_active_simings )
        {
            CHECK_ARG_SIZE( 0 )
            return _database_api->get_active_simings( {} ).simings;
        }
        
        DEFINE_API_IMPL( baiyujing_api_impl, get_block_header )
        {
            CHECK_ARG_SIZE( 1 )
            FC_ASSERT( _block_api, "block_api_plugin not enabled." );
            return _block_api->get_block_header( { args[0].as< uint32_t >() } ).header;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_block )
        {
            CHECK_ARG_SIZE( 1 )
            FC_ASSERT( _block_api, "block_api_plugin not enabled." );
            get_block_return result;
            auto b = _block_api->get_block( { args[0].as< uint32_t >() } ).block;
            
            if( b )
            {
                result = legacy_signed_block( *b );
                uint32_t n = uint32_t( b->transactions.size() );
                uint32_t block_num = block_header::num_from_id( b->block_id );
                for( uint32_t i=0; i<n; i++ )
                {
                    result->transactions[i].transaction_id = b->transactions[i].id();
                    result->transactions[i].block_num = block_num;
                    result->transactions[i].transaction_num = i;
                }
            }
            
            return result;
        }
        
        DEFINE_API_IMPL( baiyujing_api_impl, get_ops_in_block )
        {
            FC_ASSERT( args.size() == 1 || args.size() == 2, "Expected 1-2 arguments, was ${n}", ("n", args.size()) );
            FC_ASSERT( _account_history_api, "account_history_api_plugin not enabled." );
            
            auto ops = _account_history_api->get_ops_in_block( { args[0].as< uint32_t >(), args[1].as< bool >() } ).ops;
            get_ops_in_block_return result;
            
            legacy_operation l_op;
            legacy_operation_conversion_visitor visitor( l_op );
            
            for( auto& op_obj : ops )
            {
                if( op_obj.op.visit( visitor) )
                {
                    result.push_back( api_operation_object( op_obj, visitor.l_op ) );
                }
            }
            
            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_config )
        {
            CHECK_ARG_SIZE( 0 )
            return _database_api->get_config( {} );
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_dynamic_global_properties )
        {
            CHECK_ARG_SIZE( 0 )
            get_dynamic_global_properties_return gpo = _database_api->get_dynamic_global_properties( {} );            
            return gpo;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_chain_properties )
        {
            CHECK_ARG_SIZE( 0 )
            return api_chain_properties( _database_api->get_siming_schedule( {} ).median_props );
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_siming_schedule )
        {
            CHECK_ARG_SIZE( 0 )
            return _database_api->get_siming_schedule( {} );
        }
        
        DEFINE_API_IMPL( baiyujing_api_impl, get_hardfork_version )
        {
            CHECK_ARG_SIZE( 0 )
            return _database_api->get_hardfork_properties( {} ).current_hardfork_version;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_next_scheduled_hardfork )
        {
            CHECK_ARG_SIZE( 0 )
            scheduled_hardfork shf;
            const auto& hpo = _db.get( hardfork_property_id_type() );
            shf.hf_version = hpo.next_hardfork;
            shf.live_time = hpo.next_hardfork_time;
            return shf;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_reward_fund )
        {
            CHECK_ARG_SIZE( 1 )
            string name = args[0].as< string >();
            
            auto fund = _db.find< reward_fund_object, by_name >( name );
            FC_ASSERT( fund != nullptr, "Invalid reward fund name" );
            
            return api_reward_fund_object( *fund );
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_key_references )
        {
            CHECK_ARG_SIZE( 1 )
            FC_ASSERT( _account_by_key_api, "account_history_api_plugin not enabled." );
            
            return _account_by_key_api->get_key_references( { args[0].as< vector< public_key_type > >() } ).accounts;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_accounts )
        {
            CHECK_ARG_SIZE(1)
            vector< account_name_type > names = args[0].as< vector< account_name_type > >();
            
            const auto& idx  = _db.get_index< account_index >().indices().get< by_name >();
            const auto& vidx = _db.get_index< siming_adore_index >().indices().get< by_account_siming >();
            vector< extended_account > results;
            results.reserve(names.size());
            
            for( const auto& name: names )
            {
                auto itr = idx.find( name );
                if ( itr != idx.end() )
                {
                    results.emplace_back( extended_account( database_api::api_account_object( *itr, _db ) ) );
                    
                    auto vitr = vidx.lower_bound( boost::make_tuple( itr->name, account_name_type() ) );
                    while( vitr != vidx.end() && vitr->account == itr->name ) {
                        results.back().siming_adores.insert( _db.get< siming_object, by_name >( vitr->siming ).owner );
                        ++vitr;
                    }
                }
            }
            
            return results;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, lookup_account_names )
        {
            CHECK_ARG_SIZE( 1 )
            vector< account_name_type > account_names = args[0].as< vector< account_name_type > >();
            
            vector< optional< api_account_object > > result;
            result.reserve( account_names.size() );
            
            for( auto& name : account_names )
            {
                auto itr = _db.find< account_object, by_name >( name );
                
                if( itr )
                {
                    result.push_back( api_account_object( database_api::api_account_object( *itr, _db ) ) );
                }
                else
                {
                    result.push_back( optional< api_account_object >() );
                }
            }
            
            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, lookup_accounts )
        {
            CHECK_ARG_SIZE( 2 )
            account_name_type lower_bound_name = args[0].as< account_name_type >();
            uint32_t limit = args[1].as< uint32_t >();
            
            FC_ASSERT( limit <= 1000 );
            const auto& accounts_by_name = _db.get_index< account_index, by_name >();
            set<string> result;
            
            for( auto itr = accounts_by_name.lower_bound( lower_bound_name ); limit-- && itr != accounts_by_name.end(); ++itr )
            {
                result.insert( itr->name );
            }
            
            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_account_count )
        {
            CHECK_ARG_SIZE( 0 )
            return _db.get_index<account_index>().indices().size();
        }
        
        DEFINE_API_IMPL( baiyujing_api_impl, get_owner_history )
        {
            CHECK_ARG_SIZE( 1 )
            return _database_api->find_owner_histories( { args[0].as< string >() } ).owner_auths;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_recovery_request )
        {
            CHECK_ARG_SIZE( 1 )
            get_recovery_request_return result;
            
            auto requests = _database_api->find_account_recovery_requests( { { args[0].as< account_name_type >() } } ).requests;
            
            if( requests.size() )
                result = requests[0];
            
            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_withdraw_routes )
        {
            FC_ASSERT( args.size() == 1 || args.size() == 2, "Expected 1-2 arguments, was ${n}", ("n", args.size()) );
            
            auto account = args[0].as< string >();
            auto destination = args.size() == 2 ? args[1].as< withdraw_route_type >() : outgoing;
            
            get_withdraw_routes_return result;
            
            if( destination == outgoing || destination == all )
            {
                auto routes = _database_api->find_withdraw_qi_routes( { account, database_api::by_withdraw_route } ).routes;
                result.insert( result.end(), routes.begin(), routes.end() );
            }
            
            if( destination == incoming || destination == all )
            {
                auto routes = _database_api->find_withdraw_qi_routes( { account, database_api::by_destination } ).routes;
                result.insert( result.end(), routes.begin(), routes.end() );
            }
            
            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_qi_delegations )
        {
            FC_ASSERT( args.size() == 2 || args.size() == 3, "Expected 2-3 arguments, was ${n}", ("n", args.size()) );
            
            database_api::list_qi_delegations_args a;
            account_name_type account = args[0].as< account_name_type >();
            a.start = fc::variant( (vector< variant >){ args[0], args[1] } );
            a.limit = args.size() == 3 ? args[2].as< uint32_t >() : 100;
            a.order = database_api::by_delegation;
            
            auto delegations = _database_api->list_qi_delegations( a ).delegations;
            get_qi_delegations_return result;
            
            for( auto itr = delegations.begin(); itr != delegations.end() && itr->delegator == account; ++itr )
            {
                result.push_back( api_qi_delegation_object( *itr ) );
            }
            
            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_expiring_qi_delegations )
        {
            FC_ASSERT( args.size() == 2 || args.size() == 3, "Expected 2-3 arguments, was ${n}", ("n", args.size()) );
            
            database_api::list_qi_delegation_expirations_args a;
            account_name_type account = args[0].as< account_name_type >();
            a.start = fc::variant( (vector< variant >){ args[0], args[1], fc::variant( qi_delegation_expiration_id_type() ) } );
            a.limit = args.size() == 3 ? args[2].as< uint32_t >() : 100;
            a.order = database_api::by_account_expiration;
            
            auto delegations = _database_api->list_qi_delegation_expirations( a ).delegations;
            get_expiring_qi_delegations_return result;
            
            for( auto itr = delegations.begin(); itr != delegations.end() && itr->delegator == account; ++itr )
            {
                result.push_back( api_qi_delegation_expiration_object( *itr ) );
            }
            
            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_simings )
        {
            CHECK_ARG_SIZE( 1 )
            vector< siming_id_type > siming_ids = args[0].as< vector< siming_id_type > >();
            
            get_simings_return result;
            result.reserve( siming_ids.size() );
            
            std::transform(siming_ids.begin(), siming_ids.end(), std::back_inserter(result), [this](siming_id_type id) -> optional< api_siming_object > {
                if( auto o = _db.find(id) )
                    return api_siming_object( database_api::api_siming_object ( *o ) );
                return {};
            });
            
            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_siming_by_account )
        {
            CHECK_ARG_SIZE( 1 )
            auto simings = _database_api->find_simings({ { args[0].as< account_name_type >() } }).simings;
            
            get_siming_by_account_return result;
            
            if( simings.size() )
                result = api_siming_object( simings[0] );
            
            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_simings_by_adore )
        {
            CHECK_ARG_SIZE( 2 )
            account_name_type start_name = args[0].as< account_name_type >();
            vector< fc::variant > start_key;
            
            if( start_name == account_name_type() )
            {
                start_key.push_back( fc::variant( std::numeric_limits< int64_t >::max() ) );
                start_key.push_back( fc::variant( account_name_type() ) );
            }
            else
            {
                auto start = _database_api->list_simings( { args[0], 1, database_api::by_name } );
                
                if( start.simings.size() == 0 )
                    return get_simings_by_adore_return();
                
                start_key.push_back( fc::variant( start.simings[0].adores ) );
                start_key.push_back( fc::variant( start.simings[0].owner ) );
            }
            
            auto limit = args[1].as< uint32_t >();
            auto simings = _database_api->list_simings( { fc::variant( start_key ), limit, database_api::by_adore_name } ).simings;
            
            get_simings_by_adore_return result;
            
            for( auto& w : simings )
            {
                result.push_back( api_siming_object( w ) );
            }
            
            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, lookup_siming_accounts )
        {
            CHECK_ARG_SIZE( 2 )
            auto limit = args[1].as< uint32_t >();
            
            lookup_siming_accounts_return result;
            
            auto simings = _database_api->list_simings( { args[0], limit, database_api::by_name } ).simings;
            
            for( auto& w : simings )
            {
                result.push_back( w.owner );
            }
            
            return result;
        }
   
        DEFINE_API_IMPL( baiyujing_api_impl, get_siming_count )
        {
            CHECK_ARG_SIZE( 0 )
            return _db.get_index< siming_index >().indices().size();
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_transaction_hex )
        {
            CHECK_ARG_SIZE( 1 )
            return _database_api->get_transaction_hex({ signed_transaction( args[0].as< legacy_signed_transaction >() ) }).hex;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_transaction )
        {
            CHECK_ARG_SIZE( 1 )
            FC_ASSERT( _account_history_api, "account_history_api_plugin not enabled." );
            
            return legacy_signed_transaction( _account_history_api->get_transaction( { args[0].as< transaction_id_type >() } ) );
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_transaction_results )
        {
            CHECK_ARG_SIZE( 1 )
            
            const auto& index = _db.get_index<chain::transaction_index>().indices().get<chain::by_trx_id>();
            auto itr = index.find(args[0].as< transaction_id_type >());
            FC_ASSERT(itr != index.end(), "transaction results in not in recent cache.");
            
            return itr->operation_results;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_required_signatures )
        {
            CHECK_ARG_SIZE( 2 )
            return _database_api->get_required_signatures( {
                signed_transaction( args[0].as< legacy_signed_transaction >() ),
                args[1].as< flat_set< public_key_type > >() } ).keys;
        }
        
        DEFINE_API_IMPL( baiyujing_api_impl, get_potential_signatures )
        {
            CHECK_ARG_SIZE( 1 )
            return _database_api->get_potential_signatures( { signed_transaction( args[0].as< legacy_signed_transaction >() ) } ).keys;
        }
        
        DEFINE_API_IMPL( baiyujing_api_impl, verify_authority )
        {
            CHECK_ARG_SIZE( 1 )
            return _database_api->verify_authority( { signed_transaction( args[0].as< legacy_signed_transaction >() ) } ).valid;
        }
        
        DEFINE_API_IMPL( baiyujing_api_impl, verify_account_authority )
        {
            CHECK_ARG_SIZE( 2 )
            return _database_api->verify_account_authority( { args[0].as< account_name_type >(), args[1].as< flat_set< public_key_type > >() } ).valid;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_account_history )
        {
            CHECK_ARG_SIZE( 3 )
            FC_ASSERT( _account_history_api, "account_history_api_plugin not enabled." );
            
            auto history = _account_history_api->get_account_history( { args[0].as< account_name_type >(), args[1].as< uint64_t >(), args[2].as< uint32_t >() } ).history;
            get_account_history_return result;
            
            legacy_operation l_op;
            legacy_operation_conversion_visitor visitor( l_op );
            
            for( auto& entry : history )
            {
                if( entry.second.op.visit( visitor ) )
                {
                    result.emplace( entry.first, api_operation_object( entry.second, visitor.l_op ) );
                }
            }
            
            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, broadcast_transaction )
        {
            CHECK_ARG_SIZE( 1 )
            FC_ASSERT( _network_broadcast_api, "network_broadcast_api_plugin not enabled." );
            return _network_broadcast_api->broadcast_transaction( { signed_transaction( args[0].as< legacy_signed_transaction >() ) } );
        }
        
        DEFINE_API_IMPL( baiyujing_api_impl, broadcast_transaction_synchronous )
        {
            CHECK_ARG_SIZE( 1 )
            FC_ASSERT( _network_broadcast_api, "network_broadcast_api_plugin not enabled." );
            FC_ASSERT( _p2p != nullptr, "p2p_plugin not enabled." );
            
            signed_transaction trx = args[0].as< legacy_signed_transaction >();
            auto txid = trx.id();
            boost::promise< broadcast_transaction_synchronous_return > p;
            
            {
                boost::lock_guard< boost::mutex > guard( _mtx );
                FC_ASSERT( _callbacks.find( txid ) == _callbacks.end(), "Transaction is a duplicate" );
                _callbacks[ txid ] = [&p]( const broadcast_transaction_synchronous_return& r )
                {
                    p.set_value( r );
                };
                _callback_expirations[ trx.expiration ].push_back( txid );
            }
            
            try
            {
                /* It may look strange to call these without the lock and then lock again in the case of an exception,
                 * but it is correct and avoids deadlock. accept_transaction is trained along with all other writes, including
                 * pushing blocks. Pushing blocks do not originate from this code path and will never have this lock.
                 * However, it will trigger the on_post_apply_block callback and then attempt to acquire the lock. In this case,
                 * this thread will be waiting on accept_block so it can write and the block thread will be waiting on this
                 * thread for the lock.
                 */
                _chain.accept_transaction( trx );
                _p2p->broadcast_transaction( trx );
            }
            catch( fc::exception& e )
            {
                boost::lock_guard< boost::mutex > guard( _mtx );
                
                // The callback may have been cleared in the meantine, so we need to check for existence.
                auto c_itr = _callbacks.find( txid );
                if( c_itr != _callbacks.end() ) _callbacks.erase( c_itr );
                
                // We do not need to clean up _callback_expirations because on_post_apply_block handles this case.
                
                throw e;
            }
            catch( ... )
            {
                boost::lock_guard< boost::mutex > guard( _mtx );
                
                // The callback may have been cleared in the meantine, so we need to check for existence.
                auto c_itr = _callbacks.find( txid );
                if( c_itr != _callbacks.end() ) _callbacks.erase( c_itr );
                
                throw fc::unhandled_exception(FC_LOG_MESSAGE( warn, "Unknown error occured when pushing transaction" ), std::current_exception() );
            }
            
            return p.get_future().get();
        }

        DEFINE_API_IMPL( baiyujing_api_impl, broadcast_block )
        {
            CHECK_ARG_SIZE( 1 )
            FC_ASSERT( _network_broadcast_api, "network_broadcast_api_plugin not enabled." );
            
            return _network_broadcast_api->broadcast_block( { signed_block( args[0].as< legacy_signed_block >() ) } );
        }
        
        DEFINE_API_IMPL( baiyujing_api_impl, get_account_resources )
        {
            CHECK_ARG_SIZE(1)

            const auto& resources = _database_api->find_account_resources( { args[0].as< vector< account_name_type > >() } ).resources;

            get_account_resources_return result;
            for( const auto& r : resources )
                result.emplace_back( api_resource_assets( r ) );

            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, find_nfa_symbol )
        {
            CHECK_ARG_SIZE( 1 )
            
            find_nfa_symbol_return result;

            auto a = _database_api->find_nfa_symbol( { args[0].as<string>() } ).result;
            
            if(a.valid())
                result = database_api::api_nfa_symbol_object( *a );
            
            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, find_nfa_symbol_by_contract )
        {
            CHECK_ARG_SIZE( 1 )
            
            find_nfa_symbol_by_contract_return result;

            auto a = _database_api->find_nfa_symbol_by_contract( { args[0].as<string>() } ).result;
            
            if(a.valid())
                result = database_api::api_nfa_symbol_object( *a );
            
            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, find_nfa )
        {
            CHECK_ARG_SIZE( 1 )
            
            find_nfa_return result;

            auto a = _database_api->find_nfa( { args[0].as<int64_t>() } ).result;
            
            if(a.valid())
                result = api_nfa_object( *a );
            
            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, find_nfas )
        {
            CHECK_ARG_SIZE( 1 )

            const auto& nfas = _database_api->find_nfas( { args[0].as< vector< taiyi::plugins::database_api::api_id_type > >() } ).result;
            find_nfas_return result;

            for( const auto& a : nfas ) result.emplace_back( api_nfa_object( a ) );

            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, list_nfas )
        {
            CHECK_ARG_SIZE( 2 )

            auto nfas = _database_api->list_nfas( { args[0], args[1].as< uint32_t >(), database_api::by_owner } ).result;

            list_nfas_return result;
            for( const auto& a : nfas ) result.emplace_back( api_nfa_object( a ) );

            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_nfa_history )
        {
           CHECK_ARG_SIZE( 3 )
           FC_ASSERT( _account_history_api, "account_history_api_plugin not enabled." );

           auto history = _account_history_api->get_nfa_history( { args[0].as< int64_t >(), args[1].as< uint64_t >(), args[2].as< uint32_t >() } ).history;
           get_nfa_history_return result;

           legacy_operation l_op;
           legacy_operation_conversion_visitor visitor( l_op );

           for( auto& entry : history )
           {
              if( entry.second.op.visit( visitor ) )
              {
                 result.emplace( entry.first, api_operation_object( entry.second, visitor.l_op ) );
              }
           }

           return result;
        }
        
        DEFINE_API_IMPL( baiyujing_api_impl, get_nfa_action_info )
        {
            CHECK_ARG_SIZE( 2 )
            
            api_contract_action_info result;
            
            const auto& nfa = _db.get<chain::nfa_object, chain::by_id>(args[0].as<int64_t>());
            string action_name = args[1].as< string >();
            
            const auto* contract_ptr = _db.find<chain::contract_object, by_id>(nfa.is_miraged?nfa.mirage_contract:nfa.main_contract);
            if(contract_ptr == nullptr)
                return result;
            
            auto abi_itr = contract_ptr->contract_ABI.find(lua_types(lua_string(action_name)));
            if(abi_itr == contract_ptr->contract_ABI.end())
                return result;
            if(abi_itr->second.which() != lua_types::tag<lua_table>::value)
                return result;
            
            result.exist = true;

            lua_map action_def = abi_itr->second.get<lua_table>().v;
            auto def_itr = action_def.find(lua_types(lua_string("consequence")));
            if(def_itr != action_def.end())
                result.consequence = def_itr->second.get<lua_bool>().v;
                    
            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, eval_nfa_action )
        {
            CHECK_ARG_SIZE( 3 )
            
            const auto& nfa = _db.get<chain::nfa_object, chain::by_id>(args[0].as<int64_t>());
            string action_name = args[1].as< string >();
            vector<lua_types> value_list = args[2].as< vector<lua_types> >();

            api_eval_action_return result;
            contract_worker worker;

            LuaContext context;
            _db.initialize_VM_baseENV(context);
            
            long long vm_drops = 100000000;
            result.err = worker.eval_nfa_contract_action(nfa, action_name, value_list, result.eval_result, vm_drops, true, context, _db);

            if(result.err == "") {
                for(auto& temp : worker.get_result().contract_affecteds) {
                    if(temp.which() == contract_affected_type::tag<contract_narrate>::value) {
                        result.narrate_logs.push_back(temp.get<contract_narrate>().message);
                    }
                }
            }

            return result;
        }
        
        DEFINE_API_IMPL( baiyujing_api_impl, eval_nfa_action_with_string_args )
        {
            CHECK_ARG_SIZE( 3 )
            
            const auto& nfa = _db.get<chain::nfa_object, chain::by_id>(args[0].as<int64_t>());
            string action_name = args[1].as< string >();
            string action_parameters_str = args[2].as< string >();
            fc::variant action_args = fc::json::from_string(action_parameters_str);
            vector<lua_types> value_list = protocol::from_variants_to_lua_types(action_args.as<fc::variants>());

            api_eval_action_return result;
            contract_worker worker;

            LuaContext context;
            _db.initialize_VM_baseENV(context);
            
            long long vm_drops = 100000000;
            result.err = worker.eval_nfa_contract_action(nfa, action_name, value_list, result.eval_result, vm_drops, true, context, _db);

            if(result.err == "") {
                for(auto& temp : worker.get_result().contract_affecteds) {
                    if(temp.which() == contract_affected_type::tag<contract_narrate>::value) {
                        result.narrate_logs.push_back(temp.get<contract_narrate>().message);
                    }
                }
            }
            else {
                result.narrate_logs.push_back(result.err);
            }

            return result;
        }
        
        DEFINE_API_IMPL( baiyujing_api_impl, find_actor )
        {
            CHECK_ARG_SIZE( 1 )
            return _database_api->find_actor( { args[0].as<string>() } ).result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, find_actors )
        {
            CHECK_ARG_SIZE( 1 )
            return _database_api->find_actors( { args[0].as< vector< database_api::api_id_type > >() } ).result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, list_actors )
        {
            CHECK_ARG_SIZE( 2 )
            return _database_api->list_actors( { args[0], args[1].as< uint32_t >(), database_api::by_owner } ).result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, list_actors_below_health )
        {
            CHECK_ARG_SIZE( 2 )
            return _database_api->list_actors( { args[0], args[1].as< uint32_t >(), database_api::by_health } ).result;
        }
        
        DEFINE_API_IMPL( baiyujing_api_impl, get_actor_history )
        {
            CHECK_ARG_SIZE( 3 )
            FC_ASSERT( _account_history_api, "account_history_api_plugin not enabled." );
            
            const actor_object& actor = _db.get_actor(args[0].as< string >());
            
            auto history = _account_history_api->get_nfa_history( { actor.nfa_id._id, args[1].as< uint64_t >(), args[2].as< uint32_t >() } ).history;
            get_actor_history_return result;
            
            legacy_operation l_op;
            legacy_operation_conversion_visitor visitor( l_op );
            
            for( auto& entry : history )
            {
                if( entry.second.op.visit( visitor ) )
                {
                    result.emplace( entry.first, api_operation_object( entry.second, visitor.l_op ) );
                }
            }
            
            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, find_actor_talent_rules )
        {
            CHECK_ARG_SIZE( 1 )

            return _database_api->find_actor_talent_rules( { args[0].as< vector< taiyi::plugins::database_api::api_id_type > >() } ).rules;
        }
        
        DEFINE_API_IMPL( baiyujing_api_impl, list_actors_on_zone )
        {
            CHECK_ARG_SIZE( 2 )
            return _database_api->list_actors( { args[0], args[1].as< uint32_t >(), database_api::by_location } ).result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, find_zones )
        {
            CHECK_ARG_SIZE( 1 )
            return _database_api->find_zones({ args[0].as< vector< taiyi::plugins::database_api::api_id_type > >() }).result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, find_zones_by_name )
        {
            CHECK_ARG_SIZE( 1 )
            return _database_api->find_zones_by_name( { args[0].as< vector< string > >() } ).result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, list_zones )
        {
            CHECK_ARG_SIZE( 2 )
            return _database_api->list_zones( { args[0], args[1].as< uint32_t >(), database_api::by_owner } ).result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, list_zones_by_type )
        {
            CHECK_ARG_SIZE( 2 )
            return _database_api->list_zones( { args[0], args[1].as< uint32_t >(), database_api::by_type } ).result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, list_to_zones_by_from )
        {
            CHECK_ARG_SIZE( 2 )
            return _database_api->list_zones( { args[0], args[1].as< uint32_t >(), database_api::by_zone_from } ).result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, list_from_zones_by_to )
        {
            CHECK_ARG_SIZE( 2 )
            return _database_api->list_zones( { args[0], args[1].as< uint32_t >(), database_api::by_zone_to } ).result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, find_way_to_zone )
        {
            CHECK_ARG_SIZE( 2 )
            return _database_api->find_way_to_zone( { args[0].as< string >(), args[1].as< string >() } );
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_tiandao_properties )
        {
            CHECK_ARG_SIZE( 0 )
            get_tiandao_properties_return tiandao = _database_api->get_tiandao_properties( {} );

            return tiandao;
        }
        
        DEFINE_API_IMPL( baiyujing_api_impl, list_actor_groups )
        {
            CHECK_ARG_SIZE( 2 )

            return _database_api->list_actor_groups( { args[0], args[1].as< uint32_t >(), database_api::by_group_leader } );
        }

        DEFINE_API_IMPL( baiyujing_api_impl, find_actor_group )
        {
            CHECK_ARG_SIZE( 1 )

            return _database_api->find_actor_group( args[0].as< database_api::find_actor_group_args >() );
        }

        DEFINE_API_IMPL( baiyujing_api_impl, list_relations_from_actor )
        {
            CHECK_ARG_SIZE( 1 )

            const auto& relations = _database_api->list_actor_relations( { args[0].as<string>() } ).relations;
            
            list_relations_from_actor_return result;
            for( const auto& r : relations )
                result.emplace_back( api_actor_relation_data( r.id, r.target_owner, r.target_name, r.favor, r.favor_level, r.last_update ) );

            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, list_relations_to_actor )
        {
            CHECK_ARG_SIZE( 1 )

            const auto& relations = _database_api->list_target_relations( { args[0].as<string>() } ).relations;
            
            list_relations_to_actor_return result;
            for( const auto& r : relations )
                result.emplace_back( api_actor_relation_data( r.id, r.actor_owner, r.actor_name, r.favor, r.favor_level, r.last_update ) );

            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_relation_from_to_actor )
        {
            CHECK_ARG_SIZE( 2 )

            const auto& relation = _database_api->get_actors_relation( { args[0].as<string>(), args[1].as<string>() } ).relation;
            
            if(relation.valid())
                return api_actor_relation_data( relation->id, relation->actor_owner, relation->actor_name, relation->favor, relation->favor_level, relation->last_update );
            else
                return optional< api_actor_relation_data >();
        }

        DEFINE_API_IMPL( baiyujing_api_impl, get_actor_connections )
        {
            CHECK_ARG_SIZE( 2 )

            const auto& result = _database_api->get_actor_connections( { args[0].as<string>(), args[1].as<E_ACTOR_RELATION_TYPE>() } );

            return result;
        }

        //总结双方关系
        //connection_t //1=直接血亲或配偶, 2=非血亲家人或师父或徒弟, other=其他关系
        void summarize_connections(const chain::actor_object& actor1, const chain::actor_object& actor2, int& connection_t, bool& couple, bool& mentor, chain::database& db )
        {
            connection_t = 0;
            couple = false;
            mentor = false;
            
            const chain::actor_connection_object* check_con = 0;
            //2对1的关系
            if(connection_t == 0) {
                check_con = db.find< chain::actor_connection_object, chain::by_relation_type >( boost::make_tuple(actor2.id, SWORN) );
                if(check_con && check_con->people.find(actor1.id) != check_con->people.end())
                    connection_t = 2;
            }

            if(connection_t == 0) {
                check_con = db.find< chain::actor_connection_object, chain::by_relation_type >( boost::make_tuple(actor2.id, MENTOR) );
                if(check_con && check_con->people.find(actor1.id) != check_con->people.end()) {
                    connection_t = 2;
                    mentor = true;
                }
            }

            if(connection_t == 0) {
                check_con = db.find< chain::actor_connection_object, chain::by_relation_type >( boost::make_tuple(actor2.id, DESCENDANT) );
                if(check_con && check_con->people.find(actor1.id) != check_con->people.end()) {
                    connection_t = 2;
                    mentor = true;
                }
            }

            if(connection_t == 0) {
                check_con = db.find< chain::actor_connection_object, chain::by_relation_type >( boost::make_tuple(actor2.id, COUPLE) );
                if(check_con && check_con->people.find(actor1.id) != check_con->people.end()) {
                    connection_t = 1;
                    couple = true;
                }
            }

            if(connection_t == 0) {
                check_con = db.find< chain::actor_connection_object, chain::by_relation_type >( boost::make_tuple(actor2.id, PARENT) );
                if(check_con && check_con->people.find(actor1.id) != check_con->people.end())
                    connection_t = 1;
            }

            if(connection_t == 0) {
                check_con = db.find< chain::actor_connection_object, chain::by_relation_type >( boost::make_tuple(actor2.id, ADOPTIVE_PARENT) );
                if(check_con && check_con->people.find(actor1.id) != check_con->people.end())
                    connection_t = 2;
            }
            
            if(connection_t == 0) {
                check_con = db.find< chain::actor_connection_object, chain::by_relation_type >( boost::make_tuple(actor2.id, SIBLING) );
                if(check_con && check_con->people.find(actor1.id) != check_con->people.end())
                    connection_t = 1;
            }
            
            if(connection_t == 0) {
                check_con = db.find< chain::actor_connection_object, chain::by_relation_type >( boost::make_tuple(actor2.id, CHILD) );
                if(check_con && check_con->people.find(actor1.id) != check_con->people.end())
                    connection_t = 1;
            }
            
            //1对2的关系
            if(connection_t == 0) {
                check_con = db.find< chain::actor_connection_object, chain::by_relation_type >( boost::make_tuple(actor1.id, SWORN) );
                if(check_con && check_con->people.find(actor2.id) != check_con->people.end())
                    connection_t = 2;
            }

            if(connection_t == 0) {
                check_con = db.find< chain::actor_connection_object, chain::by_relation_type >( boost::make_tuple(actor1.id, MENTOR) );
                if(check_con && check_con->people.find(actor2.id) != check_con->people.end()) {
                    connection_t = 2;
                    mentor = true;
                }
            }

            if(connection_t == 0) {
                check_con = db.find< chain::actor_connection_object, chain::by_relation_type >( boost::make_tuple(actor1.id, DESCENDANT) );
                if(check_con && check_con->people.find(actor2.id) != check_con->people.end()) {
                    connection_t = 2;
                    mentor = true;
                }
            }

            if(connection_t == 0) {
                check_con = db.find< chain::actor_connection_object, chain::by_relation_type >( boost::make_tuple(actor1.id, COUPLE) );
                if(check_con && check_con->people.find(actor2.id) != check_con->people.end()) {
                    connection_t = 1;
                    couple = true;
                }
            }

            if(connection_t == 0) {
                check_con = db.find< chain::actor_connection_object, chain::by_relation_type >( boost::make_tuple(actor1.id, PARENT) );
                if(check_con && check_con->people.find(actor2.id) != check_con->people.end())
                    connection_t = 1;
            }

            if(connection_t == 0) {
                check_con = db.find< chain::actor_connection_object, chain::by_relation_type >( boost::make_tuple(actor1.id, ADOPTIVE_PARENT) );
                if(check_con && check_con->people.find(actor2.id) != check_con->people.end())
                    connection_t = 1;
            }

            if(connection_t == 0) {
                check_con = db.find< chain::actor_connection_object, chain::by_relation_type >( boost::make_tuple(actor1.id, SIBLING) );
                if(check_con && check_con->people.find(actor2.id) != check_con->people.end())
                    connection_t = 1;
            }

            if(connection_t == 0) {
                check_con = db.find< chain::actor_connection_object, chain::by_relation_type >( boost::make_tuple(actor1.id, CHILD) );
                if(check_con && check_con->people.find(actor2.id) != check_con->people.end())
                    connection_t = 1;
            }
        }

        DEFINE_API_IMPL( baiyujing_api_impl, list_actor_friends )
        {
            CHECK_ARG_SIZE( 3 )
            
            const auto& actor = _db.get< chain::actor_object, by_name >(args[0].as< string >());
            int standpoint_threshold = args[1].as< int >();
            bool only_live = args[2].as< bool >();
            
            list_actor_friends_return result;

            const auto& relation_by_actor_favor_idx = _db.get_index< chain::actor_relation_index, chain::by_actor_favor >();
            auto itr = relation_by_actor_favor_idx.lower_bound( boost::make_tuple(actor.id, std::numeric_limits<int32_t>::max()) );
            auto end = relation_by_actor_favor_idx.end();
            while( itr != end )
            {
                const chain::actor_relation_object& relation = *itr;
                ++itr;
                
                if(relation.actor != actor.id)
                    break;
                
                if(relation.get_favor_level() < standpoint_threshold)
                    break;
                
                const auto& actor_target = _db.get< actor_object, by_id >(relation.target);
                if(only_live && actor_target.health <= 0)
                    continue;
                
                //总结双方关系
                int connection_t = 0; //1=直接血亲或配偶, 2=非血亲家人或师父或徒弟, other=其他关系
                bool couple = false;
                bool mentor = false;
                summarize_connections(actor, actor_target, connection_t, couple, mentor, _db);
                
                const auto& owner = _db.get<account_object, by_id>( _db.get< nfa_object, by_id >(actor_target.nfa_id).owner_account );
                result.emplace_back(api_actor_friend_data(owner.name, actor_target.name, connection_t, couple, mentor));
            }
            
            return result;
        }
        
        DEFINE_API_IMPL( baiyujing_api_impl, get_actor_needs )
        {
            CHECK_ARG_SIZE( 1 )
            
            const auto& actor = _db.get< chain::actor_object, chain::by_name >(args[0].as< string >());
            
            get_actor_needs_return result;

            const auto* mating_target = _db.find< chain::actor_object, chain::by_id >(actor.need_mating_target);
            if(mating_target) {
                const auto& owner = _db.get<account_object, by_id>( _db.get< nfa_object, by_id >(mating_target->nfa_id).owner_account );
                result.mating_target_owner = owner.name;
                result.mating_target_name = mating_target->name;
                result.mating_end_block_num = actor.need_mating_end_block_num;
            }
            
            const auto* bullying_target = _db.find< chain::actor_object, chain::by_id >(actor.need_bullying_target);
            if(bullying_target) {
                const auto& owner = _db.get<account_object, by_id>( _db.get< nfa_object, by_id >(bullying_target->nfa_id).owner_account );
                result.bullying_target_owner = owner.name;
                result.bullying_target_name = bullying_target->name;
                result.bullying_end_block_num = actor.need_bullying_end_block_num;
            }

            return result;
        }

        //获取指定区域中的情侣或者配偶
        DEFINE_API_IMPL( baiyujing_api_impl, list_actor_mating_targets_by_zone )
        {
            CHECK_ARG_SIZE( 2 )
            
            const auto& actor = _db.get< chain::actor_object, chain::by_name >(args[0].as< string >());
            const auto& zone = _db.get< chain::zone_object, chain::by_name >(args[1].as< string >());
            
            list_actor_mating_targets_by_zone_return result;

            const auto* love_of_me = _db.find< chain::actor_connection_object, chain::by_relation_type >( boost::make_tuple(actor.id, LOVE) );
            const auto* couple_of_me = _db.find< chain::actor_connection_object, chain::by_relation_type >( boost::make_tuple(actor.id, COUPLE) );
            const auto& actor_by_location_idx = _db.get_index< chain::actor_index, chain::by_location >();
            auto itr = actor_by_location_idx.lower_bound( zone.id );
            auto end = actor_by_location_idx.end();
            while( itr != end )
            {
                const actor_object& act = *itr;
                ++itr;
                
                if( act.location != zone.id )
                    break;
                if( act.id == actor.id )
                    continue;
                
                if( love_of_me && love_of_me->people.find(act.id) != love_of_me->people.end()) {
                    const auto& owner = _db.get<account_object, by_id>( _db.get< nfa_object, by_id >(act.nfa_id).owner_account );
                    result.emplace_back(api_simple_actor_object(owner.name, act.name));
                    continue;
                }

                if( couple_of_me && couple_of_me->people.find(act.id) != couple_of_me->people.end()) {
                    const auto& owner = _db.get<account_object, by_id>( _db.get< nfa_object, by_id >(act.nfa_id).owner_account );
                    result.emplace_back(api_simple_actor_object(owner.name, act.name));
                    continue;
                }
            }

            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, stat_people_by_zone )
        {
            CHECK_ARG_SIZE( 1 )
            
            const auto& zone = _db.get< chain::zone_object, chain::by_name >(args[0].as< string >());
            
            api_people_stat_data result;

            const auto& actor_by_location_idx = _db.get_index< chain::actor_index, chain::by_location >();
            auto itr = actor_by_location_idx.lower_bound( zone.id );
            auto end = actor_by_location_idx.end();
            while( itr != end )
            {
                if( itr->location != zone.id )
                    break;
                
                if(itr->health > 0)
                    result.live_num++;
                else
                    result.dead_num++;

                itr++;
            }

            return result;
        }

        DEFINE_API_IMPL( baiyujing_api_impl, stat_people_by_base )
        {
            CHECK_ARG_SIZE( 1 )
            
            const auto& zone = _db.get< chain::zone_object, chain::by_name >(args[0].as< string >());
            
            api_people_stat_data result;

            const auto& actor_by_base_idx = _db.get_index<chain::actor_index, chain::by_base>();
            auto itrn = actor_by_base_idx.lower_bound( zone.id );
            while(itrn != actor_by_base_idx.end()) {
                if(itrn->base != zone.id)
                    break;
                
                if(itrn->health > 0)
                    result.live_num++;
                else
                    result.dead_num++;
                
                itrn++;
            }

            return result;
        }
        
        DEFINE_API_IMPL( baiyujing_api_impl, get_contract_source_code )
        {
#ifndef IS_LOW_MEM
            CHECK_ARG_SIZE( 1 )
            
            const auto* contract_ptr = _db.find<chain::contract_object, chain::by_name>( args[0].as< string >() );
            if(contract_ptr == nullptr)
                return "";

            const auto& contract_bin_code = _db.get<chain::contract_bin_code_object, chain::by_contract_id>(contract_ptr->id);
            return contract_bin_code.source_code;
#else
            FC_ASSERT(false, "not support while node compiling with IS_LOW_MEM");
            return "";
#endif
        }

        void baiyujing_api_impl::on_post_apply_block( const signed_block& b )
        { try {
            boost::lock_guard< boost::mutex > guard( _mtx );
            int32_t block_num = int32_t(b.block_num());
            if( _callbacks.size() )
            {
                for( size_t trx_num = 0; trx_num < b.transactions.size(); ++trx_num )
                {
                    const auto& trx = b.transactions[trx_num];
                    auto id = trx.id();
                    auto itr = _callbacks.find( id );
                    if( itr == _callbacks.end() ) continue;
                    itr->second( broadcast_transaction_synchronous_return( id, block_num, int32_t( trx_num ), false ) );
                    _callbacks.erase( itr );
                }
            }
            
            /// clear all expirations
            while( true )
            {
                auto exp_it = _callback_expirations.begin();
                if( exp_it == _callback_expirations.end() )
                    break;
                if( exp_it->first >= b.timestamp )
                    break;
                for( const transaction_id_type& txid : exp_it->second )
                {
                    auto cb_it = _callbacks.find( txid );
                    // If it's empty, that means the transaction has been confirmed and has been deleted by the above check.
                    if( cb_it == _callbacks.end() )
                        continue;
                    
                    confirmation_callback callback = cb_it->second;
                    transaction_id_type txid_byval = txid;    // can't pass in by reference as it's going to be deleted
                    callback( broadcast_transaction_synchronous_return( txid_byval, block_num, -1, true ) );
                    
                    _callbacks.erase( cb_it );
                }
                _callback_expirations.erase( exp_it );
            }
        } FC_LOG_AND_RETHROW() }
        
    } // detail

    baiyujing_api::baiyujing_api() : my( new detail::baiyujing_api_impl() )
    {
        JSON_RPC_REGISTER_API( TAIYI_BAIYUJING_API_PLUGIN_NAME );
    }
    
    baiyujing_api::~baiyujing_api() {}
    
    void baiyujing_api::api_startup()
    {
        auto database = appbase::app().find_plugin< database_api::database_api_plugin >();
        if( database != nullptr )
        {
            my->_database_api = database->api;
        }
        
        auto block = appbase::app().find_plugin< block_api::block_api_plugin >();
        if( block != nullptr )
        {
            my->_block_api = block->api;
        }
        
        auto account_by_key = appbase::app().find_plugin< account_by_key::account_by_key_api_plugin >();
        if( account_by_key != nullptr )
        {
            my->_account_by_key_api = account_by_key->api;
        }
        
        auto account_history = appbase::app().find_plugin< account_history::account_history_api_plugin >();
        if( account_history != nullptr )
        {
            my->_account_history_api = account_history->api;
        }
        
        auto network_broadcast = appbase::app().find_plugin< network_broadcast_api::network_broadcast_api_plugin >();
        if( network_broadcast != nullptr )
        {
            my->_network_broadcast_api = network_broadcast->api;
        }
        
        auto p2p = appbase::app().find_plugin< p2p::p2p_plugin >();
        if( p2p != nullptr )
        {
            my->_p2p = p2p;
        }
    }

    DEFINE_LOCKLESS_APIS( baiyujing_api,
        (get_version)
        (get_config)
        (broadcast_transaction)
        (broadcast_transaction_synchronous)
        (broadcast_block)
    )

    DEFINE_READ_APIS( baiyujing_api,
        (get_state)
        (get_active_simings)
        (get_block_header)
        (get_block)
        (get_ops_in_block)
        (get_dynamic_global_properties)
        (get_chain_properties)
        (get_siming_schedule)
        (get_hardfork_version)
        (get_next_scheduled_hardfork)
        (get_reward_fund)
        (get_key_references)
        (get_accounts)
        (lookup_account_names)
        (lookup_accounts)
        (get_account_count)
        (get_owner_history)
        (get_recovery_request)
        (get_withdraw_routes)
        (get_qi_delegations)
        (get_expiring_qi_delegations)
        (get_simings)
        (get_siming_by_account)
        (get_simings_by_adore)
        (lookup_siming_accounts)
        (get_siming_count)
        (get_transaction_hex)
        (get_transaction)
        (get_transaction_results)
        (get_required_signatures)
        (get_potential_signatures)
        (verify_authority)
        (verify_account_authority)
        (get_account_history)
        (get_account_resources)

        (find_nfa_symbol)
        (find_nfa_symbol_by_contract)
        (find_nfa)
        (find_nfas)
        (list_nfas)
        (get_nfa_history)
        (get_nfa_action_info)
        (eval_nfa_action)
        (eval_nfa_action_with_string_args)
                     
        (find_actor)
        (find_actors)
        (list_actors)
        (get_actor_history)
        (list_actors_below_health)
        (find_actor_talent_rules)
        (list_actors_on_zone)

        (get_tiandao_properties)
        (find_zones)
        (find_zones_by_name)
        (list_zones)
        (list_zones_by_type)
        (list_to_zones_by_from)
        (list_from_zones_by_to)
        (find_way_to_zone)

        (list_relations_from_actor)
        (list_relations_to_actor)
        (get_relation_from_to_actor)
        (get_actor_connections)
        (list_actor_groups)
        (find_actor_group)
        (list_actor_friends)
        (get_actor_needs)
        (list_actor_mating_targets_by_zone)
        (stat_people_by_zone)
        (stat_people_by_base)
                     
        (get_contract_source_code)
    )

} } } // taiyi::plugins::baiyujing_api

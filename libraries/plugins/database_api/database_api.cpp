#include <chain/taiyi_fwd.hpp>

#include <appbase/application.hpp>

#include <plugins/database_api/database_api.hpp>
#include <plugins/database_api/database_api_plugin.hpp>

#include <protocol/get_config.hpp>
#include <protocol/exceptions.hpp>
#include <protocol/transaction_util.hpp>

#include <utilities/git_revision.hpp>

#include <fc/git_revision.hpp>

namespace taiyi { namespace plugins { namespace database_api {

    class database_api_impl
    {
    public:
        database_api_impl();
        ~database_api_impl();
        
        DECLARE_API_IMPL(
            (get_config)
            (get_version)
            (get_dynamic_global_properties)
            (get_siming_schedule)
            (get_hardfork_properties)
            (get_reward_funds)
            (list_simings)
            (find_simings)
            (list_siming_adores)
            (get_active_simings)
            (list_accounts)
            (find_accounts)
            (list_owner_histories)
            (find_owner_histories)
            (list_account_recovery_requests)
            (find_account_recovery_requests)
            (list_change_recovery_account_requests)
            (find_change_recovery_account_requests)
            (list_withdraw_qi_routes)
            (find_withdraw_qi_routes)
            (list_qi_delegations)
            (find_qi_delegations)
            (list_qi_delegation_expirations)
            (find_qi_delegation_expirations)
            (list_decline_adoring_rights_requests)
            (find_decline_adoring_rights_requests)
            (get_transaction_hex)
            (get_required_signatures)
            (get_potential_signatures)
            (verify_authority)
            (verify_account_authority)
            (verify_signatures)
            (find_account_resources)
            (find_nfa)
            (find_nfas)
            (list_nfas)
            (find_actor)
            (find_actors)
            (list_actors)
            (find_actor_talent_rules)
            (find_zones)
            (list_zones)
            (find_zones_by_name)
            (find_way_to_zone)
                         
            (get_tiandao_properties)
        )

        template< typename ResultType >
        static ResultType on_push_default( const ResultType& r ) { return r; }
        
        template< typename ValueType >
        static bool filter_default( const ValueType& r ) { return true; }

        template< typename ValueType >
        static bool stop_filter_default( const ValueType& r ) { return false; }

        template<typename IndexType, typename OrderType, typename StartType, typename ResultType, typename OnPushType, typename StopFilterType, typename FilterType>
        void iterate_results(StartType start, std::vector<ResultType>& result, uint32_t limit, OnPushType&& on_push, StopFilterType&& stop_filter, FilterType&& filter, order_direction_type direction = ascending )
        {
            const auto& idx = _db.get_index< IndexType, OrderType >();
            if( direction == ascending )
            {
                auto itr = idx.lower_bound( start );
                auto end = idx.end();
                
                while( result.size() < limit && itr != end )
                {
                    if( stop_filter( *itr ) )
                        break;
                    
                    if( filter( *itr ) )
                        result.push_back( on_push( *itr ) );
                    
                    ++itr;
                }
            }
            else if( direction == descending )
            {
                auto itr = boost::make_reverse_iterator( idx.lower_bound( start ) );
                auto end = idx.rend();
                
                while( result.size() < limit && itr != end )
                {
                    if( stop_filter( *itr ) )
                        break;
                    
                    if( filter( *itr ) )
                        result.push_back( on_push( *itr ) );
                    
                    ++itr;
                }
            }
        }
        
        chain::database& _db;
    };

    //********************************************************************
    //                                                                  //
    // Constructors                                                     //
    //                                                                  //
    //********************************************************************

    database_api::database_api() : my( new database_api_impl() )
    {
        JSON_RPC_REGISTER_API( TAIYI_DATABASE_API_PLUGIN_NAME );
    }
    
    database_api::~database_api() {}
    
    database_api_impl::database_api_impl() : _db( appbase::app().get_plugin< taiyi::plugins::chain::chain_plugin >().db() ) {}
    
    database_api_impl::~database_api_impl() {}

    //********************************************************************
    //                                                                  //
    // Globals                                                          //
    //                                                                  //
    //********************************************************************

    DEFINE_API_IMPL( database_api_impl, get_config )
    {
        return taiyi::protocol::get_config();
    }
    
    DEFINE_API_IMPL( database_api_impl, get_version )
    {
        return get_version_return
        (
         std::string( TAIYI_BLOCKCHAIN_VERSION ),
         std::string( taiyi::utilities::git_revision_sha ),
         std::string( fc::git_revision_sha ),
         _db.get_chain_id()
         );
    }
    
    DEFINE_API_IMPL( database_api_impl, get_dynamic_global_properties )
    {
        return _db.get_dynamic_global_properties();
    }
    
    DEFINE_API_IMPL( database_api_impl, get_siming_schedule )
    {
        return api_siming_schedule_object( _db.get_siming_schedule_object() );
    }
    
    DEFINE_API_IMPL( database_api_impl, get_hardfork_properties )
    {
        return _db.get_hardfork_property_object();
    }
    
    DEFINE_API_IMPL( database_api_impl, get_reward_funds )
    {
        get_reward_funds_return result;
        
        const auto& rf_idx = _db.get_index< reward_fund_index, chain::by_id >();
        auto itr = rf_idx.begin();
        
        while( itr != rf_idx.end() )
        {
            result.funds.push_back( *itr );
            ++itr;
        }
        
        return result;
    }

    //********************************************************************
    //                                                                  //
    // Simings                                                          //
    //                                                                  //
    //********************************************************************

    DEFINE_API_IMPL( database_api_impl, list_simings )
    {
        FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );
        
        list_simings_return result;
        result.simings.reserve( args.limit );
        
        switch( args.order )
        {
            case( by_name ):
            {
                iterate_results< chain::siming_index, chain::by_name >(
                    args.start.as< protocol::account_name_type >(),
                    result.simings,
                    args.limit,
                    [&]( const siming_object& w ){ return api_siming_object( w ); },
                    &database_api_impl::stop_filter_default< siming_object >,
                    &database_api_impl::filter_default< siming_object >
                );
                break;
            }
            case( by_adore_name ):
            {
                auto key = args.start.as< std::pair< share_type, account_name_type > >();
                iterate_results< chain::siming_index, chain::by_adore_name >(
                    boost::make_tuple( key.first, key.second ),
                    result.simings,
                    args.limit,
                    [&]( const siming_object& w ){ return api_siming_object( w ); },
                    &database_api_impl::stop_filter_default< siming_object >,
                    &database_api_impl::filter_default< siming_object >
                );
                break;
            }
            case( by_schedule_time ):
            {
                auto key = args.start.as< std::pair< fc::uint128, account_name_type > >();
                auto wit_id = _db.get< chain::siming_object, chain::by_name >( key.second ).id;
                iterate_results< chain::siming_index, chain::by_schedule_time >(
                    boost::make_tuple( key.first, wit_id ),
                    result.simings,
                    args.limit,
                    [&]( const siming_object& w ){ return api_siming_object( w ); },
                    &database_api_impl::stop_filter_default< siming_object >,
                    &database_api_impl::filter_default< siming_object >
                );
                break;
            }
            default:
                FC_ASSERT( false, "Unknown or unsupported sort order" );
        }
        
        return result;
    }

    DEFINE_API_IMPL( database_api_impl, find_simings )
    {
        FC_ASSERT( args.owners.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );
        
        find_simings_return result;
        
        for( auto& o : args.owners )
        {
            auto siming = _db.find< chain::siming_object, chain::by_name >( o );
            
            if( siming != nullptr )
                result.simings.push_back( api_siming_object( *siming ) );
        }
        
        return result;
    }

    DEFINE_API_IMPL( database_api_impl, list_siming_adores )
    {
        FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );
        
        list_siming_adores_return result;
        result.adores.reserve( args.limit );
        
        switch( args.order )
        {
            case( by_account_siming ):
            {
                auto key = args.start.as< std::pair< account_name_type, account_name_type > >();
                    iterate_results< chain::siming_adore_index, chain::by_account_siming >(
                    boost::make_tuple( key.first, key.second ),
                    result.adores,
                    args.limit,
                    [&]( const siming_adore_object& v ){ return api_siming_adore_object( v ); },
                    &database_api_impl::stop_filter_default< api_siming_adore_object >,
                    &database_api_impl::filter_default< api_siming_adore_object >
                );
                break;
            }
            case( by_siming_account ):
            {
                auto key = args.start.as< std::pair< account_name_type, account_name_type > >();
                iterate_results< chain::siming_adore_index, chain::by_siming_account >(
                    boost::make_tuple( key.first, key.second ),
                    result.adores,
                    args.limit,
                    [&]( const siming_adore_object& v ){ return api_siming_adore_object( v ); },
                    &database_api_impl::stop_filter_default< api_siming_adore_object >,
                    &database_api_impl::filter_default< api_siming_adore_object >
                );
                break;
            }
            default:
                FC_ASSERT( false, "Unknown or unsupported sort order" );
        }
        
        return result;
    }

    DEFINE_API_IMPL( database_api_impl, get_active_simings )
    {
        const auto& wso = _db.get_siming_schedule_object();
        size_t n = wso.current_shuffled_simings.size();
        get_active_simings_return result;
        result.simings.reserve( n );
        for( size_t i=0; i<n; i++ )
            result.simings.push_back( wso.current_shuffled_simings[i] );
        return result;
    }


    //********************************************************************
    //                                                                  //
    // Accounts                                                         //
    //                                                                  //
    //********************************************************************

    DEFINE_API_IMPL( database_api_impl, list_accounts )
    {
        FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );
        
        list_accounts_return result;
        result.accounts.reserve( args.limit );
        
        switch( args.order )
        {
            case( by_name ):
            {
                iterate_results< chain::account_index, chain::by_name >(
                    args.start.as< protocol::account_name_type >(),
                    result.accounts,
                    args.limit,
                    [&]( const account_object& a ){ return api_account_object( a, _db ); },
                    &database_api_impl::stop_filter_default< account_object >,
                    &database_api_impl::filter_default< account_object >
                );
                break;
            }
            case( by_proxy ):
            {
                auto key = args.start.as< std::pair< account_name_type, account_name_type > >();
                iterate_results< chain::account_index, chain::by_proxy >(
                    boost::make_tuple( key.first, key.second ),
                    result.accounts,
                    args.limit,
                    [&]( const account_object& a ){ return api_account_object( a, _db ); },
                    &database_api_impl::stop_filter_default< account_object >,
                    &database_api_impl::filter_default< account_object >
                );
                break;
            }
            case( by_next_qi_withdrawal_time ):
            {
                auto key = args.start.as< std::pair< fc::time_point_sec, account_name_type > >();
                iterate_results< chain::account_index, chain::by_next_qi_withdrawal_time >(
                    boost::make_tuple( key.first, key.second ),
                    result.accounts,
                    args.limit,
                    [&]( const account_object& a ){ return api_account_object( a, _db ); },
                    &database_api_impl::stop_filter_default< account_object >,
                    &database_api_impl::filter_default< account_object >
                );
                break;
            }
            default:
                FC_ASSERT( false, "Unknown or unsupported sort order" );
        }
        
        return result;
    }

    DEFINE_API_IMPL( database_api_impl, find_accounts )
    {
        find_accounts_return result;
        FC_ASSERT( args.accounts.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );
        
        for( auto& a : args.accounts )
        {
            auto acct = _db.find< chain::account_object, chain::by_name >( a );
            if( acct != nullptr )
                result.accounts.push_back( api_account_object( *acct, _db ) );
        }
        
        return result;
    }

    /* Owner Auth Histories */
    
    DEFINE_API_IMPL( database_api_impl, list_owner_histories )
    {
        FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );
        
        list_owner_histories_return result;
        result.owner_auths.reserve( args.limit );
        
        auto key = args.start.as< std::pair< account_name_type, fc::time_point_sec > >();
        iterate_results< chain::owner_authority_history_index, chain::by_account >(
            boost::make_tuple( key.first, key.second ),
            result.owner_auths,
            args.limit,
            [&]( const owner_authority_history_object& o ){ return api_owner_authority_history_object( o ); },
            &database_api_impl::stop_filter_default< owner_authority_history_object >,
            &database_api_impl::filter_default< owner_authority_history_object >
        );
        
        return result;
    }

    DEFINE_API_IMPL( database_api_impl, find_owner_histories )
    {
        find_owner_histories_return result;
        
        const auto& hist_idx = _db.get_index< chain::owner_authority_history_index, chain::by_account >();
        auto itr = hist_idx.lower_bound( args.owner );
        
        while( itr != hist_idx.end() && itr->account == args.owner && result.owner_auths.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
        {
            result.owner_auths.push_back( api_owner_authority_history_object( *itr ) );
            ++itr;
        }
        
        return result;
    }

    /* Account Recovery Requests */
    
    DEFINE_API_IMPL( database_api_impl, list_account_recovery_requests )
    {
        FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );
        
        list_account_recovery_requests_return result;
        result.requests.reserve( args.limit );
        
        switch( args.order )
        {
            case( by_account ):
            {
                iterate_results< chain::account_recovery_request_index, chain::by_account >(
                    args.start.as< account_name_type >(),
                    result.requests,
                    args.limit,
                    [&]( const account_recovery_request_object& a ){ return api_account_recovery_request_object( a ); },
                    &database_api_impl::stop_filter_default< api_account_recovery_request_object >,
                    &database_api_impl::filter_default< api_account_recovery_request_object >
                );
                break;
            }
            case( by_expiration ):
            {
                auto key = args.start.as< std::pair< fc::time_point_sec, account_name_type > >();
                iterate_results< chain::account_recovery_request_index, chain::by_expiration >(
                    boost::make_tuple( key.first, key.second ),
                    result.requests,
                    args.limit,
                    [&]( const account_recovery_request_object& a ){ return api_account_recovery_request_object( a ); },
                    &database_api_impl::stop_filter_default< api_account_recovery_request_object >,
                    &database_api_impl::filter_default< api_account_recovery_request_object >
                );
                break;
            }
            default:
                FC_ASSERT( false, "Unknown or unsupported sort order" );
        }
        
        return result;
    }

    DEFINE_API_IMPL( database_api_impl, find_account_recovery_requests )
    {
        find_account_recovery_requests_return result;
        FC_ASSERT( args.accounts.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );
        
        for( auto& a : args.accounts )
        {
            auto request = _db.find< chain::account_recovery_request_object, chain::by_account >( a );
            
            if( request != nullptr )
                result.requests.push_back( api_account_recovery_request_object( *request ) );
        }
        
        return result;
    }

    /* Change Recovery Account Requests */
    
    DEFINE_API_IMPL( database_api_impl, list_change_recovery_account_requests )
    {
        FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );
        
        list_change_recovery_account_requests_return result;
        result.requests.reserve( args.limit );
        
        switch( args.order )
        {
            case( by_account ):
            {
                iterate_results< chain::change_recovery_account_request_index, chain::by_account >(
                    args.start.as< account_name_type >(),
                    result.requests,
                    args.limit,
                    &database_api_impl::on_push_default< change_recovery_account_request_object >,
                    &database_api_impl::stop_filter_default< change_recovery_account_request_object >,
                    &database_api_impl::filter_default< change_recovery_account_request_object >
                );
                break;
            }
            case( by_effective_date ):
            {
                auto key = args.start.as< std::pair< fc::time_point_sec, account_name_type > >();
                iterate_results< chain::change_recovery_account_request_index, chain::by_effective_date >(
                    boost::make_tuple( key.first, key.second ),
                    result.requests,
                    args.limit,
                    &database_api_impl::on_push_default< change_recovery_account_request_object >,
                    &database_api_impl::stop_filter_default< change_recovery_account_request_object >,
                    &database_api_impl::filter_default< change_recovery_account_request_object >
                );
                break;
            }
            default:
                FC_ASSERT( false, "Unknown or unsupported sort order" );
        }
        
        return result;
    }

    DEFINE_API_IMPL( database_api_impl, find_change_recovery_account_requests )
    {
        find_change_recovery_account_requests_return result;
        FC_ASSERT( args.accounts.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );
        
        for( auto& a : args.accounts )
        {
            auto request = _db.find< chain::change_recovery_account_request_object, chain::by_account >( a );
            
            if( request != nullptr )
                result.requests.push_back( *request );
        }
        
        return result;
    }

    /* Withdraw Qi Routes */
    
    DEFINE_API_IMPL( database_api_impl, list_withdraw_qi_routes )
    {
        FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );
        
        list_withdraw_qi_routes_return result;
        result.routes.reserve( args.limit );
        
        switch( args.order )
        {
            case( by_withdraw_route ):
            {
                auto key = args.start.as< std::pair< account_name_type, account_name_type > >();
                iterate_results< chain::withdraw_qi_route_index, chain::by_withdraw_route >(
                    boost::make_tuple( key.first, key.second ),
                    result.routes,
                    args.limit,
                    &database_api_impl::on_push_default< withdraw_qi_route_object >,
                    &database_api_impl::stop_filter_default< withdraw_qi_route_object >,
                    &database_api_impl::filter_default< withdraw_qi_route_object >
                );
                break;
            }
            case( by_destination ):
            {
                auto key = args.start.as< std::pair< account_name_type, withdraw_qi_route_id_type > >();
                iterate_results< chain::withdraw_qi_route_index, chain::by_destination >(
                    boost::make_tuple( key.first, key.second ),
                    result.routes,
                    args.limit,
                    &database_api_impl::on_push_default< withdraw_qi_route_object >,
                    &database_api_impl::stop_filter_default< withdraw_qi_route_object >,
                    &database_api_impl::filter_default< withdraw_qi_route_object >
                );
                break;
            }
            default:
                FC_ASSERT( false, "Unknown or unsupported sort order" );
        }
        
        return result;
    }

    DEFINE_API_IMPL( database_api_impl, find_withdraw_qi_routes )
    {
        find_withdraw_qi_routes_return result;
        
        switch( args.order )
        {
            case( by_withdraw_route ):
            {
                const auto& route_idx = _db.get_index< chain::withdraw_qi_route_index, chain::by_withdraw_route >();
                auto itr = route_idx.lower_bound( args.account );
                
                while( itr != route_idx.end() && itr->from_account == args.account && result.routes.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
                {
                    result.routes.push_back( *itr );
                    ++itr;
                }
                
                break;
            }
            case( by_destination ):
            {
                const auto& route_idx = _db.get_index< chain::withdraw_qi_route_index, chain::by_destination >();
                auto itr = route_idx.lower_bound( args.account );
                
                while( itr != route_idx.end() && itr->to_account == args.account && result.routes.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
                {
                    result.routes.push_back( *itr );
                    ++itr;
                }
                
                break;
            }
            default:
                FC_ASSERT( false, "Unknown or unsupported sort order" );
        }
        
        return result;
    }

    /* Qi Delegations */
    
    DEFINE_API_IMPL( database_api_impl, list_qi_delegations )
    {
        FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );
        
        list_qi_delegations_return result;
        result.delegations.reserve( args.limit );
        
        switch( args.order )
        {
            case( by_delegation ):
            {
                auto key = args.start.as< std::pair< account_name_type, account_name_type > >();
                iterate_results< chain::qi_delegation_index, chain::by_delegation >(
                    boost::make_tuple( key.first, key.second ),
                    result.delegations,
                    args.limit,
                    &database_api_impl::on_push_default< api_qi_delegation_object >,
                    &database_api_impl::stop_filter_default< qi_delegation_object >,
                    &database_api_impl::filter_default< qi_delegation_object >
                );
                break;
            }
            default:
                FC_ASSERT( false, "Unknown or unsupported sort order" );
        }
        
        return result;
    }

    DEFINE_API_IMPL( database_api_impl, find_qi_delegations )
    {
        find_qi_delegations_return result;
        const auto& delegation_idx = _db.get_index< chain::qi_delegation_index, chain::by_delegation >();
        auto itr = delegation_idx.lower_bound( args.account );
        
        while( itr != delegation_idx.end() && itr->delegator == args.account && result.delegations.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
        {
            result.delegations.push_back( api_qi_delegation_object( *itr ) );
            ++itr;
        }
        
        return result;
    }
    
    /* Qi Delegation Expirations */
    
    DEFINE_API_IMPL( database_api_impl, list_qi_delegation_expirations )
    {
        FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );
        
        list_qi_delegation_expirations_return result;
        result.delegations.reserve( args.limit );
        
        switch( args.order )
        {
            case( by_expiration ):
            {
                auto key = args.start.as< std::pair< time_point_sec, qi_delegation_expiration_id_type > >();
                iterate_results< chain::qi_delegation_expiration_index, chain::by_expiration >(
                    boost::make_tuple( key.first, key.second ),
                    result.delegations,
                    args.limit,
                    &database_api_impl::on_push_default< api_qi_delegation_expiration_object >,
                    &database_api_impl::stop_filter_default< qi_delegation_expiration_object >,
                    &database_api_impl::filter_default< qi_delegation_expiration_object >
                );
                break;
            }
            case( by_account_expiration ):
            {
                auto key = args.start.as< std::vector< fc::variant > >();
                FC_ASSERT( key.size() == 3, "by_account_expiration start requires 3 values. (account_name_type, time_point_sec, qi_delegation_expiration_id_type" );
                iterate_results< chain::qi_delegation_expiration_index, chain::by_account_expiration >(
                    boost::make_tuple( key[0].as< account_name_type >(), key[1].as< time_point_sec >(), key[2].as< qi_delegation_expiration_id_type >() ),
                    result.delegations,
                    args.limit,
                    &database_api_impl::on_push_default< api_qi_delegation_expiration_object >,
                    &database_api_impl::stop_filter_default< qi_delegation_expiration_object >,
                    &database_api_impl::filter_default< qi_delegation_expiration_object >
                );
                break;
            }
            default:
                FC_ASSERT( false, "Unknown or unsupported sort order" );
        }
        
        return result;
    }

    DEFINE_API_IMPL( database_api_impl, find_qi_delegation_expirations )
    {
        find_qi_delegation_expirations_return result;
        const auto& del_exp_idx = _db.get_index< chain::qi_delegation_expiration_index, chain::by_account_expiration >();
        auto itr = del_exp_idx.lower_bound( args.account );
        
        while( itr != del_exp_idx.end() && itr->delegator == args.account && result.delegations.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
        {
            result.delegations.push_back( *itr );
            ++itr;
        }
        
        return result;
    }
    
    /* Decline Qi Rights Requests */
    
    DEFINE_API_IMPL( database_api_impl, list_decline_adoring_rights_requests )
    {
        FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );
        
        list_decline_adoring_rights_requests_return result;
        result.requests.reserve( args.limit );
        
        switch( args.order )
        {
            case( by_account ):
            {
                iterate_results< chain::decline_adoring_rights_request_index, chain::by_account >(
                    args.start.as< account_name_type >(),
                    result.requests,
                    args.limit,
                    &database_api_impl::on_push_default< api_decline_adoring_rights_request_object >,
                    &database_api_impl::stop_filter_default< decline_adoring_rights_request_object >,
                    &database_api_impl::filter_default< decline_adoring_rights_request_object >
                );
                break;
            }
            case( by_effective_date ):
            {
                auto key = args.start.as< std::pair< time_point_sec, account_name_type > >();
                iterate_results< chain::decline_adoring_rights_request_index, chain::by_effective_date >(
                    boost::make_tuple( key.first, key.second ),
                    result.requests,
                    args.limit,
                    &database_api_impl::on_push_default< api_decline_adoring_rights_request_object >,
                    &database_api_impl::stop_filter_default< decline_adoring_rights_request_object >,
                    &database_api_impl::filter_default< decline_adoring_rights_request_object >
                );
                break;
            }
            default:
                FC_ASSERT( false, "Unknown or unsupported sort order" );
        }
        
        return result;
    }

    DEFINE_API_IMPL( database_api_impl, find_decline_adoring_rights_requests )
    {
        FC_ASSERT( args.accounts.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );
        find_decline_adoring_rights_requests_return result;
        
        for( auto& a : args.accounts )
        {
            auto request = _db.find< chain::decline_adoring_rights_request_object, chain::by_account >( a );
            
            if( request != nullptr )
                result.requests.push_back( *request );
        }
        
        return result;
    }
    
    //********************************************************************
    //                                                                  //
    // Authority / Validation                                           //
    //                                                                  //
    //********************************************************************

    DEFINE_API_IMPL( database_api_impl, get_transaction_hex )
    {
        return get_transaction_hex_return( { fc::to_hex( fc::raw::pack_to_vector( args.trx ) ) } );
    }
    
    DEFINE_API_IMPL( database_api_impl, get_required_signatures )
    {
        get_required_signatures_return result;
        result.keys = args.trx.get_required_signatures(
            _db.get_chain_id(),
            args.available_keys,
            [&]( string account_name ) {
                return authority( _db.get< chain::account_authority_object, chain::by_account >( account_name ).active );
            },
            [&]( string account_name ){
                return authority( _db.get< chain::account_authority_object, chain::by_account >( account_name ).owner );
            },
            [&]( string account_name ) {
                return authority( _db.get< chain::account_authority_object, chain::by_account >( account_name ).posting );
            },
            TAIYI_MAX_SIG_CHECK_DEPTH,
            fc::ecc::canonical_signature_type::bip_0062
        );
        
        return result;
    }

    DEFINE_API_IMPL( database_api_impl, get_potential_signatures )
    {
        get_potential_signatures_return result;
        args.trx.get_required_signatures(
            _db.get_chain_id(),
            flat_set< public_key_type >(),
            [&]( account_name_type account_name ) {
                const auto& auth = _db.get< chain::account_authority_object, chain::by_account >( account_name ).active;
                for( const auto& k : auth.get_keys() )
                    result.keys.insert( k );
                return authority( auth );
            },
            [&]( account_name_type account_name ) {
                const auto& auth = _db.get< chain::account_authority_object, chain::by_account >( account_name ).owner;
                for( const auto& k : auth.get_keys() )
                    result.keys.insert( k );
                return authority( auth );
            },
            [&]( account_name_type account_name ) {
                const auto& auth = _db.get< chain::account_authority_object, chain::by_account >( account_name ).posting;
                for( const auto& k : auth.get_keys() )
                    result.keys.insert( k );
                return authority( auth );
            },
            TAIYI_MAX_SIG_CHECK_DEPTH,
            fc::ecc::canonical_signature_type::bip_0062
        );
        
        return result;
    }

    DEFINE_API_IMPL( database_api_impl, verify_authority )
    {
        args.trx.verify_authority(
            _db.get_chain_id(),
            [&]( string account_name ) {
                return authority( _db.get< chain::account_authority_object, chain::by_account >( account_name ).active );
            },
            [&]( string account_name ) {
                return authority( _db.get< chain::account_authority_object, chain::by_account >( account_name ).owner );
            },
            [&]( string account_name ) {
                return authority( _db.get< chain::account_authority_object, chain::by_account >( account_name ).posting );
            },
            TAIYI_MAX_SIG_CHECK_DEPTH,
            TAIYI_MAX_AUTHORITY_MEMBERSHIP,
            TAIYI_MAX_SIG_CHECK_ACCOUNTS,
            fc::ecc::canonical_signature_type::bip_0062
        );
        return verify_authority_return( { true } );
    }

    // TODO: This is broken. By the look of is, it has been since BitShares. verify_authority always
    // returns false because the TX is not signed.
    DEFINE_API_IMPL( database_api_impl, verify_account_authority )
    {
        auto account = _db.find< chain::account_object, chain::by_name >( args.account );
        FC_ASSERT( account != nullptr, "no such account" );
        
        /// reuse trx.verify_authority by creating a dummy transfer
        verify_authority_args vap;
        transfer_operation op;
        op.from = account->name;
        vap.trx.operations.emplace_back( op );
        
        return verify_authority( vap );
    }
    
    DEFINE_API_IMPL( database_api_impl, verify_signatures )
    {
        // get_signature_keys can throw for dup sigs. Allow this to throw.
        flat_set< public_key_type > sig_keys;
        for( const auto&  sig : args.signatures )
        {
            TAIYI_ASSERT(
                         sig_keys.insert( fc::ecc::public_key( sig, args.hash ) ).second,
                         protocol::tx_duplicate_sig,
                         "Duplicate Signature detected" );
        }
        
        verify_signatures_return result;
        result.valid = true;
        
        // verify authority throws on failure, catch and return false
        try
        {
            taiyi::protocol::verify_authority< verify_signatures_args >(
                { args },
                sig_keys,
                [this]( const string& name ) {
                    return authority( _db.get< chain::account_authority_object, chain::by_account >( name ).owner );
                },
                [this]( const string& name ) {
                    return authority( _db.get< chain::account_authority_object, chain::by_account >( name ).active );
                },
                [this]( const string& name ) {
                    return authority( _db.get< chain::account_authority_object, chain::by_account >( name ).posting );
                },
                TAIYI_MAX_SIG_CHECK_DEPTH
            );
        }
        catch( fc::exception& ) { result.valid = false; }
        
        return result;
    }
    
    DEFINE_API_IMPL( database_api_impl, find_account_resources )
    {
        find_account_resources_return result;
        FC_ASSERT( args.accounts.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );

        for( auto& a : args.accounts )
        {
            auto acct = _db.find< chain::account_object, chain::by_name >( a );
            if( acct != nullptr ) {
                resource_assets res;
                res.gold = _db.get_balance(*acct, GOLD_SYMBOL);
                res.food = _db.get_balance(*acct, FOOD_SYMBOL);
                res.wood = _db.get_balance(*acct, WOOD_SYMBOL);
                res.fabric = _db.get_balance(*acct, FABRIC_SYMBOL);
                res.herb = _db.get_balance(*acct, HERB_SYMBOL);

                result.resources.push_back( res );
            }
        }

        return result;
    }
    
    //*************************************************************************
    //                                                                       //
    // NFAs                                                                  //
    //                                                                       //
    //*************************************************************************

    DEFINE_API_IMPL( database_api_impl, find_nfa )
    {
        find_nfa_return result;
        
        auto nfa = _db.find< chain::nfa_object, chain::by_id >( args.id );
        if( nfa != nullptr )
            result.result = api_nfa_object( *nfa, _db );

        return result;
    }

    DEFINE_API_IMPL( database_api_impl, find_nfas )
    {
        FC_ASSERT( args.ids.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );

        find_nfas_return result;
        result.result.reserve( args.ids.size() );

        std::for_each( args.ids.begin(), args.ids.end(), [&](auto& id) {
            auto nfa = _db.find< chain::nfa_object, chain::by_id >( id );
            if( nfa != nullptr )
                result.result.emplace_back( api_nfa_object( *nfa, _db ) );
        });

        return result;
    }

    DEFINE_API_IMPL( database_api_impl, list_nfas )
    {
        FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

        list_nfas_return result;
        result.result.reserve( args.limit );

        switch( args.order )
        {
            case( by_owner ):
            {
                auto key = args.start.as< protocol::account_name_type >();
                const auto& key_account = _db.get_account(key);
                iterate_results< chain::nfa_index, chain::by_owner >(
                    boost::make_tuple( key_account.id, 0 ),
                    result.result,
                    args.limit,
                    [&]( const nfa_object& o ) { return api_nfa_object( o, _db ); },
                    [&]( const nfa_object& o ) { return o.owner_account != key_account.id; },
                    &database_api_impl::filter_default< nfa_object >
                );
                break;
            }
            case( by_creator ):
            {
                auto key = args.start.as< protocol::account_name_type >();
                const auto& key_account = _db.get_account(key);
                iterate_results< chain::nfa_index, chain::by_creator >(
                    boost::make_tuple( key_account.id, 0 ),
                    result.result,
                    args.limit,
                    [&]( const nfa_object& o ) { return api_nfa_object( o, _db ); },
                    [&]( const nfa_object& o ) { return o.creator_account != key_account.id; },
                    &database_api_impl::filter_default< nfa_object >
                );
                break;
            }
            case( by_symbol ):
            {
                auto key = args.start.as< string >();
                const auto& key_symbol = _db.get<nfa_symbol_object, chain::by_symbol>(key);
                iterate_results< chain::nfa_index, chain::by_symbol >(
                    boost::make_tuple( key_symbol.id, 0 ),
                    result.result,
                    args.limit,
                    [&]( const nfa_object& o ) { return api_nfa_object( o, _db ); },
                    [&]( const nfa_object& o ) { return o.symbol_id != key_symbol.id; },
                    &database_api_impl::filter_default< nfa_object >
                );
                break;
            }
            default:
                FC_ASSERT( false, "Unknown or unsupported sort order" );
        }

        return result;
    }
    
    //************************************************************************
    //                                                                      //
    // Actors                                                               //
    //                                                                      //
    //************************************************************************

    DEFINE_API_IMPL( database_api_impl, find_actor )
    {
        find_actor_return result;
        
        auto act = _db.find< chain::actor_object, chain::by_name >( args.name );
        if( act != nullptr )
            result.result = api_actor_object( *act, _db );

        return result;
    }

    DEFINE_API_IMPL( database_api_impl, find_actors )
    {
        FC_ASSERT( args.actor_ids.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );

        find_actors_return result;
        result.result.reserve( args.actor_ids.size() );

        std::for_each( args.actor_ids.begin(), args.actor_ids.end(), [&](auto& id) {
            auto act = _db.find< chain::actor_object, chain::by_id >( id );
            if( act != nullptr )
            {
                result.result.emplace_back( api_actor_object( *act, _db ) );
            }
        });

        return result;
    }

    DEFINE_API_IMPL( database_api_impl, list_actors )
    {
        FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

        list_actors_return result;
        result.result.reserve( args.limit );

        switch( args.order )
        {
            case( by_owner ):
            {
                auto key = args.start.as< protocol::account_name_type >();
                const auto& key_account = _db.get_account(key);
                iterate_results< chain::nfa_index, chain::by_owner >(
                    boost::make_tuple( key_account.id, 0 ),
                    result.result,
                    args.limit,
                    [&]( const nfa_object& o ) { return api_actor_object( _db.get<actor_object, by_nfa_id>(o.id), _db ); },
                    [&]( const nfa_object& o ) { return o.owner_account != key_account.id; }, //stop condition
                    [&]( const nfa_object& o ) { return _db.find<actor_object, by_nfa_id>(o.id) != nullptr; }
                );
                break;
            }
            case( by_health ):
            {
                auto key = args.start.as< int16_t >();
                iterate_results< chain::actor_index, chain::by_health >(
                    boost::make_tuple( key, 0 ),
                    result.result,
                    args.limit,
                    [&]( const actor_object& a ) { return api_actor_object( a, _db ); },
                    [&]( const actor_object& a ) { return a.health > key; },  //stop condition
                    &database_api_impl::filter_default< actor_object >
                );
                break;
            }
            case( by_solor_term ):
            {
                auto key = args.start.as< int >();
                iterate_results< chain::actor_index, chain::by_solor_term >(
                    boost::make_tuple( key, 0 ),
                    result.result,
                    args.limit,
                    [&]( const actor_object& a ) { return api_actor_object( a, _db ); },
                    [&]( const actor_object& a ) { return a.born_vtimes != key; },  //stop condition
                    &database_api_impl::filter_default< actor_object >
                );
                break;
            }
            case( by_location ):
            {
                const zone_object* zone = _db.find< chain::zone_object, chain::by_name >(args.start.as< string >());
                if(zone) {
                    iterate_results< chain::actor_index, chain::by_location >(
                        zone->id,
                        result.result,
                        args.limit,
                        [&]( const actor_object& a ) { return api_actor_object( a, _db ); },
                        [&]( const actor_object& a ) { return a.location != zone->id; }, //stop condition
                        &database_api_impl::filter_default< actor_object >
                    );
                }
                break;
            }
            default:
                FC_ASSERT( false, "Unknown or unsupported sort order" );
        }

        return result;
    }

    DEFINE_API_IMPL( database_api_impl, find_actor_talent_rules )
    {
        FC_ASSERT( args.ids.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );

        find_actor_talent_rules_return result;
        result.rules.reserve( args.ids.size() );

        std::for_each( args.ids.begin(), args.ids.end(), [&](auto& id) {
            auto po = _db.find< chain::actor_talent_rule_object, chain::by_id >( id );
            if( po != nullptr && !po->removed )
            {
                result.rules.push_back( *po );
            }
        });

        return result;
    }

    /* Zones */

    DEFINE_API_IMPL( database_api_impl, find_zones )
    {
        FC_ASSERT( args.ids.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );

        find_zones_return result;
        result.result.reserve( args.ids.size() );

        std::for_each( args.ids.begin(), args.ids.end(), [&](auto& id) {
            auto z = _db.find< chain::zone_object, chain::by_id >( id );
            if( z != nullptr )
                result.result.push_back( *z );
        });

        return result;
    }

    DEFINE_API_IMPL( database_api_impl, list_zones )
    {
        FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

        list_zones_return result;
        result.result.reserve( args.limit );

        switch( args.order )
        {
            case( by_owner ):
            {
                auto key = args.start.as< protocol::account_name_type >();
                const auto& key_account = _db.get_account(key);
                iterate_results< chain::nfa_index, chain::by_owner >(
                    boost::make_tuple( key_account.id, 0 ),
                    result.result,
                    args.limit,
                    [&]( const nfa_object& o ) { return _db.get<zone_object, by_nfa_id>(o.id); },
                    [&]( const nfa_object& o ) { return o.owner_account != key_account.id; }, //stop condition
                    [&]( const nfa_object& o ) { return _db.find<zone_object, by_nfa_id>(o.id) != nullptr; }
                );
                break;
            }
            case( by_type ):
            {
                auto key = args.start.as< E_ZONE_TYPE >();
                iterate_results< chain::zone_index, chain::by_type >(
                    boost::make_tuple( key, 0 ),
                    result.result,
                    args.limit,
                    [&]( const zone_object& a ) { return a; },
                    [&]( const zone_object& a ) { return a.type != key; }, //stop condition
                    &database_api_impl::filter_default< zone_object >
                );
                break;
            }
            case( by_zone_from ):
            {
                const auto* from_zone = _db.find< chain::zone_object, chain::by_name >(args.start.as< string >());
                if(from_zone) {
                    iterate_results< chain::zone_connect_index, chain::by_zone_from >(
                        boost::make_tuple( from_zone->id, 0 ),
                        result.result,
                        args.limit,
                        [&]( const zone_connect_object& a ) { return _db.get< chain::zone_object, chain::by_id >(a.to); },
                        [&]( const zone_connect_object& a ) { return a.from != from_zone->id; }, //stop condition
                        &database_api_impl::filter_default< zone_connect_object >
                    );
                }
                break;
            }
            case( by_zone_to ):
            {
                const auto* to_zone = _db.find< chain::zone_object, chain::by_name >(args.start.as< string >());
                if(to_zone) {
                    iterate_results< chain::zone_connect_index, chain::by_zone_to >(
                        boost::make_tuple( to_zone->id, 0 ),
                        result.result,
                        args.limit,
                        [&]( const zone_connect_object& a ) { return _db.get< chain::zone_object, chain::by_id >(a.from); },
                        [&]( const zone_connect_object& a ) { return a.to != to_zone->id; }, //stop condition
                        &database_api_impl::filter_default< zone_connect_object >
                    );
                }
                break;
            }
            default:
                FC_ASSERT( false, "Unknown or unsupported sort order" );
        }

        return result;
    }

    DEFINE_API_IMPL( database_api_impl, find_zones_by_name )
    {
        FC_ASSERT( args.name_list.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );

        find_zones_by_name_return result;
        result.result.reserve( args.name_list.size() );

        std::for_each( args.name_list.begin(), args.name_list.end(), [&](auto& name) {
            auto z = _db.find< chain::zone_object, chain::by_name >( name );
            if( z != nullptr )
                result.result.push_back( *z );
        });

        return result;
    }
    
    //pre_from 用于避免相邻节点的循环通路
    bool is_connect(chain::zone_id_type from, chain::zone_id_type dest, chain::zone_id_type pre_from, int depth, const mira::index_adapter< chain::zone_connect_index, chain::by_zone_from >& zone_connect_by_from_idx, std::vector<string>& way_points, chain::database& db) {
        if(depth > 10)
            return false;
        auto itz = zone_connect_by_from_idx.lower_bound( from );
        while(itz != zone_connect_by_from_idx.end()) {
            if(itz->from != from)
                break;
            if(itz->to != pre_from) {
                if(itz->to == dest) {
                    way_points.push_back(db.get< chain::zone_object, chain::by_id >(dest).name);
                    return true;
                }
                if(is_connect(itz->to, dest, from, depth+1, zone_connect_by_from_idx, way_points, db)) {
                    way_points.insert(way_points.begin(), db.get< chain::zone_object, chain::by_id >(itz->to).name);
                    return true;
                }
            }
            itz++;
        }
        return false;
    }

    DEFINE_API_IMPL( database_api_impl, find_way_to_zone )
    {
        const auto* from_zone = _db.find< chain::zone_object, chain::by_name >( args.from_zone );
        FC_ASSERT( from_zone != nullptr );
        const auto* to_zone = _db.find< chain::zone_object, chain::by_name >( args.to_zone );
        FC_ASSERT( to_zone != nullptr );

        find_way_to_zone_return result;
        const auto& zone_connect_by_from_idx = _db.get_index< zone_connect_index >().indices().get< chain::by_zone_from >();
        auto itz = zone_connect_by_from_idx.lower_bound( from_zone->id );
        while(itz != zone_connect_by_from_idx.end()) {
            if(itz->from != from_zone->id)
                break;
            if(itz->to == to_zone->id) {
                result.way_points.push_back(to_zone->name);
                break;
            }
            if(is_connect(itz->to, to_zone->id, from_zone->id, 1, zone_connect_by_from_idx, result.way_points, _db)) {
                result.way_points.insert(result.way_points.begin(), _db.get< chain::zone_object, chain::by_id >(itz->to).name);
                break;
            }
            itz++;
        }
        return result;
    }
    
    DEFINE_API_IMPL( database_api_impl, get_tiandao_properties )
    {
       return _db.get_tiandao_properties();
    }

    DEFINE_LOCKLESS_APIS( database_api, (get_config)(get_version) )
    
    DEFINE_READ_APIS( database_api,
        (get_dynamic_global_properties)
        (get_siming_schedule)
        (get_hardfork_properties)
        (get_reward_funds)
        (list_simings)
        (find_simings)
        (list_siming_adores)
        (get_active_simings)
        (list_accounts)
        (find_accounts)
        (list_owner_histories)
        (find_owner_histories)
        (list_account_recovery_requests)
        (find_account_recovery_requests)
        (list_change_recovery_account_requests)
        (find_change_recovery_account_requests)
        (list_withdraw_qi_routes)
        (find_withdraw_qi_routes)
        (list_qi_delegations)
        (find_qi_delegations)
        (list_qi_delegation_expirations)
        (find_qi_delegation_expirations)
        (list_decline_adoring_rights_requests)
        (find_decline_adoring_rights_requests)
        (get_transaction_hex)
        (get_required_signatures)
        (get_potential_signatures)
        (verify_authority)
        (verify_account_authority)
        (verify_signatures)
        (find_account_resources)
        (find_nfa)
        (find_nfas)
        (list_nfas)
        (find_actor)
        (find_actors)
        (list_actors)
        (find_actor_talent_rules)
        (find_zones)
        (list_zones)
        (find_zones_by_name)
        (find_way_to_zone)
                     
        (get_tiandao_properties)
    )
    
} } } // taiyi::plugins::database_api

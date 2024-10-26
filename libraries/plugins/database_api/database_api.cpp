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
        )

        template< typename ResultType >
        static ResultType on_push_default( const ResultType& r ) { return r; }
        
        template< typename ValueType >
        static bool filter_default( const ValueType& r ) { return true; }
        
        template<typename IndexType, typename OrderType, typename StartType, typename ResultType, typename OnPushType, typename FilterType>
        void iterate_results(StartType start, std::vector<ResultType>& result, uint32_t limit, OnPushType&& on_push, FilterType&& filter, order_direction_type direction = ascending )
        {
            const auto& idx = _db.get_index< IndexType, OrderType >();
            if( direction == ascending )
            {
                auto itr = idx.lower_bound( start );
                auto end = idx.end();
                
                while( result.size() < limit && itr != end )
                {
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
        
        const auto& rf_idx = _db.get_index< reward_fund_index, by_id >();
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
    )
    
} } } // taiyi::plugins::database_api

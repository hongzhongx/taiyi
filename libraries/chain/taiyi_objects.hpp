#pragma once
#include <chain/taiyi_fwd.hpp>

#include <protocol/authority.hpp>
#include <protocol/taiyi_operations.hpp>

#include <chain/taiyi_object_types.hpp>

#include <boost/multiprecision/cpp_int.hpp>


namespace taiyi { namespace chain {

    using taiyi::protocol::asset;
    using taiyi::protocol::price;
    using taiyi::protocol::asset_symbol_type;

    typedef protocol::fixed_string< 16 > reward_fund_name_type;

    /**
     * @breif a route to send withdrawn qi shares.
     */
    class withdraw_qi_route_object : public object< withdraw_qi_route_object_type, withdraw_qi_route_object >
    {
    public:
        template< typename Constructor, typename Allocator >
        withdraw_qi_route_object( Constructor&& c, allocator< Allocator > a )
        {
            c( *this );
        }
        
        withdraw_qi_route_object(){}
        
        id_type  id;
        
        account_name_type from_account;
        account_name_type to_account;
        uint16_t          percent = 0;
        bool              auto_vest = false;
    };


    class decline_adoring_rights_request_object : public object< decline_adoring_rights_request_object_type, decline_adoring_rights_request_object >
    {
    public:
        template< typename Constructor, typename Allocator >
        decline_adoring_rights_request_object( Constructor&& c, allocator< Allocator > a )
        {
            c( *this );
        }
        
        decline_adoring_rights_request_object(){}
        
        id_type           id;
        
        account_name_type account;
        time_point_sec    effective_date;
    };

    class reward_fund_object : public object< reward_fund_object_type, reward_fund_object >
    {
    public:
        template< typename Constructor, typename Allocator >
        reward_fund_object( Constructor&& c, allocator< Allocator > a )
        {
            c( *this );
        }
        
        reward_fund_object() {}
        
        reward_fund_id_type     id;
        reward_fund_name_type   name;
        asset                   reward_balance = asset( 0, YANG_SYMBOL );
        asset                   reward_qi_balance = asset( 0, QI_SYMBOL );
        
        time_point_sec          last_update;
        uint16_t                percent_content_rewards = 0;
    };

    struct by_withdraw_route;
    struct by_destination;
    typedef multi_index_container<
        withdraw_qi_route_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< withdraw_qi_route_object, withdraw_qi_route_id_type, &withdraw_qi_route_object::id > >,
            ordered_unique< tag< by_withdraw_route >,
                composite_key< withdraw_qi_route_object,
                    member< withdraw_qi_route_object, account_name_type, &withdraw_qi_route_object::from_account >,
                    member< withdraw_qi_route_object, account_name_type, &withdraw_qi_route_object::to_account >
                >,
                composite_key_compare< std::less< account_name_type >, std::less< account_name_type > >
            >,
            ordered_unique< tag< by_destination >,
                composite_key< withdraw_qi_route_object,
                    member< withdraw_qi_route_object, account_name_type, &withdraw_qi_route_object::to_account >,
                    member< withdraw_qi_route_object, withdraw_qi_route_id_type, &withdraw_qi_route_object::id >
                >
            >
        >,
        allocator< withdraw_qi_route_object >
    > withdraw_qi_route_index;

    struct by_account;
    struct by_effective_date;
    typedef multi_index_container<
        decline_adoring_rights_request_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< decline_adoring_rights_request_object, decline_adoring_rights_request_id_type, &decline_adoring_rights_request_object::id > >,
            ordered_unique< tag< by_account >,
                member< decline_adoring_rights_request_object, account_name_type, &decline_adoring_rights_request_object::account >
            >,
            ordered_unique< tag< by_effective_date >,
                composite_key< decline_adoring_rights_request_object,
                    member< decline_adoring_rights_request_object, time_point_sec, &decline_adoring_rights_request_object::effective_date >,
                    member< decline_adoring_rights_request_object, account_name_type, &decline_adoring_rights_request_object::account >
                >,
                composite_key_compare< std::less< time_point_sec >, std::less< account_name_type > >
            >
        >,
        allocator< decline_adoring_rights_request_object >
    > decline_adoring_rights_request_index;

    typedef multi_index_container<
        reward_fund_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< reward_fund_object, reward_fund_id_type, &reward_fund_object::id > >,
            ordered_unique< tag< by_name >, member< reward_fund_object, reward_fund_name_type, &reward_fund_object::name > >
        >,
        allocator< reward_fund_object >
    > reward_fund_index;

} } // taiyi::chain

namespace mira {

    template<> struct is_static_length< taiyi::chain::withdraw_qi_route_object > : public boost::true_type {};
    template<> struct is_static_length< taiyi::chain::decline_adoring_rights_request_object > : public boost::true_type {};
    template<> struct is_static_length< taiyi::chain::reward_fund_object > : public boost::true_type {};

} // mira

FC_REFLECT( taiyi::chain::withdraw_qi_route_object, (id)(from_account)(to_account)(percent)(auto_vest) )
CHAINBASE_SET_INDEX_TYPE( taiyi::chain::withdraw_qi_route_object, taiyi::chain::withdraw_qi_route_index )

FC_REFLECT( taiyi::chain::decline_adoring_rights_request_object, (id)(account)(effective_date) )
CHAINBASE_SET_INDEX_TYPE( taiyi::chain::decline_adoring_rights_request_object, taiyi::chain::decline_adoring_rights_request_index )

FC_REFLECT( taiyi::chain::reward_fund_object, (id)(name)(reward_balance)(reward_qi_balance)(last_update)(percent_content_rewards) )
CHAINBASE_SET_INDEX_TYPE( taiyi::chain::reward_fund_object, taiyi::chain::reward_fund_index )

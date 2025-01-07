#pragma once
#include <chain/taiyi_fwd.hpp>

#include <fc/fixed_string.hpp>

#include <protocol/authority.hpp>
#include <protocol/taiyi_operations.hpp>

#include <chain/taiyi_object_types.hpp>
#include <chain/siming_objects.hpp>
#include <chain/shared_authority.hpp>

#include <numeric>

namespace taiyi { namespace chain {

    using taiyi::protocol::authority;
    
    class account_object : public object< account_object_type, account_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR( account_object )
        
        template<typename Constructor, typename Allocator>
        account_object( Constructor&& c, allocator< Allocator > a )
        {
            c(*this);
        };
        
        id_type           id;

        account_name_type name;
        public_key_type   memo_key;
        account_name_type proxy;
        
        time_point_sec    last_account_update;
        
        time_point_sec    created;
        account_name_type recovery_account;
        time_point_sec    last_account_recovery;
        
        bool              can_adore = true;

        asset             balance = asset( 0, YANG_SYMBOL );  ///< total liquid shares held by this account
        
        asset             reward_yang_balance = asset( 0, YANG_SYMBOL );
        asset             reward_qi_balance = asset( 0, QI_SYMBOL );
        asset             reward_feigang_balance = asset( 0, QI_SYMBOL );

        asset             qi = asset( 0, QI_SYMBOL ); ///< total qi shares held by this account, controls its voting or adoring power
        asset             delegated_qi = asset( 0, QI_SYMBOL );
        asset             received_qi = asset( 0, QI_SYMBOL );
        
        asset             qi_withdraw_rate = asset( 0, QI_SYMBOL ); ///< at the time this is updated it can be at most qi/104
        time_point_sec    next_qi_withdrawal_time = fc::time_point_sec::maximum(); ///< after every withdrawal this is incremented by 1 week
        share_type        withdrawn = 0; /// Track how many shares have been withdrawn
        share_type        to_withdraw = 0; /// Might be able to look this up with operation history.
        uint16_t          withdraw_routes = 0;
        
        fc::array<share_type, TAIYI_MAX_PROXY_RECURSION_DEPTH> proxied_vsf_adores;// = std::vector<share_type>( TAIYI_MAX_PROXY_RECURSION_DEPTH, 0 ); ///< the total VFS adores proxied to this account
        
        uint16_t          simings_adored_for = 0;
                
        /// This function should be used only when the account adores for a siming directly
        share_type        siming_adore_weight()const { return std::accumulate( proxied_vsf_adores.begin(), proxied_vsf_adores.end(), qi.amount ); }
        share_type        proxied_vsf_adores_total()const { return std::accumulate( proxied_vsf_adores.begin(), proxied_vsf_adores.end(), share_type() ); }
        
        int64_t get_effective_qi() const;
    };
    
    struct by_proxy;
    struct by_next_qi_withdrawal_time;

    /**
     * @ingroup object_index
     */
    typedef multi_index_container<
        account_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< account_object, account_id_type, &account_object::id > >,
            ordered_unique< tag< by_name >, member< account_object, account_name_type, &account_object::name > >,
            ordered_unique< tag< by_proxy >,
                composite_key< account_object,
                    member< account_object, account_name_type, &account_object::proxy >,
                    member< account_object, account_name_type, &account_object::name >
                > /// composite key by proxy
            >,
            ordered_unique< tag< by_next_qi_withdrawal_time >,
                composite_key< account_object,
                    member< account_object, time_point_sec, &account_object::next_qi_withdrawal_time >,
                    member< account_object, account_name_type, &account_object::name >
                > /// composite key by_next_qi_withdrawal_time
            >
        >,
        allocator< account_object >
    > account_index;
        
    //=========================================================================

    class account_metadata_object : public object< account_metadata_object_type, account_metadata_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR( account_metadata_object )
        
        template< typename Constructor, typename Allocator >
        account_metadata_object( Constructor&& c, allocator< Allocator > a )
        : json_metadata( a )
        {
            c( *this );
        }
        
        id_type           id;
        account_id_type   account;
        std::string       json_metadata;
    };

    struct by_account;
    typedef multi_index_container <
        account_metadata_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< account_metadata_object, account_metadata_id_type, &account_metadata_object::id > >,
            ordered_unique< tag< by_account >, member< account_metadata_object, account_id_type, &account_metadata_object::account > >
        >,
        allocator< account_metadata_object >
    > account_metadata_index;

    //=========================================================================

    class account_authority_object : public object< account_authority_object_type, account_authority_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR( account_authority_object )
        
    public:
        template< typename Constructor, typename Allocator >
        account_authority_object( Constructor&& c, allocator< Allocator > a )
        : owner( a ), active( a ), posting( a )
        {
            c( *this );
        }
        
        id_type           id;
        
        account_name_type account;
        
        shared_authority  owner;   ///< used for backup control, can set owner or active
        shared_authority  active;  ///< used for all monetary operations, can set active or posting
        shared_authority  posting; ///< used for voting adoring and posting
        
        time_point_sec    last_owner_update;
    };

    struct by_last_owner_update;
    typedef multi_index_container <
        account_authority_object,
        indexed_by <
            ordered_unique< tag< by_id >, member< account_authority_object, account_authority_id_type, &account_authority_object::id > >,
            ordered_unique< tag< by_account >,
                composite_key< account_authority_object,
                    member< account_authority_object, account_name_type, &account_authority_object::account >,
                    member< account_authority_object, account_authority_id_type, &account_authority_object::id >
                >,
                composite_key_compare< std::less< account_name_type >, std::less< account_authority_id_type > >
            >,
            ordered_unique< tag< by_last_owner_update >,
                composite_key< account_authority_object,
                    member< account_authority_object, time_point_sec, &account_authority_object::last_owner_update >,
                    member< account_authority_object, account_authority_id_type, &account_authority_object::id >
                >,
                composite_key_compare< std::greater< time_point_sec >, std::less< account_authority_id_type > >
            >
        >,
        allocator< account_authority_object >
    > account_authority_index;

    //=========================================================================

    class qi_delegation_object : public object< qi_delegation_object_type, qi_delegation_object >
    {
    public:
        template< typename Constructor, typename Allocator >
        qi_delegation_object( Constructor&& c, allocator< Allocator > a )
        {
            c( *this );
        }
        
        qi_delegation_object() {}
        
        id_type           id;
        account_name_type delegator;
        account_name_type delegatee;
        asset             qi;
        time_point_sec    min_delegation_time;
    };

    struct by_delegation;

    typedef multi_index_container <
       qi_delegation_object,
       indexed_by <
          ordered_unique< tag< by_id >,
             member< qi_delegation_object, qi_delegation_id_type, &qi_delegation_object::id > >,
          ordered_unique< tag< by_delegation >,
             composite_key< qi_delegation_object,
                member< qi_delegation_object, account_name_type, &qi_delegation_object::delegator >,
                member< qi_delegation_object, account_name_type, &qi_delegation_object::delegatee >
             >,
             composite_key_compare< std::less< account_name_type >, std::less< account_name_type > >
          >
       >,
       allocator< qi_delegation_object >
    > qi_delegation_index;

    //=========================================================================

    class qi_delegation_expiration_object : public object< qi_delegation_expiration_object_type, qi_delegation_expiration_object >
    {
    public:
        template< typename Constructor, typename Allocator >
        qi_delegation_expiration_object( Constructor&& c, allocator< Allocator > a )
        {
            c( *this );
        }
        
        qi_delegation_expiration_object() {}
        
        id_type           id;
        account_name_type delegator;
        asset             qi;
        time_point_sec    expiration;
    };
    
    struct by_expiration;
    struct by_account_expiration;
    typedef multi_index_container <
        qi_delegation_expiration_object,
        indexed_by <
            ordered_unique< tag< by_id >, member< qi_delegation_expiration_object, qi_delegation_expiration_id_type, &qi_delegation_expiration_object::id > >,
            ordered_unique< tag< by_expiration >,
                composite_key< qi_delegation_expiration_object,
                    member< qi_delegation_expiration_object, time_point_sec, &qi_delegation_expiration_object::expiration >,
                    member< qi_delegation_expiration_object, qi_delegation_expiration_id_type, &qi_delegation_expiration_object::id >
                >,
                composite_key_compare< std::less< time_point_sec >, std::less< qi_delegation_expiration_id_type > >
            >,
            ordered_unique< tag< by_account_expiration >,
                composite_key< qi_delegation_expiration_object,
                    member< qi_delegation_expiration_object, account_name_type, &qi_delegation_expiration_object::delegator >,
                    member< qi_delegation_expiration_object, time_point_sec, &qi_delegation_expiration_object::expiration >,
                    member< qi_delegation_expiration_object, qi_delegation_expiration_id_type, &qi_delegation_expiration_object::id >
                >,
                composite_key_compare< std::less< account_name_type >, std::less< time_point_sec >, std::less< qi_delegation_expiration_id_type > >
            >
        >,
        allocator< qi_delegation_expiration_object >
    > qi_delegation_expiration_index;

    //=========================================================================

    class owner_authority_history_object : public object< owner_authority_history_object_type, owner_authority_history_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR( owner_authority_history_object )
        
    public:
        template< typename Constructor, typename Allocator >
        owner_authority_history_object( Constructor&& c, allocator< Allocator > a )
        :previous_owner_authority( allocator< shared_authority >( a ) )
        {
            c( *this );
        }
        
        id_type           id;
        
        account_name_type account;
        shared_authority  previous_owner_authority;
        time_point_sec    last_valid_time;
    };

    typedef multi_index_container <
        owner_authority_history_object,
        indexed_by <
            ordered_unique< tag< by_id >, member< owner_authority_history_object, owner_authority_history_id_type, &owner_authority_history_object::id > >,
            ordered_unique< tag< by_account >,
                composite_key< owner_authority_history_object,
                    member< owner_authority_history_object, account_name_type, &owner_authority_history_object::account >,
                    member< owner_authority_history_object, time_point_sec, &owner_authority_history_object::last_valid_time >,
                    member< owner_authority_history_object, owner_authority_history_id_type, &owner_authority_history_object::id >
                >,
                composite_key_compare< std::less< account_name_type >, std::less< time_point_sec >, std::less< owner_authority_history_id_type > >
            >
        >,
        allocator< owner_authority_history_object >
    > owner_authority_history_index;

    //=========================================================================

    class account_recovery_request_object : public object< account_recovery_request_object_type, account_recovery_request_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR( account_recovery_request_object )
        
    public:
        template< typename Constructor, typename Allocator >
        account_recovery_request_object( Constructor&& c, allocator< Allocator > a )
        :new_owner_authority( allocator< shared_authority >( a ) )
        {
            c( *this );
        }
        
        id_type           id;
        
        account_name_type account_to_recover;
        shared_authority  new_owner_authority;
        time_point_sec    expires;
    };

    struct by_expiration;
    typedef multi_index_container <
        account_recovery_request_object,
        indexed_by <
            ordered_unique< tag< by_id >, member< account_recovery_request_object, account_recovery_request_id_type, &account_recovery_request_object::id > >,
            ordered_unique< tag< by_account >,
                member< account_recovery_request_object, account_name_type, &account_recovery_request_object::account_to_recover >
            >,
            ordered_unique< tag< by_expiration >,
                composite_key< account_recovery_request_object,
                    member< account_recovery_request_object, time_point_sec, &account_recovery_request_object::expires >,
                    member< account_recovery_request_object, account_name_type, &account_recovery_request_object::account_to_recover >
                >,
                composite_key_compare< std::less< time_point_sec >, std::less< account_name_type > >
            >
        >,
        allocator< account_recovery_request_object >
    > account_recovery_request_index;

    //=========================================================================

    class change_recovery_account_request_object : public object< change_recovery_account_request_object_type, change_recovery_account_request_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR( change_recovery_account_request_object )
        
    public:
        template< typename Constructor, typename Allocator >
        change_recovery_account_request_object( Constructor&& c, allocator< Allocator > a )
        {
            c( *this );
        }
        
        id_type           id;
        
        account_name_type account_to_recover;
        account_name_type recovery_account;
        time_point_sec    effective_on;
    };

    struct by_effective_date;
    typedef multi_index_container <
        change_recovery_account_request_object,
        indexed_by <
            ordered_unique< tag< by_id >, member< change_recovery_account_request_object, change_recovery_account_request_id_type, &change_recovery_account_request_object::id > >,
            ordered_unique< tag< by_account >, member< change_recovery_account_request_object, account_name_type, &change_recovery_account_request_object::account_to_recover > >,
            ordered_unique< tag< by_effective_date >,
                composite_key< change_recovery_account_request_object,
                    member< change_recovery_account_request_object, time_point_sec, &change_recovery_account_request_object::effective_on >,
                    member< change_recovery_account_request_object, account_name_type, &change_recovery_account_request_object::account_to_recover >
                >,
                composite_key_compare< std::less< time_point_sec >, std::less< account_name_type > >
            >
        >,
        allocator< change_recovery_account_request_object >
    > change_recovery_account_request_index;
    
} } //taiyi::chain

namespace mira {

    template<> struct is_static_length< taiyi::chain::account_object > : public boost::true_type {};
    template<> struct is_static_length< taiyi::chain::qi_delegation_object > : public boost::true_type {};
    template<> struct is_static_length< taiyi::chain::qi_delegation_expiration_object > : public boost::true_type {};
    template<> struct is_static_length< taiyi::chain::change_recovery_account_request_object > : public boost::true_type {};

} // mira

FC_REFLECT( taiyi::chain::account_object, (id)(name)(memo_key)(proxy)(last_account_update)(created)(recovery_account)(last_account_recovery)(can_adore)(balance)(reward_yang_balance)(reward_qi_balance)(reward_feigang_balance)(qi)(delegated_qi)(received_qi)(qi_withdraw_rate)(next_qi_withdrawal_time)(withdrawn)(to_withdraw)(withdraw_routes)(proxied_vsf_adores)(simings_adored_for) )
CHAINBASE_SET_INDEX_TYPE( taiyi::chain::account_object, taiyi::chain::account_index )

FC_REFLECT( taiyi::chain::account_metadata_object, (id)(account)(json_metadata) )
CHAINBASE_SET_INDEX_TYPE( taiyi::chain::account_metadata_object, taiyi::chain::account_metadata_index )

FC_REFLECT( taiyi::chain::account_authority_object, (id)(account)(owner)(active)(posting)(last_owner_update) )
CHAINBASE_SET_INDEX_TYPE( taiyi::chain::account_authority_object, taiyi::chain::account_authority_index )

FC_REFLECT( taiyi::chain::qi_delegation_object, (id)(delegator)(delegatee)(qi)(min_delegation_time) )
CHAINBASE_SET_INDEX_TYPE( taiyi::chain::qi_delegation_object, taiyi::chain::qi_delegation_index )

FC_REFLECT( taiyi::chain::qi_delegation_expiration_object, (id)(delegator)(qi)(expiration) )
CHAINBASE_SET_INDEX_TYPE( taiyi::chain::qi_delegation_expiration_object, taiyi::chain::qi_delegation_expiration_index )

FC_REFLECT( taiyi::chain::owner_authority_history_object, (id)(account)(previous_owner_authority)(last_valid_time) )
CHAINBASE_SET_INDEX_TYPE( taiyi::chain::owner_authority_history_object, taiyi::chain::owner_authority_history_index )

FC_REFLECT( taiyi::chain::account_recovery_request_object, (id)(account_to_recover)(new_owner_authority)(expires) )
CHAINBASE_SET_INDEX_TYPE( taiyi::chain::account_recovery_request_object, taiyi::chain::account_recovery_request_index )

FC_REFLECT( taiyi::chain::change_recovery_account_request_object, (id)(account_to_recover)(recovery_account)(effective_on) )
CHAINBASE_SET_INDEX_TYPE( taiyi::chain::change_recovery_account_request_object,  taiyi::chain::change_recovery_account_request_index )

#pragma once
#include <plugins/database_api/database_api_objects.hpp>

#include <protocol/types.hpp>
#include <protocol/transaction.hpp>
#include <protocol/block_header.hpp>

#include <plugins/json_rpc/utility.hpp>

namespace taiyi { namespace plugins { namespace database_api {

    using protocol::account_name_type;
    using protocol::signed_transaction;
    using protocol::transaction_id_type;
    using protocol::public_key_type;
    using plugins::json_rpc::void_type;

    enum sort_order_type
    {
        by_name,
        by_proxy,
        by_next_qi_withdrawal_time,
        by_account,
        by_expiration,
        by_effective_date,
        by_adore_name,
        by_schedule_time,
        by_account_siming,
        by_siming_account,
        by_from_id,
        by_ratification_deadline,
        by_withdraw_route,
        by_destination,
        by_complete_from_id,
        by_to_complete,
        by_delegation,
        by_account_expiration,
        by_conversion_date,
        by_last_update,
        by_price,
        by_symbol_contributor,
        by_symbol,
        by_control_account,
        by_symbol_time,
        by_creator,
        by_start_date,
        by_end_date,
        by_total_adores,
        by_contributor,
        by_symbol_id,
        by_uuid
    };
    
    enum order_direction_type
    {
        ascending, ///< sort with ascending order
        descending ///< sort with descending order
    };
    
    struct list_object_args_type
    {
        fc::variant       start;
        uint32_t          limit;
        sort_order_type   order;
    };
    
    /* get_config */
    
    typedef void_type          get_config_args;
    typedef fc::variant_object get_config_return;
    
    /* get_version */
    typedef void_type          get_version_args;
    struct get_version_return
    {
        get_version_return() {}
        get_version_return( std::string bc_v, std::string s_v, std::string fc_v, chain_id_type c_id ) :blockchain_version( bc_v ), taiyi_revision( s_v ), fc_revision( fc_v ), chain_id( c_id )
        {}
        
        std::string     blockchain_version;
        std::string     taiyi_revision;
        std::string     fc_revision;
        chain_id_type  chain_id;
    };
    

    /* Singletons */
    
    /* get_dynamic_global_properties */
    
    typedef void_type                            get_dynamic_global_properties_args;
    typedef api_dynamic_global_property_object   get_dynamic_global_properties_return;
    
    /* get_siming_schedule */
    
    typedef void_type                   get_siming_schedule_args;
    typedef api_siming_schedule_object get_siming_schedule_return;

    /* get_hardfork_properties */
    
    typedef void_type                      get_hardfork_properties_args;
    typedef api_hardfork_property_object   get_hardfork_properties_return;
    
    /* get_reward_funds */
    
    typedef void_type get_reward_funds_args;
    struct get_reward_funds_return
    {
        vector< api_reward_fund_object > funds;
    };

    /* Simings */
    
    typedef list_object_args_type list_simings_args;
    struct list_simings_return
    {
        vector< api_siming_object > simings;
    };
    
    struct find_simings_args
    {
        vector< account_name_type > owners;
    };
    typedef list_simings_return find_simings_return;

    typedef list_object_args_type list_siming_adores_args;
    struct list_siming_adores_return
    {
        vector< api_siming_adore_object > adores;
    };
    
    typedef void_type get_active_simings_args;
    struct get_active_simings_return
    {
        vector< account_name_type > simings;
    };

    /* Account */
    
    typedef list_object_args_type list_accounts_args;
    struct list_accounts_return
    {
        vector< api_account_object > accounts;
    };
    
    struct find_accounts_args
    {
        vector< account_name_type > accounts;
    };
    typedef list_accounts_return find_accounts_return;
    

    /* Owner Auth History */
    
    struct list_owner_histories_args
    {
        fc::variant       start;
        uint32_t          limit;
    };
    struct list_owner_histories_return
    {
        vector< api_owner_authority_history_object > owner_auths;
    };
    
    struct find_owner_histories_args
    {
        account_name_type owner;
    };
    typedef list_owner_histories_return find_owner_histories_return;
    
    /* Account Recovery Requests */
    
    typedef list_object_args_type list_account_recovery_requests_args;
    struct list_account_recovery_requests_return
    {
        vector< api_account_recovery_request_object > requests;
    };
    
    struct find_account_recovery_requests_args
    {
        vector< account_name_type > accounts;
    };
    typedef list_account_recovery_requests_return find_account_recovery_requests_return;
    
    /* Change Recovery Account Requests */

    typedef list_object_args_type list_change_recovery_account_requests_args;
    struct list_change_recovery_account_requests_return
    {
        vector< api_change_recovery_account_request_object > requests;
    };
    
    struct find_change_recovery_account_requests_args
    {
        vector< account_name_type > accounts;
    };
    typedef list_change_recovery_account_requests_return find_change_recovery_account_requests_return;

    /* Qi Withdraw Routes */
    
    typedef list_object_args_type list_withdraw_qi_routes_args;
    struct list_withdraw_qi_routes_return
    {
        vector< api_withdraw_qi_route_object > routes;
    };
    
    struct find_withdraw_qi_routes_args
    {
        account_name_type account;
        sort_order_type   order;
    };
    typedef list_withdraw_qi_routes_return find_withdraw_qi_routes_return;

    /* Qi Delegations */

    typedef list_object_args_type list_qi_delegations_args;
    struct list_qi_delegations_return
    {
        vector< api_qi_delegation_object > delegations;
    };
    
    struct find_qi_delegations_args
    {
        account_name_type account;
    };
    typedef list_qi_delegations_return find_qi_delegations_return;
    
    /* Qi Delegation Expirations */
    
    typedef list_object_args_type list_qi_delegation_expirations_args;
    struct list_qi_delegation_expirations_return
    {
        vector< api_qi_delegation_expiration_object > delegations;
    };
    
    struct find_qi_delegation_expirations_args
    {
        account_name_type account;
    };
    typedef list_qi_delegation_expirations_return find_qi_delegation_expirations_return;
    
    /* Decline Qi Rights Requests */
    
    typedef list_object_args_type list_decline_adoring_rights_requests_args;
    struct list_decline_adoring_rights_requests_return
    {
        vector< api_decline_adoring_rights_request_object > requests;
    };
    
    struct find_decline_adoring_rights_requests_args
    {
        vector< account_name_type > accounts;
    };
    typedef list_decline_adoring_rights_requests_return find_decline_adoring_rights_requests_return;
    
    struct get_transaction_hex_args
    {
        signed_transaction trx;
    };
    struct get_transaction_hex_return
    {
        std::string hex;
    };

    struct get_required_signatures_args
    {
        signed_transaction          trx;
        flat_set< public_key_type > available_keys;
    };
    struct get_required_signatures_return
    {
        set< public_key_type > keys;
    };

    struct get_potential_signatures_args
    {
        signed_transaction trx;
    };
    typedef get_required_signatures_return get_potential_signatures_return;
    
    struct verify_authority_args
    {
        signed_transaction trx;
    };
    struct verify_authority_return
    {
        bool valid;
    };
    
    struct verify_account_authority_args
    {
        account_name_type           account;
        flat_set< public_key_type > signers;
    };
    typedef verify_authority_return verify_account_authority_return;
    
    struct verify_signatures_args
    {
        fc::sha256                    hash;
        vector< signature_type >      signatures;
        vector< account_name_type >   required_owner;
        vector< account_name_type >   required_active;
        vector< account_name_type >   required_posting;
        vector< authority >           required_other;
        
        void get_required_owner_authorities( flat_set< account_name_type >& a )const
        {
            a.insert( required_owner.begin(), required_owner.end() );
        }
        
        void get_required_active_authorities( flat_set< account_name_type >& a )const
        {
            a.insert( required_active.begin(), required_active.end() );
        }
        
        void get_required_posting_authorities( flat_set< account_name_type >& a )const
        {
            a.insert( required_posting.begin(), required_posting.end() );
        }
        
        void get_required_authorities( vector< authority >& a )const
        {
            a.insert( a.end(), required_other.begin(), required_other.end() );
        }
    };
    struct verify_signatures_return
    {
        bool valid;
    };

} } } // taiyi::database_api

FC_REFLECT( taiyi::plugins::database_api::get_version_return, (blockchain_version)(taiyi_revision)(fc_revision)(chain_id) )

FC_REFLECT_ENUM( taiyi::plugins::database_api::sort_order_type, (by_name)(by_proxy)(by_next_qi_withdrawal_time)(by_account)(by_expiration)(by_effective_date)(by_adore_name)(by_schedule_time)(by_account_siming)(by_siming_account)(by_from_id)(by_ratification_deadline)(by_withdraw_route)(by_destination)(by_complete_from_id)(by_to_complete)(by_delegation)(by_account_expiration)(by_conversion_date)(by_price)(by_symbol_contributor)(by_symbol)(by_control_account)(by_symbol_time)(by_creator)(by_start_date)(by_end_date)(by_total_adores)(by_contributor)(by_symbol_id)(by_uuid) )

FC_REFLECT_ENUM( taiyi::plugins::database_api::order_direction_type, (ascending)(descending) )

FC_REFLECT( taiyi::plugins::database_api::list_object_args_type, (start)(limit)(order) )

FC_REFLECT( taiyi::plugins::database_api::get_reward_funds_return, (funds) )

FC_REFLECT( taiyi::plugins::database_api::list_simings_return, (simings) )

FC_REFLECT( taiyi::plugins::database_api::find_simings_args, (owners) )

FC_REFLECT( taiyi::plugins::database_api::list_siming_adores_return, (adores) )

FC_REFLECT( taiyi::plugins::database_api::get_active_simings_return, (simings) )

FC_REFLECT( taiyi::plugins::database_api::list_accounts_return, (accounts) )

FC_REFLECT( taiyi::plugins::database_api::find_accounts_args, (accounts) )

FC_REFLECT( taiyi::plugins::database_api::list_owner_histories_args, (start)(limit) )
FC_REFLECT( taiyi::plugins::database_api::list_owner_histories_return, (owner_auths) )

FC_REFLECT( taiyi::plugins::database_api::find_owner_histories_args, (owner) )

FC_REFLECT( taiyi::plugins::database_api::list_account_recovery_requests_return, (requests) )

FC_REFLECT( taiyi::plugins::database_api::find_account_recovery_requests_args, (accounts) )

FC_REFLECT( taiyi::plugins::database_api::list_change_recovery_account_requests_return, (requests) )

FC_REFLECT( taiyi::plugins::database_api::find_change_recovery_account_requests_args, (accounts) )

FC_REFLECT( taiyi::plugins::database_api::list_withdraw_qi_routes_return, (routes) )

FC_REFLECT( taiyi::plugins::database_api::find_withdraw_qi_routes_args, (account)(order) )

FC_REFLECT( taiyi::plugins::database_api::find_qi_delegations_args, (account) )
FC_REFLECT( taiyi::plugins::database_api::list_qi_delegations_return, (delegations) )

FC_REFLECT( taiyi::plugins::database_api::list_qi_delegation_expirations_return, (delegations) )

FC_REFLECT( taiyi::plugins::database_api::find_qi_delegation_expirations_args, (account) )

FC_REFLECT( taiyi::plugins::database_api::find_decline_adoring_rights_requests_args, (accounts) )
FC_REFLECT( taiyi::plugins::database_api::list_decline_adoring_rights_requests_return, (requests) )

FC_REFLECT( taiyi::plugins::database_api::get_transaction_hex_args, (trx) )
FC_REFLECT( taiyi::plugins::database_api::get_transaction_hex_return, (hex) )

FC_REFLECT( taiyi::plugins::database_api::get_required_signatures_args, (trx)(available_keys) )
FC_REFLECT( taiyi::plugins::database_api::get_required_signatures_return, (keys) )

FC_REFLECT( taiyi::plugins::database_api::get_potential_signatures_args,(trx) )

FC_REFLECT( taiyi::plugins::database_api::verify_authority_args, (trx) )
FC_REFLECT( taiyi::plugins::database_api::verify_authority_return, (valid) )

FC_REFLECT( taiyi::plugins::database_api::verify_account_authority_args, (account)(signers) )

FC_REFLECT( taiyi::plugins::database_api::verify_signatures_args, (hash)(signatures)(required_owner)(required_active)(required_posting)(required_other) )
FC_REFLECT( taiyi::plugins::database_api::verify_signatures_return, (valid) )

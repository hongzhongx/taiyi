#pragma once

#include <protocol/types.hpp>

#include <protocol/operation_util.hpp>
#include <protocol/taiyi_operations.hpp>
#include <protocol/taiyi_virtual_operations.hpp>

namespace taiyi { namespace protocol {

    /** NOTE: do not change the order of any operations prior to the virtual operations
     * or it will trigger a hardfork.
     */
    typedef fc::static_variant<
    
        account_create_operation,
        account_update_operation,

        transfer_operation,
        transfer_to_qi_operation,
        withdraw_qi_operation,
        set_withdraw_qi_route_operation,
        delegate_qi_operation,

        siming_update_operation,
        siming_set_properties_operation,
        account_siming_adore_operation,
        account_siming_proxy_operation,
        decline_adoring_rights_operation,

        custom_operation,
        custom_json_operation,

        request_account_recovery_operation,
        recover_account_operation,
        change_recovery_account_operation,
            
        claim_reward_balance_operation,
    
        // contract
        create_contract_operation,
        revise_contract_operation,
        call_contract_function_operation,

        // nfa (non fungible asset)
        create_nfa_symbol_operation,
        create_nfa_operation,
        transfer_nfa_operation,
        action_nfa_operation,
    
        // actor
        create_actor_operation,
    
        // zone
        create_zone_operation,

        //**** virtual operations below this point
        hardfork_operation,
        fill_qi_withdraw_operation,
        return_qi_delegation_operation,
        producer_reward_operation,
    
        nfa_convert_qi_to_resources_operation,
        nfa_trasfer_operation,
        nfa_deposit_withdraw_operation,
        reward_qi_operation,
    
        tiandao_year_change_operation,
        tiandao_month_change_operation,
        tiandao_time_change_operation

    > operation;

    bool is_market_operation( const operation& op );
    bool is_virtual_operation( const operation& op );

} } // taiyi::protocol

TAIYI_DECLARE_OPERATION_TYPE( taiyi::protocol::operation )
FC_REFLECT_TYPENAME( taiyi::protocol::operation )

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
    
        account_create_operation,           //0
        account_update_operation,           //1

        transfer_operation,                 //2
        transfer_to_qi_operation,           //3
        withdraw_qi_operation,              //4
        set_withdraw_qi_route_operation,    //5
        delegate_qi_operation,              //6

        siming_update_operation,            //7
        siming_set_properties_operation,    //8
        account_siming_adore_operation,     //9
        account_siming_proxy_operation,     //10
        decline_adoring_rights_operation,   //11

        custom_operation,                   //12
        custom_json_operation,              //13

        request_account_recovery_operation, //14
        recover_account_operation,          //15
        change_recovery_account_operation,  //16
            
        claim_reward_balance_operation,     //17
    
        // contract
        create_contract_operation,          //18
        revise_contract_operation,          //19
        call_contract_function_operation,   //20

        // nfa (non fungible asset)
        create_nfa_symbol_operation,        //21
        create_nfa_operation,               //22
        transfer_nfa_operation,             //23
        approve_nfa_active_operation,       //24
        action_nfa_operation,               //25
    
        // zone
        create_zone_operation,              //26

        // actor
        create_actor_talent_rule_operation, //27
        create_actor_operation,             //28

        //**** virtual operations below this point
        hardfork_operation,                 //29
        fill_qi_withdraw_operation,         //30
        return_qi_delegation_operation,     //31
        producer_reward_operation,          //32
    
        nfa_convert_qi_to_resources_operation,  //33
        nfa_trasfer_operation,              //34
        nfa_deposit_withdraw_operation,     //35
        reward_feigang_operation,           //36
        reward_cultivation_operation,       //37

        tiandao_year_change_operation,      //38
        tiandao_month_change_operation,     //39
        tiandao_time_change_operation,      //40

        actor_born_operation,               //41
        actor_talent_trigger_operation,     //42
        actor_movement_operation,           //43
        actor_grown_operation,              //44
    
        narrate_log_operation               //45

    > operation;

    bool is_market_operation( const operation& op );
    bool is_virtual_operation( const operation& op );

} } // taiyi::protocol

TAIYI_DECLARE_OPERATION_TYPE( taiyi::protocol::operation )
FC_REFLECT_TYPENAME( taiyi::protocol::operation )

#pragma once

#include <protocol/taiyi_operations.hpp>

#include <chain/evaluator.hpp>

namespace taiyi { namespace chain {

    using namespace taiyi::protocol;
    
    TAIYI_DEFINE_EVALUATOR( account_create )
    TAIYI_DEFINE_EVALUATOR( account_update )
    TAIYI_DEFINE_EVALUATOR( transfer )
    TAIYI_DEFINE_EVALUATOR( transfer_to_qi )
    TAIYI_DEFINE_EVALUATOR( siming_update )
    TAIYI_DEFINE_EVALUATOR( account_siming_adore )
    TAIYI_DEFINE_EVALUATOR( account_siming_proxy )
    TAIYI_DEFINE_EVALUATOR( withdraw_qi )
    TAIYI_DEFINE_EVALUATOR( set_withdraw_qi_route )
    TAIYI_DEFINE_EVALUATOR( custom )
    TAIYI_DEFINE_EVALUATOR( custom_json )
    TAIYI_DEFINE_EVALUATOR( request_account_recovery )
    TAIYI_DEFINE_EVALUATOR( recover_account )
    TAIYI_DEFINE_EVALUATOR( change_recovery_account )
    TAIYI_DEFINE_EVALUATOR( decline_adoring_rights )
    TAIYI_DEFINE_EVALUATOR( claim_reward_balance )
    TAIYI_DEFINE_EVALUATOR( delegate_qi )
    TAIYI_DEFINE_EVALUATOR( siming_set_properties )
        
    TAIYI_DEFINE_EVALUATOR( create_contract )
    TAIYI_DEFINE_EVALUATOR( revise_contract )
    TAIYI_DEFINE_EVALUATOR( call_contract_function )

    TAIYI_DEFINE_EVALUATOR( create_nfa_symbol )
    TAIYI_DEFINE_EVALUATOR( create_nfa )
    TAIYI_DEFINE_EVALUATOR( transfer_nfa )
    TAIYI_DEFINE_EVALUATOR( deposit_qi_to_nfa )
    TAIYI_DEFINE_EVALUATOR( withdraw_qi_from_nfa )
    TAIYI_DEFINE_EVALUATOR( action_nfa )

    TAIYI_DEFINE_EVALUATOR( create_actor )

    TAIYI_DEFINE_EVALUATOR( create_zone )
    TAIYI_DEFINE_EVALUATOR( connect_to_zone )

    inline void validate_permlink_0_1( const string& permlink );

} } // taiyi::chain

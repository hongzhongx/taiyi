#include <chain/taiyi_fwd.hpp>

#include <chain/taiyi_evaluator.hpp>
#include <chain/database.hpp>
#include <chain/taiyi_objects.hpp>
#include <chain/account_object.hpp>
#include <chain/block_summary_object.hpp>
#include <chain/nfa_objects.hpp>
#include <chain/contract_objects.hpp>

#include <chain/lua_context.hpp>
#include <chain/contract_worker.hpp>

#include <fc/macros.hpp>

extern std::wstring utf8_to_wstring(const std::string& str);
extern std::string wstring_to_utf8(const std::wstring& str);

namespace taiyi { namespace chain {
    
    operation_result action_nfa_evaluator::do_apply( const action_nfa_operation& o )
    { try {
        
        const auto& caller = _db.get_account( o.caller );

        const auto* nfa = _db.find<nfa_object, by_id>(o.id);
        FC_ASSERT(nfa != nullptr, "#t&&y#序号为${i}的实体不存在#a&&i#", ("i", o.id));
        FC_ASSERT(caller.id == nfa->owner_account || caller.id == nfa->active_account, "#t&&y#没有权限操作实体#a&&i#");
        
        vector<lua_types> value_list;
        if( o.extensions.size() > 0 && o.extensions[0].size() > 0) {
            fc::variant action_args = fc::json::from_string(o.extensions[0]);
            value_list = protocol::from_variants_to_lua_types(action_args.as<fc::variants>());
        }
        else
            value_list = o.value_list;
        
        contract_worker worker;
        LuaContext context;
        
        //qi可能在执行合约中被进一步使用，所以这里记录当前的qi来计算虚拟机的执行消耗
        long long old_drops = caller.qi.amount.value / TAIYI_USEMANA_EXECUTION_SCALE;
        long long vm_drops = old_drops;
        vector<lua_types> action_result;
        _db.clear_contract_handler_exe_point(); //初始化api执行消耗统计
        
        string err = worker.do_nfa_contract_action(caller, *nfa, o.action, value_list, action_result, vm_drops, true, context, _db);
        
        FC_ASSERT(err == "", "NFA do contract action fail: ${err}", ("err", err));
        int64_t api_exe_point = _db.get_contract_handler_exe_point();
        int64_t used_drops = old_drops - vm_drops;

        //reward contract owner
        int64_t used_qi = used_drops * TAIYI_USEMANA_EXECUTION_SCALE;
        int64_t used_qi_for_treasury = used_qi * TAIYI_CONTRACT_USEMANA_REWARD_TREASURY_PERCENT / TAIYI_100_PERCENT;
        used_qi -= used_qi_for_treasury;
        FC_ASSERT( caller.qi.amount.value >= used_qi, "#t&&y#没有足够的真气操作实体#a&&i#" );
        const auto& contract = _db.get<contract_object, by_id>(nfa->main_contract);
        const auto& contract_owner = _db.get<account_object, by_id>(contract.owner);
        _db.reward_feigang(contract_owner, caller, asset(used_qi, QI_SYMBOL));
        
        //reward to treasury
        used_qi = (50 + api_exe_point) * TAIYI_USEMANA_EXECUTION_SCALE + used_qi_for_treasury;
        FC_ASSERT( caller.qi.amount.value >= used_qi, "#t&&y#没有足够的真气操作实体#a&&i#" );
        _db.reward_feigang(_db.get<account_object, by_name>(TAIYI_TREASURY_ACCOUNT), caller, asset(used_qi, QI_SYMBOL));

        return worker.get_result();
    } FC_CAPTURE_AND_RETHROW( (o) ) }

} } // taiyi::chain


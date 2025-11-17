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
    
    operation_result transfer_nfa_evaluator::do_apply( const transfer_nfa_operation& o )
    { try {
        const auto& from_account = _db.get_account(o.from);
        const auto& to_account = _db.get_account(o.to);
        
        const auto* nfa = _db.find<nfa_object, by_id>(o.id);
        FC_ASSERT(nfa != nullptr, "NFA with id ${i} not found", ("i", o.id));

        const auto* parent_nfa = _db.find<nfa_object, by_id>(nfa->parent);
        FC_ASSERT(parent_nfa == nullptr, "Can not transfer child NFA, only can transfer root NFA");
        
        FC_ASSERT(from_account.id == nfa->owner_account, "Can not transfer NFA not ownd by you");
        
        _db.modify(*nfa, [&](nfa_object &obj) {
            obj.owner_account = to_account.id;
        });
        
        //TODO: 转移子节点里面所有子节点的所有权，是否同时也要转移使用权？
        std::set<nfa_id_type> look_checker;
        _db.modify_nfa_children_owner(*nfa, to_account, look_checker);
        
        contract_result result;
        
        protocol::nfa_affected affected;
        affected.affected_account = o.from;
        affected.affected_item = nfa->id;
        affected.action = nfa_affected_type::transfer_from;
        result.contract_affecteds.push_back(std::move(affected));
        
        affected.affected_account = o.to;
        affected.affected_item = nfa->id;
        affected.action = nfa_affected_type::transfer_to;
        result.contract_affecteds.push_back(std::move(affected));
        
        //reward to treasury
        int64_t used_qi = 10 * TAIYI_USEMANA_EXECUTION_SCALE;
        FC_ASSERT( from_account.qi.amount.value >= used_qi, "From account does not have enough qi to operation." );
        _db.reward_feigang(_db.get<account_object, by_name>(TAIYI_TREASURY_ACCOUNT), from_account, asset(used_qi, QI_SYMBOL));
        
        return result;
    } FC_CAPTURE_AND_RETHROW( (o) ) }
    //=============================================================================
    operation_result approve_nfa_active_evaluator::do_apply( const approve_nfa_active_operation& o )
    { try {
        const auto& owner_account = _db.get_account(o.owner);
        const auto& to_account = _db.get_account(o.active_account);
        
        const auto* nfa = _db.find<nfa_object, by_id>(o.id);
        FC_ASSERT(nfa != nullptr, "NFA with id ${i} not found", ("i", o.id));
        
        FC_ASSERT(owner_account.id == nfa->owner_account, "Can not change active operator of NFA not ownd by you");
        
        _db.modify(*nfa, [&](nfa_object &obj) {
            obj.active_account = to_account.id;
        });
                
        contract_result result;
        
        protocol::nfa_affected affected;
        affected.affected_account = o.owner;
        affected.affected_item = nfa->id;
        affected.action = nfa_affected_type::modified;
        result.contract_affecteds.push_back(std::move(affected));
        
        if(o.owner != o.active_account) {
            affected.affected_account = o.active_account;
            affected.affected_item = nfa->id;
            affected.action = nfa_affected_type::modified;
            result.contract_affecteds.push_back(std::move(affected));
        }

        //reward to treasury
        int64_t used_qi = 1 * TAIYI_USEMANA_EXECUTION_SCALE;
        FC_ASSERT( owner_account.qi.amount.value >= used_qi, "Owner account does not have enough qi to operation." );
        _db.reward_feigang(_db.get<account_object, by_name>(TAIYI_TREASURY_ACCOUNT), owner_account, asset(used_qi, QI_SYMBOL));

        return result;
    } FC_CAPTURE_AND_RETHROW( (o) ) }
    //=============================================================================
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
        _db.initialize_VM_baseENV(context);
        
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


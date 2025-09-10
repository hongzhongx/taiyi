#include <chain/taiyi_fwd.hpp>

#include <chain/taiyi_evaluator.hpp>
#include <chain/database.hpp>

#include <chain/taiyi_objects.hpp>
#include <chain/block_summary_object.hpp>
#include <chain/account_object.hpp>
#include <chain/contract_objects.hpp>

#include <chain/lua_context.hpp>
#include <chain/contract_worker.hpp>

#include <lua.hpp>

#include <fc/macros.hpp>

FC_TODO( "After we vendor fc, also vendor diff_match_patch and fix these warnings" )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wsign-compare"
#   pragma GCC diagnostic push
#   if !defined( __clang__ )
#       pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#   endif
#   include <diff_match_patch.h>
#   pragma GCC diagnostic pop
#   pragma GCC diagnostic pop
#   include <boost/locale/encoding_utf.hpp>

#include <limits>

extern std::wstring utf8_to_wstring(const std::string& str);
extern std::string wstring_to_utf8(const std::wstring& str);

namespace taiyi { namespace chain {

    operation_result create_contract_evaluator::do_apply( const create_contract_operation& o )
    { try {
        const auto& creator = _db.get_account(o.owner);
                
        if(o.name == "contract.blacklist")
            FC_ASSERT(o.owner == TAIYI_COMMITTEE_ACCOUNT);
        
        //qi可能在执行合约中被进一步使用，所以这里记录当前的qi来计算虚拟机的执行消耗
        long long old_drops = creator.qi.amount.value / TAIYI_USEMANA_EXECUTION_SCALE;
        long long vm_drops = old_drops;
        _db.clear_contract_handler_exe_point(); //初始化api执行消耗统计
        size_t new_state_size = _db.create_contract_objects( creator, o.name, o.data, o.contract_authority, vm_drops );
        int64_t api_exe_point = _db.get_contract_handler_exe_point();
        int64_t used_drops = old_drops - vm_drops;

        //reward to treasury
        int64_t used_qi = used_drops * TAIYI_USEMANA_EXECUTION_SCALE + new_state_size * TAIYI_USEMANA_STATE_BYTES_SCALE + (100 + api_exe_point) * TAIYI_USEMANA_EXECUTION_SCALE;
        FC_ASSERT( creator.qi.amount.value >= used_qi, "Creator account does not have enough qi to create contract. need ${n}", ("n", used_qi) );
        _db.reward_feigang(_db.get<account_object, by_name>(TAIYI_TREASURY_ACCOUNT), creator, asset(used_qi, QI_SYMBOL));
        
        return void_result();
    } FC_CAPTURE_AND_RETHROW( (o) ) }
    //=============================================================================
    operation_result revise_contract_evaluator::do_apply( const revise_contract_operation& o )
    { try {
        const auto &contract = _db.get<contract_object, by_name>(o.contract_name);
        const auto &contract_owner = _db.get<account_object, by_id>(contract.owner);
        
        if(o.reviser != TAIYI_COMMITTEE_ACCOUNT)
            FC_ASSERT(contract.can_do(_db), "The current contract may have been listed in the forbidden call list");
        
        FC_ASSERT(!contract.is_release," The current contract is release version cannot be change ");
        FC_ASSERT(contract_owner.name == o.reviser, "You do not have the authority to modify the contract, the contract owner is ${owner}", ("owner", contract_owner.name));

        const auto& reviser = _db.get_account(o.reviser);
        
        vector<char> lua_code_b;
        contract_worker worker;

        //qi可能在执行合约中被进一步使用，所以这里记录当前的qi来计算虚拟机的执行消耗
        long long old_drops = reviser.qi.amount.value / TAIYI_USEMANA_EXECUTION_SCALE;
        long long vm_drops = old_drops;
        _db.clear_contract_handler_exe_point(); //初始化api执行消耗统计
        lua_table aco = worker.do_contract(contract.id, contract.name, o.data, lua_code_b, vm_drops, _db);
        int64_t api_exe_point = _db.get_contract_handler_exe_point();
        int64_t used_drops = old_drops - vm_drops;

        const auto& code_bin_object = _db.get<contract_bin_code_object, by_id>(contract.lua_code_b_id);
        _db.modify(code_bin_object, [&](contract_bin_code_object&cbo) {
            cbo.lua_code_b = lua_code_b;
#ifndef IS_LOW_MEM
            cbo.source_code = o.data;
#endif
        });
        
        string previous_version = contract.current_version.str();
        _db.modify(contract, [&](contract_object &c) {
            c.current_version = _db.get_current_trx();
            c.contract_ABI = aco.v;
        });
        
        //reward to treasury
        size_t new_state_size = fc::raw::pack_size(contract.contract_ABI) + fc::raw::pack_size(code_bin_object.lua_code_b);
        int64_t used_qi = used_drops * TAIYI_USEMANA_EXECUTION_SCALE + new_state_size * TAIYI_USEMANA_STATE_BYTES_SCALE + (50 + api_exe_point) * TAIYI_USEMANA_EXECUTION_SCALE;
        FC_ASSERT( reviser.qi.amount.value >= used_qi, "reviser account does not have enough qi to revise contract." );
        _db.reward_feigang(_db.get<account_object, by_name>(TAIYI_TREASURY_ACCOUNT), reviser, asset(used_qi, QI_SYMBOL));
        
        return logger_result(previous_version);
    } FC_CAPTURE_AND_RETHROW( (o) ) }
    //=============================================================================
    operation_result call_contract_function_evaluator::do_apply( const call_contract_function_operation& o )
    { try {
        
        const auto& caller = _db.get<account_object, by_name>(o.caller);
        const auto& contract = _db.get<contract_object, by_name>(o.contract_name);

        const auto* current_trx = _db.get_current_trx_ptr();
        FC_ASSERT(current_trx);
        const flat_set<public_key_type>& sigkeys = current_trx->get_signature_keys(_db.get_chain_id(), fc::ecc::fc_canonical);
        
        //evaluate contract authority
        if(o.caller != TAIYI_COMMITTEE_ACCOUNT)
            FC_ASSERT(contract.can_do(_db), "The current contract \"${n}\" may have been listed in the forbidden call list", ("n", o.contract_name));
        if (contract.check_contract_authority)
        {
            auto skip = _db.node_properties().skip_flags;
            if (!(skip & (database::validation_steps::skip_transaction_signatures | database::validation_steps::skip_authority_check)))
            {
                auto key_itr = std::find(sigkeys.begin(), sigkeys.end(), contract.contract_authority);
                FC_ASSERT(key_itr != sigkeys.end(), "No contract related permissions were found in the signature, contract_authority:${c}", ("c", contract.contract_authority));
            }
        }
                
        contract_worker worker;
        
        LuaContext context;
        _db.initialize_VM_baseENV(context);
        
        //qi可能在执行合约中被进一步使用，所以这里记录当前的qi来计算虚拟机的执行消耗
        long long old_drops = caller.qi.amount.value / TAIYI_USEMANA_EXECUTION_SCALE;
        long long vm_drops = old_drops;
        int64_t backup_api_exe_point = _db.get_contract_handler_exe_point();
        _db.clear_contract_handler_exe_point(); //初始化api执行消耗统计
        worker.do_contract_function(caller, o.function_name, o.value_list, sigkeys, contract, vm_drops, true,  context, _db);
        int64_t api_exe_point = _db.get_contract_handler_exe_point();
        _db.clear_contract_handler_exe_point(backup_api_exe_point);
        int64_t used_drops = old_drops - vm_drops;

        //reward contract owner
        int64_t used_qi = used_drops * TAIYI_USEMANA_EXECUTION_SCALE;
        int64_t used_qi_for_treasury = used_qi * TAIYI_CONTRACT_USEMANA_REWARD_TREASURY_PERCENT / TAIYI_100_PERCENT;
        used_qi -= used_qi_for_treasury;
        FC_ASSERT( caller.qi.amount.value >= used_qi, "Caller account does not have enough qi to call contract." );
        const auto& contract_owner = _db.get<account_object, by_id>(contract.owner);
        _db.reward_feigang(contract_owner, caller, asset(used_qi, QI_SYMBOL));

        //reward to treasury
        used_qi = (50 + api_exe_point) * TAIYI_USEMANA_EXECUTION_SCALE + used_qi_for_treasury;
        FC_ASSERT( caller.qi.amount.value >= used_qi, "Caller account does not have enough qi to call contract." );
        _db.reward_feigang(_db.get<account_object, by_name>(TAIYI_TREASURY_ACCOUNT), caller, asset(used_qi, QI_SYMBOL));

        return worker.get_result();
    } FC_CAPTURE_AND_RETHROW( (o) ) }
    
} } // taiyi::chain


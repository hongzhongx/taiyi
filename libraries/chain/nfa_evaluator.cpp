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

#include <chain/util/manabar.hpp>

#include <fc/macros.hpp>

extern std::wstring utf8_to_wstring(const std::string& str);
extern std::string wstring_to_utf8(const std::wstring& str);

namespace taiyi { namespace chain {
    
    namespace util {
        //对get_effective_qi_shares模版函数的特化
        template<> int64_t get_effective_qi_shares<nfa_object>( const nfa_object& obj )
        {
            return obj.qi_shares.amount.value;
        }
    }

    operation_result create_nfa_symbol_evaluator::do_apply( const create_nfa_symbol_operation& o )
    { try {
        const auto& creator = _db.get_account( o.creator );
        
        _db.modify( creator, [&]( account_object& a ) {
            util::update_manabar( _db.get_dynamic_global_properties(), a, true );
        });
        
        const auto* nfa_symbol = _db.find<nfa_symbol_object, by_symbol>(o.symbol);
        FC_ASSERT(nfa_symbol == nullptr, "NFA symbol named \"${n}\" is already exist.", ("n", o.symbol));
        
        const auto& contract = _db.get<contract_object, by_name>(o.default_contract);
        auto abi_itr = contract.contract_ABI.find(lua_types(lua_string(TAIYI_NFA_INIT_FUNC_NAME)));
        FC_ASSERT(abi_itr != contract.contract_ABI.end(), "contract ${c} has not init function named ${i}", ("c", contract.name)("i", TAIYI_NFA_INIT_FUNC_NAME));
        
        const auto& nfa_symbol_obj = _db.create<nfa_symbol_object>([&](nfa_symbol_object& obj) {
            obj.creator = creator.name;
            obj.symbol = o.symbol;
            obj.describe = o.describe;
            obj.default_contract = contract.id;
            obj.count = 0;
        });
        
        size_t new_state_size = fc::raw::pack_size(nfa_symbol_obj);
        int64_t used_mana = new_state_size * TAIYI_USEMANA_STATE_BYTES_SCALE + 1000 * TAIYI_USEMANA_EXECUTION_SCALE;
        FC_ASSERT( creator.manabar.has_mana(used_mana), "Creator account does not have enough mana to create nfa symbol." );
        _db.modify( creator, [&]( account_object& a ) {
            a.manabar.use_mana( used_mana );
        });
        
        return void_result();
    } FC_CAPTURE_AND_RETHROW( (o) ) }
    //=============================================================================
    operation_result create_nfa_evaluator::do_apply( const create_nfa_operation& o )
    { try {
        const auto& creator = _db.get_account( o.creator );
        const auto* nfa_symbol = _db.find<nfa_symbol_object, by_symbol>(o.symbol);
        FC_ASSERT(nfa_symbol != nullptr, "NFA symbol named \"${n}\" is not exist.", ("n", o.symbol));
        
        const auto* current_trx = _db.get_current_trx_ptr();
        FC_ASSERT(current_trx);
        const flat_set<public_key_type>& sigkeys = current_trx->get_signature_keys(_db.get_chain_id(), fc::ecc::fc_canonical);
        
        LuaContext context;
        _db.initialize_VM_baseENV(context);
        
        const auto& nfa = _db.create_nfa(creator, *nfa_symbol, sigkeys, true, context);
        
        contract_result result;
        
        protocol::nfa_affected affected;
        affected.affected_account = creator.name;
        affected.affected_item = nfa.id;
        affected.action = nfa_affected_type::create_for;
        result.contract_affecteds.push_back(std::move(affected));
        
        affected.affected_account = creator.name;
        affected.action = nfa_affected_type::create_by;
        result.contract_affecteds.push_back(std::move(affected));
        
        return result;
    } FC_CAPTURE_AND_RETHROW( (o) ) }
    //=============================================================================
    operation_result transfer_nfa_evaluator::do_apply( const transfer_nfa_operation& o )
    { try {
        const auto& from_account = _db.get_account(o.from);
        const auto& to_account = _db.get_account(o.to);
        
        const auto* nfa = _db.find<nfa_object, by_id>(o.id);
        FC_ASSERT(nfa != nullptr, "NFA with id ${i} not found.", ("i", o.id));
        
        FC_ASSERT(from_account.id == nfa->owner_account, "Can not transfer NFA not ownd by you.");
        
        _db.modify(*nfa, [&](nfa_object &obj) {
            obj.owner_account = to_account.id;
        });
        
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
        
        return result;
    } FC_CAPTURE_AND_RETHROW( (o) ) }
    //=============================================================================
    operation_result deposit_qi_to_nfa_evaluator::do_apply( const deposit_qi_to_nfa_operation& o )
    {
        const auto& account = _db.get_account(o.account);

        const auto* nfa = _db.find<nfa_object, by_id>(o.id);
        FC_ASSERT(nfa != nullptr, "NFA with id ${i} not found.", ("i", o.id));

        _db.adjust_balance( account, -o.amount );
        
        _db.modify(*nfa, [&](nfa_object& obj) {
            obj.qi_shares += o.amount;
            util::update_manabar( _db.get_dynamic_global_properties(), obj, true );
        });

        contract_result result;
        
        protocol::nfa_affected affected;
        affected.affected_account = o.account;
        affected.affected_item = nfa->id;
        affected.action = nfa_affected_type::deposit_qi;
        result.contract_affecteds.push_back(std::move(affected));

        return result;
    }
    //=============================================================================
    operation_result withdraw_qi_from_nfa_evaluator::do_apply( const withdraw_qi_from_nfa_operation& o )
    {
        const auto& owner = _db.get_account(o.owner);

        const auto* nfa = _db.find<nfa_object, by_id>(o.id);
        FC_ASSERT(nfa != nullptr, "NFA with id ${i} not found.", ("i", o.id));
        FC_ASSERT(owner.id == nfa->owner_account, "Can not withdraw from NFA not ownd by you.");
        
        FC_ASSERT(nfa->qi_shares >= o.amount, "NFA has not enough Qi to withrawn.");


        _db.modify(*nfa, [&](nfa_object& obj) {
            obj.qi_shares -= o.amount;
            util::update_manabar( _db.get_dynamic_global_properties(), obj, true );
        });

        _db.adjust_balance( owner, o.amount );
                
        contract_result result;
        
        protocol::nfa_affected affected;
        affected.affected_account = o.owner;
        affected.affected_item = nfa->id;
        affected.action = nfa_affected_type::withdraw_qi;
        result.contract_affecteds.push_back(std::move(affected));

        return result;
    }
    //=============================================================================
    operation_result action_nfa_evaluator::do_apply( const action_nfa_operation& o )
    { try {
        
        const auto& owner = _db.get_account( o.owner );

        const auto* nfa = _db.find<nfa_object, by_id>(o.id);
        FC_ASSERT(nfa != nullptr, "NFA with id ${i} not found.", ("i", o.id));
        FC_ASSERT(owner.id == nfa->owner_account, "Can not action NFA not ownd by you.");

        _db.modify( owner, [&]( account_object& a ) {
            util::update_manabar( _db.get_dynamic_global_properties(), a, true );
        });

        //重整规范参数，使得最终传入合约参数为[me, params]
        lua_table params;
        for(size_t i=0; i<o.value_list.size(); i++)
            params.v[lua_types(lua_int(i+1))] = o.value_list[i]; //lua中数组下标在table中用key是从1开始的
        vector<lua_types> value_list;
        value_list.emplace_back(params);
        
        contract_result cresult;
        contract_worker worker;

        LuaContext context;
        _db.initialize_VM_baseENV(context);
        
        //mana可能在执行合约中被进一步使用，所以这里记录当前的mana来计算虚拟机的执行消耗
        long long old_drops = owner.manabar.current_mana / TAIYI_USEMANA_EXECUTION_SCALE;
        long long vm_drops = old_drops;
        bool bOK = worker.do_nfa_contract_action(*nfa, o.action, value_list, cresult, vm_drops, true, context, _db);
        FC_ASSERT(bOK, "NFA do contract action fail.");
        int64_t used_drops = old_drops - vm_drops;

        int64_t used_mana = used_drops * TAIYI_USEMANA_EXECUTION_SCALE + 50 * TAIYI_USEMANA_EXECUTION_SCALE;
        FC_ASSERT( owner.manabar.has_mana(used_mana), "caller account does not have enough mana to action nfa." );
        _db.modify( owner, [&]( account_object& a ) {
            a.manabar.use_mana( used_mana );
        });
        
        //reward contract owner
        const auto& contract = _db.get<contract_object, by_id>(nfa->main_contract);
        const auto& contract_owner = _db.get<account_object, by_id>(contract.owner);
        _db.reward_contract_owner(contract_owner.name, asset(used_mana, QI_SYMBOL));

        return cresult;
    } FC_CAPTURE_AND_RETHROW( (o) ) }

} } // taiyi::chain


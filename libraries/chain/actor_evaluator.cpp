#include <chain/taiyi_fwd.hpp>

#include <chain/taiyi_evaluator.hpp>
#include <chain/database.hpp>
#include <chain/taiyi_objects.hpp>
#include <chain/block_summary_object.hpp>
#include <chain/account_object.hpp>
#include <chain/nfa_objects.hpp>
#include <chain/actor_objects.hpp>
#include <chain/contract_objects.hpp>

#include <chain/lua_context.hpp>
#include <chain/contract_worker.hpp>

#include <fc/macros.hpp>

#include <limits>

extern std::wstring utf8_to_wstring(const std::string& str);
extern std::string wstring_to_utf8(const std::wstring& str);

namespace taiyi { namespace chain {

    extern std::string s_debug_actor;
    
    //=============================================================================
    operation_result create_actor_talent_rule_evaluator::do_apply( const create_actor_talent_rule_operation& o )
    { try {
        //未来实现社区化的天赋规则创建机制
        FC_ASSERT(o.creator == TAIYI_COMMITTEE_ACCOUNT, "currently, talent rule must be created by account ${a}.", ("a", TAIYI_COMMITTEE_ACCOUNT));
        
        const auto& creator = _db.get_account( o.creator );

        const auto& contract = _db.get<contract_object, by_name>(o.contract);
        auto abi_itr = contract.contract_ABI.find(lua_types(lua_string(TAIYI_ACTOR_TALENT_RULE_INIT_FUNC_NAME)));
        FC_ASSERT(abi_itr != contract.contract_ABI.end(), "contract ${c} has not init function named ${i}", ("c", contract.name)("i", TAIYI_ACTOR_TALENT_RULE_INIT_FUNC_NAME));

        //qi可能在执行合约中被进一步使用，所以这里记录当前的qi来计算虚拟机的执行消耗
        long long old_drops = creator.qi.amount.value / TAIYI_USEMANA_EXECUTION_SCALE;
        long long vm_drops = old_drops;
        _db.clear_contract_handler_exe_point(); //初始化api执行消耗统计

        auto now = _db.head_block_time();
        const auto& new_rule = _db.create< actor_talent_rule_object >( [&]( actor_talent_rule_object& tr ) {
            tr.main_contract = contract.id;
            tr.last_update = now;
            tr.created = tr.last_update;
            
            _db.initialize_actor_talent_rule_object(creator, tr, vm_drops);
        });
        size_t new_state_size = fc::raw::pack_size(new_rule.title) + fc::raw::pack_size(new_rule.description);
        int64_t api_exe_point = _db.get_contract_handler_exe_point();
        int64_t used_drops = old_drops - vm_drops;

        //reward to treasury
        int64_t used_qi = used_drops * TAIYI_USEMANA_EXECUTION_SCALE + new_state_size * TAIYI_USEMANA_STATE_BYTES_SCALE + (100 + api_exe_point) * TAIYI_USEMANA_EXECUTION_SCALE;
        FC_ASSERT( creator.qi.amount.value >= used_qi, "Creator account does not have enough qi to create actor talent rule." );
        _db.reward_feigang(_db.get<account_object, by_name>(TAIYI_TREASURY_ACCOUNT), creator, asset(used_qi, QI_SYMBOL));

        return void_result();
    } FC_CAPTURE_AND_RETHROW( (o) ) }
    //=============================================================================
    operation_result create_actor_evaluator::do_apply( const create_actor_operation& o )
    {
        const auto& creator = _db.get_account( o.creator );
        
        //check existence
        std::string name = o.family_name + o.last_name;
        auto check_act = _db.find< actor_object, by_name >( name );
        FC_ASSERT( check_act == nullptr, "There is already exist actor named \"${a}\".", ("a", name) );
        
        const auto& props = _db.get_dynamic_global_properties();
        const siming_schedule_object& wso = _db.get_siming_schedule_object();

        FC_ASSERT( o.fee <= asset( TAIYI_MAX_ACTOR_CREATION_FEE, QI_SYMBOL ), "Actor creation fee cannot be too large" );
        FC_ASSERT( o.fee == (wso.median_props.account_creation_fee * TAIYI_QI_SHARE_PRICE), "Must pay the exact actor creation fee. paid: ${p} fee: ${f}", ("p", o.fee) ("f", wso.median_props.account_creation_fee * TAIYI_QI_SHARE_PRICE) );

        _db.adjust_balance( creator, -o.fee );
        
        contract_result result;
        
        //先创建NFA
        string nfa_symbol_name = "nfa.actor.default";
        const auto* nfa_symbol = _db.find<nfa_symbol_object, by_symbol>(nfa_symbol_name);
        FC_ASSERT(nfa_symbol != nullptr, "NFA symbol named \"${n}\" is not exist.", ("n", nfa_symbol_name));
        
        const auto* current_trx = _db.get_current_trx_ptr();
        FC_ASSERT(current_trx);
        const flat_set<public_key_type>& sigkeys = current_trx->get_signature_keys(_db.get_chain_id(), fc::ecc::fc_canonical);
        
        LuaContext context;
        _db.initialize_VM_baseENV(context);
        
        const auto& nfa = _db.create_nfa(creator, *nfa_symbol, sigkeys, true, context);
        
        protocol::nfa_affected affected;
        affected.affected_account = creator.name;
        affected.affected_item = nfa.id;
        affected.action = nfa_affected_type::create_for;
        result.contract_affecteds.push_back(std::move(affected));
        
        affected.affected_account = creator.name;
        affected.action = nfa_affected_type::create_by;
        result.contract_affecteds.push_back(std::move(affected));
        
        const auto& new_actor = _db.create< actor_object >( [&]( actor_object& act ) {
            _db.initialize_actor_object( act, name, nfa );
            
            act.family_name = o.family_name;
            act.last_name = o.last_name;
        });
        
        _db.initialize_actor_talents(new_actor);
                
        if( o.fee.amount > 0 ) {
            _db.modify(nfa, [&](nfa_object& obj) {
                obj.qi += o.fee;
            });
        }
        
        affected.affected_account = creator.name;
        affected.affected_item = nfa.id;
        affected.action = nfa_affected_type::deposit_qi;
        result.contract_affecteds.push_back(std::move(affected));
        
        //reward to treasury
        int64_t used_qi = TAIYI_ACTOR_OBJ_STATE_BYTES * TAIYI_USEMANA_STATE_BYTES_SCALE + 1000 * TAIYI_USEMANA_EXECUTION_SCALE;
        FC_ASSERT( creator.qi.amount.value >= used_qi, "Creator account does not have enough qi to create actor." );
        _db.reward_feigang(_db.get<account_object, by_name>(TAIYI_TREASURY_ACCOUNT), creator, asset(used_qi, QI_SYMBOL));

        return result;
    }

} } // taiyi::chain


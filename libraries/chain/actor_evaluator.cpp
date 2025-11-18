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
                
        LuaContext context;
        _db.initialize_VM_baseENV(context);
        
        const auto& nfa = _db.create_nfa(creator, *nfa_symbol, true, context);
        
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


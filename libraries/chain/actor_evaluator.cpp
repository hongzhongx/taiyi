#include <chain/taiyi_fwd.hpp>

#include <chain/taiyi_evaluator.hpp>
#include <chain/database.hpp>
#include <chain/taiyi_objects.hpp>
#include <chain/block_summary_object.hpp>
#include <chain/account_object.hpp>
#include <chain/nfa_objects.hpp>
#include <chain/actor_objects.hpp>

#include <chain/lua_context.hpp>
#include <chain/contract_worker.hpp>

#include <chain/util/manabar.hpp>

#include <fc/macros.hpp>

#include <limits>

extern std::wstring utf8_to_wstring(const std::string& str);
extern std::string wstring_to_utf8(const std::wstring& str);

namespace taiyi { namespace chain {

extern std::string s_debug_actor;

//=============================================================================
operation_result create_actor_evaluator::do_apply( const create_actor_operation& o )
{
    FC_ASSERT( o.fee <= asset( TAIYI_MAX_ACTOR_CREATION_FEE, YANG_SYMBOL ), "Actor creation fee cannot be too large" );
    
    const auto& creator = _db.get_account( o.creator );
    
    std::string name = o.family_name + o.last_name;
    auto check_act = _db.find< actor_object, by_name >( name );
    FC_ASSERT( check_act == nullptr, "There is already exist actor named \"${a}\".", ("a", name) );
    
    //const auto& props = _db.get_dynamic_global_properties();
    
    _db.adjust_balance( creator, -o.fee );
    _db.adjust_balance( _db.get< account_object, by_name >( TAIYI_NULL_ACCOUNT ), o.fee );
    
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
        
        act.sexuality = o.sexuality;
    });
    
    _db.initialize_actor_talents( new_actor, o.gender, o.sexuality );
    
    return void_result();
}


} } // taiyi::chain


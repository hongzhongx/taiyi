#pragma once
#include <protocol/lua_types.hpp>

#include <chain/taiyi_object_types.hpp>

struct lua_State;

namespace taiyi { namespace chain {

    class database;
    class LuaContext;
    using protocol::contract_result;

    class contract_worker
    {
    public:
        protocol::lua_table do_contract(contract_id_type id, const string& name, const string& lua_code, vector<char>& lua_code_b, long long& vm_drops, database &db);
        
        protocol::lua_table do_contract_function(const account_object& caller, string function_name, vector<lua_types> value_list, const flat_set<public_key_type> &sigkeys, const contract_object& contract, long long& vm_drops, bool reset_vm_memused, LuaContext& context, database &db);
        
        std::string eval_nfa_contract_action(const nfa_object& caller_nfa, const string& action, vector<lua_types> value_list, vector<lua_types>&result, long long& vm_drops, bool reset_vm_memused, LuaContext& context, database &db);
        std::string do_nfa_contract_action(const account_object& caller, const nfa_object& nfa, const string& action, vector<lua_types> value_list, vector<lua_types>&result, long long& vm_drops, bool reset_vm_memused, LuaContext& context, database &db);
        protocol::lua_table do_nfa_contract_function(const account_object& caller, const nfa_object& nfa, const string& function_name, vector<lua_types> value_list, const flat_set<public_key_type> &sigkeys, const contract_object& contract, long long& vm_drops, bool reset_vm_memused, LuaContext& context, database &db, bool eval);

        const contract_result& get_result() { return result; }
        
    protected:
        contract_result result;
    };

} } // taiyi::chain

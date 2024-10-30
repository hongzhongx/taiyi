#pragma once
#include <protocol/lua_types.hpp>

#include <chain/taiyi_object_types.hpp>

struct lua_State;

namespace taiyi { namespace chain {

    static const std::string Key_Special[] = {
        "table", "_G", "_VERSION", "coroutine", "debug", "math", "io",
        "utf8", "bit", "package", "string", "os", "cjson","baseENV",
        "require"
    };

    class database;
    class LuaContext;
    using protocol::contract_result;

    class contract_worker
    {
    public:
        protocol::lua_table do_contract(contract_id_type id, const string& name, const string& lua_code, vector<char>& lua_code_b, long long& vm_drops, lua_State *L, database &db);
        
        void do_contract_function(const account_object& caller, string function_name, vector<lua_types> value_list, lua_map &account_data, const flat_set<public_key_type> &sigkeys, contract_result &apply_result, const contract_object& contract, database &db);
        protocol::lua_table do_contract_function_return_table(const account_object& caller, string function_name, vector<lua_types> value_list, lua_map &account_data, const flat_set<public_key_type> &sigkeys, contract_result &apply_result, const contract_object& contract, database &db);
        
        contract_result get_result() { return result; }
        
    protected:
        contract_result result;
    };

} } // taiyi::chain

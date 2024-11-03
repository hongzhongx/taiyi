#pragma once

#include <chain/contract_objects.hpp>
#include <chain/contract_worker.hpp>

namespace taiyi { namespace chain {

    using protocol::lua_key;
    using protocol::lua_types;
    
    using protocol::lua_table;
    using protocol::lua_string;
    using protocol::lua_int;
    using protocol::lua_bool;
    
    using protocol::memo_data;

#define LUA_C_ERR_THROW(L,eMsg) \
    LuaContext::Pusher<string>::push(L, eMsg).release();\
    LuaContext::luaError(L);

    class database;

    struct contract_base_info
    {
        database& db;
        LuaContext& context;
        
        string owner;
        string name;
        string caller;
        string creation_date;
        string contract_authority;
        string invoker_contract_name;
        
        contract_base_info(database& db_, LuaContext &context_, const account_name_type& owner_, const string& name_, const account_name_type& caller_, const string& creation_date_, const string& contract_authority_, const string& invoker_contract_name_)
            : db(db_), context(context_), owner(owner_), name(name_), caller(caller_), creation_date(creation_date_), contract_authority(contract_authority_), invoker_contract_name(invoker_contract_name_)
        {}
        
        const contract_object& get_contract(string name);
    };

    struct contract_handler
    {
        database&                           db;
        const contract_object&              contract;
        const account_object&               caller;
        contract_result&                    result;
        LuaContext&                         context;
        const flat_set<public_key_type>&    sigkeys;
        contract_result&                    apply_result;
        map<lua_key, lua_types>&            account_conntract_data;
        
        contract_handler(database &db, const account_object& caller, const contract_object &contract, contract_result &result, LuaContext &context, const flat_set<public_key_type>& sigkeys, contract_result& apply_result, map<lua_key,lua_types>& account_data)
            : db(db), contract(contract), caller(caller), result(result), context(context), sigkeys(sigkeys), apply_result(apply_result), account_conntract_data(account_data)
        {
            result.contract_name = contract.name;
        }
    
        bool is_owner();
        void log(string message);
        int contract_random();
        void set_permissions_flag(bool flag);
        void set_invoke_share_percent(uint32_t percent);
        void read_cache();
        void fllush_cache();
        lua_Number nummin();
        lua_Number nummax();
        int64_t integermax();
        int64_t integermin();
        string hash256(string source);
        string hash512(string source);
        uint32_t head_block_time();
        const account_object& get_account(string name);
        void fllush_context(const lua_map& keys, lua_map &data_table, vector<lua_types>&stacks, string tablename);
        void read_context(lua_map& keys, lua_map &data_table, vector<lua_types>&stacks, string tablename);
        void transfer_by_contract(account_id_type from, account_id_type to, asset token, contract_result &result, bool enable_logger=false);
        int64_t get_account_balance(account_id_type account, asset_symbol_type symbol);
        void transfer_from(account_id_type from, account_name_type to, double amount, string symbol_or_id, bool enable_logger=false);
        void change_contract_authority(string authority);
        memo_data make_memo(string receiver_id_or_name, string key, string value, uint64_t ss, bool enable_logger=false);
        void invoke_contract_function(string contract_id_or_name, string function_name, string value_list_json);
        const contract_object& get_contract(string name);
        void make_release();

        lua_map get_contract_public_data(string name_or_id);
        
        static void filter_context(const lua_map &data_table, lua_map keys,vector<lua_types>&stacks, lua_map *result_table);
        static std::pair<bool, lua_types *> find_luaContext(lua_map* context, vector<lua_types> keys, int start=0, bool is_clean=false);
    };

} } //taiyi::chain
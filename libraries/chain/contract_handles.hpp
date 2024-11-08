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
    
    struct contract_asset_resources
    {
        int64_t               gold;
        int64_t               food;
        int64_t               wood;
        int64_t               fabric;
        int64_t               herb;

        contract_asset_resources(const nfa_object& nfa, database& db);
    };

    struct contract_nfa_base_info
    {
        int64_t id;
        bool is_nfa = true;
        string symbol;
        string owner_account;
        
        lua_map data;
        
        contract_nfa_base_info(const nfa_object& nfa, database& db);
        
        lua_table to_lua_table() const
        {
            lua_table t;
            t.v[lua_types(lua_string("id"))] = lua_int(id);
            t.v[lua_types(lua_string("is_nfa"))] = lua_bool(is_nfa);
            t.v[lua_types(lua_string("symbol"))] = lua_string(symbol);
            t.v[lua_types(lua_string("owner_account"))] = lua_string(owner_account);
            t.v[lua_types(lua_string("data"))] = lua_table(data);
            return t;
        }
    };

    //=========================================================================
    //NFA绑定合约被调用的角度来处理事务，隐含了发起这个调用相关的NFA对象
    struct contract_nfa_handler
    {
        const nfa_object&                   _caller;
        LuaContext&                         _context;
        database&                           _db;

        contract_nfa_handler(const nfa_object& caller, LuaContext &context, database &db)
            : _caller(caller), _context(context), _db(db)
        {}

        contract_nfa_base_info get_info();
        contract_asset_resources get_resources();
        

        void enable_tick();
        void disable_tick();
        void convert_qi_to_resource(int64_t amount, string resource_symbol_name);
    };

    //合约本身被账号直接调用的角度来处理事务，隐含了合约调用账号
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
        void transfer_from(account_id_type from, account_name_type to, double amount, string symbol_name, bool enable_logger=false);
        void change_contract_authority(string authority);
        memo_data make_memo(string receiver_id_or_name, string key, string value, uint64_t ss, bool enable_logger=false);
        void invoke_contract_function(string contract_id_or_name, string function_name, string value_list_json);
        const contract_object& get_contract(string name);
        void make_release();

        lua_map get_contract_public_data(string name_or_id);
        
        static void filter_context(const lua_map &data_table, lua_map keys,vector<lua_types>&stacks, lua_map *result_table);
        static std::pair<bool, lua_types *> find_luaContext(lua_map* context, vector<lua_types> keys, int start=0, bool is_clean=false);
        
        //NFA
        string get_nfa_contract(int64_t nfa_id);
        contract_nfa_base_info get_nfa_info(int64_t nfa_id);
        contract_asset_resources get_nfa_resources(int64_t id);

        void eval_nfa_action(int64_t nfa_id, const string& action, const lua_map& params);
        void do_nfa_action(int64_t nfa_id, const string& action, const lua_map& params);
    };
    
    asset_symbol_type s_get_symbol_type_from_string(const string name);

} } //taiyi::chain

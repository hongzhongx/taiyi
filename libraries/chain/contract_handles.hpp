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
    struct contract_handler;

    struct contract_base_info
    {
        database& db;
        LuaContext& context;
        
        string owner;
        string name;
        string caller;
        string creation_date;
        string invoker_contract_name;
        
        contract_base_info(database& db_, LuaContext &context_, const account_name_type& owner_, const string& name_, const account_name_type& caller_, const string& creation_date_, const string& invoker_contract_name_)
            : db(db_), context(context_), owner(owner_), name(name_), caller(caller_), creation_date(creation_date_), invoker_contract_name(invoker_contract_name_)
        {}
        
        const contract_object& get_contract(string name);
    };
    
    struct contract_asset_resources
    {
        int64_t     gold;
        int64_t     food;
        int64_t     wood;
        int64_t     fabric;
        int64_t     herb;

        contract_asset_resources(const nfa_object& nfa, database& db, bool is_material);
    };
    
    struct contract_tiandao_property
    {
        uint32_t    v_years;
        uint32_t    v_months;
        uint32_t    v_days;         //当月第几日
        uint32_t    v_timeonday;    //当天什么时候
        uint32_t    v_times;        //same as solar term number
        
        uint32_t    v_1day_blocks = TAIYI_VDAY_BLOCK_NUM; //虚拟的一日有多少息

        contract_tiandao_property(const tiandao_property_object& obj, database& db);
    };

    struct contract_nfa_base_info
    {
        int64_t     id;     //nfa id
        string      symbol;
        string      owner_account;
        string      creator_account;
        string      active_account;
        int64_t     qi;
        int64_t     parent;
        int         five_phase;
        string      main_contract;
        lua_map     data;
                
        contract_nfa_base_info(const nfa_object& nfa, database& db);
    };
    
    struct contract_zone_base_info
    {
        int64_t     nfa_id;
        string      name;
        string      type;
        int         type_id;
        string      ref_prohibited_contract_zone;
        
        contract_zone_base_info(const zone_object& z, database& db);        
    };
    
    struct contract_actor_base_info
    {
        int64_t     nfa_id;
        string      name;

        uint16_t    age;
        int16_t     health;
        int16_t     health_max;

        bool        born;
        int         born_vyears;
        int         born_vmonths;
        int         born_vdays;
        int         born_vtod;
        int         born_vtimes;            //solar term, 0-23
        int         five_phase;

        int         gender;
        int         standpoint;
        int         standpoint_type;
        
        string      location;   //current zone name
        string      base;       //从属地名称

        contract_actor_base_info(const actor_object& act, database& db);
    };

    struct contract_actor_core_attributes
    {
        int16_t             strength        = 0;
        int16_t             strength_max    = 0;
        int16_t             physique        = 0;
        int16_t             physique_max    = 0;
        int16_t             agility         = 0;
        int16_t             agility_max     = 0;
        int16_t             vitality        = 0;
        int16_t             vitality_max    = 0;
        int16_t             comprehension   = 0;
        int16_t             comprehension_max   = 0;
        int16_t             willpower       = 0;
        int16_t             willpower_max   = 0;
        int16_t             charm           = 0;
        int16_t             charm_max       = 0;
        int16_t             mood            = 0;
        int16_t             mood_max        = 0;

        contract_actor_core_attributes(const actor_object& act, database& db);
    };

    //=========================================================================
    //NFA绑定合约被调用的角度来处理事务，隐含了发起这个调用相关的NFA对象
    struct contract_nfa_handler
    {
        const account_object&               _caller_account;
        const nfa_object&                   _caller;
        LuaContext&                         _context;
        database&                           _db;
        contract_handler&                   _ch;

        contract_nfa_handler(const account_object& caller_account, const nfa_object& caller, LuaContext &context, database &db, contract_handler& ch);
        ~contract_nfa_handler();

        contract_nfa_base_info get_info();
        contract_asset_resources get_resources();
        contract_asset_resources get_materials();

        void enable_tick();
        void disable_tick();
        void convert_qi_to_resource(int64_t amount, const string& resource_symbol_name);
        void convert_resource_to_qi(int64_t amount, const string& resource_symbol_name);
        void add_child(int64_t nfa_id);
        void add_to_parent(int64_t parent_nfa_id);
        void add_child_from_contract_owner(int64_t nfa_id);
        void add_to_parent_from_contract_owner(int64_t parent_nfa_id);
        void remove_from_parent();
        lua_map read_contract_data(const lua_map& read_list);
        void write_contract_data(const lua_map& data, const lua_map& write_list);
        void destroy();
        lua_map eval_action(const string& action, const lua_map& params);
        lua_map do_action(const string& action, const lua_map& params);
        
        //Actor
        void modify_actor_attributes(const lua_map& values);
        void talk_to_actor(const string& target_actor_name, const string& something);

        //以下是不直接暴露到合约的辅助函数
        void transfer_from(nfa_id_type from, nfa_id_type to, double amount, const string& symbol_name, bool enable_logger=false);
        void transfer_by_contract(nfa_id_type from, nfa_id_type to, asset token, contract_result &result, bool enable_logger=false);
        void withdraw_to(nfa_id_type from, const account_name_type& to, double amount, const string& symbol_name, bool enable_logger=false);
        void withdraw_by_contract(nfa_id_type from, account_id_type to, asset token, contract_result &result, bool enable_logger=false);
        void deposit_from(account_id_type from, nfa_id_type to, double amount, const string& symbol_name, bool enable_logger=false);
        void deposit_by_contract(account_id_type from, nfa_id_type to, asset token, contract_result &result, bool enable_logger=false);

        void inject_material_from(nfa_id_type from, nfa_id_type to, double amount, const string& symbol_name, bool enable_logger=false);
        void inject_material_by_contract(nfa_id_type from, nfa_id_type to, asset token, contract_result &result, bool enable_logger=false);
        void separate_material_out(nfa_id_type from, nfa_id_type to, double amount, const string& symbol_name, bool enable_logger=false);
        void separate_material_by_contract(nfa_id_type from, nfa_id_type to, asset token, contract_result &result, bool enable_logger=false);
    };
    
    //Actor NFA绑定合约被调用的角度来处理事务，隐含了发起这个调用相关的Actor对象
    struct contract_actor_handler
    {
        const account_object&               _caller_account;
        const actor_object&                 _caller;
        LuaContext&                         _context;
        database&                           _db;
        contract_handler&                   _ch;

        contract_actor_handler(const account_object& caller_account, const actor_object& caller, LuaContext &context, database &db, contract_handler& ch)
            : _caller_account(caller_account), _caller(caller), _context(context), _db(db), _ch(ch)
        {}
    };

    //合约本身被账号直接调用的角度来处理事务，隐含了合约调用账号
    struct contract_handler
    {
        database&                           db;
        const contract_object&              contract;
        const account_object&               caller;
        const nfa_object*                   nfa_caller = 0; //隐含由某个NFA发起的调用
        contract_result&                    result;
        LuaContext&                         context;
        lua_map                             account_contract_data_cache;
        lua_map                             contract_data_cache;
        bool                                is_in_eval; //只读模式标记，表示在eval调用中
        
        std::vector<contract_handler*>      _sub_chs;
        
        contract_handler(database &db, const account_object& caller, const nfa_object* nfa_caller, const contract_object &contract, contract_result &result, LuaContext &context, bool eval);
        ~contract_handler();
    
        void assert_contract_data_size();
        bool is_owner();
        int64_t get_nfa_caller();
        void log(string message);
        void narrate(string message, bool time_prefix = false);
        uint32_t contract_random();
        string zuowangdao_account_name();
        string get_contract_source_code(const string& contract_name);
        lua_map get_contract_data(const string& contract_name, const lua_map& read_list);
        lua_map read_contract_data(const lua_map& read_list);
        void write_contract_data(const lua_map& data, const lua_map& write_list);
        lua_map get_account_contract_data(const string& account_name, const string& contract_name, const lua_map& read_list);
        lua_map read_account_contract_data(const lua_map& read_list);
        void write_account_contract_data(const lua_map& data, const lua_map& write_list);
        lua_Number nummin();
        lua_Number nummax();
        int64_t integermax();
        int64_t integermin();
        string hash256(string source);
        string hash512(string source);
        uint32_t head_block_time();
        uint32_t head_block_num();
        int64_t generate_hash(uint32_t offset);
        int64_t get_account_balance(const string& account_name, const string& symbol_name);
        memo_data make_memo(const string& receiver_account_name, const string& key, const string& value, uint64_t ss, bool enable_logger = false);
        void invoke_contract_function(const string& contract_name, const string& function_name, const string& value_list_json);
        void make_release();
        contract_tiandao_property get_tiandao_property();
        void create_named_contract(const string& name, const string& code);
        int64_t create_proposal(const string& contract_name, const string& function_name, const lua_map& params, const string& subject, const uint32_t end_time);
        void update_proposal_votes(const lua_map& proposal_ids, bool approve);
        void remove_proposals(const lua_map& proposal_ids);
        void grant_xinsu(const string& receiver_account);
        void revoke_xinsu(const string& account_name);
        
        static std::pair<bool, const lua_types*> search_in_table(const lua_map* table, const vector<lua_types>& path, int start = 0);
        static std::pair<bool, lua_types*> get_in_table(lua_map* table, const vector<lua_types>& path, int start = 0);
        static std::pair<bool, lua_types*> erase_in_table(lua_map* table, const vector<lua_types>& path, int start = 0);
        static bool write_in_table(lua_map* table, const vector<lua_types>& path, const lua_types& value, int start = 0);

        //NFA
        void create_nfa_symbol(const string& symbol, const string& describe, const string& default_contract, uint64_t max_count, uint64_t min_equivalent_qi);
        void change_nfa_symbol_authority(const string& symbol, const string& authority_account);
        int64_t create_nfa_to_actor(int64_t to_actor_nfa_id, string symbol, lua_map data);
        int64_t create_nfa_to_account(const string& to_account_name, string symbol, lua_map data);
        string get_nfa_contract(int64_t nfa_id);
        bool is_nfa_valid(int64_t nfa_id);
        contract_nfa_base_info get_nfa_info(int64_t nfa_id);
        int64_t get_nfa_balance(int64_t nfa_id, const string& symbol_name);
        contract_asset_resources get_nfa_resources(int64_t id);
        contract_asset_resources get_nfa_materials(int64_t id);
        vector<contract_nfa_base_info> list_nfa_inventory(int64_t nfa_id, const string& symbol_name);
        string get_nfa_location(int64_t nfa_id);

        lua_map read_nfa_contract_data(int64_t nfa_id, const lua_map& read_list);
        void write_nfa_contract_data(int64_t nfa_id, const lua_map& data, const lua_map& write_list);
        bool is_nfa_action_exist(int64_t nfa_id, const string& action);
        lua_map eval_nfa_action(int64_t nfa_id, const string& action, const lua_map& params);
        lua_map do_nfa_action(int64_t nfa_id, const string& action, const lua_map& params);
        void change_nfa_contract(int64_t nfa_id, const string& contract_name);

        //Zone
        int64_t create_zone(const string& name, const string& type_name);
        bool is_zone_valid(int64_t nfa_id);
        bool is_zone_valid_by_name(const string& name);
        contract_zone_base_info get_zone_info(int64_t nfa_id);
        contract_zone_base_info get_zone_info_by_name(const string& name);
        void connect_zones(int64_t from_zone_nfa_id, int64_t to_zone_nfa_id);
        vector<contract_actor_base_info> list_actors_on_zone(int64_t nfa_id);
        string exploit_zone(const string& actor_name, const string& zone_name);
        bool is_contract_allowed_by_zone(const string& zone_name, const string& contract_name);
        void set_zone_contract_permission(const string& zone_name, const string& contract_name, bool allowed);
        void remove_zone_contract_permission(const string& zone_name, const string& contract_name);
        void set_zone_ref_prohibited_contract_zone(const string& zone_name, const string& ref_zone_name);

        //Actor
        int64_t create_actor_talent_rule(const string& contract_name);
        int64_t create_actor(const string& family_name, const string& last_name);
        bool is_actor_valid(int64_t nfa_id);
        bool is_actor_valid_by_name(const string& name);
        contract_actor_base_info get_actor_info(int64_t nfa_id);
        contract_actor_base_info get_actor_info_by_name(const string& name);
        contract_actor_core_attributes get_actor_core_attributes(int64_t nfa_id);
        void born_actor(const string& name, int gender, int sexuality, const lua_map& init_attrs, const string& zone_name);
        string move_actor(const string& actor_name, const string& zone_name);
        
        //Cultivation
        int64_t create_cultivation(int64_t nfa_id, const lua_map& beneficiary_nfa_ids, const lua_map& beneficiary_shares, uint32_t prepare_time_blocks);
        string participate_cultivation(int64_t cult_id, int64_t nfa_id, uint64_t value);
        string start_cultivation(int64_t cult_id);
        string stop_and_close_cultivation(int64_t cult_id);
        bool is_cultivation_exist(int64_t cult_id);

        //以下是不直接暴露到合约的辅助函数
        void transfer_from(account_id_type from, const account_name_type& to, double amount, const string& symbol_name, bool enable_logger=false);
        void transfer_by_contract(account_id_type from, account_id_type to, asset token, contract_result &result, bool enable_logger=false);
        void transfer_nfa_from(account_id_type from, const account_name_type& to, int64_t nfa_id, bool enable_logger=false);
        void transfer_nfa_by_contract(account_id_type from, account_id_type to, int64_t nfa_id, contract_result &result, bool enable_logger=false);
        void approve_nfa_active_from(account_id_type from, const account_name_type& new_active_account, int64_t nfa_id, bool enable_logger=false);
        void approve_nfa_active_by_contract(account_id_type from, account_id_type new_active_account, int64_t nfa_id, contract_result &result, bool enable_logger=false);
        void write_table_data(lua_map& target_table, const lua_map& keys, const lua_map& data, vector<lua_types>& stacks);
        void read_table_data(lua_map& out_data, const lua_map& keys, const lua_map& target_table, vector<lua_types>& stacks);
        lua_map call_nfa_function(int64_t nfa_id, const string& function_name, const lua_map& params, bool assert_when_function_not_exist = true);
        lua_map call_nfa_function_with_caller(const account_object& caller, int64_t nfa_id, const string& function_name, const lua_map& params, bool assert_when_function_not_exist = true);
    };
    
    asset_symbol_type s_get_symbol_type_from_string(const string name);
    void get_connected_zones(const zone_object& zone, std::set<zone_id_type>& connected_zones, database& db);

} } //taiyi::chain

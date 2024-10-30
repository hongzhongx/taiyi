#pragma once
#include <protocol/authority.hpp>
#include <protocol/lua_types.hpp>

#include <chain/taiyi_object_types.hpp>

namespace taiyi { namespace chain {

    using protocol::lua_map;
    using protocol::lua_types;
    using protocol::public_key_type;
    
    class database;
    class LuaContext;

    class contract_object : public object < contract_object_type, contract_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR(contract_object)
        
    public:
        template< typename Constructor, typename Allocator >
        contract_object(Constructor&& c, allocator< Allocator > a)
            : name(a), contract_data(a), contract_ABI(a)
        {
            c(*this);
        }
        
        id_type             id;
        
        account_id_type     owner;
        std::string         name;
        uint32_t            user_invoke_share_percent = 100;
        transaction_id_type current_version;
        public_key_type     contract_authority;
        bool                is_release = false;
        bool                check_contract_authority = false;
        lua_map             contract_data;
        lua_map             contract_ABI;
        contract_bin_code_id_type lua_code_b_id;
        
        time_point_sec      creation_date;
        
    public:
        bool check_contract_authority_falg() { return check_contract_authority; }
        optional<lua_types> get_lua_data(LuaContext &context, int index, bool check_fc = false);
        void push_global_parameters(LuaContext &context, lua_map &global_variable_list, string tablename = "");
        void push_table_parameters(LuaContext &context, lua_map &table_variable, string tablename);
        bool can_do(const database&db) const;
    };

    struct by_name;
    struct by_owner;
    typedef multi_index_container<
        contract_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< contract_object, contract_id_type, &contract_object::id > >,
            ordered_unique< tag< by_name >, member< contract_object, std::string, &contract_object::name > >,
            ordered_unique< tag< by_owner >,
                composite_key< contract_object,
                    member< contract_object, account_id_type, &contract_object::owner >,
                    member< contract_object, contract_id_type, &contract_object::id >
                >
            >
        >,
        allocator< contract_object >
    > contract_index;

    //=============================================================================

    struct boost_lua_variant_visitor : public boost::static_visitor<lua_types>
    {
        template <typename T>
        lua_types operator()(T t) const { return t; }
    };

    class account_contract_data_object : public object < account_contract_data_object_type, account_contract_data_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR(account_contract_data_object)
        
    public:
        template< typename Constructor, typename Allocator >
        account_contract_data_object(Constructor&& c, allocator< Allocator > a)
            : contract_data(a)
        {
            c(*this);
        }
        
        id_type             id;
        
        account_id_type     owner;
        contract_id_type    contract_id;
        lua_map             contract_data;
    };

    struct by_account_contract;
    typedef multi_index_container<
        account_contract_data_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< account_contract_data_object, account_contract_data_id_type, &account_contract_data_object::id > >,
            ordered_unique< tag< by_account_contract >,
                composite_key< account_contract_data_object,
                    member< account_contract_data_object, account_id_type, &account_contract_data_object::owner >,
                    member< account_contract_data_object, contract_id_type, &account_contract_data_object::contract_id >
                >
            >
        >,
        allocator< account_contract_data_object >
    > account_contract_data_index;

    //=============================================================================

    class contract_bin_code_object : public object < contract_bin_code_object_type, contract_bin_code_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR(contract_bin_code_object)
        
    public:
        template< typename Constructor, typename Allocator >
        contract_bin_code_object(Constructor&& c, allocator< Allocator > a)
            : lua_code_b(a)
        {
            c(*this);
        }
        
        id_type             id;
        
        contract_id_type    contract_id;
        vector<char>        lua_code_b;
    };

    struct by_contract_id;
    typedef multi_index_container<
        contract_bin_code_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< contract_bin_code_object, contract_bin_code_id_type, &contract_bin_code_object::id > >,
            ordered_unique< tag< by_contract_id >, member< contract_bin_code_object, contract_id_type, &contract_bin_code_object::contract_id > >
        >,
        allocator< contract_bin_code_object >
    > contract_bin_code_index;

} } // taiyi::chain


FC_REFLECT(taiyi::chain::contract_object, (id)(owner)(name)(user_invoke_share_percent)(current_version)(contract_authority)(is_release)(check_contract_authority)(contract_data)(contract_ABI)(lua_code_b_id)(creation_date) )
CHAINBASE_SET_INDEX_TYPE(taiyi::chain::contract_object, taiyi::chain::contract_index)

FC_REFLECT(taiyi::chain::account_contract_data_object, (id)(owner)(contract_id)(contract_data) )
CHAINBASE_SET_INDEX_TYPE(taiyi::chain::account_contract_data_object, taiyi::chain::account_contract_data_index)

FC_REFLECT(taiyi::chain::contract_bin_code_object, (id)(contract_id)(lua_code_b) )
CHAINBASE_SET_INDEX_TYPE(taiyi::chain::contract_bin_code_object, taiyi::chain::contract_bin_code_index)

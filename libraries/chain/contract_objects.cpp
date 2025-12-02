#include <lua.hpp>

#include <chain/taiyi_fwd.hpp>
#include <chain/taiyi_objects.hpp>
#include <chain/account_object.hpp>
#include <chain/contract_objects.hpp>
#include <chain/contract_handles.hpp>
#include <chain/database.hpp>
#include <chain/lua_context.hpp>

namespace taiyi { namespace chain {

    static const std::string Key_Special[] = {
        "table", "_G", "_VERSION", "coroutine", "debug", "math", "io",
        "utf8", "bit", "package", "string", "os", "cjson","baseENV",
        "require"
    };

    struct boost_lua_variant_visitor : public boost::static_visitor<lua_types>
    {
        template <typename T>
        lua_types operator()(T t) const { return t; }
    };

    optional<lua_types> contract_object::get_lua_data(LuaContext &context, int index, bool check_fc)
    {
        static uint16_t stack = 0;
        stack++;
        FC_ASSERT(context.mState);
        int type = lua_type(context.mState, index);
        switch (type)
        {
            case LUA_TSTRING:
            {
                stack--;
                return lua_string(lua_tostring(context.mState, index));
            }
            case LUA_TBOOLEAN:
            {
                lua_bool v_bool;
                v_bool.v = lua_toboolean(context.mState, index) ? true : false;
                stack--;
                return v_bool;
            }
            case LUA_TNUMBER:
            {
                stack--;
                return lua_number(lua_tonumber(context.mState, index));
            }
            case LUA_TTABLE:
            {
                lua_table v_table;
                lua_pushnil(context.mState); // nil 入栈作为初始 key
                while (0 != lua_next(context.mState, index))
                {
                    auto op_key = get_lua_data(context, index + 1, check_fc);
                    if (op_key)
                    {
                        lua_key key = *op_key;
                        if (stack == 1)
                        {
                            auto len = (sizeof(Key_Special) / sizeof(Key_Special[0]));
                            if ( std::find(Key_Special, Key_Special + len, key.key.get<lua_string>().v) == Key_Special + len &&
                                (LUA_TFUNCTION == lua_type(context.mState, index + 2)) == check_fc)
                            {
                                auto val = get_lua_data(context, index + 2, check_fc);
                                if (val)
                                    v_table.v[key] = *val;
                            }
                        }
                        else
                        {
                            bool skip = false;
                            if (key.key.which() == lua_key_variant::tag<lua_string>::value)
                                if (key.key.get<lua_string>().v == "__index")
                                    skip = true;
                            if (!skip)
                            {
                                auto val = get_lua_data(context, index + 2, check_fc);
                                if (val)
                                    v_table.v[key] = *val;
                            }
                        }
                    }
                    lua_pop(context.mState, 1);
                }
                stack--;
                return v_table;
            }
            case LUA_TFUNCTION:
            {
                auto sfunc = LuaContext::Reader<FunctionSummary>::read(context.mState, index);
                if (sfunc)
                {
                    stack--;
                    return *sfunc;
                }
                break;
            }
            case LUA_TTHREAD:
            {
                break;
            }
            case LUA_TNIL:
            {
                break;
            }
            default: // LUA_TUSERDATA ,  LUA_TLIGHTUSERDATA
            {
                try
                {
                    auto boost_lua_variant_object = LuaContext::readTopAndPop<lua_types::boost_variant>(context.mState, index);
                    if (boost_lua_variant_object.which() != 0)
                    {
                        stack--;
                        return boost::apply_visitor(boost_lua_variant_visitor(), boost_lua_variant_object);
                    }
                }
                catch (...)
                {
                    wlog("readTopAndPop error :type->${type}", ("type", type));
                }
            }
        }
        stack--;
        return {};
    }
    //=============================================================================
    void contract_object::push_global_parameters(LuaContext &context, lua_map &global_variable_list, string tablename)
    {
        static vector<lua_types::boost_variant> values;
        for (auto itr = global_variable_list.begin(); itr != global_variable_list.end(); itr++)
        {
            switch (itr->second.which())
            {
                case protocol::lua_type_num::nlua_table:
                {
                    lua_key key = itr->first;
                    values.push_back(key.cast_to_lua_types().to_boost_variant());
                    LuaContext::set_emptyArray(context, tablename, values);
                    LUATYPE_NAME(table)
                    table_value = itr->second.get<LUATYPE_NAME(table)>();
                    push_global_parameters(context, table_value.v, tablename);
                    values.pop_back();
                    break;
                }
                default:
                {
                    lua_key key = itr->first;
                    values.push_back(key.cast_to_lua_types().to_boost_variant());
                    values.push_back(itr->second.to_boost_variant());
                    LuaContext::push_key_and_value_stack(context, tablename, values);
                    values.pop_back();
                    values.pop_back();
                    break;
                }
            }
        }
    }
    //=============================================================================
    void contract_object::push_table_parameters(LuaContext &context, lua_map &table_variable, string tablename)
    {
        vector<lua_types::boost_variant> tvalues;
        tvalues.push_back(tablename);
        LuaContext::set_emptyArray(context, tvalues);
        push_global_parameters(context, table_variable, tablename);
    }
    //=============================================================================
    bool contract_object::can_do(const database&db) const
    {
        const auto* contract = db.find<contract_object, by_name>(TAIYI_BLACKLIST_CONTRACT_NAME);
        if(contract != nullptr)
        {
            const auto black_list_p = contract->contract_data.find(lua_key(lua_string("black_list")));
            if(black_list_p == contract->contract_data.end() || black_list_p->second.which() != lua_types::tag<lua_table>::value)
            {
                return true;
            }
            else
            {
                const auto& black_list = black_list_p->second.get<lua_table>().v;
                auto isfind = black_list.find(lua_key(lua_string(name)));
                if(isfind == black_list.end())
                    return true;
                else
                    return false;
            }
        }
        else
            return true;
    }

} } // namespace taiyi::chain

#include <chain/taiyi_fwd.hpp>

#include <chain/database.hpp>

#include <chain/taiyi_objects.hpp>
#include <chain/block_summary_object.hpp>
#include <chain/account_object.hpp>
#include <chain/nfa_objects.hpp>
#include <chain/contract_objects.hpp>

#include <chain/lua_context.hpp>
#include <chain/contract_handles.hpp>

extern "C" {
#include "lobject.h"
#include "lstate.h"
}

//debug allocator
void* Allocator(void* ud, void* ptr, size_t osize, size_t nsize)
{
    Tracker* pTracker = (Tracker*)ud;
    void* pRet = NULL;
    if( nsize == 0 )
    {
        pTracker->m_usage -= osize;
        printf("Free %zu bytes; ", osize);
        free(ptr);
    }
    else
    {
        pTracker->m_usage -= osize;
        printf("first Free %zu bytes; ", osize);
        pTracker->m_usage += nsize;
        printf("then alloc %zu bytes; ", nsize);
        pRet = realloc(ptr, nsize);
    }

    printf("current usage: %zu bytes\n", pTracker->m_usage);
    return pRet;
}

namespace taiyi { namespace chain {

    static int get_account_contract_data(lua_State *L)
    {
        try
        {
            LuaContext context(L);
            
            FC_ASSERT(lua_isstring(L, -2) && lua_istable(L, -1));
            auto name_or_id = LuaContext::readTopAndPop<string>(L, -2);
            auto read_list = LuaContext::readTopAndPop<lua_table>(L, -1);
            auto current_contract_name = context.readVariable<string>("current_contract");
            auto ch = context.readVariable<contract_handler *>(current_contract_name, "contract_helper");
            auto &temp_account = ch->get_account(name_or_id);
            const auto* contract_udata = ch->db.find<account_contract_data_object, by_account_contract>(boost::make_tuple(temp_account.id, ch->contract.id));
            optional<account_contract_data_object> op_acd;
            if (contract_udata != nullptr)
                op_acd = *contract_udata;
            else
                op_acd = account_contract_data_object();
            if (read_list.v.size() > 0)
            {
                vector<lua_types> stacks;
                lua_map result_table;
                contract_handler::filter_context(op_acd->contract_data, read_list.v, stacks, &result_table);
                LuaContext::Pusher<lua_types>::push(L, lua_table(result_table)).release();
            }
            else
            {
                LuaContext::Pusher<lua_types>::push(L, lua_table(op_acd->contract_data)).release();
            }
            return 1;
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(L, e.to_string());
        }
        catch (std::exception e)
        {
            LUA_C_ERR_THROW(L, e.what());
        }
        return 0;
    }
    //=============================================================================
    static int import_contract(lua_State *L)
    {
        const contract_object* contract = nullptr;
        try
        {
            FC_ASSERT(lua_isstring(L, -1));
            auto imported_contract_name = LuaContext::readTopAndPop<string>(L, -1);
            
            lua_getglobal(L, "current_contract");
            auto current_contract_name = LuaContext::readTopAndPop<string>(L, -1);
            
            if(current_contract_name != imported_contract_name) {
                //在当前运行合约的环境下创建并缓存新合约环境，作为导入合约要使用的_ENV空间
                
                FC_ASSERT(lua_getglobal(L, current_contract_name.c_str()) == LUA_TTABLE, "current contract (${n}) space not exist.", ("n", current_contract_name));
                if (lua_getfield(L, -1, imported_contract_name.c_str()) == LUA_TTABLE) {
                    //在当前合约运行的过程中，已经导入过这个合约了，因此有缓存的环境
                    wlog("imported contract name space (${n}) is already exist.", ("n", imported_contract_name));
                    return 1;
                }
                lua_pop(L, -1);
                
                //创建新合约环境
                LuaContext context(L);
                
                auto cbi = context.readVariable<contract_base_info*>(current_contract_name, "contract_base_info");
                FC_ASSERT(cbi, "contract_base_info must be available!");
                contract = &cbi->get_contract(imported_contract_name);
                
                const auto &baseENV = cbi->db.get<contract_bin_code_object, by_id>(0);
                context.new_sandbox(contract->name, baseENV.lua_code_b.data(), baseENV.lua_code_b.size());
                
                context.writeVariable(contract->name, "contract_base_info", cbi);
                auto ch = context.readVariable<contract_handler*>(current_contract_name, "contract_helper");
                if(ch) context.writeVariable(contract->name, "contract_helper", ch);
                
                FC_ASSERT(lua_getglobal(context.mState, current_contract_name.c_str()) == LUA_TTABLE);
                FC_ASSERT(lua_getfield(context.mState, -1, contract->name.c_str()) == LUA_TNIL); //just check
                lua_pop(context.mState, 1);
                lua_getglobal(context.mState, contract->name.c_str());
                lua_setfield(context.mState, -2, contract->name.c_str());
                lua_pushnil(context.mState);
                lua_setglobal(context.mState, contract->name.c_str());
                
                const auto &contract_code = cbi->db.get<contract_bin_code_object, by_id>(contract->lua_code_b_id);
                //lua加载脚本之后会返回一个函数(即此时栈顶的chunk块)，lua_pcall将默认调用此块
                luaL_loadbuffer(context.mState, contract_code.lua_code_b.data(), contract_code.lua_code_b.size(), contract->name.data());
                lua_getglobal(context.mState, current_contract_name.c_str());
                lua_getfield(context.mState, -1, contract->name.c_str());
                //将栈顶变量赋值给距栈顶二格的函数的第一个upvalue(这个函数为load返回的函数，第一个upvalue为_ENV)
                lua_setupvalue(context.mState, -3, 1);
                lua_pop(context.mState, 1);
                
                int err = lua_pcall(context.mState, 0, 0, 0);
                FC_ASSERT(err == 0, "import_contract ${c} error: ${e}", ("c", contract->name)("e", string(lua_tostring(context.mState, -1))));
                
                lua_getglobal(context.mState, current_contract_name.c_str());
                lua_getfield(context.mState, -1, contract->name.c_str());
            }
            else {
                //正在当前合约环境中载入同样的合约，直接获取当前的环境，作为返回值
                lua_getglobal(L, current_contract_name.c_str());
                wlog("imported contract is in current contract session (${n}).", ("n", current_contract_name));
            }
            
            FC_ASSERT(lua_istable(L, -1), "import_contract must get a table");
            return 1;
        }
        catch (fc::exception e)
        {
            if(contract) {
                lua_pushnil(L);
                lua_setglobal(L, contract->name.c_str());
            }
            
            wdump((e.to_detail_string()));
            LUA_C_ERR_THROW(L, e.to_string());
        }
        catch (std::runtime_error e)
        {
            if(contract) {
                lua_pushnil(L);
                lua_setglobal(L, contract->name.c_str());
            }
            
            wdump((e.what()));
            LUA_C_ERR_THROW(L, e.what());
        }
        
        return 0;
    }
    //=============================================================================
    static int format_vector_with_table(lua_State *L)
    {
        try
        {
            LuaContext context(L);
            FC_ASSERT(lua_istable(L, -1));
            auto map_list = LuaContext::readTopAndPop<lua_table>(L, -1);
            vector<lua_types> temp;
            for(auto itr=map_list.v.begin();itr!=map_list.v.end();itr++)
                temp.emplace_back(itr->second);
            LuaContext::Pusher<string>::push(L,fc::json::to_string(fc::variant(temp)) ).release();
            return 1;
        }
        catch (fc::exception e)
        {
            wdump((e.to_detail_string()));
            LUA_C_ERR_THROW(L, e.to_string());
        }
        catch (std::runtime_error e)
        {
            wdump((e.what()));
            LUA_C_ERR_THROW(L, e.what());
        }
        return 0;
    }
    //=============================================================================
    void LuaContext::chain_function_bind()
    {
        //contract base info
        registerMember("owner", &contract_base_info::owner);
        registerMember("name", &contract_base_info::name);
        registerMember("caller", &contract_base_info::caller);
        registerMember("creation_date", &contract_base_info::creation_date);
        registerMember("contract_authority", &contract_base_info::contract_authority);
        registerMember("invoker_contract_name", &contract_base_info::invoker_contract_name);
        
        //resources
        registerMember("gold", &contract_asset_resources::gold);
        registerMember("food", &contract_asset_resources::food);
        registerMember("wood", &contract_asset_resources::wood);
        registerMember("fabric", &contract_asset_resources::fabric);
        registerMember("herb", &contract_asset_resources::herb);

        //contract handler
        registerFunction("log", &contract_handler::log);
        registerFunction("number_max", &contract_handler::nummax);
        registerFunction("number_min", &contract_handler::nummin);
        registerFunction("integer_max", &contract_handler::integermax);
        registerFunction("integer_min", &contract_handler::integermin);
        registerFunction("time", &contract_handler::head_block_time);
        registerFunction("hash256", &contract_handler::hash256);
        registerFunction("hash512", &contract_handler::hash512);
        registerFunction("make_memo", &contract_handler::make_memo);
        registerFunction("make_release", &contract_handler::make_release);
        registerFunction("random", &contract_handler::contract_random);
        registerFunction("is_owner", &contract_handler::is_owner);
        registerFunction("read_chain", &contract_handler::read_chain);
        registerFunction("write_chain", &contract_handler::write_chain);
        registerFunction("set_permissions_flag", &contract_handler::set_permissions_flag);
        registerFunction("set_invoke_share_percent", &contract_handler::set_invoke_share_percent);
        registerFunction("invoke_contract_function", &contract_handler::invoke_contract_function);
        registerFunction("change_contract_authority", &contract_handler::change_contract_authority);
        registerFunction("get_contract_public_data", &contract_handler::get_contract_public_data);
        registerFunction("get_nfa_contract", &contract_handler::get_nfa_contract);
        registerFunction("eval_nfa_action", &contract_handler::eval_nfa_action);
        registerFunction("do_nfa_action", &contract_handler::do_nfa_action);
        registerFunction("get_nfa_info", &contract_handler::get_nfa_info);
        registerFunction("is_nfa_valid", &contract_handler::is_nfa_valid);        
        registerFunction("get_nfa_resources", &contract_handler::get_nfa_resources);
        registerFunction("change_zone_type", &contract_handler::change_zone_type);
        registerFunction("get_zone_info", &contract_handler::get_zone_info);
        registerFunction("get_zone_info_by_name", &contract_handler::get_zone_info_by_name);
        registerFunction("connect_zones", &contract_handler::connect_zones);

        lua_register(mState, "import_contract", &import_contract);
        lua_register(mState, "get_account_contract_data", &get_account_contract_data);
        lua_register(mState, "format_vector_with_table", &format_vector_with_table);
        
        registerFunction<contract_handler, void(const string&, double, const string&, bool)>("transfer_from_owner", [](contract_handler &handler, const string& to, double amount, const string& symbol, bool enable_logger = false) {
            handler.transfer_from(handler.contract.owner, to, amount, symbol, enable_logger);
        });
        registerFunction<contract_handler, void(const string&, double, const string&, bool)>("transfer_from_caller", [](contract_handler &handler, const string& to, double amount, const string& symbol, bool enable_logger = false) {
            handler.transfer_from(handler.caller.id, to, amount, symbol, enable_logger);
        });
        
        //nfa base info
        registerMember("id", &contract_nfa_base_info::id);
        registerMember("symbol", &contract_nfa_base_info::symbol);
        registerMember("creator_account", &contract_nfa_base_info::creator_account);
        registerMember("owner_account", &contract_nfa_base_info::owner_account);
        registerMember("qi", &contract_nfa_base_info::qi);
        registerMember("parent", &contract_nfa_base_info::parent);
        registerMember("data", &contract_nfa_base_info::data);

        //nfa handler
        registerFunction("enable_tick", &contract_nfa_handler::enable_tick);
        registerFunction("disable_tick", &contract_nfa_handler::disable_tick);
        registerFunction("get_info", &contract_nfa_handler::get_info);
        registerFunction("get_resources", &contract_nfa_handler::get_resources);
        registerFunction("convert_qi_to_resource", &contract_nfa_handler::convert_qi_to_resource);
        registerFunction("add_child", &contract_nfa_handler::add_child);
        registerFunction("add_to_parent", &contract_nfa_handler::add_to_parent);
        registerFunction("remove_from_parent", &contract_nfa_handler::remove_from_parent);        
        registerFunction("read_chain", &contract_nfa_handler::read_chain);
        registerFunction("write_chain", &contract_nfa_handler::write_chain);
        registerFunction("destroy", &contract_nfa_handler::destroy);        
        registerFunction<contract_nfa_handler, void(int64_t to, double, const string&, bool)>("transfer_to", [](contract_nfa_handler &handler, int64_t to, double amount, const string& symbol, bool enable_logger = false) {
            handler.transfer_from(handler._caller.id, to, amount, symbol, enable_logger);
        });
        registerFunction<contract_nfa_handler, void(const string&, double, const string&, bool)>("deposit_from", [](contract_nfa_handler &handler, const string& from, double amount, const string& symbol, bool enable_logger = false) {
            handler.deposit_from(from, handler._caller.id, amount, symbol, enable_logger);
        });
        registerFunction<contract_nfa_handler, void(const string&, double, const string&, bool)>("withdraw_to", [](contract_nfa_handler &handler, const string& to, double amount, const string& symbol, bool enable_logger = false) {
            handler.withdraw_to(handler._caller.id, to, amount, symbol, enable_logger);
        });

        //zone base info
        registerMember("nfa_id", &contract_zone_base_info::nfa_id);
        registerMember("name", &contract_zone_base_info::name);
        registerMember("type", &contract_zone_base_info::type);
    }
    //=============================================================================
    bool LuaContext::new_sandbox(string spacename, const char *condition, size_t condition_size)
    {
        lua_getglobal(mState, "baseENV");
        if (lua_isnil(mState, -1))
        {
            lua_pop(mState, 1);
            luaL_loadbuffer(mState, condition, condition_size, (spacename + " baseENV").data());
            lua_setglobal(mState, "baseENV");
            lua_getglobal(mState, "baseENV");
        }
        int err = lua_pcall(mState, 0, 1, 0);
        if (err)
            FC_THROW("Contract loading infrastructure failure,${message}", ("message", string(lua_tostring(mState, -1))));
        if(!spacename.empty())
        {
            lua_setglobal(mState, spacename.data());
            lua_pop(mState, -1);
        }
        return true;
    }
    //=============================================================================
    bool LuaContext::get_sandbox(string spacename)
    {
        lua_getglobal(mState, spacename.data());
        if (lua_istable(mState, -1))
            return true;
        lua_pop(mState, 1);
        return false;
    }
    //=============================================================================
    bool LuaContext::close_sandbox(string spacename)
    {
        lua_getglobal(mState, "_G"); /* table for ns list */
        lua_getfield(mState, -1, spacename.data());
        if (!lua_istable(mState, -1)) /* no such ns */
            return false;
        lua_pop(mState, 1);
        lua_pushnil(mState);
        lua_setfield(mState, -2, spacename.data());
        lua_gc(mState, LUA_GCCOLLECT, 0);
        return true;
    }
    //=============================================================================
    bool LuaContext::load_script_to_sandbox(string spacename, const char *script, size_t script_size)
    {
        //lua加载脚本之后会返回一个函数(即此时栈顶的chunk块)，lua_pcall将默认调用此块
        int sta = luaL_loadbuffer(mState, script, script_size, spacename.data());
        lua_getglobal(mState, spacename.data()); //想要使用的_ENV备用空间
        //将栈顶变量赋值给栈顶第二个函数的第一个upvalue(当前第二个函数为load返回的函数，第一个upvalue为_ENV)
        //注：upvalue:函数的外部引用变量在赋值成功以后，栈顶自动回弹一层
        lua_setupvalue(mState, -2, 1);
        /****************************************************/
        //lua_getglobal(mState, "_G");
        //lua_getfield(mState, -1, spacename.data());
        //lua_setupvalue(mState, -3, 1);
        //lua_pop(mState, 1);//在赋值成功以后，栈顶自动回弹一层,当前栈顶指向“_G”,需要手动pop一层，指向lua_pcall需要呼叫的chunk块
        /**************************************************/
        sta |= lua_pcall(mState, 0, 0, 0);
        return sta ? false : true;
    }
    //=============================================================================
    bool LuaContext::get_function(string spacename, string func)
    {
        lua_getglobal(mState, spacename.data());
        if (lua_istable(mState, -1))
        {
            lua_getfield(mState, -1, func.data());
            if (lua_isfunction(mState, -1))
            {
                return true;
            }
        }
        return false;
    }
    //=============================================================================
    boost::optional<FunctionSummary> LuaContext::Reader<FunctionSummary>::read(lua_State *state, int index, int depth)
    {
        Closure *pt = (Closure *)lua_topointer(state, index);
        if (pt != 0 && ttisclosure((TValue *)pt) && pt->l.p != 0)
        {
            struct FunctionSummary sfunc;
            Proto *pro = pt->l.p;
            auto arg_num = (int)pro->numparams;
            sfunc.is_var_arg = (bool)pro->is_vararg;
            LocVar *var = pro->locvars;
            if (arg_num != 0 && arg_num <= pro->sizelocvars && var != nullptr)
            {
                LocVar *per = nullptr;
                for (int pos = 0; pos < arg_num; pos++)
                {
                    per = var + pos;
                    sfunc.arglist.push_back(getstr(per->varname));
                }
            }
            return sfunc;
        }
        return boost::none;
    }

} } // namespace taiyi::chain

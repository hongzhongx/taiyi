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

    static int import_contract(lua_State *L)
    {
        const contract_object* contract = nullptr;
        try
        {
            FC_ASSERT(lua_isstring(L, -1));
            auto imported_contract_name = LuaContext::readTopAndPop<string>(L, -1);
            
            lua_getglobal(L, "current_contract"); //注意，这个是顶层合约，不是导入函数所在的当前合约
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
                
                auto current_cbi = context.readVariable<contract_base_info*>(current_contract_name, "contract_base_info");
                FC_ASSERT(current_cbi, "contract_base_info must be available!");
                contract = &current_cbi->get_contract(imported_contract_name);
                
                current_cbi->db.add_contract_handler_exe_point(3);
                
                const auto &baseENV = current_cbi->db.get<contract_bin_code_object, by_id>(0);
                context.new_sandbox(contract->name, baseENV.lua_code_b.data(), baseENV.lua_code_b.size());
                
                contract_base_info cbi(current_cbi->db, context, current_cbi->db.get<account_object, by_id>(contract->owner).name, contract->name, current_cbi->caller, string(contract->creation_date), current_cbi->invoker_contract_name);
                context.writeVariable(contract->name, "contract_base_info", &cbi);
                
                auto current_ch = context.readVariable<contract_handler*>(current_contract_name, "contract_helper");
                contract_result tmp_result;
                contract_handler* ch = 0;
                if(current_ch) {
                    //导入的合约需要创建新的ch，由于在导入后会对合约函数进行调用，因此这里创建的ch必须有效，因此需要保留在上层ch中，直到上层ch释放时释放
                    ch = new contract_handler(current_cbi->db, current_cbi->db.get_account(current_cbi->caller), current_ch->nfa_caller, *contract, current_ch->result, current_ch->context, current_ch->is_in_eval);
                    FC_ASSERT(current_ch->context.mState == L);
                    current_ch->_sub_chs.push_back(ch);
                    context.writeVariable(contract->name, "contract_helper", ch);
                }
                auto current_cnh = context.readVariable<contract_nfa_handler*>(current_contract_name, "nfa_helper");
                context.writeVariable(contract->name, "nfa_helper", current_cnh);

                //将导入合约同名的表放到顶层合约同名的表中作为子项（之后有需要只能从这个子项里面获取），然后清除上下文中刚才位导入合约环境创建的表
                FC_ASSERT(lua_getglobal(context.mState, current_contract_name.c_str()) == LUA_TTABLE);
                FC_ASSERT(lua_getfield(context.mState, -1, contract->name.c_str()) == LUA_TNIL); //just check
                lua_pop(context.mState, 1);
                lua_getglobal(context.mState, contract->name.c_str());
                lua_setfield(context.mState, -2, contract->name.c_str());
                lua_pushnil(context.mState);
                lua_setglobal(context.mState, contract->name.c_str());
                
                const auto &contract_code = cbi.db.get<contract_bin_code_object, by_id>(contract->lua_code_b_id);
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
        catch (const fc::exception& e)
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
        catch (const fc::exception& e)
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
        registerMember("invoker_contract_name", &contract_base_info::invoker_contract_name);
        
        //resources
        registerMember("gold", &contract_asset_resources::gold);
        registerMember("food", &contract_asset_resources::food);
        registerMember("wood", &contract_asset_resources::wood);
        registerMember("fabric", &contract_asset_resources::fabric);
        registerMember("herb", &contract_asset_resources::herb);
        
        //tiandao
        registerMember("v_years", &contract_tiandao_property::v_years);
        registerMember("v_months", &contract_tiandao_property::v_months);
        registerMember("v_times", &contract_tiandao_property::v_times);
        registerMember("v_days", &contract_tiandao_property::v_days);
        registerMember("v_timeonday", &contract_tiandao_property::v_timeonday);
        registerMember("v_1day_blocks", &contract_tiandao_property::v_1day_blocks);

        //contract handler
        registerFunction("log", &contract_handler::log);
        registerFunction("narrate", &contract_handler::narrate);
        registerFunction("number_max", &contract_handler::nummax);
        registerFunction("number_min", &contract_handler::nummin);
        registerFunction("integer_max", &contract_handler::integermax);
        registerFunction("integer_min", &contract_handler::integermin);
        registerFunction("time", &contract_handler::head_block_time);
        registerFunction("block", &contract_handler::head_block_num);
        registerFunction("hash256", &contract_handler::hash256);
        registerFunction("hash512", &contract_handler::hash512);
        registerFunction("generate_hash", &contract_handler::generate_hash);        
        registerFunction("make_memo", &contract_handler::make_memo);
        registerFunction("make_release", &contract_handler::make_release);
        registerFunction("random", &contract_handler::contract_random);
        registerFunction("zuowangdao_account_name", &contract_handler::zuowangdao_account_name);
        registerFunction("is_owner", &contract_handler::is_owner);
        registerFunction("get_nfa_caller", &contract_handler::get_nfa_caller);
        registerFunction("read_contract_data", &contract_handler::read_contract_data);
        registerFunction("write_contract_data", &contract_handler::write_contract_data);
        registerFunction("get_account_contract_data", &contract_handler::get_account_contract_data);
        registerFunction("read_account_contract_data", &contract_handler::read_account_contract_data);
        registerFunction("write_account_contract_data", &contract_handler::write_account_contract_data);
        registerFunction("get_account_balance", &contract_handler::get_account_balance);        
        registerFunction("invoke_contract_function", &contract_handler::invoke_contract_function);
        registerFunction("get_contract_source_code", &contract_handler::get_contract_source_code);
        registerFunction("get_contract_data", &contract_handler::get_contract_data);
        registerFunction("create_nfa_symbol", &contract_handler::create_nfa_symbol);
        registerFunction("change_nfa_symbol_authority", &contract_handler::change_nfa_symbol_authority);
        registerFunction("get_nfa_contract", &contract_handler::get_nfa_contract);
        registerFunction("change_nfa_contract", &contract_handler::change_nfa_contract);
        registerFunction("create_nfa_to_actor", &contract_handler::create_nfa_to_actor);
        registerFunction("create_nfa_to_account", &contract_handler::create_nfa_to_account);
        registerFunction("read_nfa_contract_data", &contract_handler::read_nfa_contract_data);
        registerFunction("write_nfa_contract_data", &contract_handler::write_nfa_contract_data);
        registerFunction("is_nfa_action_exist", &contract_handler::is_nfa_action_exist);
        registerFunction("eval_nfa_action", &contract_handler::eval_nfa_action);
        registerFunction("do_nfa_action", &contract_handler::do_nfa_action);
        registerFunction("get_nfa_info", &contract_handler::get_nfa_info);
        registerFunction("is_nfa_valid", &contract_handler::is_nfa_valid);
        registerFunction("get_nfa_balance", &contract_handler::get_nfa_balance);
        registerFunction("get_nfa_resources", &contract_handler::get_nfa_resources);
        registerFunction("get_nfa_materials", &contract_handler::get_nfa_materials);
        registerFunction("list_nfa_inventory", &contract_handler::list_nfa_inventory);
        registerFunction("get_nfa_location", &contract_handler::get_nfa_location);
        registerFunction("create_zone", &contract_handler::create_zone);
        registerFunction("get_zone_info", &contract_handler::get_zone_info);
        registerFunction("get_zone_info_by_name", &contract_handler::get_zone_info_by_name);
        registerFunction("is_zone_valid", &contract_handler::is_zone_valid);        
        registerFunction("is_zone_valid_by_name", &contract_handler::is_zone_valid_by_name);
        registerFunction("connect_zones", &contract_handler::connect_zones);
        registerFunction("create_actor_talent_rule", &contract_handler::create_actor_talent_rule);
        registerFunction("create_actor", &contract_handler::create_actor);
        registerFunction("list_actors_on_zone", &contract_handler::list_actors_on_zone);
        registerFunction("is_actor_valid", &contract_handler::is_actor_valid);
        registerFunction("is_actor_valid_by_name", &contract_handler::is_actor_valid_by_name);
        registerFunction("get_actor_info", &contract_handler::get_actor_info);
        registerFunction("get_actor_info_by_name", &contract_handler::get_actor_info_by_name);
        registerFunction("get_actor_core_attributes", &contract_handler::get_actor_core_attributes);        
        registerFunction("born_actor", &contract_handler::born_actor);
        registerFunction("move_actor", &contract_handler::move_actor);
        registerFunction("exploit_zone", &contract_handler::exploit_zone);
        registerFunction("is_contract_allowed_by_zone", &contract_handler::is_contract_allowed_by_zone);
        registerFunction("set_zone_contract_permission", &contract_handler::set_zone_contract_permission);
        registerFunction("remove_zone_contract_permission", &contract_handler::remove_zone_contract_permission);
        registerFunction("set_zone_ref_prohibited_contract_zone", &contract_handler::set_zone_ref_prohibited_contract_zone);
        registerFunction("get_tiandao_property", &contract_handler::get_tiandao_property);
        registerFunction("create_cultivation", &contract_handler::create_cultivation);
        registerFunction("participate_cultivation", &contract_handler::participate_cultivation);
        registerFunction("start_cultivation", &contract_handler::start_cultivation);
        registerFunction("stop_and_close_cultivation", &contract_handler::stop_and_close_cultivation);
        registerFunction("is_cultivation_exist", &contract_handler::is_cultivation_exist);        
        registerFunction("create_named_contract", &contract_handler::create_named_contract);
        registerFunction("create_proposal", &contract_handler::create_proposal);
        registerFunction("update_proposal_votes", &contract_handler::update_proposal_votes);
        registerFunction("remove_proposals", &contract_handler::remove_proposals);
        registerFunction("grant_xinsu", &contract_handler::grant_xinsu);
        registerFunction("revoke_xinsu", &contract_handler::revoke_xinsu);

        lua_register(mState, "import_contract", &import_contract);
        lua_register(mState, "format_vector_with_table", &format_vector_with_table);
        
        registerFunction<contract_handler, void(const string&, double, const string&, bool)>("transfer_from_owner", [](contract_handler &handler, const string& to, double amount, const string& symbol, bool enable_logger = false) {
            handler.transfer_from(handler.contract.owner, to, amount, symbol, enable_logger);
        });
        registerFunction<contract_handler, void(const string&, double, const string&, bool)>("transfer_from_caller", [](contract_handler &handler, const string& to, double amount, const string& symbol, bool enable_logger = false) {
            handler.transfer_from(handler.caller.id, to, amount, symbol, enable_logger);
        });
        registerFunction<contract_handler, void(const string&, int64_t, bool)>("transfer_nfa_from_owner", [](contract_handler &handler, const string& to, int64_t nfa_id, bool enable_logger = false) {
            handler.transfer_nfa_from(handler.contract.owner, to, nfa_id, enable_logger);
        });
        registerFunction<contract_handler, void(const string&, int64_t, bool)>("transfer_nfa_from_caller", [](contract_handler &handler, const string& to, int64_t nfa_id, bool enable_logger = false) {
            handler.transfer_nfa_from(handler.caller.id, to, nfa_id, enable_logger);
        });
        registerFunction<contract_handler, void(const string&, int64_t, bool)>("approve_nfa_active_from_owner", [](contract_handler &handler, const string& to, int64_t nfa_id, bool enable_logger = false) {
            handler.approve_nfa_active_from(handler.contract.owner, to, nfa_id, enable_logger);
        });
        registerFunction<contract_handler, void(const string&, int64_t, bool)>("approve_nfa_active_from_caller", [](contract_handler &handler, const string& to, int64_t nfa_id, bool enable_logger = false) {
            handler.approve_nfa_active_from(handler.caller.id, to, nfa_id, enable_logger);
        });

        //nfa base info
        registerMember("id", &contract_nfa_base_info::id);
        registerMember("symbol", &contract_nfa_base_info::symbol);
        registerMember("creator_account", &contract_nfa_base_info::creator_account);
        registerMember("owner_account", &contract_nfa_base_info::owner_account);
        registerMember("active_account", &contract_nfa_base_info::active_account);
        registerMember("qi", &contract_nfa_base_info::qi);
        registerMember("parent", &contract_nfa_base_info::parent);
        registerMember("five_phase", &contract_nfa_base_info::five_phase);
        registerMember("data", &contract_nfa_base_info::data);
        registerMember("main_contract", &contract_nfa_base_info::main_contract);

        //nfa handler
        registerFunction("enable_tick", &contract_nfa_handler::enable_tick);
        registerFunction("disable_tick", &contract_nfa_handler::disable_tick);
        registerFunction("get_info", &contract_nfa_handler::get_info);
        registerFunction("get_resources", &contract_nfa_handler::get_resources);
        registerFunction("get_materials", &contract_nfa_handler::get_materials);        
        registerFunction("convert_qi_to_resource", &contract_nfa_handler::convert_qi_to_resource);
        registerFunction("convert_resource_to_qi", &contract_nfa_handler::convert_resource_to_qi);
        registerFunction("add_child", &contract_nfa_handler::add_child);
        registerFunction("add_to_parent", &contract_nfa_handler::add_to_parent);
        registerFunction("add_child_from_contract_owner", &contract_nfa_handler::add_child_from_contract_owner);
        registerFunction("add_to_parent_from_contract_owner", &contract_nfa_handler::add_to_parent_from_contract_owner);
        registerFunction("remove_from_parent", &contract_nfa_handler::remove_from_parent);
        registerFunction("read_contract_data", &contract_nfa_handler::read_contract_data);
        registerFunction("write_contract_data", &contract_nfa_handler::write_contract_data);
        registerFunction("destroy", &contract_nfa_handler::destroy);
        registerFunction("eval_action", &contract_nfa_handler::eval_action);
        registerFunction("do_action", &contract_nfa_handler::do_action);
        //nfa handler - actor
        registerFunction("modify_actor_attributes", &contract_nfa_handler::modify_actor_attributes);
        registerFunction("talk_to_actor", &contract_nfa_handler::talk_to_actor);

        registerFunction<contract_nfa_handler, void(int64_t to, double, const string&, bool)>("transfer_to", [](contract_nfa_handler &handler, int64_t to, double amount, const string& symbol, bool enable_logger = false) {
            handler.transfer_from(handler._caller.id, to, amount, symbol, enable_logger);
        });
        registerFunction<contract_nfa_handler, void(int64_t to, double, const string&, bool)>("inject_material_to", [](contract_nfa_handler &handler, int64_t to, double amount, const string& symbol, bool enable_logger = false) {
            handler.inject_material_from(handler._caller.id, to, amount, symbol, enable_logger);
        });
        registerFunction<contract_nfa_handler, void(int64_t to, double, const string&, bool)>("separate_material_out", [](contract_nfa_handler &handler, int64_t to, double amount, const string& symbol, bool enable_logger = false) {
            handler.separate_material_out(handler._caller.id, to, amount, symbol, enable_logger);
        });
        registerFunction<contract_nfa_handler, void(double, const string&, bool)>("deposit_from_owner", [](contract_nfa_handler &handler, double amount, const string& symbol, bool enable_logger = false) {
            handler.deposit_from(handler._ch.contract.owner, handler._caller.id, amount, symbol, enable_logger);
        });
        registerFunction<contract_nfa_handler, void(const string&, double, const string&, bool)>("deposit_from", [](contract_nfa_handler &handler, const string& from, double amount, const string& symbol, bool enable_logger = false) {
            auto from_id = handler._db.get_account(from).id;
            FC_ASSERT(handler._ch.caller.id == from_id, "caller have no authority to transfer assets from account \"${a}\"", ("a", from));
            handler.deposit_from(from_id, handler._caller.id, amount, symbol, enable_logger);
        });
        registerFunction<contract_nfa_handler, void(const string&, double, const string&, bool)>("withdraw_to", [](contract_nfa_handler &handler, const string& to, double amount, const string& symbol, bool enable_logger = false) {
            handler.withdraw_to(handler._caller.id, to, amount, symbol, enable_logger);
        });

        //zone base info
        registerMember("nfa_id", &contract_zone_base_info::nfa_id);
        registerMember("name", &contract_zone_base_info::name);
        registerMember("type", &contract_zone_base_info::type);
        registerMember("type_id", &contract_zone_base_info::type_id);
        registerMember("ref_prohibited_contract_zone", &contract_zone_base_info::ref_prohibited_contract_zone);

        //actor base info
        registerMember("nfa_id", &contract_actor_base_info::nfa_id);
        registerMember("name", &contract_actor_base_info::name);
        registerMember("age", &contract_actor_base_info::age);
        registerMember("health", &contract_actor_base_info::health);
        registerMember("health_max", &contract_actor_base_info::health_max);
        registerMember("born", &contract_actor_base_info::born);
        registerMember("born_vyears", &contract_actor_base_info::born_vyears);
        registerMember("born_vmonths", &contract_actor_base_info::born_vmonths);
        registerMember("born_vdays", &contract_actor_base_info::born_vdays);
        registerMember("born_vtod", &contract_actor_base_info::born_vtod);
        registerMember("born_vtimes", &contract_actor_base_info::born_vtimes);
        registerMember("five_phase", &contract_actor_base_info::five_phase);
        registerMember("gender", &contract_actor_base_info::gender);
        registerMember("standpoint", &contract_actor_base_info::standpoint);
        registerMember("standpoint_type", &contract_actor_base_info::standpoint_type);
        registerMember("location", &contract_actor_base_info::location);
        registerMember("base", &contract_actor_base_info::base);

        //actor core attributes
        registerMember("strength", &contract_actor_core_attributes::strength);
        registerMember("strength_max", &contract_actor_core_attributes::strength_max);
        registerMember("physique", &contract_actor_core_attributes::physique);
        registerMember("physique_max", &contract_actor_core_attributes::physique_max);
        registerMember("agility", &contract_actor_core_attributes::agility);
        registerMember("agility_max", &contract_actor_core_attributes::agility_max);
        registerMember("vitality", &contract_actor_core_attributes::vitality);
        registerMember("vitality_max", &contract_actor_core_attributes::vitality_max);
        registerMember("comprehension", &contract_actor_core_attributes::comprehension);
        registerMember("comprehension_max", &contract_actor_core_attributes::comprehension_max);
        registerMember("willpower", &contract_actor_core_attributes::willpower);
        registerMember("willpower_max", &contract_actor_core_attributes::willpower_max);
        registerMember("charm", &contract_actor_core_attributes::charm);
        registerMember("charm_max", &contract_actor_core_attributes::charm_max);
        registerMember("mood", &contract_actor_core_attributes::mood);
        registerMember("mood_max", &contract_actor_core_attributes::mood_max);
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

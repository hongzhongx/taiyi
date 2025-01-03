#include <lua.hpp>

#include <chain/taiyi_fwd.hpp>
#include <chain/taiyi_objects.hpp>
#include <chain/account_object.hpp>
#include <chain/nfa_objects.hpp>
#include <chain/contract_objects.hpp>
#include <chain/contract_handles.hpp>

#include <chain/database.hpp>
#include <chain/lua_context.hpp>
#include <chain/contract_worker.hpp>

extern "C" {
#include "lundump.h"
#include "lstate.h"
}

namespace taiyi { namespace chain {

    void contract_worker::do_contract_function(const account_object& caller, string function_name, vector<lua_types> value_list, const flat_set<public_key_type> &sigkeys, const contract_object& contract, long long& vm_drops, bool reset_vm_memused, LuaContext& context, database &db)
    { try {

        int pre_drops_enable = lua_enabledrops(context.mState, 1, reset_vm_memused?1:0);
        lua_setdrops(context.mState, vm_drops);
        
        try {
            const auto &baseENV = db.get<contract_bin_code_object, by_id>(0);
            auto abi_itr = contract.contract_ABI.find(lua_types(lua_string(function_name)));
            FC_ASSERT(abi_itr != contract.contract_ABI.end(), "${function_name} maybe a internal function", ("function_name", function_name));
            if(!abi_itr->second.get<lua_function>().is_var_arg)
                FC_ASSERT(value_list.size() == abi_itr->second.get<lua_function>().arglist.size(), "input values count is ${n}, but ${function_name}`s parameter list is ${plist}...", ("n", value_list.size())("function_name", function_name)("plist", abi_itr->second.get<lua_function>().arglist));
            FC_ASSERT(value_list.size()<=20,"value list is greater than 20 limit");
                        
            const auto &contract_owner = db.get<account_object, by_id>(contract.owner).name;
            contract_base_info cbi(db, context, contract_owner, contract.name, caller.name, string(contract.creation_date), string(contract.contract_authority), contract.name);
            contract_handler ch(db, caller, contract, result, context, sigkeys);

            const auto& name = contract.name;
            context.new_sandbox(name, baseENV.lua_code_b.data(), baseENV.lua_code_b.size()); //sandbox
            
            const auto& contract_code = db.get<contract_bin_code_object, by_id>(contract.lua_code_b_id);
            FC_ASSERT(contract_code.lua_code_b.size()>0);
            context.load_script_to_sandbox(name, contract_code.lua_code_b.data(), contract_code.lua_code_b.size());
            context.writeVariable("current_contract", name);
            context.writeVariable(name, "_G", "protected");
            
            context.writeVariable(name, "contract_helper", &ch);
            context.writeVariable(name, "contract_base_info", &cbi);

            bool bOK = context.get_function(name, function_name);
            FC_ASSERT(bOK);
            //push function actual parameters
            for (vector<lua_types>::iterator itr = value_list.begin(); itr != value_list.end(); itr++)
                LuaContext::Pusher<lua_types>::push(context.mState, *itr).release();
            
            int err = lua_pcall(context.mState, value_list.size(), 0, 0);
            lua_types error_message;
            try
            {
                if (err)
                    error_message = LuaContext::readTopAndPop<lua_types>(context.mState, -1);
            }
            catch (...)
            {
                error_message = lua_string(" Unexpected errors ");
            }
            lua_pop(context.mState, -1);
            context.close_sandbox(name);
                        
            if (err)
                FC_THROW("Try the contract resolution execution failure,${message}", ("message", error_message));

            ch.assert_contract_data_size();

            for(auto& temp : result.contract_affecteds)
            {
                if(temp.which() == contract_affected_type::tag<contract_result>::value)
                    result.relevant_datasize += temp.get<contract_result>().relevant_datasize;
            }
            result.relevant_datasize += fc::raw::pack_size(ch.contract_data_cache) + fc::raw::pack_size(ch.account_contract_data_cache) + fc::raw::pack_size(result.contract_affecteds);
        }
        catch (LuaContext::VMcollapseErrorException e)
        {
            vm_drops = lua_getdrops(context.mState);
            lua_enabledrops(context.mState, pre_drops_enable, reset_vm_memused?1:0);
            throw e;
        }
        catch(fc::exception e)
        {
            vm_drops = lua_getdrops(context.mState);
            lua_enabledrops(context.mState, pre_drops_enable, reset_vm_memused?1:0);
            throw e;
        }
        
        vm_drops = lua_getdrops(context.mState);
        lua_enabledrops(context.mState, pre_drops_enable, reset_vm_memused?1:0);

    } FC_CAPTURE_AND_RETHROW() }
    //=============================================================================
    lua_table contract_worker::do_contract_function_return_table(const account_object& caller, string function_name, vector<lua_types> value_list, const flat_set<public_key_type> &sigkeys, const contract_object& contract, long long& vm_drops, bool reset_vm_memused, LuaContext &context, database &db)
    { try {
        
        const auto &baseENV = db.get<contract_bin_code_object, by_id>(0);
        auto abi_itr = contract.contract_ABI.find(lua_types(lua_string(function_name)));
        FC_ASSERT(abi_itr != contract.contract_ABI.end(), "${function_name} maybe a internal function", ("function_name", function_name));
        if(!abi_itr->second.get<lua_function>().is_var_arg)
            FC_ASSERT(value_list.size() == abi_itr->second.get<lua_function>().arglist.size(),
                      "input values count is ${n}, but ${function_name}`s parameter list is ${plist}...",
                      ("n", value_list.size())("function_name", function_name)("plist", abi_itr->second.get<lua_function>().arglist));
        FC_ASSERT(value_list.size()<=20,"value list is greater than 20 limit");
        
        string backup_current_contract_name;
        lua_getglobal(context.mState, "current_contract");
        if( !lua_isnil(context.mState, -1) )
            backup_current_contract_name = string(lua_tostring(context.mState, -1));
        lua_pop(context.mState, 1);
        
        int pre_drops_enable = lua_enabledrops(context.mState, 1, reset_vm_memused?1:0);
        lua_setdrops(context.mState, vm_drops);
        
        const auto &contract_owner = db.get<account_object, by_id>(contract.owner).name;
        contract_base_info cbi(db, context, contract_owner, contract.name, caller.name, string(contract.creation_date), string(contract.contract_authority), contract.name);
        contract_handler ch(db, caller, contract, result, context, sigkeys);

        const auto& name = contract.name;
        context.new_sandbox(name, baseENV.lua_code_b.data(), baseENV.lua_code_b.size()); //sandbox
        
        const auto& contract_code = db.get<contract_bin_code_object, by_id>(contract.lua_code_b_id);
        FC_ASSERT(contract_code.lua_code_b.size()>0);
        context.load_script_to_sandbox(name, contract_code.lua_code_b.data(), contract_code.lua_code_b.size());
        context.writeVariable("current_contract", name);
        context.writeVariable(name, "_G", "protected");
        
        context.writeVariable(name, "contract_helper", &ch);
        context.writeVariable(name, "contract_base_info", &cbi);

        bool bOK = context.get_function(name, function_name);
        FC_ASSERT(bOK);
        //push function actual parameters
        for (vector<lua_types>::iterator itr = value_list.begin(); itr != value_list.end(); itr++)
            LuaContext::Pusher<lua_types>::push(context.mState, *itr).release();
        
        int err = lua_pcall(context.mState, value_list.size(), 1, 0);
        lua_types error_message;
        lua_table result_table;
        try
        {
            if (err)
                error_message = LuaContext::readTopAndPop<lua_types>(context.mState, -1);
            else {
                if(!lua_istable(context.mState, -1))
                    error_message = lua_string(FORMAT_MESSAGE("function \"${f}\" not return a table.", ("f", function_name)));
                else
                    result_table = LuaContext::readTopAndPop<lua_table>(context.mState, -1);
            }
        }
        catch (...)
        {
            error_message = lua_string(" Unexpected errors ");
        }
        lua_pop(context.mState, -1);
        context.close_sandbox(name);
        
        //restore current_contract
        if(backup_current_contract_name != "")
            context.writeVariable("current_contract", backup_current_contract_name);
        
        vm_drops = lua_getdrops(context.mState);
        lua_enabledrops(context.mState, pre_drops_enable, reset_vm_memused?1:0);
        
        if (err)
            FC_THROW("Try the contract resolution execution failure, ${m}", ("m", error_message));

        ch.assert_contract_data_size();

        for(auto& temp : result.contract_affecteds)
        {
            if(temp.which() == contract_affected_type::tag<contract_result>::value)
                result.relevant_datasize += temp.get<contract_result>().relevant_datasize;
        }
        result.relevant_datasize += fc::raw::pack_size(ch.contract_data_cache) + fc::raw::pack_size(ch.account_contract_data_cache) + fc::raw::pack_size(result.contract_affecteds);
        
        return result_table;
            
    } FC_CAPTURE_AND_RETHROW() }
    //=============================================================================
    int compiling_contract_writer(lua_State *L, const void *p, size_t size, void *u)
    {
        vector<char> *s = (vector<char> *)u;
        s->insert(s->end(), (const char *)p, (const char *)p + size);
        return 0;
    }
    
    optional<lua_types> parse_contract_summary(lua_State *L, int index, const lua_map& filter)
    {
        FC_ASSERT(L);
        FC_ASSERT(lua_type(L, index) == LUA_TTABLE);
        lua_table v_table;
        lua_pushnil(L); // nil 入栈作为初始 key
        while (0 != lua_next(L, index))
        {
            try
            {
                auto op_key = lua_string(lua_tostring(L, index + 1));
                if(filter.find(lua_types(op_key)) == filter.end()) {
                    switch (lua_type(L, index + 2))
                    {
                        case LUA_TFUNCTION:
                        {
                            auto sfunc = LuaContext::Reader<FunctionSummary>::read(L, index + 2);
                            if (sfunc)
                                v_table.v[lua_types(op_key)] = *sfunc;
                            break;
                        }
                        case LUA_TTABLE:
                        {
                            auto stable = LuaContext::Reader<lua_table>::read(L, index + 2);
                            if (stable)
                                v_table.v[lua_types(op_key)] = *stable;
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
            FC_CAPTURE_AND_RETHROW()
            lua_pop(L, 1);
        }
        return v_table;
    }

    optional<lua_types> parse_base_contract_summary(lua_State *L, int index)
    {
        FC_ASSERT(L);
        FC_ASSERT(lua_type(L, index) == LUA_TTABLE);
        lua_table v_table;
        lua_pushnil(L); // nil 入栈作为初始 key
        while (0 != lua_next(L, index))
        {
            try
            {
                auto op_key = lua_string(lua_tostring(L, index + 1));
                switch (lua_type(L, index + 2))
                {
                    case LUA_TFUNCTION:
                    {
                        v_table.v[lua_types(op_key)] = lua_bool(true);
                        break;
                    }
                    case LUA_TTABLE:
                    {
                        v_table.v[lua_types(op_key)] = lua_bool(true);
                        break;
                    }
                    default:
                        break;
                }
            }
            FC_CAPTURE_AND_RETHROW()
            lua_pop(L, 1);
        }
        return v_table;
    }
    
    lua_table contract_worker::do_contract(contract_id_type id, const string& name, const string& lua_code, vector<char>& lua_code_b, long long& vm_drops, database &db)
    { try {
        if (id == contract_id_type())
        {
            LuaContext context;
            lua_State *L = context.mState;
            FC_ASSERT(L != nullptr);
                        
            //load code as A function
            int err = luaL_loadbuffer(L, lua_code.data(), lua_code.size(), name.c_str());
            FC_ASSERT(err == 0, "Try the contract resolution compile failure, ${m}", ("m", string(lua_tostring(L, -1))));
            
            //dump bin code
            Proto *f = getproto(L->top - 1);
            lua_lock(L);
            luaU_dump(L, f, compiling_contract_writer, &lua_code_b, 0);
            lua_unlock(L);
            
            //set THE function as baseENV
            lua_setglobal(L, "baseENV");
            
            //check baseENV function must return a table
            lua_getglobal(L, "baseENV");
            err = lua_pcall(L, 0, 1, 0);
            FC_ASSERT(err == 0, "Try the contract resolution compile failure, ${m}", ("m", string(lua_tostring(L, -1))));
            if (!lua_istable(L, -1))
            {
                lua_pushnil(L);
                lua_setglobal(L, "baseENV");
                FC_THROW("Not a legitimate basic contract");
            }

            return parse_base_contract_summary(L, lua_gettop(L))->get<lua_table>();
        }
        else
        {
            LuaContext context;
            
            lua_enabledrops(context.mState, 1, 1);
            lua_setdrops(context.mState, vm_drops);
            
            //check name existence
            lua_getglobal(context.mState, name.c_str());
            FC_ASSERT(lua_isnil(context.mState, -1), "The contract already exists, name:${n}", ("n", name));
            
            //用baseENV初始化合约环境（记为α表）
            const auto &baseENV = db.get<contract_bin_code_object, by_id>(0);
            luaL_loadbuffer(context.mState, baseENV.lua_code_b.data(), baseENV.lua_code_b.size(), "baseENV");
            int err = lua_pcall(context.mState, 0, 1, 0);
            FC_ASSERT(err == 0, "Contract loading infrastructure failure, ${m}", ("m", string(lua_tostring(context.mState, -1))));
            lua_setglobal(context.mState, name.c_str());
            
            const auto &contract = db.get<contract_object, by_id>(id);
            const auto &contract_owner_account = db.get<account_object, by_id>(contract.owner);
            contract_base_info cbi(db, context, contract_owner_account.name, name, contract_owner_account.name, string(contract.creation_date), string(contract.contract_authority), contract.name);
            
            context.writeVariable("current_contract", name);
            context.writeVariable(name, "_G", "protected");
            context.writeVariable(name, "contract_helper", (contract_handler*)0);
            context.writeVariable(name, "nfa_helper", (contract_nfa_handler*)0);
            context.writeVariable(name, "contract_base_info", &cbi);
            
            //load code as A function
            err = luaL_loadbuffer(context.mState, lua_code.data(), lua_code.size(), name.c_str());
            FC_ASSERT(err == 0, "Try the contract resolution compile failure, ${m}", ("m", string(lua_tostring(context.mState, -1))));
            
            //set this empty table as THE function's upvalue
            lua_getglobal(context.mState, name.c_str());
            lua_setupvalue(context.mState, -2, 1);
            
            //dump bin code
            Proto *f = getproto(context.mState->top - 1);
            lua_lock(context.mState);
            luaU_dump(context.mState, f, compiling_contract_writer, &lua_code_b, 0);
            lua_unlock(context.mState);
            
            //run THE function and get all things in upvalue (i.e., α table)
            err = lua_pcall(context.mState, 0, 0, 0);
            FC_ASSERT(err == 0, "Try the contract resolution compile failure, ${m}", ("m", string(lua_tostring(context.mState, -1))));
            
            //get α table's information
            lua_getglobal(context.mState, name.c_str());
            const auto &baseENVContract = db.get<contract_object, by_id>(0);
            auto ret = parse_contract_summary(context.mState, lua_gettop(context.mState), baseENVContract.contract_ABI)->get<lua_table>();
            
            lua_pushnil(context.mState);
            lua_setglobal(context.mState, name.c_str());
            
            vm_drops = lua_getdrops(context.mState);
            return ret;
        }
    } FC_CAPTURE_AND_RETHROW() }
    //=============================================================================
    string contract_worker::eval_nfa_contract_action(const nfa_object& caller_nfa, const string& action, vector<lua_types> value_list, long long& vm_drops, bool reset_vm_memused, LuaContext& context, database &db)
    { try {
        //check existence and consequence type
        const auto* contract_ptr = db.find<chain::contract_object, by_id>(caller_nfa.main_contract);
        if(contract_ptr == nullptr)
            return "NFA main contract not exist";

        auto abi_itr = contract_ptr->contract_ABI.find(lua_types(lua_string(action)));
        if(abi_itr == contract_ptr->contract_ABI.end())
            return FORMAT_MESSAGE("action \"${a}\" is not exist", ("a", action));
        if(abi_itr->second.which() != lua_types::tag<lua_table>::value)
            return FORMAT_MESSAGE("action \"${a}\" defination is invalid", ("a", action));;

        lua_map action_def = abi_itr->second.get<lua_table>().v;
        auto def_itr = action_def.find(lua_types(lua_string("consequence")));
        if(def_itr != action_def.end())
            FC_ASSERT(def_itr->second.get<lua_bool>().v == false, "Can not eval action ${a} in nfa with consequence history. should signing it in transaction.", ("a", action));
        
        //evaluate contract authority
        const auto& caller = db.get<account_object, by_id>(caller_nfa.owner_account);
        if(caller.name != TAIYI_COMMITTEE_ACCOUNT)
            FC_ASSERT(contract_ptr->can_do(db), "The current contract \"${n}\" may have been listed in the forbidden call list", ("n", contract_ptr->name));
                            
        flat_set<public_key_type> sigkeys; //no signature keys
        string function_name = "eval_" + action;
        do_nfa_contract_function(caller_nfa, function_name, value_list, sigkeys, *contract_ptr, vm_drops, reset_vm_memused, context, db);
        
        return "";        
    } FC_CAPTURE_AND_RETHROW() }
    //=============================================================================
    std::string contract_worker::do_nfa_contract_action(const nfa_object& caller_nfa, const string& action, vector<lua_types> value_list, long long& vm_drops, bool reset_vm_memused, LuaContext& context, database &db)
    { try {
        //check existence and consequence type
        const auto* contract_ptr = db.find<chain::contract_object, by_id>(caller_nfa.main_contract);
        if(contract_ptr == nullptr)
            return "NFA main contract not exist";
        
        auto abi_itr = contract_ptr->contract_ABI.find(lua_types(lua_string(action)));
        if(abi_itr == contract_ptr->contract_ABI.end())
            return FORMAT_MESSAGE("action \"${a}\" is not exist", ("a", action));
        if(abi_itr->second.which() != lua_types::tag<lua_table>::value)
            return FORMAT_MESSAGE("action \"${a}\" defination is invalid", ("a", action));;
        
        lua_map action_def = abi_itr->second.get<lua_table>().v;
        auto def_itr = action_def.find(lua_types(lua_string("consequence")));
        FC_ASSERT(def_itr != action_def.end(), "Can not perform action ${a} in nfa without consequence type defined.", ("a", action));
        FC_ASSERT(def_itr->second.get<lua_bool>().v == true, "Can not perform action ${a} in nfa without consequence history. should eval it in api.", ("a", action));

        //evaluate contract authority
        const auto& caller = db.get<account_object, by_id>(caller_nfa.owner_account);
        if(caller.name != TAIYI_COMMITTEE_ACCOUNT)
            FC_ASSERT(contract_ptr->can_do(db), "The current contract \"${n}\" may have been listed in the forbidden call list", ("n", contract_ptr->name));
                            
        flat_set<public_key_type> sigkeys; //no signature keys
        string function_name = "do_" + action;
        do_nfa_contract_function(caller_nfa, function_name, value_list, sigkeys, *contract_ptr, vm_drops, reset_vm_memused, context, db);

        return "";
    } FC_CAPTURE_AND_RETHROW() }
    //=============================================================================
    void contract_worker::do_nfa_contract_function(const nfa_object& caller_nfa, const string& function_name, vector<lua_types> value_list, const flat_set<public_key_type> &sigkeys, const contract_object& contract, long long& vm_drops, bool reset_vm_memused, LuaContext& context, database &db)
    { try {
        int pre_drops_enable = lua_enabledrops(context.mState, 1, reset_vm_memused?1:0);
        lua_setdrops(context.mState, vm_drops);

        try
        {            
            const auto& caller_account = db.get<account_object, by_id>(caller_nfa.owner_account);
            const auto &contract_owner = db.get<account_object, by_id>(contract.owner).name;
            const auto &baseENV = db.get<contract_bin_code_object, by_id>(0);
            
            auto abi_itr = contract.contract_ABI.find(lua_types(lua_string(function_name)));
            FC_ASSERT(abi_itr != contract.contract_ABI.end(), "${f} not found, maybe a internal function", ("f", function_name));
            if(!abi_itr->second.get<lua_function>().is_var_arg)
                FC_ASSERT(value_list.size() == abi_itr->second.get<lua_function>().arglist.size(),
                          "input values count is ${n}, but ${f}`s parameter list is ${p}...",
                          ("n", value_list.size())("f", function_name)("p", abi_itr->second.get<lua_function>().arglist));
            FC_ASSERT(value_list.size() <= 20, "value list is greater than 20 limit");
            
            contract_base_info cbi(db, context, contract_owner, contract.name, caller_account.name, string(contract.creation_date), string(contract.contract_authority), contract.name);
            contract_handler ch(db, caller_account, contract, result, context, sigkeys);
            contract_nfa_handler cnh(caller_account, caller_nfa, context, db, ch);

            const auto& name = contract.name;
            context.new_sandbox(name, baseENV.lua_code_b.data(), baseENV.lua_code_b.size()); //sandbox
            
            const auto& contract_code = db.get<contract_bin_code_object, by_id>(contract.lua_code_b_id);
            FC_ASSERT(contract_code.lua_code_b.size()>0);
            context.load_script_to_sandbox(name, contract_code.lua_code_b.data(), contract_code.lua_code_b.size());
            context.writeVariable("current_contract", name);
            context.writeVariable(name, "_G", "protected");

            context.writeVariable(name, "contract_helper", &ch);
            context.writeVariable(name, "contract_base_info", &cbi);
            context.writeVariable(name, "nfa_helper", &cnh);

            context.get_function(name, function_name);
            //push function actual parameters
            for (vector<lua_types>::iterator itr = value_list.begin(); itr != value_list.end(); itr++)
                LuaContext::Pusher<lua_types>::push(context.mState, *itr).release();
            
            int err = lua_pcall(context.mState, value_list.size(), 0, 0);
            lua_types error_message;
            try
            {
                if (err)
                    error_message = LuaContext::readTopAndPop<lua_types>(context.mState, -1);
            }
            catch (...)
            {
                error_message = lua_string(" Unexpected errors ");
            }
            lua_pop(context.mState, -1);
            context.close_sandbox(name);
            if (err)
                FC_THROW("Try the contract resolution execution failure, ${message}", ("message", error_message));
            
            ch.assert_contract_data_size();
            
            for(auto& temp : result.contract_affecteds)
            {
                if(temp.which() == contract_affected_type::tag<contract_result>::value)
                    result.relevant_datasize += temp.get<contract_result>().relevant_datasize;
            }
            result.relevant_datasize += fc::raw::pack_size(ch.contract_data_cache) + fc::raw::pack_size(ch.account_contract_data_cache) + fc::raw::pack_size(result.contract_affecteds);
        }
        catch (LuaContext::VMcollapseErrorException e)
        {
            vm_drops = lua_getdrops(context.mState);
            lua_enabledrops(context.mState, pre_drops_enable, reset_vm_memused?1:0);
            throw e;
        }
        catch(fc::exception e)
        {
            vm_drops = lua_getdrops(context.mState);
            lua_enabledrops(context.mState, pre_drops_enable, reset_vm_memused?1:0);
            throw e;
        }
        
        vm_drops = lua_getdrops(context.mState);
        lua_enabledrops(context.mState, pre_drops_enable, reset_vm_memused?1:0);
        
    } FC_CAPTURE_AND_RETHROW() }
    //=============================================================================
    protocol::lua_table contract_worker::do_nfa_contract_function_return_table(const nfa_object& caller_nfa, const string& function_name, vector<lua_types> value_list, const flat_set<public_key_type> &sigkeys, const contract_object& contract, long long& vm_drops, bool reset_vm_memused, LuaContext& context, database &db)
    { try {
        lua_table result_table;

        int pre_drops_enable = lua_enabledrops(context.mState, 1, reset_vm_memused?1:0);
        lua_setdrops(context.mState, vm_drops);

        try
        {
            const auto& caller_account = db.get<account_object, by_id>(caller_nfa.owner_account);
            const auto &contract_owner = db.get<account_object, by_id>(contract.owner).name;
            const auto &baseENV = db.get<contract_bin_code_object, by_id>(0);
            
            auto abi_itr = contract.contract_ABI.find(lua_types(lua_string(function_name)));
            FC_ASSERT(abi_itr != contract.contract_ABI.end(), "${f} not found, maybe a internal function", ("f", function_name));
            if(!abi_itr->second.get<lua_function>().is_var_arg)
                FC_ASSERT(value_list.size() == abi_itr->second.get<lua_function>().arglist.size(),
                          "input values count is ${n}, but ${f}`s parameter list is ${p}...",
                          ("n", value_list.size())("f", function_name)("p", abi_itr->second.get<lua_function>().arglist));
            FC_ASSERT(value_list.size() <= 20, "value list is greater than 20 limit");
            
            contract_base_info cbi(db, context, contract_owner, contract.name, caller_account.name, string(contract.creation_date), string(contract.contract_authority), contract.name);
            contract_handler ch(db, caller_account, contract, result, context, sigkeys);
            contract_nfa_handler cnh(caller_account, caller_nfa, context, db, ch);

            const auto& name = contract.name;
            context.new_sandbox(name, baseENV.lua_code_b.data(), baseENV.lua_code_b.size()); //sandbox
            
            const auto& contract_code = db.get<contract_bin_code_object, by_id>(contract.lua_code_b_id);
            FC_ASSERT(contract_code.lua_code_b.size()>0);
            context.load_script_to_sandbox(name, contract_code.lua_code_b.data(), contract_code.lua_code_b.size());
            context.writeVariable("current_contract", name);
            context.writeVariable(name, "_G", "protected");

            context.writeVariable(name, "contract_helper", &ch);
            context.writeVariable(name, "contract_base_info", &cbi);
            context.writeVariable(name, "nfa_helper", &cnh);

            context.get_function(name, function_name);
            //push function actual parameters
            for (vector<lua_types>::iterator itr = value_list.begin(); itr != value_list.end(); itr++)
                LuaContext::Pusher<lua_types>::push(context.mState, *itr).release();
            
            int err = lua_pcall(context.mState, value_list.size(), 1, 0);
            lua_types error_message;
            try
            {
                if (err)
                    error_message = LuaContext::readTopAndPop<lua_types>(context.mState, -1);
                else {
                    if(!lua_istable(context.mState, -1))
                        error_message = lua_string(FORMAT_MESSAGE("function \"${f}\" not return a table.", ("f", function_name)));
                    else
                        result_table = LuaContext::readTopAndPop<lua_table>(context.mState, -1);
                }
            }
            catch (...)
            {
                error_message = lua_string(" Unexpected errors ");
            }
            lua_pop(context.mState, -1);
            context.close_sandbox(name);
            if (err)
                FC_THROW("Try the contract resolution execution failure, ${message}", ("message", error_message));
            
            ch.assert_contract_data_size();
            
            for(auto& temp : result.contract_affecteds)
            {
                if(temp.which() == contract_affected_type::tag<contract_result>::value)
                    result.relevant_datasize += temp.get<contract_result>().relevant_datasize;
            }
            result.relevant_datasize += fc::raw::pack_size(ch.contract_data_cache) + fc::raw::pack_size(ch.account_contract_data_cache) + fc::raw::pack_size(result.contract_affecteds);
        }
        catch (LuaContext::VMcollapseErrorException e)
        {
            vm_drops = lua_getdrops(context.mState);
            lua_enabledrops(context.mState, pre_drops_enable, reset_vm_memused?1:0);
            throw e;
        }
        catch(fc::exception e)
        {
            vm_drops = lua_getdrops(context.mState);
            lua_enabledrops(context.mState, pre_drops_enable, reset_vm_memused?1:0);
            throw e;
        }
        
        vm_drops = lua_getdrops(context.mState);
        lua_enabledrops(context.mState, pre_drops_enable, reset_vm_memused?1:0);

        return result_table;

    } FC_CAPTURE_AND_RETHROW() }

} } // namespace taiyi::chain

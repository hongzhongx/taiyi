#include <chain/taiyi_fwd.hpp>

#include <chain/taiyi_evaluator.hpp>
#include <chain/database.hpp>

#include <chain/taiyi_objects.hpp>
#include <chain/block_summary_object.hpp>
#include <chain/account_object.hpp>
#include <chain/contract_objects.hpp>
#include <chain/nfa_objects.hpp>

#include <chain/lua_context.hpp>

#include <chain/contract_handles.hpp>

namespace taiyi { namespace chain {

    contract_nfa_base_info::contract_nfa_base_info(const nfa_object& nfa, database& db)
        : id(nfa.id), data(nfa.data)
    {
        const auto& nfa_symbol = db.get<nfa_symbol_object, by_id>(nfa.symbol_id);
        symbol = nfa_symbol.symbol;
        
        const auto& owner = db.get<account_object, by_id>(nfa.owner_account);
        owner_account = owner.name;
    }
    //=============================================================================
    string contract_nfa_handler::get_nfa_contract(int64_t nfa_id)
    {
        try
        {
            const nfa_object& nfa = _db.get<nfa_object, by_id>(nfa_id);
            const contract_object& contract = _db.get<chain::contract_object, by_id>(nfa.main_contract);
            return contract.name;
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
            return "";
        }
    }
    //=============================================================================
    contract_nfa_base_info contract_nfa_handler::get_nfa(int64_t nfa_id)
    {
        try
        {
            const nfa_object& nfa = _db.get<nfa_object, by_id>(nfa_id);
            return contract_nfa_base_info(nfa, _db);
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::eval_nfa_action(int64_t nfa_id, const string& action, const lua_map& params)
    {
        LuaContext nfa_context;
        try
        {
            const nfa_object& nfa = _db.get<nfa_object, by_id>(nfa_id);
            
            //check existence and consequence type
            const auto* contract_ptr = _db.find<chain::contract_object, by_id>(nfa.main_contract);
            if(contract_ptr == nullptr)
                return;
            
            auto abi_itr = contract_ptr->contract_ABI.find(lua_types(lua_string(action)));
            if(abi_itr == contract_ptr->contract_ABI.end())
                return;
            if(abi_itr->second.which() != lua_types::tag<lua_table>::value)
                return;
            
            lua_map action_def = abi_itr->second.get<lua_table>().v;
            auto def_itr = action_def.find(lua_types(lua_string("consequence")));
            if(def_itr != action_def.end())
                FC_ASSERT(def_itr->second.get<lua_bool>().v == false, "Can not eval action ${a} in nfa ${nfa} with consequence history. should signing it in transaction.", ("a", action)("nfa", nfa_id));
            
            string function_name = "do_" + action;
            
            //准备参数，默认首个参数me，第二个参数是params
            vector<lua_types> value_list;
            contract_nfa_base_info cnbi(nfa, _db);
            lua_table me_value = cnbi.to_lua_table();
            value_list.push_back(me_value);
            value_list.push_back(lua_table(params));
            
            auto func_abi_itr = contract_ptr->contract_ABI.find(lua_types(lua_string(function_name)));
            FC_ASSERT(func_abi_itr != contract_ptr->contract_ABI.end(), "${f} not found, maybe a internal function", ("f", function_name));
            if(!func_abi_itr->second.get<lua_function>().is_var_arg)
                FC_ASSERT(value_list.size() == func_abi_itr->second.get<lua_function>().arglist.size(),
                          "input values count is ${n}, but ${f}`s parameter list is ${p}...",
                          ("n", value_list.size())("f", function_name)("p", func_abi_itr->second.get<lua_function>().arglist));
            FC_ASSERT(value_list.size() <= 20, "value list is greater than 20 limit");
            
            auto current_contract_name = _context.readVariable<string>("current_contract");
            auto current_cbi = _context.readVariable<contract_base_info*>(current_contract_name, "contract_base_info");
            auto current_ch = _context.readVariable<contract_handler*>(current_contract_name, "chainhelper");
            const auto &baseENV = _db.get<contract_bin_code_object, by_id>(0);
            
            const auto& nfa_contract = *contract_ptr;
            const auto& nfa_contract_owner = _db.get<account_object, by_id>(nfa_contract.owner).name;
            const auto& nfa_contract_code = _db.get<contract_bin_code_object, by_id>(nfa_contract.lua_code_b_id);
            
            contract_base_info cbi(_db, nfa_context, nfa_contract_owner, nfa_contract.name, current_cbi->caller, string(nfa_contract.creation_date), string(nfa_contract.contract_authority), current_cbi->invoker_contract_name);
            contract_handler ch(_db, current_ch->caller, nfa_contract, current_ch->result, nfa_context, current_ch->sigkeys, current_ch->apply_result, current_ch->account_conntract_data);
            contract_nfa_handler cnh(current_ch->caller, nfa_context, _db);
            
            const auto& name = nfa_contract.name;
            nfa_context.new_sandbox(name, baseENV.lua_code_b.data(), baseENV.lua_code_b.size()); //sandbox
            
            FC_ASSERT(nfa_contract_code.lua_code_b.size()>0);
            nfa_context.load_script_to_sandbox(name, nfa_contract_code.lua_code_b.data(), nfa_contract_code.lua_code_b.size());
            
            nfa_context.writeVariable("current_contract", name);
            nfa_context.writeVariable(name, "_G", "protected");
            nfa_context.writeVariable(name, "chainhelper", &ch);
            nfa_context.writeVariable(name, "nfahelper", &cnh);
            nfa_context.writeVariable(name, "contract_base_info", &cbi);
            nfa_context.writeVariable(name, "private_data", LuaContext::EmptyArray /*,account_data*/);
            nfa_context.writeVariable(name, "public_data", LuaContext::EmptyArray /*,contract_data*/);
            nfa_context.writeVariable(name, "read_list", "private_data", LuaContext::EmptyArray);
            nfa_context.writeVariable(name, "read_list", "public_data", LuaContext::EmptyArray);
            nfa_context.writeVariable(name, "write_list", "private_data", LuaContext::EmptyArray);
            nfa_context.writeVariable(name, "write_list", "public_data", LuaContext::EmptyArray);
            
            nfa_context.get_function(name, function_name);
            //push function actual parameters
            for (vector<lua_types>::iterator itr = value_list.begin(); itr != value_list.end(); itr++)
                LuaContext::Pusher<lua_types>::push(nfa_context.mState, *itr).release();
            
            int err = lua_pcall(nfa_context.mState, value_list.size(), 0, 0);
            lua_types error_message;
            try
            {
                if (err)
                    error_message = LuaContext::readTopAndPop<lua_types>(nfa_context.mState, -1);
            }
            catch (...)
            {
                error_message = lua_string(" Unexpected errors ");
            }
            lua_pop(nfa_context.mState, -1);
            nfa_context.close_sandbox(name);
            if (err)
                FC_THROW("Try the contract resolution execution failure, ${message}", ("message", error_message));
            
            for(auto& temp : ch.result.contract_affecteds) {
                if(temp.which() == contract_affected_type::tag<contract_result>::value)
                    ch.result.relevant_datasize += temp.get<contract_result>().relevant_datasize;
            }
            ch.result.relevant_datasize += fc::raw::pack_size(nfa_contract.contract_data) + fc::raw::pack_size(ch.account_conntract_data) + fc::raw::pack_size(ch.result.contract_affecteds);
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::do_nfa_action(int64_t nfa_id, const string& action, const lua_map& params)
    {
        LuaContext nfa_context;
        try
        {
            const nfa_object& nfa = _db.get<nfa_object, by_id>(nfa_id);
            
            //check existence and consequence type
            const auto* contract_ptr = _db.find<chain::contract_object, by_id>(nfa.main_contract);
            if(contract_ptr == nullptr)
                return;
            
            auto abi_itr = contract_ptr->contract_ABI.find(lua_types(lua_string(action)));
            if(abi_itr == contract_ptr->contract_ABI.end())
                return;
            if(abi_itr->second.which() != lua_types::tag<lua_table>::value)
                return;
            
            lua_map action_def = abi_itr->second.get<lua_table>().v;
            auto def_itr = action_def.find(lua_types(lua_string("consequence")));
            if(def_itr != action_def.end())
                FC_ASSERT(def_itr->second.get<lua_bool>().v == true, "Can not perform action ${a} in nfa ${nfa} without consequence history. should eval it in api.", ("a", action)("nfa", nfa_id));
            
            string function_name = "do_" + action;
            
            //准备参数，默认首个参数me，第二个参数是params
            vector<lua_types> value_list;
            contract_nfa_base_info cnbi(nfa, _db);
            lua_table me_value = cnbi.to_lua_table();
            value_list.push_back(me_value);
            value_list.push_back(lua_table(params));
            
            auto func_abi_itr = contract_ptr->contract_ABI.find(lua_types(lua_string(function_name)));
            FC_ASSERT(func_abi_itr != contract_ptr->contract_ABI.end(), "${f} not found, maybe a internal function", ("f", function_name));
            if(!func_abi_itr->second.get<lua_function>().is_var_arg)
                FC_ASSERT(value_list.size() == func_abi_itr->second.get<lua_function>().arglist.size(),
                          "input values count is ${n}, but ${f}`s parameter list is ${p}...",
                          ("n", value_list.size())("f", function_name)("p", func_abi_itr->second.get<lua_function>().arglist));
            FC_ASSERT(value_list.size() <= 20, "value list is greater than 20 limit");
            
            auto current_contract_name = _context.readVariable<string>("current_contract");
            auto current_cbi = _context.readVariable<contract_base_info*>(current_contract_name, "contract_base_info");
            auto current_ch = _context.readVariable<contract_handler*>(current_contract_name, "chainhelper");
            const auto &baseENV = _db.get<contract_bin_code_object, by_id>(0);
            
            const auto& nfa_contract = *contract_ptr;
            const auto& nfa_contract_owner = _db.get<account_object, by_id>(nfa_contract.owner).name;
            const auto& nfa_contract_code = _db.get<contract_bin_code_object, by_id>(nfa_contract.lua_code_b_id);
            
            contract_base_info cbi(_db, nfa_context, nfa_contract_owner, nfa_contract.name, current_cbi->caller, string(nfa_contract.creation_date), string(nfa_contract.contract_authority), current_cbi->invoker_contract_name);
            contract_handler ch(_db, current_ch->caller, nfa_contract, current_ch->result, nfa_context, current_ch->sigkeys, current_ch->apply_result, current_ch->account_conntract_data);
            contract_nfa_handler cnh(current_ch->caller, nfa_context, _db);
            
            const auto& name = nfa_contract.name;
            nfa_context.new_sandbox(name, baseENV.lua_code_b.data(), baseENV.lua_code_b.size()); //sandbox
            
            FC_ASSERT(nfa_contract_code.lua_code_b.size()>0);
            nfa_context.load_script_to_sandbox(name, nfa_contract_code.lua_code_b.data(), nfa_contract_code.lua_code_b.size());
            
            nfa_context.writeVariable("current_contract", name);
            nfa_context.writeVariable(name, "_G", "protected");
            nfa_context.writeVariable(name, "chainhelper", &ch);
            nfa_context.writeVariable(name, "nfahelper", &cnh);
            nfa_context.writeVariable(name, "contract_base_info", &cbi);
            nfa_context.writeVariable(name, "private_data", LuaContext::EmptyArray /*,account_data*/);
            nfa_context.writeVariable(name, "public_data", LuaContext::EmptyArray /*,contract_data*/);
            nfa_context.writeVariable(name, "read_list", "private_data", LuaContext::EmptyArray);
            nfa_context.writeVariable(name, "read_list", "public_data", LuaContext::EmptyArray);
            nfa_context.writeVariable(name, "write_list", "private_data", LuaContext::EmptyArray);
            nfa_context.writeVariable(name, "write_list", "public_data", LuaContext::EmptyArray);
            
            nfa_context.get_function(name, function_name);
            //push function actual parameters
            for (vector<lua_types>::iterator itr = value_list.begin(); itr != value_list.end(); itr++)
                LuaContext::Pusher<lua_types>::push(nfa_context.mState, *itr).release();
            
            int err = lua_pcall(nfa_context.mState, value_list.size(), 0, 0);
            lua_types error_message;
            try
            {
                if (err)
                    error_message = LuaContext::readTopAndPop<lua_types>(nfa_context.mState, -1);
            }
            catch (...)
            {
                error_message = lua_string(" Unexpected errors ");
            }
            lua_pop(nfa_context.mState, -1);
            nfa_context.close_sandbox(name);
            if (err)
                FC_THROW("Try the contract resolution execution failure, ${message}", ("message", error_message));
            
            for(auto& temp : ch.result.contract_affecteds) {
                if(temp.which() == contract_affected_type::tag<contract_result>::value)
                    ch.result.relevant_datasize += temp.get<contract_result>().relevant_datasize;
            }
            ch.result.relevant_datasize += fc::raw::pack_size(nfa_contract.contract_data) + fc::raw::pack_size(ch.account_conntract_data) + fc::raw::pack_size(ch.result.contract_affecteds);
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::enable_tick(int64_t nfa_id)
    {
        try
        {
            const nfa_object& nfa = _db.get<nfa_object, by_id>(nfa_id);
            FC_ASSERT(nfa.owner_account == _caller.id, "can not enable_tick on nfa (${nfa}) not owned by caller.", ("nfa", nfa_id));
            _db.modify(nfa, [&]( nfa_object& obj ) {
                obj.next_tick_time = time_point_sec::min();
            });
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }

} } // namespace taiyi::chain

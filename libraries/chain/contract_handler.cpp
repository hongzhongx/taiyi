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

    bool contract_handler::is_owner()
    {
        return caller.id == contract.owner;
    }
    //=============================================================================
    void contract_handler::set_permissions_flag(bool flag)
    {
        try
        {
            FC_ASSERT(is_owner(), "You`re not the contract`s owner");
            db.modify(db.get<contract_object, by_id>(contract.id), [&](contract_object &co) {
                co.check_contract_authority = flag;
            });
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(this->context.mState,e.to_string());
        }
    };
    //=============================================================================
    void contract_handler::set_invoke_share_percent(uint32_t percent)
    {
        try
        {
            FC_ASSERT(percent <= 100, "percent should be in range 0-100 ");
            FC_ASSERT(is_owner(), "You`re not the contract`s owner");
            db.modify(db.get<contract_object, by_id>(contract.id), [&](contract_object &co) {
                co.user_invoke_share_percent = percent;
            });
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(this->context.mState,e.to_string());
        }
    };
    //=============================================================================
    void contract_handler::change_contract_authority(string authority)
    {
        try
        {
            FC_ASSERT(is_owner(), "You`re not the contract`s owner");
            auto new_authority= public_key_type(authority);
            db.modify(db.get<contract_object, by_id>(contract.id), [&](contract_object &co) {
                co.contract_authority = new_authority;
            });
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(this->context.mState,e.to_string());
        }
    }
    //=============================================================================
    const contract_object& contract_base_info::get_contract(string name)
    {
        try
        {
            const auto* contract = db.find<contract_object, by_name>(name);
            FC_ASSERT(contract != nullptr, "not find contract: ${contract}", ("contract", name));
            return *contract;
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    int contract_handler::contract_random()
    {
        try
        {
            result.existed_pv = true;
            auto hblock_id = db.head_block_id();
            return (int)(protocol::hasher::hash( hblock_id._hash[4] ));
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
            return 0;
        }
    }
    //=============================================================================
    memo_data contract_handler::make_memo(string receiver_id_or_name, string key, string value, uint64_t ss, bool enable_logger)
    {
        auto &receiver = get_account(receiver_id_or_name);
        try
        {
            //Get_random_private_key();
            fc::ecc::private_key rand_key = fc::ecc::private_key::regenerate(fc::sha256::hash(key + std::to_string(ss)));
            
            memo_data md;
            md.set_message(rand_key, receiver.memo_key, value, ss);
            
            if (enable_logger)
            {
                contract_memo_message cms;
                cms.affected_account = receiver.name;
                cms.memo = md;
                result.contract_affecteds.push_back(std::move(cms));
            }
            
            return md;
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
            return memo_data();
        }
    }
    //=============================================================================
    void contract_handler::invoke_contract_function(string contract_id_or_name, string function_name, string value_list_json)
    {
        auto &contract_obj = get_contract(contract_id_or_name);
        auto contract_id = contract_obj.id;
        static vector<string> s_invoking_path;
        try
        {
            if (std::find(s_invoking_path.begin(), s_invoking_path.end(), contract_obj.name) != s_invoking_path.end()) {
                s_invoking_path.clear();
                FC_THROW("Contract cicular references are prohibitted in invoking");
            }
            else {
                s_invoking_path.push_back(contract_obj.name);
            }
            
            FC_ASSERT(contract_id != this->contract.id, " You can't use it to make recursive calls. ");
            auto value_list = fc::json::from_string(value_list_json).as<vector<lua_types>>();
            contract_result _contract_result = contract_result();
            auto backup_current_contract_name = context.readVariable<string>("current_contract");
            auto ocr = optional<contract_result>(_contract_result);
            
            call_contract_function_operation op;
            op.caller = caller.name;
            op.contract_name = this->contract.name;
            op.function_name = function_name;
            op.value_list = value_list;
            call_contract_function_evaluator evaluator(db);
            evaluator.apply(op);
            context.writeVariable("current_contract", backup_current_contract_name);
            
            s_invoking_path.pop_back();
        }
        catch (fc::exception e)
        {
            s_invoking_path.clear();
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_handler::log(string message)
    {
        try
        {
            wlog(message);
            contract_logger logger(caller.name);
            logger.message = message;
            result.contract_affecteds.push_back(std::move(logger));
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
    }
    //=============================================================================
    lua_Number contract_handler::nummax()
    {
        return std::numeric_limits<lua_Number>::max();
    }
    //=============================================================================
    lua_Number contract_handler::nummin()
    {
        return std::numeric_limits<lua_Number>::min();
    }
    //=============================================================================
    int64_t contract_handler::integermax()
    {
        return LUA_MAXINTEGER;
    }
    //=============================================================================
    int64_t contract_handler::integermin()
    {
        return LUA_MININTEGER;
    }
    //=============================================================================
    uint32_t contract_handler::head_block_time()
    {
        return db.head_block_time().sec_since_epoch();
    }
    //=============================================================================
    string contract_handler::hash256(string source)
    {
        return fc::sha256::hash(source).str();
    }
    //=============================================================================
    string contract_handler::hash512(string source)
    {
        return fc::sha512::hash(source).str();
    }
    //=============================================================================
    const contract_object& contract_handler::get_contract(string name)
    {
        try
        {
            const auto* contract = db.find<contract_object, by_name>(name);
            FC_ASSERT(contract != nullptr, "not find contract: ${contract}", ("contract", name));
            return *contract;
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_handler::make_release()
    {
        db.modify(contract, [](contract_object& cb) { cb.is_release = true; });
    }
    //=============================================================================
    lua_map contract_handler::get_contract_public_data(string name_or_id)
    {
        try
        {
            optional<contract_object> contract = get_contract(name_or_id);
            return contract->contract_data;
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
    }
    //=============================================================================
    const account_object &contract_handler::get_account(string name)
    {
        try
        {
            return db.get_account(name);
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
            std::terminate();
        }
    }
    //=============================================================================
    void contract_handler::transfer_by_contract(account_id_type from, account_id_type to, asset token, contract_result &result, bool enable_logger)
    {
        try
        {
            FC_ASSERT(from != to, "It's no use transferring money to yourself");
            const account_object &from_account = db.get<account_object, by_id>(from);
            const account_object &to_account = db.get<account_object, by_id>(to);
            try
            {
                FC_ASSERT(db.get_balance(from_account, token.symbol).amount >= token.amount,
                          "Insufficient Balance: ${balance}, unable to transfer '${total_transfer}' from account '${a}' to '${t}'",
                          ("a", from_account.name)("t", to_account.name)("total_transfer", token)("balance", db.get_balance(from_account, token.symbol)));
            }
            FC_RETHROW_EXCEPTIONS(error, "Unable to transfer ${a} from ${f} to ${t}", ("a", token)("f", from_account.name)("t", to_account.name));
            
            FC_ASSERT(token.amount >= share_type(0), "token amount must big than zero");
            db.adjust_balance(from_account, -token);
            db.adjust_balance(to_account, token);
            
            if(enable_logger) {
                variant v;
                fc::to_variant(token, v);
                log(from_account.name + " transfer " + fc::json::to_string(v) + " to " + to_account.name);
            }
        }
        catch (fc::exception e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
    }
    //=============================================================================
    int64_t contract_handler::get_account_balance(account_id_type account, asset_symbol_type symbol)
    {
        try
        {
            const auto& account_obj = db.get<account_object, by_id>(account);
            return db.get_balance(account_obj, symbol).amount.value;
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
            return 0;
        }
    }
    //=============================================================================
    asset_symbol_type s_get_symbol_type_from_string(const string name)
    {
        if(name.size() == (TAIYI_ASSET_SYMBOL_NAI_STRING_LENGTH - 1) && name[0] == '@' && name[1] == '@') {
            //NAI string
            return asset_symbol_type::from_nai_string(name.c_str(), 3); //自定义创建的资产仅支持3位精度
        }
        else {
            //legacy symbol
            if(name == "YANG")
                return YANG_SYMBOL;
            else if(name == "QI")
                return QI_SYMBOL;
            else if(name == "GOLD")
                return GOLD_SYMBOL;
            else if(name == "FOOD")
                return FOOD_SYMBOL;
            else if(name == "WOOD")
                return WOOD_SYMBOL;
            else if(name == "FABR")
                return FABRIC_SYMBOL;
            else if(name == "HERB")
                return HERB_SYMBOL;
            else {
                FC_ASSERT(false, "invalid asset symbol name string '${n}'", ("n", name));
                return asset_symbol_type();
            }
        }
    }

    void contract_handler::transfer_from(account_id_type from, account_name_type to, double amount, string symbol_name, bool enable_logger)
    {
        try
        {
            account_id_type account_to = db.get_account(to).id;
            asset_symbol_type symbol = s_get_symbol_type_from_string(symbol_name);
            auto token = asset(amount, symbol);
            transfer_by_contract(from, account_to, token, result, enable_logger);
        }
        catch (fc::exception e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
    }
    //=============================================================================
    string contract_handler::get_nfa_contract(int64_t nfa_id)
    {
        try
        {
            const nfa_object& nfa = db.get<nfa_object, by_id>(nfa_id);
            const contract_object& contract = db.get<chain::contract_object, by_id>(nfa.main_contract);
            return contract.name;
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
            return "";
        }
    }
    //=============================================================================
    contract_nfa_base_info contract_handler::get_nfa_info(int64_t nfa_id)
    {
        try
        {
            const nfa_object& nfa = db.get<nfa_object, by_id>(nfa_id);
            return contract_nfa_base_info(nfa, db);
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_handler::eval_nfa_action(int64_t nfa_id, const string& action, const lua_map& params)
    {
        //TODO: eval产生的消耗
        LuaContext nfa_context;
        try
        {
            const nfa_object& nfa = db.get<nfa_object, by_id>(nfa_id);
            
            //check existence and consequence type
            const auto* contract_ptr = db.find<chain::contract_object, by_id>(nfa.main_contract);
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
            contract_nfa_base_info cnbi(nfa, db);
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
            
            auto current_contract_name = context.readVariable<string>("current_contract");
            auto current_cbi = context.readVariable<contract_base_info*>(current_contract_name, "contract_base_info");
            auto current_ch = context.readVariable<contract_handler*>(current_contract_name, "contract_helper");
            const auto &baseENV = db.get<contract_bin_code_object, by_id>(0);
            
            const auto& nfa_contract = *contract_ptr;
            const auto& nfa_contract_owner = db.get<account_object, by_id>(nfa_contract.owner).name;
            const auto& nfa_contract_code = db.get<contract_bin_code_object, by_id>(nfa_contract.lua_code_b_id);
            
            contract_base_info cbi(db, nfa_context, nfa_contract_owner, nfa_contract.name, current_cbi->caller, string(nfa_contract.creation_date), string(nfa_contract.contract_authority), current_cbi->invoker_contract_name);
            contract_handler ch(db, current_ch->caller, nfa_contract, current_ch->result, nfa_context, current_ch->sigkeys, current_ch->apply_result, current_ch->account_conntract_data);
            contract_nfa_handler cnh(nfa, nfa_context, db);
            
            const auto& name = nfa_contract.name;
            nfa_context.new_sandbox(name, baseENV.lua_code_b.data(), baseENV.lua_code_b.size()); //sandbox
            
            FC_ASSERT(nfa_contract_code.lua_code_b.size()>0);
            nfa_context.load_script_to_sandbox(name, nfa_contract_code.lua_code_b.data(), nfa_contract_code.lua_code_b.size());
            
            nfa_context.writeVariable("current_contract", name);
            nfa_context.writeVariable(name, "_G", "protected");
            nfa_context.writeVariable(name, "contract_helper", &ch);
            nfa_context.writeVariable(name, "nfa_helper", &cnh);
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
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_handler::do_nfa_action(int64_t nfa_id, const string& action, const lua_map& params)
    {
        //TODO: do的权限以及产生的消耗
        LuaContext nfa_context;
        try
        {
            const nfa_object& nfa = db.get<nfa_object, by_id>(nfa_id);
            
            //check existence and consequence type
            const auto* contract_ptr = db.find<chain::contract_object, by_id>(nfa.main_contract);
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
            contract_nfa_base_info cnbi(nfa, db);
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
            
            auto current_contract_name = context.readVariable<string>("current_contract");
            auto current_cbi = context.readVariable<contract_base_info*>(current_contract_name, "contract_base_info");
            auto current_ch = context.readVariable<contract_handler*>(current_contract_name, "contract_helper");
            const auto &baseENV = db.get<contract_bin_code_object, by_id>(0);
            
            const auto& nfa_contract = *contract_ptr;
            const auto& nfa_contract_owner = db.get<account_object, by_id>(nfa_contract.owner).name;
            const auto& nfa_contract_code = db.get<contract_bin_code_object, by_id>(nfa_contract.lua_code_b_id);
            
            contract_base_info cbi(db, nfa_context, nfa_contract_owner, nfa_contract.name, current_cbi->caller, string(nfa_contract.creation_date), string(nfa_contract.contract_authority), current_cbi->invoker_contract_name);
            contract_handler ch(db, current_ch->caller, nfa_contract, current_ch->result, nfa_context, current_ch->sigkeys, current_ch->apply_result, current_ch->account_conntract_data);
            contract_nfa_handler cnh(nfa, nfa_context, db);
            
            const auto& name = nfa_contract.name;
            nfa_context.new_sandbox(name, baseENV.lua_code_b.data(), baseENV.lua_code_b.size()); //sandbox
            
            FC_ASSERT(nfa_contract_code.lua_code_b.size()>0);
            nfa_context.load_script_to_sandbox(name, nfa_contract_code.lua_code_b.data(), nfa_contract_code.lua_code_b.size());
            
            nfa_context.writeVariable("current_contract", name);
            nfa_context.writeVariable(name, "_G", "protected");
            nfa_context.writeVariable(name, "contract_helper", &ch);
            nfa_context.writeVariable(name, "nfa_helper", &cnh);
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
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    //find_luaContext:按深度查找合约树形结构上下文
    //context:树形结构合约相关上下文
    //keys:目标路径(树形结构的某一条分支)
    //start:指定查找路径的起点
    //is_clean:是否清理目标
    std::pair<bool, lua_types *> contract_handler::find_luaContext(lua_map *context, vector<lua_types> keys, int start, bool is_clean)
    {
        lua_types *result = nullptr;
        vector<lua_key> temp_keys;
        lua_types *temp = nullptr;
        for (unsigned int i = start; i < keys.size(); i++)
        {
            auto reslut_itr = context->find(keys[i]);
            if (reslut_itr != context->end())
            {
                temp_keys.push_back(keys[i]);
                result = &reslut_itr->second;
                if (i == keys.size() - 1)
                {
                    if (is_clean)
                    {
                        context->erase(reslut_itr);        //找到目标并清理目标
                        return std::make_pair(true, temp); //返回上一级树形结构节点指针，temp=nullptr时表示路径查找从start处失败
                    }
                    else
                        return std::make_pair(true, result); //返回目标节点指针
                }
                if (reslut_itr->second.which() == lua_types::tag<lua_table>::value)
                {
                    temp = &reslut_itr->second;
                    context = &reslut_itr->second.get<lua_table>().v; //深度递进
                }
                else
                    return std::make_pair(false, result); //原目标不是table对象，返回上级table地址
            }
            else
                return std::make_pair(false, result); //路径查找keys[i]失败，返回上一级节点指针，result=nullptr时表示路径查找从start处失败
        }
        return std::make_pair(false, result); //start值超出keys深度，未进行查找
    }
    //=============================================================================
    void contract_handler::read_cache()
    {
        try
        {
            auto keys = LuaContext::read_lua_value<lua_table>(this->context, contract.name.data(), vector<string>{"read_list"});
            for (auto &itr : keys.v)
            {
                if (itr.first.key.which() == lua_key_variant::tag<lua_string>::value && itr.first.key.get<lua_string>().v == "private_data")
                {
                    vector<lua_types> stacks = {lua_string("private_data")};
                    read_context(itr.second.get<lua_table>().v, this->account_conntract_data, stacks, contract.name);
                }
                if (itr.first.key.which() == lua_key_variant::tag<lua_string>::value && itr.first.key.get<lua_string>().v == "public_data")
                {
                    vector<lua_types> stacks = {lua_string("public_data")};
                    lua_map contract_data = this->contract.contract_data;
                    read_context(itr.second.get<lua_table>().v, contract_data, stacks, contract.name);
                }
            }
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
        catch (std::exception e)
        {
            LUA_C_ERR_THROW(this->context.mState, e.what());
        }
    }
    //=============================================================================
    void contract_handler::fllush_cache()
    {
        try
        {
            auto keys = LuaContext::read_lua_value<lua_table>(this->context, contract.name.data(), vector<string>{"write_list"});
            for (auto &itr : keys.v)
            {
                if (itr.first.key.which() == lua_key_variant::tag<lua_string>::value && itr.first.key.get<lua_string>().v == "private_data")
                {
                    vector<lua_types> stacks = {lua_string("private_data")};
                    fllush_context(itr.second.get<lua_table>().v, this->account_conntract_data, stacks, contract.name);
                }
                if (itr.first.key.which() == lua_key_variant::tag<lua_string>::value && itr.first.key.get<lua_string>().v == "public_data")
                {
                    vector<lua_types> stacks = {lua_string("public_data")};
                    lua_map contract_data;
                    fllush_context(itr.second.get<lua_table>().v, contract_data, stacks, contract.name);
                    db.modify(contract, [&](contract_object& c) { c.contract_data = contract_data; });
                }
            }
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
        catch (std::exception e)
        {
            LUA_C_ERR_THROW(this->context.mState, e.what());
        }
    }
    /******************************************************************************
     *read_context:按指定的树型表推送对应的合约上下文
     *keys:指定的需要推送的树形结构路径图
     *data_table:树形结构合约上下文数据源
     *stacks:将keys树型路径图分割为单一路径的路径缓存栈
     *tablename:指定VM指定的环境目标
     ************************************************************************************************************************/
    void contract_handler::read_context(lua_map &keys, lua_map &data_table, vector<lua_types> &stacks, string tablename)
    {
        static auto start_key = lua_types(lua_string("start"));
        static auto stop_key = lua_types(lua_string("stop"));
        uint32_t start = 0, stop = 0, index = 0;
        if (keys.find(start_key) != keys.end() && keys[start_key].which() == lua_types::tag<lua_int>::value)
        {
            start = keys[start_key].get<lua_int>().v;
            keys.erase(start_key);
        }
        if (keys.find(stop_key) != keys.end() && keys[stop_key].which() == lua_types::tag<lua_int>::value)
        {
            stop = keys[stop_key].get<lua_int>().v;
            keys.erase(stop_key);
        }
        if (start || stop)
        {
            lua_map *itr_prt = nullptr;
            if (stacks.size() == 1)
                itr_prt = &data_table;
            else
            {
                auto finded = find_luaContext(&data_table, stacks, 1);
                if (finded.first && finded.second->which() == lua_types::tag<lua_table>::value)
                    itr_prt = &finded.second->get<lua_table>().v;
            }
            if (itr_prt)
                for (auto &itr_index : *itr_prt)
                {
                    if (index >= start && stop > index)
                    {
                        stacks.push_back(itr_index.first.cast_to_lua_types());
                        stacks.push_back(itr_index.second);
                        LuaContext::push_key_and_value_stack(context, lua_string(tablename), stacks);
                        stacks.pop_back();
                        stacks.pop_back();
                    }
                    if (stop <= index)
                        break;
                    index++;
                }
        }
        for (auto itr = keys.begin(); itr != keys.end(); itr++)
        {
            if (itr->second.which() != lua_types::tag<lua_table>::value || itr->second.get<lua_table>().v.size() == 0)
            {
                lua_key key = itr->first;
                stacks.push_back(key.cast_to_lua_types());
                auto finded = find_luaContext(&data_table, stacks, 1);
                if (finded.first)
                {
                    stacks.push_back(*finded.second);
                    LuaContext::push_key_and_value_stack(context, lua_string(tablename), stacks);
                    stacks.pop_back();
                }
                stacks.pop_back();
            }
            else if (itr->second.which() == lua_types::tag<lua_table>::value && itr->second.get<lua_table>().v.size() > 0)
            {
                stacks.push_back(itr->first.cast_to_lua_types());
                read_context(itr->second.get<lua_table>().v, data_table, stacks, tablename);
                stacks.pop_back();
            }
        }
        if (keys.size() == 0 && stacks.size() == 1 && !(start || stop))
        {
            LuaContext::push_key_and_value_stack(context, lua_string(tablename), vector<lua_types>{stacks[0], lua_table(data_table)});
        }
    }
    /******************************************************************************
     *fllush_context:按指定的树型表更新链上对应的合约上下文
     *keys:指定的需要推送的树形结构路径图
     *data_table:树形结构合约上下文数据源
     *stacks:将keys树型路径图分割为单一路径的路径缓存栈
     *tablename:指定VM指定的环境目标
     ************************************************************************************************************************/
    void contract_handler::fllush_context(const lua_map &keys, lua_map &data_table, vector<lua_types> &stacks, string table_name)
    {
        for (auto itr = keys.begin(); itr != keys.end(); itr++)
        {
            if (itr->second.which() != lua_types::tag<lua_table>::value || itr->second.get<lua_table>().v.size() == 0)
            {
                lua_key key = itr->first;
                bool flag = (itr->second.which() == lua_types::tag<lua_bool>::value) ? itr->second.get<lua_bool>().v : true; //默认不清理对应的上下文
                stacks.push_back(key.cast_to_lua_types());
                if (!flag)
                {
                    find_luaContext(&data_table, stacks, 1, true);
                }
                else
                {
                    auto finded = find_luaContext(&data_table, stacks, 1);
                    if (finded.first)
                    {
                        *finded.second = LuaContext::read_lua_value<lua_types>(context, table_name.data(), stacks);
                    }
                    else if (finded.second)
                    {
                        lua_table &temp_table = finded.second->get<lua_table>();
                        temp_table.v[key] = LuaContext::read_lua_value<lua_types>(context, table_name.data(), stacks);
                    }
                    else
                    {
                        data_table[key] = LuaContext::read_lua_value<lua_types>(context, table_name.data(), vector<lua_types>{stacks[0], stacks[1]});
                    }
                }
                stacks.pop_back();
            }
            else if (itr->second.which() == lua_types::tag<lua_table>::value && itr->second.get<lua_table>().v.size() > 0)
            {
                lua_key key = itr->first;
                stacks.push_back(key.cast_to_lua_types());
                auto finded = find_luaContext(&data_table, stacks, 1);
                if (finded.first)
                {
                    fllush_context(itr->second.get<lua_table>().v, data_table, stacks, table_name);
                }
                else if (finded.second)
                {
                    lua_table &temp_table = finded.second->get<lua_table>();
                    temp_table.v[key] = LuaContext::read_lua_value<lua_types>(context, table_name.data(), stacks);
                }
                else
                {
                    data_table[key] = LuaContext::read_lua_value<lua_types>(context, table_name.data(), vector<lua_types>{stacks[0], stacks[1]});
                }
                stacks.pop_back();
            }
        }
        if (keys.size() == 0)
        {
            data_table = LuaContext::read_lua_value<lua_table>(context, table_name.data(), vector<lua_types>{stacks[0]}).v;
        }
    }
    /******************************************************************************
     *filter_context:按指定的树型表筛选对应的合约上下文
     *data_table:树形结构合约上下文数据源
     *keys:指定的需要推送的树形结构路径图
     *stacks:将keys树型路径图分割为单一路径的路径缓存栈
     *result:存储筛选结果
     ************************************************************************************************************************/
    void contract_handler::filter_context(const lua_map &data_table,  lua_map keys, vector<lua_types> &stacks, lua_map *result)
    {
        static auto start_key = lua_types(lua_string("start"));
        static auto stop_key = lua_types(lua_string("stop"));
        uint32_t start = 0, stop = 0, index = 0;
        if (keys.find(start_key) != keys.end() && keys[start_key].which() == lua_types::tag<lua_int>::value)
        {
            start = keys[start_key].get<lua_int>().v;
            keys.erase(start_key);
        }
        if (keys.find(stop_key) != keys.end() && keys[stop_key].which() == lua_types::tag<lua_int>::value)
        {
            stop = keys[stop_key].get<lua_int>().v;
            keys.erase(stop_key);
        }
        if (start || stop)
        {
            const lua_map *itr_prt = nullptr;
            if (stacks.size() == 0)
                itr_prt = &data_table;
            else
            {
                auto finded = find_luaContext(const_cast< lua_map* >(&data_table), stacks);
                if (finded.first && finded.second->which() == lua_types::tag<lua_table>::value)
                    itr_prt = &finded.second->get<lua_table>().v;
            }
            if (itr_prt)
                for (auto &itr_index : *itr_prt)
                {
                    if (index >= start && stop > index)
                        (*result)[itr_index.first] = itr_index.second;
                    if (stop <= index)
                        break;
                    index++;
                }
        }
        for (auto itr = keys.begin(); itr != keys.end(); itr++)
        {
            if (itr->second.which() != lua_types::tag<lua_table>::value || itr->second.get<lua_table>().v.size() == 0)
            {
                lua_key key = itr->first;
                stacks.push_back(key.cast_to_lua_types());
                auto finded = find_luaContext(const_cast<lua_map *>(&data_table), stacks);
                if (finded.first)
                    (*result)[key] = *finded.second;
                stacks.pop_back();
            }
            else if (itr->second.which() == lua_types::tag<lua_table>::value && itr->second.get<lua_table>().v.size() > 0)
            {
                lua_key key = itr->first;
                stacks.push_back(key.cast_to_lua_types());
                (*result)[key] = lua_table();
                filter_context(data_table, itr->second.get<lua_table>().v, stacks, &(*result)[key].get<lua_table>().v);
                if ((*result)[key].get<lua_table>().v.size() == 0)
                    result->erase(key);
                stacks.pop_back();
            }
        }
    }

} } // namespace taiyi::chain

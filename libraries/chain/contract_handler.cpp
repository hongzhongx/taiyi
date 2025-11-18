#include <chain/taiyi_fwd.hpp>

#include <chain/taiyi_evaluator.hpp>
#include <chain/database.hpp>

#include <chain/taiyi_objects.hpp>
#include <chain/block_summary_object.hpp>
#include <chain/account_object.hpp>
#include <chain/tiandao_property_object.hpp>
#include <chain/contract_objects.hpp>
#include <chain/nfa_objects.hpp>
#include <chain/zone_objects.hpp>
#include <chain/actor_objects.hpp>
#include <chain/cultivation_objects.hpp>

#include <chain/lua_context.hpp>

#include <chain/contract_handles.hpp>
#include <chain/taiyi_geography.hpp>

#include <chain/util/name_generator.hpp>

namespace taiyi { namespace chain {
    
    asset_symbol_type s_get_symbol_type_from_string(const string name)
    {
        if(name.size() == (TAIYI_ASSET_SYMBOL_FAI_STRING_LENGTH - 1) && name[0] == '@' && name[1] == '@') {
            //FAI string
            return asset_symbol_type::from_fai_string(name.c_str(), 3); //自定义创建的资产仅支持3位精度
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
    //=============================================================================
    void get_connected_zones(const zone_object& zone, std::set<zone_id_type>& connected_zones, database& db)
    {
        const auto& connect_by_from_idx = db.get_index< zone_connect_index, by_zone_from >();
        auto itrf = connect_by_from_idx.lower_bound( zone.id );
        auto endf = connect_by_from_idx.end();
        while( itrf != endf )
        {
            if(itrf->from != zone.id)
                break;
            if(connected_zones.find(itrf->to) == connected_zones.end())
                connected_zones.insert(itrf->to);
            ++itrf;
        }
        const auto& connect_by_to_idx = db.get_index< zone_connect_index, by_zone_to >();
        auto itrt = connect_by_to_idx.lower_bound( zone.id );
        auto endt = connect_by_to_idx.end();
        while( itrt != endt )
        {
            if(itrt->to != zone.id)
                break;
            if(connected_zones.find(itrt->from) == connected_zones.end())
                connected_zones.insert(itrt->from);
            ++itrt;
        }
    }
    //=============================================================================
    contract_asset_resources::contract_asset_resources(const nfa_object & nfa, database& db, bool is_material)
    {
        if (is_material) {
            const auto& material = db.get<nfa_material_object, by_nfa_id>(nfa.id);
            gold = material.gold.amount.value;
            food = material.food.amount.value;
            wood = material.wood.amount.value;
            fabric = material.fabric.amount.value;
            herb = material.herb.amount.value;
        }
        else {
            gold = db.get_nfa_balance(nfa, GOLD_SYMBOL).amount.value;
            food = db.get_nfa_balance(nfa, FOOD_SYMBOL).amount.value;
            wood = db.get_nfa_balance(nfa, WOOD_SYMBOL).amount.value;
            fabric = db.get_nfa_balance(nfa, FABRIC_SYMBOL).amount.value;
            herb = db.get_nfa_balance(nfa, HERB_SYMBOL).amount.value;
        }
    }
    //=============================================================================
    contract_tiandao_property::contract_tiandao_property(const tiandao_property_object& obj, database& db)
    : v_years(obj.v_years), v_months(obj.v_months), v_days(obj.v_days), v_timeonday(obj.v_timeonday), v_times(obj.v_times)
    {
    }
    //=============================================================================
    contract_handler::contract_handler(database &db, const account_object& caller, const nfa_object* nfa_caller, const contract_object &contract, contract_result &result, LuaContext &context, bool eval)
    : db(db), contract(contract), caller(caller), nfa_caller(nfa_caller), result(result), context(context), is_in_eval(eval)
    {
        result.contract_name = contract.name;
        account_contract_data_cache = db.prepare_account_contract_data(caller, contract);
        contract_data_cache = contract.contract_data;
    }
    //=============================================================================
    contract_handler::~contract_handler()
    {
        const auto& acd = db.get<account_contract_data_object, by_account_contract>( boost::make_tuple(caller.id, contract.id) );
        db.modify(acd, [&](account_contract_data_object &obj) { obj.contract_data = account_contract_data_cache; });
        db.modify(contract, [&](contract_object& c) { c.contract_data = contract_data_cache; });
        
        for (auto c : _sub_chs)
            delete c;
    }
    //=============================================================================
    void contract_handler::assert_contract_data_size()
    {
        uint64_t contract_private_data_size    = 3L * 1024;
        uint64_t contract_total_data_size      = 10L * 1024 * 1024;
        uint64_t contract_max_data_size        = 2L * 1024 * 1024 * 1024;
        FC_ASSERT(fc::raw::pack_size(account_contract_data_cache) <= contract_private_data_size, "the contract private data size is too large.");
        FC_ASSERT(fc::raw::pack_size(contract_data_cache) <= contract_total_data_size, "the contract total data size is too large.");
    }
    //=============================================================================
    bool contract_handler::is_owner()
    {
        return caller.id == contract.owner;
    }
    //=============================================================================
    int64_t contract_handler::get_nfa_caller()
    {
        if (nfa_caller)
            return nfa_caller->id._id;
        else
            return nfa_id_type::max()._id;
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
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    uint32_t contract_handler::contract_random()
    {
        try
        {
            result.existed_pv = true;
            auto hblock_id = db.head_block_id();
            return protocol::hasher::hash( hblock_id._hash[4] );
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
            return 0;
        }
    }
    //=============================================================================
    memo_data contract_handler::make_memo(const string& receiver_account_name, const string& key, const string& value, uint64_t ss, bool enable_logger)
    {
        auto &receiver = db.get_account(receiver_account_name);
        try
        {
            db.add_contract_handler_exe_point(2);
            
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
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
            return memo_data();
        }
    }
    //=============================================================================
    void contract_handler::invoke_contract_function(const string& contract_name, const string& function_name, const string& value_list_json)
    {
        const auto& contract = db.get<contract_object, by_name>(contract_name);
        auto contract_id = contract.id;
        static vector<string> s_invoking_path;
        try
        {
            if (std::find(s_invoking_path.begin(), s_invoking_path.end(), contract.name) != s_invoking_path.end()) {
                s_invoking_path.clear();
                FC_THROW("Contract cicular references are prohibitted in invoking");
            }
            else {
                s_invoking_path.push_back(contract.name);
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

            db.add_contract_handler_exe_point(5);

            s_invoking_path.pop_back();
        }
        catch (const fc::exception& e)
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
            db.add_contract_handler_exe_point(1);

            wlog(message);
            contract_logger logger(caller.name);
            logger.message = message;
            result.contract_affecteds.push_back(std::move(logger));
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
    }
    //=========================================================================
    void contract_handler::narrate(string message, bool time_prefix)
    {
        try
        {
            db.add_contract_handler_exe_point(2);

            const auto& tiandao = db.get_tiandao_properties();

            if(time_prefix) {
                static vector<string> s_todstr = { "凌晨", "上午", "下午", "晚上" };
                message = FORMAT_MESSAGE("&YEL&${y}年${m}月${d}日${tod}&NOR&，${str}", ("y", tiandao.v_years)("m", tiandao.v_months)("d", tiandao.v_days)("tod", s_todstr[tiandao.v_timeonday])("str", message));
            }
                        
            wlog(message); //for dubug
                        
            nfa_id_type affected_nfa = nfa_id_type::max();
            account_name_type affected_account;
            
            //不论是不是import导入的合约，顶层合约名的表中一定有nfa_helper
            lua_getglobal(context.mState, "current_contract");
            auto current_contract_name = LuaContext::readTopAndPop<string>(context.mState, -1);
            auto current_cnh = context.readVariable<contract_nfa_handler*>(current_contract_name, "nfa_helper");
            if (current_cnh) {
                affected_nfa = current_cnh->_caller.id;
                affected_account = db.get<account_object, by_id>(db.get<nfa_object, by_id>(affected_nfa).owner_account).name;
            }
            else
                affected_account = db.get_account(TAIYI_DANUO_ACCOUNT).name;

            contract_narrate narrator(affected_account, affected_nfa._id);
            narrator.message = message;
            result.contract_affecteds.push_back(std::move(narrator));
            
            if(!is_in_eval) {
                //push event message
                db.push_virtual_operation( narrate_log_operation( affected_account, affected_nfa, tiandao.v_years, tiandao.v_months, tiandao.v_days, tiandao.v_timeonday, tiandao.v_times, message ) );
            }

        }
        catch (const fc::exception& e)
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
    uint32_t contract_handler::head_block_num()
    {
        return db.head_block_num();
    }
    //=============================================================================
    int64_t contract_handler::generate_hash(uint32_t offset)
    {
        auto hblock_id = db.head_block_id();
        uint32_t seed = hblock_id._hash[4];
        return (int64_t)hasher::hash(seed + offset);
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
    void contract_handler::make_release()
    {
        db.modify(contract, [](contract_object& cb) { cb.is_release = true; });
    }
    //=============================================================================
    string contract_handler::get_contract_source_code(const string& contract_name)
    {
        try
        {
#ifndef IS_LOW_MEM
            const auto& contract = db.get<contract_object, by_name>(contract_name);
            const auto& contract_bin_code = db.get<chain::contract_bin_code_object, chain::by_contract_id>(contract.id);
            return contract_bin_code.source_code;
#else
            return "";
#endif
        }
        catch (const fc::exception& e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
        catch (std::exception e)
        {
            wdump((e.what()));
            LUA_C_ERR_THROW(this->context.mState, e.what());
        }
    }
    //=============================================================================
    lua_map contract_handler::get_contract_data(const string& contract_name, const lua_map& read_list)
    {
        try
        {
            const auto& contract = db.get<contract_object, by_name>(contract_name);

            vector<lua_types> stacks = {};
            lua_map result;
            read_table_data(result, read_list, contract.contract_data, stacks);

            db.add_contract_handler_exe_point(1 + fc::raw::pack_size(read_list) / 5);

            return result;
        }
        catch (const fc::exception& e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
        catch (std::exception e)
        {
            wdump((e.what()));
            LUA_C_ERR_THROW(this->context.mState, e.what());
        }
    }
    //=============================================================================
    contract_tiandao_property contract_handler::get_tiandao_property()
    {
        try
        {
            return contract_tiandao_property(db.get_tiandao_properties(), db);
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_handler::transfer_by_contract(account_id_type from, account_id_type to, asset token, contract_result &result, bool enable_logger)
    {
        try
        {
            db.add_contract_handler_exe_point(1);

            FC_ASSERT(from != to, "It's no use transferring money to yourself");
            const account_object &from_account = db.get<account_object, by_id>(from);
            const account_object &to_account = db.get<account_object, by_id>(to);
            try
            {
                FC_ASSERT(db.get_balance(from_account, token.symbol).amount >= token.amount, "Insufficient Balance: ${balance}, unable to transfer '${total_transfer}' from account '${a}' to '${t}'", ("a", from_account.name)("t", to_account.name)("total_transfer", token)("balance", db.get_balance(from_account, token.symbol)));
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
        catch (const fc::exception& e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_handler::transfer_nfa_by_contract(account_id_type from, account_id_type to, int64_t nfa_id, contract_result &result, bool enable_logger)
    {
        try
        {
            db.add_contract_handler_exe_point(1);

            FC_ASSERT(from != to, "It's no use transferring nfa to yourself");
            const account_object &from_account = db.get<account_object, by_id>(from);
            const account_object &to_account = db.get<account_object, by_id>(to);

            const auto* nfa = db.find<nfa_object, by_id>(nfa_id);
            FC_ASSERT(nfa != nullptr, "NFA with id ${i} not found", ("i", nfa_id));

            const auto* parent_nfa = db.find<nfa_object, by_id>(nfa->parent);
            FC_ASSERT(parent_nfa == nullptr, "Can not transfer child NFA, only can transfer root NFA");
            
            FC_ASSERT(from_account.id == nfa->owner_account, "Can not transfer NFA not ownd by ${a}", ("a", from_account.name));

            operation vop = nfa_transfer_operation(from_account.name, to_account.name, nfa_id);
            db.pre_push_virtual_operation( vop );

            db.modify(*nfa, [&](nfa_object &obj) {
                obj.owner_account = to_account.id;
            });
            
            //TODO: 转移子节点里面所有子节点的所有权，是否同时也要转移使用权？
            std::set<nfa_id_type> look_checker;
            db.modify_nfa_children_owner(*nfa, to_account, look_checker);
            
            db.post_push_virtual_operation( vop );

            db.add_contract_handler_exe_point(1);

            if(enable_logger) {
                protocol::nfa_affected affected;
                affected.affected_account = from_account.name;
                affected.affected_item = nfa->id;
                affected.action = nfa_affected_type::transfer_from;
                result.contract_affecteds.push_back(std::move(affected));
                
                affected.affected_account = to_account.name;
                affected.affected_item = nfa->id;
                affected.action = nfa_affected_type::transfer_to;
                result.contract_affecteds.push_back(std::move(affected));

                log(from_account.name + " transfer nfa #" + fc::json::to_string(nfa->id) + " to " + to_account.name);
            }
        }
        catch (const fc::exception& e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_handler::approve_nfa_active_by_contract(account_id_type from, account_id_type new_active_account, int64_t nfa_id, contract_result &result, bool enable_logger)
    {
        try
        {
            db.add_contract_handler_exe_point(1);

            const auto* nfa = db.find<nfa_object, by_id>(nfa_id);
            FC_ASSERT(nfa != nullptr, "NFA with id ${i} not found", ("i", nfa_id));

            const account_object &from_account = db.get<account_object, by_id>(from);
            FC_ASSERT(from_account.id == nfa->owner_account, "Can not change active operator of NFA not ownd by ${a}", ("a", from_account.name));
            
            const auto& to_active_account = db.get<account_object, by_id>(new_active_account);

            operation vop = nfa_active_approve_operation(from_account.name, to_active_account.name, nfa_id);
            db.pre_push_virtual_operation( vop );

            db.modify(*nfa, [&](nfa_object &obj) {
                obj.active_account = to_active_account.id;
            });

            db.post_push_virtual_operation( vop );

            db.add_contract_handler_exe_point(1);

            if(enable_logger) {
                protocol::nfa_affected affected;
                affected.affected_account = from_account.name;
                affected.affected_item = nfa->id;
                affected.action = nfa_affected_type::modified;
                result.contract_affecteds.push_back(std::move(affected));
                
                affected.affected_account = to_active_account.name;
                affected.affected_item = nfa->id;
                affected.action = nfa_affected_type::modified;
                result.contract_affecteds.push_back(std::move(affected));
            }
        }
        catch (const fc::exception& e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
    }
    //=============================================================================
    int64_t contract_handler::get_account_balance(const string& account_name, const string& symbol_name)
    {
        try
        {
            asset_symbol_type symbol = s_get_symbol_type_from_string(symbol_name);
            const auto& account_obj = db.get<account_object, by_name>(account_name);
            return db.get_balance(account_obj, symbol).amount.value;
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
            return 0;
        }
    }
    //=============================================================================
    void contract_handler::transfer_from(account_id_type from, const account_name_type& to, double amount, const string& symbol_name, bool enable_logger)
    {
        try
        {
            db.add_contract_handler_exe_point(1);

            account_id_type account_to = db.get_account(to).id;
            asset_symbol_type symbol = s_get_symbol_type_from_string(symbol_name);
            auto token = asset(amount, symbol);
            transfer_by_contract(from, account_to, token, result, enable_logger);
        }
        catch (const fc::exception& e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_handler::transfer_nfa_from(account_id_type from, const account_name_type& to, int64_t nfa_id, bool enable_logger)
    {
        try
        {
            db.add_contract_handler_exe_point(1);
            
            validate_account_name( to);
            const auto& to_account = db.find<account_object, by_name>(to);
            FC_ASSERT(to_account != nullptr, "account named ${n} is not exist", ("n", to));
            transfer_nfa_by_contract(from, to_account->id, nfa_id, result, enable_logger);
        }
        catch (const fc::exception& e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_handler::approve_nfa_active_from(account_id_type from, const account_name_type& new_active_account, int64_t nfa_id, bool enable_logger)
    {
        try
        {
            db.add_contract_handler_exe_point(1);
            
            validate_account_name(new_active_account);
            
            const auto& to_new_active_account = db.find<account_object, by_name>(new_active_account);
            FC_ASSERT(to_new_active_account != nullptr, "account named ${n} is not exist", ("n", new_active_account));
            
            approve_nfa_active_by_contract(from, to_new_active_account->id, nfa_id, result, enable_logger);
        }
        catch (const fc::exception& e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_handler::create_nfa_symbol(const string& symbol, const string& describe, const string& default_contract, uint64_t max_count, uint64_t min_equivalent_qi)
    {
        try
        {
            db.add_contract_handler_exe_point(2);
                                
            FC_ASSERT(memcmp(symbol.data(), "nfa.", 4) == 0, "symbol name is not start with \"nfa.\"");
            FC_ASSERT(is_valid_nfa_symbol(symbol), "symbol ${q} is invalid", ("n", symbol));
            
            FC_ASSERT(describe.size() > 0, "describe is empty");
            FC_ASSERT(describe.size() < 512, "describe is too long");
            FC_ASSERT(fc::is_utf8(describe), "describe not formatted in UTF8");

            FC_ASSERT(memcmp(default_contract.data(), "contract.", 9) == 0, "contract name is not strat with \"contract.\"");
            FC_ASSERT(is_valid_contract_name(default_contract), "contract name ${n} is invalid", ("n", default_contract));
            
            const auto& creator = caller;
            operation vop = nfa_symbol_create_operation( creator.name, symbol, describe, default_contract, max_count, min_equivalent_qi );
            db.pre_push_virtual_operation( vop );

            size_t new_state_size = db.create_nfa_symbol_object(creator, symbol, describe, default_contract, max_count, min_equivalent_qi);
            
            db.post_push_virtual_operation( vop );
            
            db.add_contract_handler_exe_point(new_state_size + 100);
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
    }
    //=============================================================================
    int64_t contract_handler::create_nfa_to_actor(int64_t to_actor_nfa_id, string symbol, lua_map data, bool enable_logger)
    {
        try
        {
            db.add_contract_handler_exe_point(2);
            
            const auto* actor_nfa = db.find<nfa_object, by_id>(to_actor_nfa_id);
            FC_ASSERT(actor_nfa != nullptr, "target nfa #${n} is not exist", ("n", to_actor_nfa_id));
            FC_ASSERT(is_actor_valid(to_actor_nfa_id), "target nfa #${n} is not an actor", ("n", to_actor_nfa_id));

            FC_ASSERT(memcmp(symbol.data(), "nfa.", 4) == 0, "symbol name is not start with \"nfa.\"");
            FC_ASSERT(is_valid_nfa_symbol(symbol), "symbol ${q} is invalid", ("n", symbol));
            const auto* nfa_symbol = db.find<nfa_symbol_object, by_symbol>(symbol);
            FC_ASSERT(nfa_symbol != nullptr, "NFA symbol named \"${n}\" is not exist.", ("n", symbol));

            operation vop = nfa_create_operation(caller.name, symbol);
            db.pre_push_virtual_operation( vop );

            const nfa_object& nfa = db.create_nfa(caller, *nfa_symbol, false, context);
            db.modify(nfa, [&](nfa_object& obj) {
                for(const auto& p : data) {
                    if(obj.contract_data.find(p.first) != obj.contract_data.end())
                        obj.contract_data[p.first] = p.second;
                    else {
                        FC_ASSERT(false, "nfa data not support the key \"${k}\"", ("k", p.first));
                    }
                }
                
                obj.owner_account = actor_nfa->owner_account;
                obj.active_account = actor_nfa->active_account;
                obj.parent = nfa_id_type(to_actor_nfa_id);
            });

            db.post_push_virtual_operation( vop );

            if (enable_logger)
            {
                protocol::nfa_affected affected;
                affected.affected_account = db.get<account_object, by_id>(actor_nfa->owner_account).name;
                affected.affected_item = nfa.id;
                affected.action = nfa_affected_type::create_for;
                result.contract_affecteds.push_back(affected);

                affected.affected_account = caller.name;
                affected.action = nfa_affected_type::create_by;
                result.contract_affecteds.push_back(affected);
            }

            return nfa.id;
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
            return -1;
        }
    }
    //=============================================================================
    int64_t contract_handler::create_nfa_to_account(const string& to_account_name, string symbol, lua_map data, bool enable_logger)
    {
        try
        {
            db.add_contract_handler_exe_point(2);
            
            validate_account_name( to_account_name );
            const auto* to_account = db.find<account_object, by_name>(to_account_name);
            FC_ASSERT(to_account != nullptr, "account named \"${n}\" is not exist", ("n", to_account_name));

            FC_ASSERT(memcmp(symbol.data(), "nfa.", 4) == 0, "symbol name is not start with \"nfa.\"");
            FC_ASSERT(is_valid_nfa_symbol(symbol), "symbol ${q} is invalid", ("n", symbol));
            const auto* nfa_symbol = db.find<nfa_symbol_object, by_symbol>(symbol);
            FC_ASSERT(nfa_symbol != nullptr, "NFA symbol named \"${n}\" is not exist.", ("n", symbol));

            operation vop = nfa_create_operation(caller.name, symbol);
            db.pre_push_virtual_operation( vop );

            const nfa_object& nfa = db.create_nfa(caller, *nfa_symbol, false, context);
            db.modify(nfa, [&](nfa_object& obj) {
                for(const auto& p : data) {
                    if(obj.contract_data.find(p.first) != obj.contract_data.end())
                        obj.contract_data[p.first] = p.second;
                    else {
                        FC_ASSERT(false, "nfa data not support the key \"${k}\"", ("k", p.first));
                    }
                }
                
                obj.owner_account = to_account->id;
                obj.active_account = to_account->id;
            });

            db.post_push_virtual_operation( vop );

            if (enable_logger)
            {
                protocol::nfa_affected affected;
                affected.affected_account = to_account->name;
                affected.affected_item = nfa.id;
                affected.action = nfa_affected_type::create_for;
                result.contract_affecteds.push_back(affected);

                affected.affected_account = caller.name;
                affected.action = nfa_affected_type::create_by;
                result.contract_affecteds.push_back(affected);
            }

            return nfa.id;
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
            return -1;
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
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
            return "";
        }
    }
    //=============================================================================
    string contract_handler::get_nfa_location(int64_t nfa_id)
    {
        try
        {
            const nfa_object& nfa = db.get<nfa_object, by_id>(nfa_id);
            const zone_object* check_zone = db.find_location_with_parents(nfa);
            return check_zone ? check_zone->name : "";
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
            return "";
        }
    }
    //=============================================================================
    bool contract_handler::is_nfa_valid(int64_t nfa_id)
    {
        return db.find<nfa_object, by_id>(nfa_id) != nullptr;
    }
    //=============================================================================
    contract_nfa_base_info contract_handler::get_nfa_info(int64_t nfa_id)
    {
        try
        {
            const nfa_object& nfa = db.get<nfa_object, by_id>(nfa_id);
            return contract_nfa_base_info(nfa, db);
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    bool contract_handler::is_nfa_action_exist(int64_t nfa_id, const string& action)
    {
        try
        {
            const nfa_object& nfa = db.get<nfa_object, by_id>(nfa_id);
            
            //check existence and consequence type
            const auto& contract = db.get<chain::contract_object, by_id>(nfa.main_contract);
            auto abi_itr = contract.contract_ABI.find(lua_types(lua_string(action)));
            if(abi_itr == contract.contract_ABI.end())
                return false;
            if(abi_itr->second.which() != lua_types::tag<lua_table>::value)
                return false;

            return true;
        }
        catch(const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    lua_map contract_handler::eval_nfa_action(int64_t nfa_id, const string& action, const lua_map& params)
    {
        try
        {
            db.add_contract_handler_exe_point(2);

            const nfa_object* nfa = db.find<nfa_object, by_id>(nfa_id);
            FC_ASSERT(nfa != nullptr, "#t&&y#序号为${i}的实体不存在#a&&i#", ("i", nfa_id));

            //check existence and consequence type
            const auto& contract = db.get<chain::contract_object, by_id>(nfa->main_contract);
            auto abi_itr = contract.contract_ABI.find(lua_types(lua_string(action)));
            if(abi_itr == contract.contract_ABI.end())
                return lua_map();
            if(abi_itr->second.which() != lua_types::tag<lua_table>::value)
                return lua_map();
            
            lua_map action_def = abi_itr->second.get<lua_table>().v;
            auto def_itr = action_def.find(lua_types(lua_string("consequence")));
            if(def_itr != action_def.end())
                FC_ASSERT(def_itr->second.get<lua_bool>().v == false, "Can not eval action ${a} in nfa ${nfa} with consequence history. should signing it in transaction.", ("a", action)("nfa", nfa_id));
            
            string function_name = "eval_" + action;
            
            //准备参数
            vector<lua_types> value_list;
            size_t params_ct = params.size();
            for(int i=1; i< (params_ct+1); i++) {
                auto p = params.find(lua_key(lua_int(i)));
                FC_ASSERT(p != params.end(), "eval_nfa_action input invalid params");
                value_list.push_back(p->second);
            }
            
            auto func_abi_itr = contract.contract_ABI.find(lua_types(lua_string(function_name)));
            FC_ASSERT(func_abi_itr != contract.contract_ABI.end(), "${f} not found, maybe a internal function", ("f", function_name));
            if(!func_abi_itr->second.get<lua_function>().is_var_arg)
                FC_ASSERT(value_list.size() == func_abi_itr->second.get<lua_function>().arglist.size(), "input values count is ${n}, but ${f}`s parameter list is ${p}...", ("n", value_list.size())("f", function_name)("p", func_abi_itr->second.get<lua_function>().arglist));
            FC_ASSERT(value_list.size() <= 20, "value list is greater than 20 limit");
            
            db.add_contract_handler_exe_point(5);
            
            auto current_contract_name = context.readVariable<string>("current_contract");
            auto current_cbi = context.readVariable<contract_base_info*>(current_contract_name, "contract_base_info");
            auto current_ch = context.readVariable<contract_handler*>(current_contract_name, "contract_helper");
            const auto &baseENV = db.get<contract_bin_code_object, by_id>(0);
            
            const auto& nfa_contract = contract;
            const auto& nfa_contract_owner = db.get<account_object, by_id>(nfa_contract.owner).name;
            const auto& nfa_contract_code = db.get<contract_bin_code_object, by_id>(nfa_contract.lua_code_b_id);
            
            contract_base_info cbi(db, context, nfa_contract_owner, nfa_contract.name, current_cbi->caller, string(nfa_contract.creation_date), current_cbi->invoker_contract_name);
            contract_handler ch(db, current_ch->caller, 0, nfa_contract, current_ch->result, context, current_ch->is_in_eval);
            contract_nfa_handler cnh(current_ch->caller, *nfa, context, db, ch);
            
            const auto& name = nfa_contract.name;
            context.new_sandbox(name, baseENV.lua_code_b.data(), baseENV.lua_code_b.size()); //sandbox
            
            FC_ASSERT(nfa_contract_code.lua_code_b.size()>0);
            context.load_script_to_sandbox(name, nfa_contract_code.lua_code_b.data(), nfa_contract_code.lua_code_b.size());
            
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
            lua_table result_table;
            try
            {
                if (err)
                    error_message = *LuaContext::Reader<lua_types>::read(context.mState, -1);
                else if(lua_istable(context.mState, -1))
                    result_table = *LuaContext::Reader<lua_table>::read(context.mState, -1);
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
            
            for(auto& temp : ch.result.contract_affecteds) {
                if(temp.which() == contract_affected_type::tag<contract_result>::value)
                    ch.result.relevant_datasize += temp.get<contract_result>().relevant_datasize;
            }
            ch.result.relevant_datasize += fc::raw::pack_size(ch.contract_data_cache) + fc::raw::pack_size(ch.account_contract_data_cache) + fc::raw::pack_size(ch.result.contract_affecteds);

            return result_table.v;
        }
        catch(const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    lua_map contract_handler::do_nfa_action(int64_t nfa_id, const string& action, const lua_map& params)
    {
        zone_id_type pre_contract_run_zone = db.get_contract_run_zone();

        try
        {
            db.add_contract_handler_exe_point(2);
            
            const nfa_object* nfa = db.find<nfa_object, by_id>(nfa_id);
            FC_ASSERT(nfa != nullptr, "#t&&y#序号为${i}的实体不存在#a&&i#", ("i", nfa_id));
            FC_ASSERT(caller.id == nfa->owner_account || caller.id == nfa->active_account, "#t&&y#没有权限操作实体#a&&i#");

            //设置db的当前运行zone标记
            if (pre_contract_run_zone == zone_id_type::max()) {
                const auto* check_zone = db.find_location_with_parents(*nfa);
                if(check_zone)
                    db.set_contract_run_zone(check_zone->id);
            }

            //check material valid
            FC_ASSERT(db.is_nfa_material_equivalent_qi_insufficient(*nfa), "NFA material equivalent qi is insufficient(#t&&y#实体完整性不足#a&&i#)");
            db.consume_nfa_material_random(*nfa, db.head_block_id()._hash[4] + 13997);

            //check existence and consequence type
            const auto& contract = db.get<chain::contract_object, by_id>(nfa->main_contract);

            FC_ASSERT(db.is_contract_allowed_by_zone(contract, db.get_contract_run_zone()), "contract ${c} is not allowed by zone #${z}(#t&&y#所在区域禁止该天道运行#a&&i#)", ("c", contract.name)("z", db.get_contract_run_zone()));
            
            auto abi_itr = contract.contract_ABI.find(lua_types(lua_string(action)));
            if(abi_itr == contract.contract_ABI.end())
                return lua_map();
            if(abi_itr->second.which() != lua_types::tag<lua_table>::value)
                return lua_map();
            
            lua_map action_def = abi_itr->second.get<lua_table>().v;
            auto def_itr = action_def.find(lua_types(lua_string("consequence")));
            if(def_itr != action_def.end())
                FC_ASSERT(def_itr->second.get<lua_bool>().v == true, "Can not perform action ${a} in nfa ${nfa} without consequence history. should eval it in api.", ("a", action)("nfa", nfa_id));
            
            string function_name = "do_" + action;
            
            //准备参数
            vector<lua_types> value_list;
            size_t params_ct = params.size();
            for(int i=1; i< (params_ct+1); i++) {
                auto p = params.find(lua_key(lua_int(i)));
                FC_ASSERT(p != params.end(), "eval_nfa_action input invalid params");
                value_list.push_back(p->second);
            }
                        
            auto func_abi_itr = contract.contract_ABI.find(lua_types(lua_string(function_name)));
            FC_ASSERT(func_abi_itr != contract.contract_ABI.end(), "${f} not found, maybe a internal function", ("f", function_name));
            if(!func_abi_itr->second.get<lua_function>().is_var_arg)
                FC_ASSERT(value_list.size() == func_abi_itr->second.get<lua_function>().arglist.size(), "input values count is ${n}, but ${f}`s parameter list is ${p}...", ("n", value_list.size())("f", function_name)("p", func_abi_itr->second.get<lua_function>().arglist));
            FC_ASSERT(value_list.size() <= 20, "value list is greater than 20 limit");
            
            db.add_contract_handler_exe_point(5);
            
            auto current_contract_name = context.readVariable<string>("current_contract");
            auto current_cbi = context.readVariable<contract_base_info*>(current_contract_name, "contract_base_info");
            auto current_ch = context.readVariable<contract_handler*>(current_contract_name, "contract_helper");
            const auto &baseENV = db.get<contract_bin_code_object, by_id>(0);
            
            const auto& nfa_contract = contract;
            const auto& nfa_contract_owner = db.get<account_object, by_id>(nfa_contract.owner).name;
            const auto& nfa_contract_code = db.get<contract_bin_code_object, by_id>(nfa_contract.lua_code_b_id);
            
            contract_base_info cbi(db, context, nfa_contract_owner, nfa_contract.name, current_cbi->caller, string(nfa_contract.creation_date), current_cbi->invoker_contract_name);
            contract_handler ch(db, current_ch->caller, 0, nfa_contract, current_ch->result, context, false);
            contract_nfa_handler cnh(current_ch->caller, *nfa, context, db, ch);
            
            const auto& name = nfa_contract.name;
            context.new_sandbox(name, baseENV.lua_code_b.data(), baseENV.lua_code_b.size()); //sandbox
            
            FC_ASSERT(nfa_contract_code.lua_code_b.size()>0);
            context.load_script_to_sandbox(name, nfa_contract_code.lua_code_b.data(), nfa_contract_code.lua_code_b.size());
            
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
            lua_table result_table;
            try
            {
                if (err)
                    error_message = *LuaContext::Reader<lua_types>::read(context.mState, -1);
                else if(lua_istable(context.mState, -1))
                    result_table = *LuaContext::Reader<lua_table>::read(context.mState, -1);
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

            for(auto& temp : ch.result.contract_affecteds) {
                if(temp.which() == contract_affected_type::tag<contract_result>::value)
                    ch.result.relevant_datasize += temp.get<contract_result>().relevant_datasize;
            }
            ch.result.relevant_datasize += fc::raw::pack_size(ch.contract_data_cache) + fc::raw::pack_size(ch.account_contract_data_cache) + fc::raw::pack_size(ch.result.contract_affecteds);

            db.set_contract_run_zone(pre_contract_run_zone);

            return result_table.v;
        }
        catch (const fc::exception& e)
        {
            db.set_contract_run_zone(pre_contract_run_zone);

            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
        
        db.set_contract_run_zone(pre_contract_run_zone);
    }
    //=============================================================================
    lua_map contract_handler::call_nfa_function_with_caller(const account_object& caller, int64_t nfa_id, const string& function_name, const lua_map& params, bool assert_when_function_not_exist)
    {
        //TODO: call产生的消耗
        LuaContext nfa_context;
        try
        {
            db.add_contract_handler_exe_point(2);

            const nfa_object* nfa = db.find<nfa_object, by_id>(nfa_id);
            FC_ASSERT(nfa != nullptr, "#t&&y#序号为${i}的实体不存在#a&&i#", ("i", nfa_id));

            //check function existence
            const auto& contract = db.get<chain::contract_object, by_id>(nfa->main_contract);
                        
            //准备参数
            vector<lua_types> value_list;
            size_t params_ct = params.size();
            for(int i=1; i< (params_ct+1); i++) {
                auto p = params.find(lua_key(lua_int(i)));
                FC_ASSERT(p != params.end(), "eval_nfa_action input invalid params");
                value_list.push_back(p->second);
            }
                        
            auto func_abi_itr = contract.contract_ABI.find(lua_types(lua_string(function_name)));
            if(func_abi_itr == contract.contract_ABI.end()) {
                if (assert_when_function_not_exist)
                    FC_ASSERT(false, "${f} not found", ("f", function_name));
                else
                    return lua_map();
            }
            if(!func_abi_itr->second.get<lua_function>().is_var_arg)
                FC_ASSERT(value_list.size() == func_abi_itr->second.get<lua_function>().arglist.size(), "input values count is ${n}, but ${f}`s parameter list is ${p}...", ("n", value_list.size())("f", function_name)("p", func_abi_itr->second.get<lua_function>().arglist));
            FC_ASSERT(value_list.size() <= 20, "value list is greater than 20 limit");
            
            db.add_contract_handler_exe_point(3);
            
            auto current_contract_name = context.readVariable<string>("current_contract");
            auto current_cbi = context.readVariable<contract_base_info*>(current_contract_name, "contract_base_info");
            auto current_ch = context.readVariable<contract_handler*>(current_contract_name, "contract_helper");
            const auto &baseENV = db.get<contract_bin_code_object, by_id>(0);
            
            const auto& nfa_contract = contract;
            const auto& nfa_contract_owner = db.get<account_object, by_id>(nfa_contract.owner).name;
            const auto& nfa_contract_code = db.get<contract_bin_code_object, by_id>(nfa_contract.lua_code_b_id);
            
            contract_base_info cbi(db, nfa_context, nfa_contract_owner, nfa_contract.name, caller.name, string(nfa_contract.creation_date), current_cbi->invoker_contract_name);
            contract_handler ch(db, caller, 0, nfa_contract, current_ch->result, nfa_context, current_ch->is_in_eval);
            contract_nfa_handler cnh(caller, *nfa, nfa_context, db, ch);
            
            const auto& name = nfa_contract.name;
            nfa_context.new_sandbox(name, baseENV.lua_code_b.data(), baseENV.lua_code_b.size()); //sandbox
            
            FC_ASSERT(nfa_contract_code.lua_code_b.size()>0);
            nfa_context.load_script_to_sandbox(name, nfa_contract_code.lua_code_b.data(), nfa_contract_code.lua_code_b.size());
            
            nfa_context.writeVariable("current_contract", name);
            nfa_context.writeVariable(name, "_G", "protected");

            nfa_context.writeVariable(name, "contract_helper", &ch);
            nfa_context.writeVariable(name, "contract_base_info", &cbi);
            nfa_context.writeVariable(name, "nfa_helper", &cnh);

            nfa_context.get_function(name, function_name);
            //push function actual parameters
            for (vector<lua_types>::iterator itr = value_list.begin(); itr != value_list.end(); itr++)
                LuaContext::Pusher<lua_types>::push(nfa_context.mState, *itr).release();
            
            int err = lua_pcall(nfa_context.mState, value_list.size(), 1, 0);
            lua_types error_message;
            lua_table result_table;
            try
            {
                if (err)
                    error_message = *LuaContext::Reader<lua_types>::read(nfa_context.mState, -1);
                else if(lua_istable(nfa_context.mState, -1))
                    result_table = *LuaContext::Reader<lua_table>::read(nfa_context.mState, -1);
            }
            catch (...)
            {
                error_message = lua_string(" Unexpected errors ");
            }
            lua_pop(nfa_context.mState, -1);
            nfa_context.close_sandbox(name);
            if (err)
                FC_THROW("Try the contract resolution execution failure, ${message}", ("message", error_message));
            
            ch.assert_contract_data_size();

            for(auto& temp : ch.result.contract_affecteds) {
                if(temp.which() == contract_affected_type::tag<contract_result>::value)
                    ch.result.relevant_datasize += temp.get<contract_result>().relevant_datasize;
            }
            ch.result.relevant_datasize += fc::raw::pack_size(ch.contract_data_cache) + fc::raw::pack_size(ch.account_contract_data_cache) + fc::raw::pack_size(ch.result.contract_affecteds);

            return result_table.v;
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    lua_map contract_handler::call_nfa_function(int64_t nfa_id, const string& function_name, const lua_map& params, bool assert_when_function_not_exist)
    {
        auto current_contract_name = context.readVariable<string>("current_contract");
        auto current_ch = context.readVariable<contract_handler*>(current_contract_name, "contract_helper");
        return call_nfa_function_with_caller(current_ch->caller, nfa_id, function_name, params, assert_when_function_not_exist);
    }
    //=============================================================================
    void contract_handler::change_nfa_contract(int64_t nfa_id, const string& contract_name)
    {
        try
        {
            const auto& nfa = db.get<nfa_object, by_id>(nfa_id);
            FC_ASSERT(nfa.owner_account == caller.id || nfa.active_account == caller.id, "无权操作NFA #${n}", ("n", nfa_id));

            const auto* contract = db.find<contract_object, by_name>(contract_name);
            FC_ASSERT(contract != nullptr, "contract named ${a} is not exist", ("a", contract_name));
            auto abi_itr = contract->contract_ABI.find(lua_types(lua_string(TAIYI_NFA_INIT_FUNC_NAME)));
            FC_ASSERT(abi_itr != contract->contract_ABI.end(), "contract ${c} has not init function named ${i}", ("c", contract_name)("i", TAIYI_NFA_INIT_FUNC_NAME));
            
            db.add_contract_handler_exe_point(5);
            
            contract_worker worker;
            vector<lua_types> value_list;
            
            long long old_drops = caller.qi.amount.value / TAIYI_USEMANA_EXECUTION_SCALE;
            long long vm_drops = old_drops;
            lua_table result_table = worker.do_contract_function(caller, TAIYI_NFA_INIT_FUNC_NAME, value_list, *contract, vm_drops, true, context, db);
            int64_t used_drops = old_drops - vm_drops;
            
            int64_t used_qi = used_drops * TAIYI_USEMANA_EXECUTION_SCALE + 100 * TAIYI_USEMANA_EXECUTION_SCALE;
            FC_ASSERT( caller.qi.amount.value >= used_qi, "真气不足以修改天道" );
            
            //reward contract owner
            const auto& contract_owner = db.get<account_object, by_id>(contract->owner);
            db.reward_feigang(contract_owner, caller, asset(used_qi, QI_SYMBOL));
            
            db.add_contract_handler_exe_point(5 + fc::raw::pack_size(result_table) / 5);

            db.modify(nfa, [&](nfa_object& obj) {
                //仅仅改变主合约，不改变nfa的symbol
                obj.main_contract = contract->id;
                //仅仅增加数据中没有的字段
                for (auto& p : result_table.v) {
                    if (obj.contract_data.find(p.first) == obj.contract_data.end())
                        obj.contract_data[p.first] = p.second;
                }
            });
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=========================================================================
    //按深度搜索表
    //table: 待搜索表
    //path: 查找路径(表树形结构的某一条分支)
    //start: 指定查找路径的起点
    std::pair<bool, const lua_types*> contract_handler::search_in_table(const lua_map* table, const vector<lua_types>& path, int start)
    {
        const lua_types* node = nullptr;
        for (size_t i = start; i < path.size(); i++)
        {
            auto itr = table->find(path[i]);
            if (itr == table->end())
                return std::make_pair(false, node); //未找到路径节点，返回上一级节点指针，nullptr时则表示路径查找从start处就失败了

            node = &itr->second;
            if (i == (path.size() - 1))
                return std::make_pair(true, node);

            if (node->which() != lua_types::tag<lua_table>::value)
                return std::make_pair(false, node); //因为下一层不再是table，按原目标路径无法深入，返回上级table地址

            table = &node->get<lua_table>().v; //继续下一层搜索
        }
        
        return std::make_pair(false, node);
    }
    //=========================================================================
    std::pair<bool, lua_types*> contract_handler::get_in_table(lua_map* table, const vector<lua_types>& path, int start)
    {
        lua_types* node = nullptr;
        for (size_t i = start; i < path.size(); i++)
        {
            auto itr = table->find(path[i]);
            if (itr == table->end())
                return std::make_pair(false, node); //未找到路径节点，返回上一级节点指针，nullptr时则表示路径查找从start处就失败了

            node = &itr->second;
            if (i == (path.size() - 1))
                return std::make_pair(true, node);

            if (node->which() != lua_types::tag<lua_table>::value)
                return std::make_pair(false, node); //因为下一层不再是table，按原目标路径无法深入，返回上级table地址

            table = &node->get<lua_table>().v; //继续下一层搜索
        }
        
        return std::make_pair(false, node);
    }
    //=========================================================================
    //按深度搜索表并删除目标
    //table: 待搜索表
    //path: 查找路径(表树形结构的某一条分支)
    //start: 指定查找路径的起点
    std::pair<bool, lua_types*> contract_handler::erase_in_table(lua_map* table, const vector<lua_types>& path, int start)
    {
        lua_types* node = nullptr;
        for (size_t i = start; i < path.size(); i++)
        {
            auto itr = table->find(path[i]);
            if (itr == table->end())
                return std::make_pair(false, node); //未找到路径节点，返回上一级节点指针，nullptr时则表示路径查找从start处就失败了

            if (i == (path.size() - 1))
            {
                table->erase(itr); //找到目标，删除之
                return std::make_pair(true, node); //返回上一级节点指针，nullptr时表示从start处删除的
            }

            node = &itr->second;
            if (node->which() != lua_types::tag<lua_table>::value)
                return std::make_pair(false, node); //因为下一层不再是table，按原目标路径无法深入，返回上级table地址

            table = &node->get<lua_table>().v; //继续下一层搜索
        }

        return std::make_pair(false, node);
    }
    //=========================================================================
    //按深度搜索表并写入目标
    //table: 待写入表
    //path: 写入路径(表树形结构的某一条分支)
    //value: 写入路径最后一个key对应的value
    //start: 指定写入路径的起点
    bool contract_handler::write_in_table(lua_map* table, const vector<lua_types>& path, const lua_types& value, int start)
    {
        lua_types* node = nullptr;
        for (size_t i = start; i < path.size(); i++)
        {
            auto itr = table->find(path[i]);
            if (itr == table->end())
                return false; //未找到路径节点

            if (i == (path.size() - 1))
            {
                //找到目标，写入值（或者覆盖原值）
                (*table)[itr->first] = value;
                return true;
            }

            node = &itr->second;
            if (node->which() != lua_types::tag<lua_table>::value)
                return false; //因为下一层不再是table，按原目标路径无法深入

            table = &node->get<lua_table>().v; //继续下一层搜索
        }

        return false;
    }
    //=============================================================================
    lua_map contract_handler::read_contract_data(const lua_map& read_list)
    {
        try
        {
            vector<lua_types> stacks = {};
            lua_map result;
            read_table_data(result, read_list, contract_data_cache, stacks);
            
            db.add_contract_handler_exe_point(2 + fc::raw::pack_size(read_list) / 5);

            return result;
        }
        catch (const fc::exception& e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
        catch (std::exception e)
        {
            wdump((e.what()));
            LUA_C_ERR_THROW(this->context.mState, e.what());
        }
    }
    //=========================================================================
    void contract_handler::write_contract_data(const lua_map& data, const lua_map& write_list)
    {
        try
        {
            db.add_contract_handler_exe_point(2 + fc::raw::pack_size(write_list) / 5);
            
            vector<lua_types> stacks = {};
            write_table_data(contract_data_cache, write_list, data, stacks);
        }
        catch (const fc::exception& e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
        catch (std::exception e)
        {
            wdump((e.what()));
            LUA_C_ERR_THROW(this->context.mState, e.what());
        }
    }
    //=========================================================================
    lua_map contract_handler::get_account_contract_data(const string& account_name, const string& contract_name, const lua_map& read_list)
    {
        try
        {
            const auto& account = db.get_account(account_name);
            const auto& contract = db.get<contract_object, by_name>(contract_name);
            auto account_contract_data = db.prepare_account_contract_data(account, contract);

            vector<lua_types> stacks = {};
            lua_map result;
            read_table_data(result, read_list, account_contract_data, stacks);

            db.add_contract_handler_exe_point(2 + fc::raw::pack_size(read_list) / 5);

            return result;
        }
        catch (const fc::exception& e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
        catch (std::exception e)
        {
            wdump((e.what()));
            LUA_C_ERR_THROW(this->context.mState, e.what());
        }
    }
    //=========================================================================
    lua_map contract_handler::read_account_contract_data(const lua_map& read_list)
    {
        try
        {
            vector<lua_types> stacks = {};
            lua_map result;
            read_table_data(result, read_list, account_contract_data_cache, stacks);

            db.add_contract_handler_exe_point(2 + fc::raw::pack_size(read_list) / 5);

            return result;
        }
        catch (const fc::exception& e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
        catch (std::exception e)
        {
            wdump((e.what()));
            LUA_C_ERR_THROW(this->context.mState, e.what());
        }
    }
    //=========================================================================
    void contract_handler::write_account_contract_data(const lua_map& data, const lua_map& write_list)
    {
        try
        {
            vector<lua_types> stacks = {};
            write_table_data(account_contract_data_cache, write_list, data, stacks);
            
            db.add_contract_handler_exe_point(2 + fc::raw::pack_size(write_list) / 5);
        }
        catch (const fc::exception& e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
        catch (std::exception e)
        {
            wdump((e.what()));
            LUA_C_ERR_THROW(this->context.mState, e.what());
        }
    }
    //=========================================================================
    lua_map contract_handler::read_nfa_contract_data(int64_t nfa_id, const lua_map& read_list)
    {
        try
        {
            const auto& nfa = db.get<nfa_object, by_id>(nfa_id);
            
            vector<lua_types> stacks = {};
            lua_map result;
            read_table_data(result, read_list, nfa.contract_data, stacks);

            db.add_contract_handler_exe_point(2 + fc::raw::pack_size(read_list) / 5);
            
            return result;
        }
        catch (const fc::exception& e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
        catch (std::exception e)
        {
            wdump((e.what()));
            LUA_C_ERR_THROW(this->context.mState, e.what());
        }
    }
    //=========================================================================
    void contract_handler::write_nfa_contract_data(int64_t nfa_id, const lua_map& data, const lua_map& write_list)
    {
        try
        {
            const auto& nfa = db.get<nfa_object, by_id>(nfa_id);
            FC_ASSERT(nfa.owner_account == caller.id || nfa.active_account == caller.id, "无权操作NFA");

            db.modify(nfa, [&](nfa_object& obj) {
                vector<lua_types> stacks = {};
                write_table_data(obj.contract_data, write_list, data, stacks);
            });
            
            db.add_contract_handler_exe_point(2 + fc::raw::pack_size(write_list) / 5);
        }
        catch (const fc::exception& e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
        catch (std::exception e)
        {
            wdump((e.what()));
            LUA_C_ERR_THROW(this->context.mState, e.what());
        }
    }
    //=========================================================================
    void contract_handler::read_table_data(lua_map& out_data, const lua_map& rkeys, const lua_map& target_table, vector<lua_types>& stacks)
    {
        static auto start_key = lua_types(lua_string("start"));
        static auto stop_key = lua_types(lua_string("stop"));
        uint32_t start = 0, stop = 0, index = 0;
        lua_map keys = rkeys;
        
        if (keys.find(start_key) != keys.end() && keys[start_key].which() == lua_types::tag<lua_int>::value) {
            start = keys[start_key].get<lua_int>().v;
            keys.erase(start_key);
        }
        
        if (keys.find(stop_key) != keys.end() && keys[stop_key].which() == lua_types::tag<lua_int>::value) {
            stop = keys[stop_key].get<lua_int>().v;
            keys.erase(stop_key);
        }
        
        if (start || stop)
        {
            const lua_map *prt = nullptr;
            if (stacks.size() == 0)
                prt = &target_table;
            else {
                auto finded = search_in_table(&target_table, stacks);
                if (finded.first && finded.second->which() == lua_types::tag<lua_table>::value)
                    prt = &finded.second->get<lua_table>().v;
            }
            
            if (prt) {
                for (auto& itr : *prt)
                {
                    if (index >= start && stop > index)
                    {
                        stacks.push_back(itr.first.cast_to_lua_types());

                        auto finded = get_in_table(&out_data, stacks);
                        if (finded.first)
                        {
                            bool bOK = write_in_table(&out_data, stacks, itr.second);
                            FC_ASSERT(bOK, "write_table_data error, should not be here!");
                        }
                        else if (finded.second)
                        {
                            lua_table &temp_table = finded.second->get<lua_table>();
                            temp_table.v[itr.first] = itr.second;
                        }
                        else
                        {
                            auto data_finded = search_in_table(&target_table, vector<lua_types>{stacks[0]});
                            FC_ASSERT(data_finded.first == true, "failed in write_table_data while path in data is not exist");
                            out_data[itr.first] = *data_finded.second;
                        }

                        stacks.pop_back();
                    }
                    
                    if (stop <= index)
                        break;
                    
                    index++;
                }
            }
        }
        
        for (auto itr = keys.begin(); itr != keys.end(); itr++)
        {
            if (itr->second.which() != lua_types::tag<lua_table>::value || itr->second.get<lua_table>().v.size() == 0)
            {
                lua_key key = itr->first;
                stacks.push_back(key.cast_to_lua_types());
                auto data_finded = search_in_table(&target_table, stacks);
                if (data_finded.first)
                {
                    auto finded = get_in_table(&out_data, stacks);
                    if (finded.first)
                    {
                        bool bOK = write_in_table(&out_data, stacks, *data_finded.second);
                        FC_ASSERT(bOK, "write_table_data error, should not be here!");
                    }
                    else if (finded.second)
                    {
                        lua_table &temp_table = finded.second->get<lua_table>();
                        temp_table.v[key] = *data_finded.second;
                    }
                    else
                    {
                        auto data_finded = search_in_table(&target_table, vector<lua_types>{stacks[0]});
                        FC_ASSERT(data_finded.first == true, "failed in write_table_data while path in data is not exist");
                        out_data[key] = *data_finded.second;
                    }

                }
                stacks.pop_back();
            }
            else if (itr->second.which() == lua_types::tag<lua_table>::value && itr->second.get<lua_table>().v.size() > 0)
            {
                stacks.push_back(itr->first.cast_to_lua_types());
                read_table_data(out_data, itr->second.get<lua_table>().v, target_table, stacks);
                stacks.pop_back();
            }
        }
        
        if (keys.size() == 0 && stacks.size() == 0 && !(start || stop))
            out_data = target_table;
    }
    //=========================================================================
    void contract_handler::write_table_data(lua_map& target_table, const lua_map& keys, const lua_map& data, vector<lua_types>& stacks)
    {
        for (auto itr = keys.begin(); itr != keys.end(); itr++)
        {
            if (itr->second.which() != lua_types::tag<lua_table>::value || itr->second.get<lua_table>().v.size() == 0)
            {
                lua_key key = itr->first;
                bool flag = (itr->second.which() == lua_types::tag<lua_bool>::value) ? itr->second.get<lua_bool>().v : true; //默认不清除对应的目标
                stacks.push_back(key.cast_to_lua_types());
                if (!flag)
                {
                    erase_in_table(&target_table, stacks);
                }
                else
                {
                    auto finded = get_in_table(&target_table, stacks);
                    if (finded.first)
                    {
                        auto data_finded = search_in_table(&data, stacks);
                        FC_ASSERT(data_finded.first == true, "failed in write_table_data while path in data is not exist");
                        bool bOK = write_in_table(&target_table, stacks, *data_finded.second);
                        FC_ASSERT(bOK, "write_table_data error, should not be here!");
                    }
                    else if (finded.second)
                    {
                        auto data_finded = search_in_table(&data, stacks);
                        FC_ASSERT(data_finded.first == true, "failed in write_table_data while path in data is not exist");

                        lua_table &temp_table = finded.second->get<lua_table>();
                        temp_table.v[key] = *data_finded.second;
                    }
                    else
                    {
                        auto data_finded = search_in_table(&data, vector<lua_types>{stacks[0]});
                        FC_ASSERT(data_finded.first == true, "failed in write_table_data while path in data is not exist");
                        target_table[key] = *data_finded.second;
                    }
                }
                stacks.pop_back();
            }
            else if (itr->second.which() == lua_types::tag<lua_table>::value && itr->second.get<lua_table>().v.size() > 0)
            {
                lua_key key = itr->first;
                stacks.push_back(key.cast_to_lua_types());
                auto finded = get_in_table(&target_table, stacks);
                if (finded.first)
                {
                    write_table_data(target_table, itr->second.get<lua_table>().v, data, stacks);
                }
                else if (finded.second)
                {
                    auto data_finded = search_in_table(&data, stacks);
                    FC_ASSERT(data_finded.first == true, "failed in write_table_data while path in data is not exist");
                    
                    lua_table &temp_table = finded.second->get<lua_table>();
                    temp_table.v[key] = *data_finded.second;
                }
                else
                {
                    auto data_finded = search_in_table(&data, vector<lua_types>{stacks[0]});
                    FC_ASSERT(data_finded.first == true, "failed in write_table_data while path in data is not exist");
                    target_table[key] = *data_finded.second;
                }
                stacks.pop_back();
            }
        }
        
        if (keys.size() == 0)
            target_table = data;
    }
    //=============================================================================
    int64_t contract_handler::get_nfa_balance(int64_t nfa_id, const string& symbol_name)
    {
        try
        {
            asset_symbol_type symbol = s_get_symbol_type_from_string(symbol_name);
            const auto& nfa = db.get<nfa_object, by_id>(nfa_id);
            return db.get_nfa_balance(nfa, symbol).amount.value;
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    contract_asset_resources contract_handler::get_nfa_resources(int64_t id)
    {
        try
        {
            const auto& nfa = db.get<nfa_object, by_id>(id);
            return contract_asset_resources(nfa, db, false);
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    contract_asset_resources contract_handler::get_nfa_materials(int64_t id)
    {
        try
        {
            const auto& nfa = db.get<nfa_object, by_id>(id);
            return contract_asset_resources(nfa, db, true);
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    vector<contract_nfa_base_info> contract_handler::list_nfa_inventory(int64_t nfa_id, const string& symbol_name)
    {
        try
        {
            db.add_contract_handler_exe_point(2);

            nfa_symbol_id_type symbol_id = nfa_symbol_id_type::max();
            if(symbol_name != "") {
                const auto* symbol = db.find<nfa_symbol_object, by_symbol>(symbol_name);
                if(symbol)
                    symbol_id = symbol->id;
            }
            
            std::vector<contract_nfa_base_info> result;
            const auto& nfa_by_parent_idx = db.get_index<nfa_index, by_parent>();
            auto itr = nfa_by_parent_idx.lower_bound( nfa_id );
            while(itr != nfa_by_parent_idx.end())
            {
                if(itr->parent != nfa_id_type(nfa_id))
                    break;
                if(symbol_name != "") {
                    if(itr->symbol_id != symbol_id) {
                        itr++;
                        continue;
                    }
                }
                result.emplace_back(contract_nfa_base_info(*itr, db));
                itr++;
            }

            return result;
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    extern E_ZONE_TYPE g_generate_zone_refining_result(const zone_object& zone, const nfa_material_object& zone_materials, uint32_t seed);
    string contract_handler::refine_zone(int64_t nfa_id)
    {
        try
        {
            db.add_contract_handler_exe_point(2);

            const auto& nfa = db.get<nfa_object, by_id>(nfa_id);
            FC_ASSERT(nfa.owner_account == caller.id || nfa.active_account == caller.id, "caller account not the zone nfa #${z}'s owner or active operator", ("z", nfa_id));
            
            const auto* zone = db.find<zone_object, by_nfa_id>(nfa_id);
            FC_ASSERT(zone != nullptr, "nfa #${z} is not a zone", ("z", nfa_id));
            
            uint32_t rand_seed = contract_random();
            
            const auto& m = db.get<nfa_material_object, by_nfa_id>(zone->nfa_id);
            auto new_type = g_generate_zone_refining_result(*zone, m, rand_seed);
            FC_ASSERT(new_type != _ZONE_INVALID_TYPE, "invalid zone type \"${t}\"", ("t", new_type));
            if(new_type != zone->type) {
                db.modify(*zone, [&](zone_object& obj){
                    obj.type = new_type;
                });
            }
            
            //随机消耗大量真气（用执行点表示）
            int64_t used_exe_point = 100 + rand_seed % 1000;
            db.add_contract_handler_exe_point(used_exe_point);

            return get_zone_type_string(new_type);
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    int64_t contract_handler::create_zone(const string& name, const string& type_name)
    {
        try
        {
            db.add_contract_handler_exe_point(2);

            E_ZONE_TYPE ztype = get_zone_type_from_string(type_name);
            FC_ASSERT(ztype != _ZONE_INVALID_TYPE, "\"${s}\" is invalid zone type", ("s", type_name));

            FC_ASSERT( name.size() > 0, "Name is empty" );
            FC_ASSERT( name.size() < TAIYI_ZONE_NAME_LIMIT, "Name size limit exceeded. Max: ${max} Current: ${n}", ("max", TAIYI_ZONE_NAME_LIMIT - 1)("n", name.size()) );
            FC_ASSERT( fc::is_utf8( name ), "Name not formatted in UTF8" );
            
            //check zone existence
            auto check_zone = db.find< zone_object, by_name >( name );
            FC_ASSERT( check_zone == nullptr, "There is already exist zone named \"${a}\".", ("a", name) );

            //先创建NFA
            string nfa_symbol_name = "nfa.zone.default";
            const auto* nfa_symbol = db.find<nfa_symbol_object, by_symbol>(nfa_symbol_name);
            FC_ASSERT(nfa_symbol != nullptr, "NFA symbol named \"${n}\" is not exist.", ("n", nfa_symbol_name));
            
            const auto& creator = caller;
            const auto& nfa = db.create_nfa(creator, *nfa_symbol, false, context);

            protocol::nfa_affected affected;
            affected.affected_account = creator.name;
            affected.affected_item = nfa.id;
            affected.action = nfa_affected_type::create_for;
            result.contract_affecteds.push_back(affected);
            
            affected.affected_account = creator.name;
            affected.action = nfa_affected_type::create_by;
            result.contract_affecteds.push_back(affected);

            //创建一个新区域
            /*const auto& new_zone = */db.create< zone_object >( [&]( zone_object& zone ) {
                db.initialize_zone_object( zone, name, nfa, ztype);
            });

            db.add_contract_handler_exe_point(TAIYI_ZONE_OBJ_STATE_BYTES + 1000);

            return nfa.id;
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    bool contract_handler::is_contract_allowed_by_zone(const string& zone_name, const string& contract_name)
    {
        try
        {
            const auto* zone = db.find<zone_object, by_name>(zone_name);
            FC_ASSERT(zone != nullptr, "zone named ${n} is not exist.", ("n", zone_name));

            const auto* contract = db.find<contract_object, by_name>(contract_name);
            FC_ASSERT(contract != nullptr, "contract named ${n} is not exist.", ("n", contract_name));

            return db.is_contract_allowed_by_zone(*contract, zone->id);
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_handler::set_zone_contract_permission(const string& zone_name, const string& contract_name, bool allowed)
    {
        try
        {
            db.add_contract_handler_exe_point(2);

            const auto* zone = db.find<zone_object, by_name>(zone_name);
            FC_ASSERT(zone != nullptr, "zone named ${n} is not exist.", ("n", zone_name));
            const auto& nfa = db.get<nfa_object, by_id>(zone->nfa_id);
            FC_ASSERT(nfa.owner_account == caller.id || nfa.active_account == caller.id, "caller account not the zone nfa #${z}'s owner or active operator", ("z", zone->id));

            const auto* contract = db.find<contract_object, by_name>(contract_name);
            FC_ASSERT(contract != nullptr, "contract named ${n} is not exist.", ("n", contract_name));

            const auto* permission = db.find<zone_contract_permission_object, by_zone>(boost::make_tuple( zone->id, contract->id ));
            if (permission) {
                db.modify(*permission, [&](zone_contract_permission_object &obj) {
                    obj.allowed = allowed;
                });
            }
            else {
                db.create<zone_contract_permission_object>([&](zone_contract_permission_object& obj) {
                    obj.zone = zone->id;
                    obj.contract = contract->id;
                    obj.allowed = allowed;
                });
            }
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_handler::remove_zone_contract_permission(const string& zone_name, const string& contract_name)
    {
        try
        {
            db.add_contract_handler_exe_point(1);

            const auto* zone = db.find<zone_object, by_name>(zone_name);
            FC_ASSERT(zone != nullptr, "zone named ${n} is not exist.", ("n", zone_name));
            const auto& nfa = db.get<nfa_object, by_id>(zone->nfa_id);
            FC_ASSERT(nfa.owner_account == caller.id || nfa.active_account == caller.id, "caller account not the zone nfa #${z}'s owner or active operator", ("z", zone->id));

            const auto* contract = db.find<contract_object, by_name>(contract_name);
            FC_ASSERT(contract != nullptr, "contract named ${n} is not exist.", ("n", contract_name));

            const auto* permission = db.find<zone_contract_permission_object, by_zone>(boost::make_tuple( zone->id, contract->id ));
            FC_ASSERT(permission != nullptr, "zone contract permission item is not exist.");
            db.remove(*permission);
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_handler::set_zone_ref_prohibited_contract_zone(const string& zone_name, const string& ref_zone_name)
    {
        try
        {
            db.add_contract_handler_exe_point(1);

            const auto* zone = db.find<zone_object, by_name>(zone_name);
            FC_ASSERT(zone != nullptr, "zone named ${n} is not exist.", ("n", zone_name));
            const auto& nfa = db.get<nfa_object, by_id>(zone->nfa_id);
            FC_ASSERT(nfa.owner_account == caller.id || nfa.active_account == caller.id, "caller account not the zone nfa #${z}'s owner or active operator", ("z", zone->id));

            if (ref_zone_name == "") {
                db.modify(*zone, [&](zone_object& obj) {
                    obj.ref_prohibited_contract_zone = zone_id_type::max();
                });
            }
            else {
                const auto* ref_zone = db.find<zone_object, by_name>(ref_zone_name);
                FC_ASSERT(ref_zone != nullptr, "ref zone named ${n} is not exist.", ("n", ref_zone_name));
                db.modify(*zone, [&](zone_object& obj) {
                    obj.ref_prohibited_contract_zone = ref_zone->id;
                });
            }
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    bool contract_handler::is_zone_valid(int64_t nfa_id)
    {
        return db.find<zone_object, by_nfa_id>(nfa_id) != nullptr;
    }
    //=============================================================================
    bool contract_handler::is_zone_valid_by_name(const string& name)
    {
        return db.find<zone_object, by_name>(name) != nullptr;
    }
    //=============================================================================
    contract_zone_base_info contract_handler::get_zone_info(int64_t nfa_id)
    {
        try
        {
            const auto* zone = db.find<zone_object, by_nfa_id>(nfa_id);
            FC_ASSERT(zone != nullptr, "NFA #${i} is not a zone", ("i", nfa_id));
            return contract_zone_base_info(*zone, db);
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    contract_zone_base_info contract_handler::get_zone_info_by_name(const string& name)
    {
        try
        {
            const auto* zone = db.find<zone_object, by_name>(name);
            FC_ASSERT(zone != nullptr, "Zone named ${n} is not exist", ("n", name));
            return contract_zone_base_info(*zone, db);
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    vector<contract_actor_base_info> contract_handler::list_actors_on_zone(int64_t nfa_id)
    {
        try
        {
            db.add_contract_handler_exe_point(2);

            const auto* zone = db.find<zone_object, by_nfa_id>(nfa_id);
            FC_ASSERT(zone != nullptr, "NFA #${i} is not a zone", ("i", nfa_id));
            
            vector<contract_actor_base_info> result;
            const auto& actor_by_location_idx = db.get_index< chain::actor_index, chain::by_location >();
            auto itr = actor_by_location_idx.lower_bound( zone->id );
            auto end = actor_by_location_idx.end();
            while( itr != end )
            {
                const actor_object& act = *itr;
                ++itr;
                
                if( act.location != zone_id_type(zone->id) )
                    break;
                
                result.emplace_back(contract_actor_base_info(act, db));
            }
            return result;
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_handler::connect_zones(int64_t from_zone_nfa_id, int64_t to_zone_nfa_id)
    {
        try
        {
            db.add_contract_handler_exe_point(2);
            
            const auto& from_zone_nfa = db.get<nfa_object, by_id>(from_zone_nfa_id);
            const auto& to_zone_nfa = db.get<nfa_object, by_id>(to_zone_nfa_id);
            if(from_zone_nfa.owner_account != caller.id && from_zone_nfa.active_account != caller.id && to_zone_nfa.owner_account != caller.id && to_zone_nfa.active_account != caller.id)
                FC_ASSERT(false, "没有权限操作区域");

            db.add_contract_handler_exe_point(2);

            const auto* from_zone = db.find<zone_object, by_nfa_id>(from_zone_nfa_id);
            FC_ASSERT(from_zone != nullptr, "实体${z}不是一个区域", ("z", from_zone_nfa_id));
            const auto* to_zone = db.find<zone_object, by_nfa_id>(to_zone_nfa_id);
            FC_ASSERT(to_zone != nullptr, "实体${z}不是一个区域", ("z", to_zone_nfa_id));

            const auto* check_connect = db.find<zone_connect_object, by_zone_from>( boost::make_tuple(from_zone->id, to_zone->id) );
            FC_ASSERT( check_connect == nullptr, "从&YEL&${a}&NOR&到&YEL&${b}&NOR&之间的连接已经存在", ("a", from_zone->name)("b", to_zone->name) );
            
            db.add_contract_handler_exe_point(5);

            const auto& tiandao = db.get_tiandao_properties();

            //检查连接区域是否达到上限
            std::set<zone_id_type> connected_zones;
            uint max_num = tiandao.zone_type_connection_max_num_map.at(from_zone->type);
            get_connected_zones(*from_zone, connected_zones, db);
            FC_ASSERT(connected_zones.size() < max_num || connected_zones.find(to_zone->id) != connected_zones.end(), "区域&YEL&${z}&NOR&的连接已经存在或者达到上限", ("z", from_zone->name));
            connected_zones.clear();
            max_num = tiandao.zone_type_connection_max_num_map.at(to_zone->type);
            FC_ASSERT(connected_zones.size() < max_num || connected_zones.find(from_zone->id) != connected_zones.end(), "区域&YEL&${z}&NOR&的连接已经存在或者达到上限", ("z", to_zone->name));

            db.add_contract_handler_exe_point(5);

            //create connection
            db.create< zone_connect_object >( [&]( zone_connect_object& o ) {
                o.from = from_zone->id;
                o.to = to_zone->id;
            });
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    bool contract_handler::is_actor_valid(int64_t nfa_id)
    {
        return db.find<actor_object, by_nfa_id>(nfa_id) != nullptr;
    }
    //=============================================================================
    bool contract_handler::is_actor_valid_by_name(const string& name)
    {
        return db.find_actor(name) != nullptr;
    }
    //=============================================================================
    contract_actor_base_info contract_handler::get_actor_info(int64_t nfa_id)
    {
        try
        {
            const auto* actor = db.find<actor_object, by_nfa_id>(nfa_id);
            FC_ASSERT(actor != nullptr, "NFA #${i} is not an actor", ("i", nfa_id));
            return contract_actor_base_info(*actor, db);
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    contract_actor_base_info contract_handler::get_actor_info_by_name(const string& name)
    {
        try
        {
            const auto* actor = db.find<actor_object, by_name>(name);
            FC_ASSERT(actor != nullptr, "Actor named ${n} is not exist", ("n", name));
            return contract_actor_base_info(*actor, db);
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    contract_actor_core_attributes contract_handler::get_actor_core_attributes(int64_t nfa_id)
    {
        try
        {
            const auto* actor = db.find<actor_object, by_nfa_id>(nfa_id);
            FC_ASSERT(actor != nullptr, "NFA #${i} is not an actor", ("i", nfa_id));
            return contract_actor_core_attributes(*actor, db);
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_handler::born_actor(const string& name, int gender, int sexuality, const lua_map& init_attrs, const string& zone_name)
    {
        try
        {
            db.add_contract_handler_exe_point(2);

            const auto* actor = db.find<actor_object, by_name>(name);
            FC_ASSERT(actor != nullptr, "Actor named ${n} is not exist", ("n", name));
            FC_ASSERT(!actor->born, "Actor named ${n} is already born", ("n", name));
            
            const auto& actor_nfa = db.get<nfa_object, by_id>(actor->nfa_id);
            FC_ASSERT(actor_nfa.owner_account == caller.id || actor_nfa.active_account == caller.id, "无权操作角色\"${n}\"", ("n", name));

            const auto* zone = db.find<zone_object, by_name>(zone_name);
            FC_ASSERT(zone != nullptr, "Zone named ${n} is not exist", ("n", zone_name));
            
            db.add_contract_handler_exe_point(3);

            //转换初始属性值列表
            FC_ASSERT(init_attrs.size() == 8, "the number of actor init attributes is not 8");
            vector<uint16_t> value_list;
            for(int i=1; i<=8; i++) {
                auto p = init_attrs.find(lua_key(lua_int(i)));
                FC_ASSERT(p != init_attrs.end(), "born_actor input invalid init attributes");
                value_list.push_back(p->second.get<lua_int>().v);
            }
            
            const auto* check_core_attrs = db.find< actor_core_attributes_object, by_actor >( actor->id );
            FC_ASSERT( check_core_attrs == nullptr, "Core attributes should not exist before actor born" );

            uint16_t amount = 0;
            for(auto i : value_list)
                amount += i;
            FC_ASSERT(amount <= actor->init_attribute_amount_max, "Amount of init attribute values is more than ${m}.", ("m", actor->init_attribute_amount_max));
            
            db.add_contract_handler_exe_point(5);

            //create attributes
            db.initialize_actor_attributes( *actor, value_list );
            
            db.add_contract_handler_exe_point(10);

            //born on zone
            db.born_actor( *actor, gender, sexuality, *zone );
            
            //设定从属
            db.modify( *actor, [&]( actor_object& obj ) {
                obj.base = zone->id;
            });
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=========================================================================
    string contract_handler::move_actor(const string& actor_name, const string& zone_name)
    {
        try
        {
            db.add_contract_handler_exe_point(2);

            const auto* actor = db.find<actor_object, by_name>(actor_name);
            if(actor == nullptr)
                return FORMAT_MESSAGE("名为${n}的角色不存在", ("n", actor_name));
            if(!actor->born)
                return FORMAT_MESSAGE("${a}还未出生", ("a", actor_name));
            if(actor->health <= 0)
                return FORMAT_MESSAGE("${a}已经去世了", ("a", actor_name));

            const auto& actor_nfa = db.get<nfa_object, by_id>(actor->nfa_id);
            if(actor_nfa.owner_account != caller.id && actor_nfa.active_account != caller.id)
                return FORMAT_MESSAGE("无权操作角色");

            const auto& current_zone = db.get< zone_object, by_id >(actor->location);
            if(current_zone.name == zone_name)
                return FORMAT_MESSAGE("${a}已经在${z}了", ("a", actor_name)("z", zone_name));

            const auto* target_zone = db.find< zone_object, by_name >( zone_name );
            if(target_zone == nullptr)
                return FORMAT_MESSAGE("没有名叫\"${a}\"的地方", ("a", zone_name));
                    
            const auto* zone_connection = db.find< zone_connect_object, by_zone_from >(boost::make_tuple(actor->location, target_zone->id));
            if(zone_connection == nullptr)
                return FORMAT_MESSAGE("${a}无法直接通往${b}", ("a", current_zone.name)("b", target_zone->name));
            
            db.add_contract_handler_exe_point(3);

            int take_days = db.calculate_moving_days_to_zone(*target_zone);
            int64_t take_qi = take_days * TAIYI_USEMANA_ACTOR_ACTION_SCALE;
            if( take_qi > actor_nfa.qi.amount.value )
                return FORMAT_MESSAGE("需要气力${p}（${t}天），但气力只剩余${c}了", ("p", take_qi)("t", take_days)("c", actor_nfa.qi.amount.value));
            //reward take_qi to treasury
            db.reward_feigang(db.get<account_object, by_name>(TAIYI_TREASURY_ACCOUNT), actor_nfa, asset(take_qi, QI_SYMBOL));

            //finish movement
            auto now = db.head_block_time();
            db.modify( *actor, [&]( actor_object& act ) {
                act.location = target_zone->id;
                act.last_update = now;
            });
            
            //以目的地视角回调目的地函数：on_actor_enter
            lua_map param;
            param[lua_key(lua_int(1))] = lua_types(lua_int(actor_nfa.id));
            call_nfa_function(target_zone->nfa_id, "on_actor_enter", param);
            
            db.push_virtual_operation( actor_movement_operation( caller.name, actor->name, current_zone.name, target_zone->name, actor_nfa.id ) );
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
        
        return "";
    }
    //=========================================================================
    //返回错误信息
    string contract_handler::exploit_zone(const string& actor_name, const string& zone_name)
    {
        try
        {
            db.add_contract_handler_exe_point(5);

            const auto* actor = db.find<actor_object, by_name>(actor_name);
            if(actor == nullptr)
                return FORMAT_MESSAGE("名为${n}的角色不存在", ("n", actor_name));
            if(!actor->born)
                return FORMAT_MESSAGE("${a}还未出生", ("a", actor_name));
            if(actor->health <= 0)
                return FORMAT_MESSAGE("${a}已经去世了", ("a", actor_name));

            const auto& actor_nfa = db.get<nfa_object, by_id>(actor->nfa_id);
            if(actor_nfa.owner_account != caller.id && actor_nfa.active_account != caller.id)
                return FORMAT_MESSAGE("无权操作角色");

            const auto* target_zone = db.find< zone_object, by_name >( zone_name );
            if(target_zone == nullptr)
                return FORMAT_MESSAGE("没有名叫\"${a}\"的地方", ("a", zone_name));

            const auto& current_zone = db.get< zone_object, by_id >(actor->location);
            if(current_zone.name != zone_name)
                return FORMAT_MESSAGE("${a}不能探索${z}，因为${a}不在${z}", ("a", actor_name)("z", zone_name));
                    
            int hard_level = 1; //TODO: 根据区域类型有不同难度
            int64_t take_qi = hard_level * 1000 * TAIYI_USEMANA_ACTOR_ACTION_SCALE;
            if( take_qi > actor_nfa.qi.amount.value )
                return FORMAT_MESSAGE("需要气力${p}，但气力只剩余${c}了", ("p", take_qi)("c", actor_nfa.qi.amount.value));
            //reward take_qi to treasury
            db.reward_feigang(db.get<account_object, by_name>(TAIYI_TREASURY_ACCOUNT), actor_nfa, asset(take_qi, QI_SYMBOL));

            //以目的地视角回调目的地函数：on_actor_exploit
            lua_map param;
            param[lua_key(lua_int(1))] = lua_types(lua_int(actor_nfa.id));
            call_nfa_function_with_caller(db.get_account(TAIYI_DANUO_ACCOUNT), target_zone->nfa_id, "on_actor_exploit", param, false);
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
        
        return "";
    }
    //=========================================================================
    int64_t contract_handler::create_cultivation(int64_t nfa_id, const lua_map& beneficiary_nfa_ids, const lua_map& beneficiary_shares, uint32_t prepare_time_blocks)
    {
        try
        {
            db.add_contract_handler_exe_point(5);

            const auto& manager_nfa = db.get<nfa_object, by_id>(nfa_id);
            FC_ASSERT(manager_nfa.owner_account == caller.id || manager_nfa.active_account == caller.id, "无权操作NFA");
            
            FC_ASSERT(beneficiary_nfa_ids.size() == beneficiary_shares.size(), "受益者ID列表和分配列表不匹配");
            chainbase::t_flat_map<nfa_id_type, uint> beneficiaries_map;
            uint64_t total_share_check = 0;
            for (size_t i=0; i<beneficiary_nfa_ids.size(); ++i) {
                auto itr_id = beneficiary_nfa_ids.find(lua_key(lua_int(i+1)));
                FC_ASSERT(itr_id != beneficiary_nfa_ids.end(), "受益者ID列表参数${i}无效", ("i", i));
                FC_ASSERT(itr_id->second.which() == lua_key_variant::tag<lua_int>::value, "受益者ID列表参数${i}类型错误", ("i", i));
                const auto* check_nfa = db.find<nfa_object, by_id>(itr_id->second.get<lua_int>().v);
                FC_ASSERT(check_nfa != nullptr, "传入不存在的受益者NFA ID（参数${i}）", ("i", i));
                
                auto itr_share = beneficiary_shares.find(lua_key(lua_int(i+1)));
                FC_ASSERT(itr_share != beneficiary_shares.end(), "受益者分配比率列表参数${i}无效", ("i", i));
                FC_ASSERT(itr_share->second.which() == lua_key_variant::tag<lua_int>::value, "传入受益者分配比率表参数${i}类型错误", ("i", i));
                int64_t share = itr_share->second.get<lua_int>().v;
                FC_ASSERT(share > 0, "传入受益者分配比率表参数${i}无效", ("i", i));
                beneficiaries_map[check_nfa->id] = uint(share);
                total_share_check += share;
            }
                        
            FC_ASSERT(total_share_check == TAIYI_100_PERCENT, "传入受益者分配比率总合不等于${a}", ("a", TAIYI_100_PERCENT));
            
            FC_ASSERT(prepare_time_blocks >= TAIYI_CULTIVATION_PREPARE_MIN_TIME_BLOCK_NUM, "传入准备时间不够，必须大于${a}息", ("a", TAIYI_CULTIVATION_PREPARE_MIN_TIME_BLOCK_NUM));
            
            return db.create_cultivation(manager_nfa, beneficiaries_map, prepare_time_blocks).id;

        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }
    //=========================================================================
    string contract_handler::participate_cultivation(int64_t cult_id, int64_t nfa_id, uint64_t value)
    {
        try
        {
            db.add_contract_handler_exe_point(2);

            const auto* cult = db.find<cultivation_object, by_id>(cult_id);
            if(cult == nullptr)
                return "指定修真活动不存在";
            if(cult->start_time != std::numeric_limits<uint32_t>::max())
                return "修真活动已经开始，不能再加入了";
            
            const auto& nfa = db.get<nfa_object, by_id>(nfa_id);
            if(nfa.owner_account != caller.id && nfa.active_account != caller.id)
                return "无权操作NFA";
            if(nfa.cultivation_value != 0)
                return "指定参与者已经在修真了，不能再参加新的修真活动";
            if(value == 0)
                return "参与者没有设置参与修真活动的有效真气";
            if(nfa.qi.amount.value < value)
                return "参与者体内真气不够";
            
            db.participate_cultivation(*cult, nfa, value);
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
        return "";
    }
    //=========================================================================
    string contract_handler::start_cultivation(int64_t cult_id)
    {
        try
        {
            db.add_contract_handler_exe_point(2);

            const auto* cult = db.find<cultivation_object, by_id>(cult_id);
            if(cult == nullptr)
                return "指定修真活动不存在";
            const auto& manager_nfa = db.get<nfa_object, by_id>(cult->manager_nfa_id);
            if(manager_nfa.owner_account != caller.id && manager_nfa.active_account != caller.id)
                return "无权操作修真活动";
            
            if(cult->start_time != std::numeric_limits<uint32_t>::max())
                return "指定修真活动已经开始";
            if(cult->participants.empty())
                return "指定修真活动没有任何参与者";

            db.start_cultivation(*cult);
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
        return "";
    }
    //=========================================================================
    string contract_handler::stop_and_close_cultivation(int64_t cult_id)
    {
        try
        {
            db.add_contract_handler_exe_point(5);

            const auto* cult = db.find<cultivation_object, by_id>(cult_id);
            if(cult == nullptr)
                return "指定修真活动不存在";
            const auto& manager_nfa = db.get<nfa_object, by_id>(cult->manager_nfa_id);
            if(manager_nfa.owner_account != caller.id && manager_nfa.active_account != caller.id)
                return "无权操作修真活动";
            
            if(cult->start_time != std::numeric_limits<uint32_t>::max()) {
                //修真活动已经开始，结束修真获得奖励
                db.stop_cultivation(*cult);
            }
            else {
                //修真活动还未开始，直接解散
                db.dissolve_cultivation(*cult);
            }
            
            db.remove(*cult);
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
        return "";
    }
    //=========================================================================
    bool contract_handler::is_cultivation_exist(int64_t cult_id)
    {
        const auto* cult = db.find<cultivation_object, by_id>(cult_id);
        return cult != nullptr;
    }
    //=========================================================================
    void contract_handler::create_named_contract(const string& name, const string& code)
    {
        try
        {
            db.add_contract_handler_exe_point(5);

            const auto& creator = caller;
            long long old_drops = creator.qi.amount.value / TAIYI_USEMANA_EXECUTION_SCALE;
            long long vm_drops = old_drops;
            size_t new_state_size = db.create_contract_objects( creator, name, code, vm_drops );
            int64_t used_drops = old_drops - vm_drops;
            
            int64_t used_qi = used_drops * TAIYI_USEMANA_EXECUTION_SCALE + new_state_size * TAIYI_USEMANA_STATE_BYTES_SCALE + 100 * TAIYI_USEMANA_EXECUTION_SCALE;
            FC_ASSERT( creator.qi.amount.value >= used_qi, "神识没有足够的真气创建天道合约，需要真气${n}", ("n", used_qi) );
            
            //reward to treasury
            db.reward_feigang(db.get<account_object, by_name>(TAIYI_TREASURY_ACCOUNT), creator, asset(used_qi, QI_SYMBOL));
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(context.mState, e.to_string());
        }
    }

} } // namespace taiyi::chain

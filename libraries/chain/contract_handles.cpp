#include <chain/taiyi_fwd.hpp>

#include <chain/taiyi_evaluator.hpp>
#include <chain/database.hpp>

#include <chain/taiyi_objects.hpp>
#include <chain/block_summary_object.hpp>
#include <chain/account_object.hpp>
#include <chain/contract_objects.hpp>

#include <chain/lua_context.hpp>

#include <chain/contract_handles.hpp>

namespace taiyi { namespace chain {

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

} } // namespace taiyi::chain

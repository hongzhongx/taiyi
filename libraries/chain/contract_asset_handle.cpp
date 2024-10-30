#include <chain/taiyi_fwd.hpp>

#include <chain/taiyi_evaluator.hpp>
#include <chain/database.hpp>
#include <chain/taiyi_objects.hpp>
#include <chain/block_summary_object.hpp>
#include <chain/account_object.hpp>

#include <chain/lua_context.hpp>
#include <chain/contract_objects.hpp>
#include <chain/contract_handles.hpp>
#include <chain/contract_worker.hpp>

#include <lua.hpp>

#include <fc/macros.hpp>

namespace taiyi { namespace chain {

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

    void contract_handler::transfer_from(account_id_type from, account_name_type to, double amount, string symbol_or_id, bool enable_logger)
    {
        try
        {
            account_id_type account_to = db.get_account(to).id;
            asset_symbol_type symbol = s_get_symbol_type_from_string(symbol_or_id);
            auto token = asset(amount, symbol);
            transfer_by_contract(from, account_to, token, result, enable_logger);
        }
        catch (fc::exception e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(this->context.mState, e.to_string());
        }
    }

} } // namespace taiyi::chain

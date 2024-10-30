#include <lua.hpp>

#include <chain/taiyi_fwd.hpp>
#include <chain/taiyi_evaluator.hpp>
#include <chain/database.hpp>

#include <chain/taiyi_objects.hpp>
#include <chain/block_summary_object.hpp>
#include <chain/account_object.hpp>
#include <chain/contract_objects.hpp>

#include <chain/lua_context.hpp>
#include <chain/contract_handles.hpp>

#include <fc/macros.hpp>

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

} } // namespace taiyi::chain

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
    void contract_nfa_handler::enable_tick()
    {
        try
        {
            _db.modify(_caller, [&]( nfa_object& obj ) {
                obj.next_tick_time = time_point_sec::min();
            });
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::disable_tick()
    {
        try
        {
            _db.modify(_caller, [&]( nfa_object& obj ) {
                obj.next_tick_time = time_point_sec::maximum();
            });
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }

} } // namespace taiyi::chain

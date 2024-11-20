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
    : id(nfa.id), qi(nfa.qi.amount.value), parent(nfa.parent), data(nfa.contract_data)
    {
        const auto& nfa_symbol = db.get<nfa_symbol_object, by_id>(nfa.symbol_id);
        symbol = nfa_symbol.symbol;
        
        const auto& owner = db.get<account_object, by_id>(nfa.owner_account);
        owner_account = owner.name;
    }
    //=============================================================================
    contract_nfa_handler::contract_nfa_handler(const account_object& caller_account, const nfa_object& caller, LuaContext &context, database &db, contract_handler& ch)
        : _caller_account(caller_account), _caller(caller), _context(context), _db(db), _ch(ch)
    {
        _contract_data_cache = _caller.contract_data;
    }
    //=============================================================================
    contract_nfa_handler::~contract_nfa_handler()
    {
        _db.modify(_caller, [&](nfa_object& obj){ obj.contract_data = _contract_data_cache; });
    }
    //=============================================================================
    void contract_nfa_handler::enable_tick()
    {
        try
        {
            FC_ASSERT(_caller.owner_account == _caller_account.id, "caller account not the owner");
            
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
            FC_ASSERT(_caller.owner_account == _caller_account.id, "caller account not the owner");

            _db.modify(_caller, [&]( nfa_object& obj ) {
                obj.next_tick_time = time_point_sec::maximum();
            });
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    contract_nfa_base_info contract_nfa_handler::get_info()
    {
        try
        {
            return contract_nfa_base_info(_caller, _db);
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    contract_asset_resources contract_nfa_handler::get_resources()
    {
        try
        {
            return contract_asset_resources(_caller, _db);
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=========================================================================
    void contract_nfa_handler::convert_qi_to_resource(int64_t amount, string resource_symbol_name)
    {
        try
        {
            FC_ASSERT(_caller.owner_account == _caller_account.id, "caller account not the owner");

            asset qi(amount, QI_SYMBOL);
            FC_ASSERT(_db.get_nfa_balance(_caller, QI_SYMBOL) >= qi, "NFA not have enough qi to convert.");
            
            asset_symbol_type symbol = s_get_symbol_type_from_string(resource_symbol_name);
            price p;
            switch (symbol.asset_num) {
                case TAIYI_ASSET_NUM_GOLD:
                    p = TAIYI_GOLD_QI_PRICE;
                    break;
                case TAIYI_ASSET_NUM_FOOD:
                    p = TAIYI_FOOD_QI_PRICE;
                    break;
                case TAIYI_ASSET_NUM_WOOD:
                    p = TAIYI_WOOD_QI_PRICE;
                    break;
                case TAIYI_ASSET_NUM_FABRIC:
                    p = TAIYI_FABRIC_QI_PRICE;
                    break;
                case TAIYI_ASSET_NUM_HERB:
                    p = TAIYI_HERB_QI_PRICE;
                    break;
                default:
                    FC_ASSERT(false, "NFA can not convert qi to asset (${a}) which is not resource.", ("a", resource_symbol_name));
                    break;
            }
            asset new_resource = qi * p;
            
            const auto& caller_owner = _db.get<account_object, by_id>(_caller.owner_account);
            operation vop = nfa_convert_qi_to_resources_operation( _caller.id, caller_owner.name, qi, new_resource );
            _db.pre_push_virtual_operation( vop );

            _db.adjust_nfa_balance(_caller, -qi);
            _db.adjust_nfa_balance(_caller, new_resource);
            
            _db.post_push_virtual_operation( vop );
        }
        catch (fc::exception e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=========================================================================    
    void contract_nfa_handler::add_child(int64_t nfa_id)
    {
        try
        {
            FC_ASSERT(_caller.owner_account == _caller_account.id, "caller account not the owner");

            const auto& child_nfa = _db.get<nfa_object, by_id>(nfa_id);
            FC_ASSERT(child_nfa.owner_account == _caller_account.id, "caller account not the input nfa's owner");
            FC_ASSERT(child_nfa.parent == nfa_id_type::max(), "input nfa already have parent");
                        
            _db.modify(child_nfa, [&]( nfa_object& obj ) {
                obj.parent = _caller.id;
            });
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::add_to_parent(int64_t parent_nfa_id)
    {
        try
        {
            FC_ASSERT(_caller.owner_account == _caller_account.id, "caller account not the owner");
            FC_ASSERT(_caller.parent == nfa_id_type::max(), "caller already have parent");

            const auto& parent_nfa = _db.get<nfa_object, by_id>(parent_nfa_id);
            FC_ASSERT(parent_nfa.owner_account == _caller_account.id, "caller account not the input nfa's owner");
                        
            _db.modify(_caller, [&]( nfa_object& obj ) {
                obj.parent = parent_nfa.id;
            });
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::remove_from_parent()
    {
        try
        {
            FC_ASSERT(_caller.owner_account == _caller_account.id, "caller account not the owner");
            FC_ASSERT(_caller.parent != nfa_id_type::max(), "caller have no parent");

            const auto& parent_nfa = _db.get<nfa_object, by_id>(_caller.parent);
            FC_ASSERT(parent_nfa.owner_account == _caller_account.id, "caller account not the parent nfa's owner");
                        
            _db.modify(_caller, [&]( nfa_object& obj ) {
                obj.parent = nfa_id_type::max();
            });
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::transfer_from(nfa_id_type from, nfa_id_type to, double amount, string symbol_name, bool enable_logger)
    {
        try
        {
            asset_symbol_type symbol = s_get_symbol_type_from_string(symbol_name);
            auto token = asset(amount, symbol);
            transfer_by_contract(from, to, token, _ch.result, enable_logger);
        }
        catch (fc::exception e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::transfer_by_contract(nfa_id_type from, nfa_id_type to, asset token, contract_result &result, bool enable_logger)
    {
        try
        {
            FC_ASSERT(from != to, "It's no use transferring resource to yourself");
            const nfa_object &from_nfa = _db.get<nfa_object, by_id>(from);
            const nfa_object &to_nfa = _db.get<nfa_object, by_id>(to);
            try
            {
                FC_ASSERT(_db.get_nfa_balance(from_nfa, token.symbol).amount >= token.amount, "Insufficient Balance: ${balance}, unable to transfer '${total_transfer}' from nfa '${a}' to '${t}'", ("a", from_nfa.id)("t", to_nfa.id)("total_transfer", token)("balance", _db.get_nfa_balance(from_nfa, token.symbol)));
            }
            FC_RETHROW_EXCEPTIONS(error, "Unable to transfer ${a} from ${f} to ${t}", ("a", token)("f", from_nfa.id)("t", to_nfa.id));
            
            FC_ASSERT(token.amount >= share_type(0), "resource amount must big than zero");

            _db.adjust_nfa_balance(from_nfa, -token);
            _db.adjust_nfa_balance(to_nfa, token);
            
            if(enable_logger) {
                variant v;
                fc::to_variant(token, v);
                _ch.log(FORMAT_MESSAGE("nfa #${a} transfer ${token} to nfa #${b}", ("a", from_nfa.id)("token", fc::json::to_string(v))("b", to_nfa.id)));
            }
        }
        catch (fc::exception e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::read_chain(const lua_map& read_list)
    {
        try
        {
            vector<lua_types> stacks = { lua_string("nfa_data") };
            _ch.read_context(read_list, _contract_data_cache, stacks, _ch.contract.name);
        }
        catch (fc::exception e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
        catch (std::exception e)
        {
            wdump((e.what()));
            LUA_C_ERR_THROW(_context.mState, e.what());
        }
    }
    //=============================================================================
    void contract_nfa_handler::write_chain(const lua_map& write_list)
    {
        try
        {
            vector<lua_types> stacks = { lua_string("nfa_data") };
            _ch.flush_context(write_list, _contract_data_cache, stacks, _ch.contract.name);
        }
        catch (fc::exception e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
        catch (std::exception e)
        {
            wdump((e.what()));
            LUA_C_ERR_THROW(_context.mState, e.what());
        }
    }

} } // namespace taiyi::chain

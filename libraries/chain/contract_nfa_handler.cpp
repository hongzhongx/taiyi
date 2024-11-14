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
        : id(nfa.id), qi(nfa.qi.amount.value), data(nfa.data)
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
    void contract_nfa_handler::set_data(const lua_map& data)
    {
        try
        {
            FC_ASSERT(_caller.owner_account == _caller_account.id, "caller account not the owner");
            
            _db.modify(_caller, [&]( nfa_object& obj ) {
                obj.data = data;
            });
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=========================================================================
    void modify_nfa_children_owner(const nfa_object& nfa, const account_object& new_owner, std::set<nfa_id_type>& recursion_loop_check, database& db)
    {
        for (auto _id: nfa.children) {
            if(recursion_loop_check.find(_id) != recursion_loop_check.end())
                continue;

            const auto& child_nfa = db.get<nfa_object, by_id>(_id);
            db.modify(child_nfa, [&]( nfa_object& obj ) {
                obj.owner_account = new_owner.id;
            });
            
            recursion_loop_check.insert(_id);
            
            modify_nfa_children_owner(child_nfa, new_owner, recursion_loop_check, db);
        }
    }
    
    void contract_nfa_handler::add_child(int64_t nfa_id)
    {
        try
        {
            FC_ASSERT(_caller.owner_account == _caller_account.id, "caller account not the owner");

            const auto& child_nfa = _db.get<nfa_object, by_id>(nfa_id);
            FC_ASSERT(child_nfa.owner_account == _caller_account.id, "caller account not the input nfa's owner");
            
            FC_ASSERT(child_nfa.parent == std::numeric_limits<nfa_id_type>::max(), "input nfa already have parent");
            FC_ASSERT(std::find(_caller.children.begin(), _caller.children.end(), nfa_id_type(nfa_id)) == _caller.children.end(), "input nfa is already child");
            
            _db.modify(_caller, [&]( nfa_object& obj ) {
                obj.children.push_back(nfa_id);
            });
            
            _db.modify(child_nfa, [&]( nfa_object& obj ) {
                obj.parent = _caller.id;
            });
            
            //TODO: 转移子节点里面所有子节点的所有权，在NFA增加使用权账号后，是否也要转移使用权？
            std::set<nfa_id_type> look_checker;
            modify_nfa_children_owner(child_nfa, _caller_account, look_checker, _db);
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    
} } // namespace taiyi::chain

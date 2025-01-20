#include <chain/taiyi_fwd.hpp>

#include <chain/taiyi_evaluator.hpp>
#include <chain/database.hpp>

#include <chain/taiyi_objects.hpp>
#include <chain/block_summary_object.hpp>
#include <chain/account_object.hpp>
#include <chain/contract_objects.hpp>
#include <chain/nfa_objects.hpp>
#include <chain/actor_objects.hpp>

#include <chain/lua_context.hpp>

#include <chain/contract_handles.hpp>

namespace taiyi { namespace chain {

    contract_nfa_base_info::contract_nfa_base_info(const nfa_object& nfa, database& db)
    : id(nfa.id), qi(nfa.qi.amount.value), parent(nfa.parent), data(nfa.contract_data)
    {
        const auto& nfa_symbol = db.get<nfa_symbol_object, by_id>(nfa.symbol_id);
        symbol = nfa_symbol.symbol;
        
        owner_account = db.get<account_object, by_id>(nfa.owner_account).name;
        creator_account = db.get<account_object, by_id>(nfa.creator_account).name;
        active_account = db.get<account_object, by_id>(nfa.active_account).name;
        
        five_phase = db.get_nfa_five_phase(nfa);
    }
    //=============================================================================
    contract_nfa_handler::contract_nfa_handler(const account_object& caller_account, const nfa_object& caller, LuaContext &context, database &db, contract_handler& ch)
        : _caller_account(caller_account), _caller(caller), _context(context), _db(db), _ch(ch)
    {
    }
    //=============================================================================
    contract_nfa_handler::~contract_nfa_handler()
    {
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
    //=========================================================================
    contract_asset_resources contract_nfa_handler::get_resources()
    {
        try
        {
            return contract_asset_resources(_caller, _db, false);
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=========================================================================
    contract_asset_resources contract_nfa_handler::get_materials()
    {
        try
        {
            return contract_asset_resources(_caller, _db, true);
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
            FC_ASSERT(_caller.owner_account == _caller_account.id || _caller.active_account == _caller_account.id, "caller account not the owner or active operator");
            FC_ASSERT(amount > 0, "convert qi is zero");

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
            qi = new_resource * p; //计算实际消耗的气，避免整型除法导致的误差
            
            const auto& caller_owner = _db.get<account_object, by_id>(_caller.owner_account);
            operation vop = nfa_convert_qi_to_resources_operation( _caller.id, caller_owner.name, qi, new_resource );
            _db.pre_push_virtual_operation( vop );

            _db.adjust_nfa_balance(_caller, -qi);
            _db.adjust_nfa_balance(_caller, new_resource);
            
            _db.post_push_virtual_operation( vop );
            
            //更新统计自由真气量和统计资源
            _db.modify( _db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo ) {
                gpo.total_qi -= qi;
                switch (symbol.asset_num) {
                    case TAIYI_ASSET_NUM_GOLD:
                        gpo.total_gold += new_resource;
                        break;
                    case TAIYI_ASSET_NUM_FOOD:
                        gpo.total_food += new_resource;
                        break;
                    case TAIYI_ASSET_NUM_WOOD:
                        gpo.total_wood += new_resource;
                        break;
                    case TAIYI_ASSET_NUM_FABRIC:
                        gpo.total_fabric += new_resource;
                        break;
                    case TAIYI_ASSET_NUM_HERB:
                        gpo.total_herb += new_resource;
                        break;
                    default:
                        FC_ASSERT(false, "can not be here", ("a", resource_symbol_name));
                        break;
                }
            });
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
            FC_ASSERT(_caller.owner_account == _caller_account.id || _caller.active_account == _caller_account.id, "caller account not the owner or active operator");

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
            FC_ASSERT(_caller.owner_account == _caller_account.id || _caller.active_account == _caller_account.id, "caller account not the owner or active operator");
            FC_ASSERT(_caller.parent == nfa_id_type::max(), "caller already have parent");

            const auto& parent_nfa = _db.get<nfa_object, by_id>(parent_nfa_id);
            FC_ASSERT(parent_nfa.owner_account == _caller_account.id, "caller account not the parent nfa's owner");
                        
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
            FC_ASSERT(_caller.owner_account == _caller_account.id || _caller.active_account == _caller_account.id, "caller account not the owner or active operator");
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
    void contract_nfa_handler::transfer_from(nfa_id_type from, nfa_id_type to, double amount, const string& symbol_name, bool enable_logger)
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
            FC_ASSERT(from_nfa.owner_account == _caller_account.id || from_nfa.active_account == _caller_account.id, "caller account not the owner or active operator");
            const nfa_object &to_nfa = _db.get<nfa_object, by_id>(to);
            try
            {
                FC_ASSERT(_db.get_nfa_balance(from_nfa, token.symbol).amount >= token.amount, "Insufficient Balance: ${balance}, unable to transfer '${token}' from nfa #${a} to nfa #${t}", ("a", from_nfa.id)("t", to_nfa.id)("token", token)("balance", _db.get_nfa_balance(from_nfa, token.symbol)));
            }
            FC_RETHROW_EXCEPTIONS(error, "Unable to transfer ${a} from ${f} to ${t}", ("a", token)("f", from_nfa.id)("t", to_nfa.id));
            
            FC_ASSERT(token.amount >= share_type(0), "resource amount must big than zero");

            operation vop = nfa_trasfer_operation(from, _db.get<account_object, by_id>(from_nfa.owner_account).name, to, _db.get<account_object, by_id>(to_nfa.owner_account).name, token );
            _db.pre_push_virtual_operation( vop );
            
            _db.adjust_nfa_balance(from_nfa, -token);
            _db.adjust_nfa_balance(to_nfa, token);

            _db.post_push_virtual_operation( vop );
            
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
    void contract_nfa_handler::withdraw_to(nfa_id_type from, const account_name_type& to, double amount, const string& symbol_name, bool enable_logger)
    {
        try
        {
            asset_symbol_type symbol = s_get_symbol_type_from_string(symbol_name);
            auto token = asset(amount, symbol);
            withdraw_by_contract(from, _db.get_account(to).id, token, _ch.result, enable_logger);
        }
        catch (fc::exception e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::withdraw_by_contract(nfa_id_type from, account_id_type to, asset token, contract_result &result, bool enable_logger)
    {
        try
        {
            const nfa_object &from_nfa = _db.get<nfa_object, by_id>(from);
            FC_ASSERT(from_nfa.owner_account == _caller_account.id || from_nfa.active_account == _caller_account.id, "caller account not the owner or active operator");
            const account_object &to_account = _db.get<account_object, by_id>(to);
            try
            {
                FC_ASSERT(_db.get_nfa_balance(from_nfa, token.symbol).amount >= token.amount, "Insufficient Balance: ${balance}, unable to transfer '${token}' from nfa #${a} to account '${t}'", ("a", from_nfa.id)("t", to_account.name)("token", token)("balance", _db.get_nfa_balance(from_nfa, token.symbol)));
            }
            FC_RETHROW_EXCEPTIONS(error, "Unable to transfer ${a} from nfa #${f} to ${t}", ("a", token)("f", from_nfa.id)("t", to_account.name));
            
            FC_ASSERT(token.amount >= share_type(0), "resource amount must big than zero");

            operation vop = nfa_deposit_withdraw_operation(from, to_account.name, asset( 0, YANG_SYMBOL ), token );
            _db.pre_push_virtual_operation( vop );

            _db.adjust_nfa_balance(from_nfa, -token);
            _db.adjust_balance(to_account, token);

            _db.post_push_virtual_operation( vop );

            if(enable_logger) {
                variant v;
                fc::to_variant(token, v);
                _ch.log(FORMAT_MESSAGE("nfa #${a} transfer ${token} to account ${b}", ("a", from_nfa.id)("token", fc::json::to_string(v))("b", to_account.name)));
            }
        }
        catch (fc::exception e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::deposit_from(const account_name_type& from, nfa_id_type to, double amount, const string& symbol_name, bool enable_logger)
    {
        try
        {
            asset_symbol_type symbol = s_get_symbol_type_from_string(symbol_name);
            auto token = asset(amount, symbol);
            deposit_by_contract(_db.get_account(from).id, to, token, _ch.result, enable_logger);
        }
        catch (fc::exception e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::deposit_by_contract(account_id_type from, nfa_id_type to, asset token, contract_result &result, bool enable_logger)
    {
        try
        {
            FC_ASSERT(_caller_account.id == from, "caller have no authority to transfer assets from this account");
            const account_object &from_account = _db.get<account_object, by_id>(from);
            const nfa_object &to_nfa = _db.get<nfa_object, by_id>(to);
            try
            {
                FC_ASSERT(_db.get_balance(from_account, token.symbol).amount >= token.amount, "Insufficient Balance: ${balance}, unable to transfer '${token}' from account ${a} to nfa #'${t}'", ("a", from_account.name)("t", to_nfa.id)("token", token)("balance", _db.get_balance(from_account, token.symbol)));
            }
            FC_RETHROW_EXCEPTIONS(error, "Unable to transfer ${a} from account ${f} to nfa #${t}", ("a", token)("f", from_account.name)("t", to_nfa.id));
            
            FC_ASSERT(token.amount >= share_type(0), "resource amount must big than zero");

            operation vop = nfa_deposit_withdraw_operation(to, from_account.name, token, asset( 0, YANG_SYMBOL ) );
            _db.pre_push_virtual_operation( vop );

            _db.adjust_balance(from_account, -token);
            _db.adjust_nfa_balance(to_nfa, token);

            _db.post_push_virtual_operation( vop );

            if(enable_logger) {
                variant v;
                fc::to_variant(token, v);
                _ch.log(FORMAT_MESSAGE("account ${a} transfer ${token} to nfa #${b}", ("a", from_account.name)("token", fc::json::to_string(v))("b", to_nfa.id)));
            }
        }
        catch (fc::exception e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    lua_map contract_nfa_handler::read_contract_data(const lua_map& read_list)
    {
        try
        {
            return _ch.read_nfa_contract_data(_caller.id, read_list);
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
    void contract_nfa_handler::write_contract_data(const lua_map& data, const lua_map& write_list)
    {
        try
        {
            FC_ASSERT(_caller.owner_account == _caller_account.id || _caller.active_account == _caller_account.id, "caller account not the owner or active operator");
            
            _ch.write_nfa_contract_data(_caller.id, data, write_list);
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
    void contract_nfa_handler::destroy()
    {
        try
        {
            FC_ASSERT(_caller.owner_account == _caller_account.id, "caller account not the owner");
            FC_ASSERT(_caller.parent == nfa_id_type::max(), "caller with parent can not be destroyed");
            
            const auto& nfa_by_parent_idx = _db.get_index< nfa_index >().indices().get< chain::by_parent >();
            auto itn = nfa_by_parent_idx.lower_bound( _caller.id );
            FC_ASSERT(itn == nfa_by_parent_idx.end() || itn->parent != _caller.id, "caller with child can not be destroyed");
                        
            _db.modify(_caller, [&]( nfa_object& obj ) {
                obj.owner_account = _db.get_account(TAIYI_NULL_ACCOUNT).id;
                obj.active_account = obj.owner_account;
                obj.next_tick_time = time_point_sec::maximum(); //disable tick
            });
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::modify_actor_attributes(const lua_map& values)
    {
        try
        {
            FC_ASSERT(_caller.owner_account == _caller_account.id || _caller.active_account == _caller_account.id, "caller account not the owner or active operator");

            auto now = _db.head_block_time();

            static string health_key    = "health";
            static string strength_key  = "strength";
            static string physique_key  = "physique";
            static string agility_key   = "agility";
            static string vitality_key  = "vitality";
            static string comprehension_key  = "comprehension";
            static string willpower_key = "willpower";
            static string charm_key     = "charm";
            static string mood_key      = "mood";

            
            const actor_object& actor = _db.get< actor_object, by_nfa_id >( _caller.id );
            const actor_core_attributes_object& core_attrs = _db.get< actor_core_attributes_object, by_actor >( actor.id );

            for (auto itr = values.begin(); itr != values.end(); itr++)
            {
                lua_types key = itr->first.cast_to_lua_types();
                if (key.which() == lua_types::tag<lua_string>::value) {
                    auto k = key.get<lua_string>().v;
                    if( k == strength_key) {
                        FC_ASSERT(itr->second.which() == lua_types::tag<lua_int>::value, "attributes value type invalid (must be int)");
                        auto v = itr->second.get<lua_int>().v;
                        if(v != 0) {
                            _db.modify(core_attrs, [&]( actor_core_attributes_object& attrs ) {
                                attrs.strength_max = std::max<int16_t>(0, attrs.strength_max + v);
                                attrs.last_update = now;
                            });
                        }
                    }
                    else if( k == physique_key) {
                        FC_ASSERT(itr->second.which() == lua_types::tag<lua_int>::value, "attributes value type invalid (must be int)");
                        auto v = itr->second.get<lua_int>().v;
                        if(v != 0) {
                            _db.modify(core_attrs, [&]( actor_core_attributes_object& attrs ) {
                                attrs.physique_max = std::max<int16_t>(0, attrs.physique_max + v);
                                attrs.last_update = now;
                            });
                        }
                    }
                    else if( k == agility_key) {
                        FC_ASSERT(itr->second.which() == lua_types::tag<lua_int>::value, "attributes value type invalid (must be int)");
                        auto v = itr->second.get<lua_int>().v;
                        if(v != 0) {
                            _db.modify(core_attrs, [&]( actor_core_attributes_object& attrs ) {
                                attrs.agility_max = std::max<int16_t>(0, attrs.agility_max + v);
                                attrs.last_update = now;
                            });
                        }
                    }
                    else if( k == vitality_key) {
                        FC_ASSERT(itr->second.which() == lua_types::tag<lua_int>::value, "attributes value type invalid (must be int)");
                        auto v = itr->second.get<lua_int>().v;
                        if(v != 0) {
                            _db.modify(core_attrs, [&]( actor_core_attributes_object& attrs ) {
                                attrs.vitality_max = std::max<int16_t>(0, attrs.vitality_max + v);
                                attrs.last_update = now;
                            });
                        }
                    }
                    else if( k == comprehension_key) {
                        FC_ASSERT(itr->second.which() == lua_types::tag<lua_int>::value, "attributes value type invalid (must be int)");
                        auto v = itr->second.get<lua_int>().v;
                        if(v != 0) {
                            _db.modify(core_attrs, [&]( actor_core_attributes_object& attrs ) {
                                attrs.comprehension_max = std::max<int16_t>(0, attrs.comprehension_max + v);
                                attrs.last_update = now;
                            });
                        }
                    }
                    else if( k == willpower_key) {
                        FC_ASSERT(itr->second.which() == lua_types::tag<lua_int>::value, "attributes value type invalid (must be int)");
                        auto v = itr->second.get<lua_int>().v;
                        if(v != 0) {
                            _db.modify(core_attrs, [&]( actor_core_attributes_object& attrs ) {
                                attrs.willpower_max = std::max<int16_t>(0, attrs.willpower_max + v);
                                attrs.last_update = now;
                            });
                        }
                    }
                    else if( k == charm_key) {
                        FC_ASSERT(itr->second.which() == lua_types::tag<lua_int>::value, "attributes value type invalid (must be int)");
                        auto v = itr->second.get<lua_int>().v;
                        if(v != 0) {
                            _db.modify(core_attrs, [&]( actor_core_attributes_object& attrs ) {
                                attrs.charm_max = std::max<int16_t>(0, attrs.charm_max + v);
                                attrs.last_update = now;
                            });
                        }
                    }
                    else if( k == mood_key) {
                        FC_ASSERT(itr->second.which() == lua_types::tag<lua_int>::value, "attributes value type invalid (must be int)");
                        auto v = itr->second.get<lua_int>().v;
                        if(v != 0) {
                            _db.modify(core_attrs, [&]( actor_core_attributes_object& attrs ) {
                                attrs.mood_max = std::max<int16_t>(0, attrs.mood_max + v);
                                attrs.last_update = now;
                            });
                        }
                    }
                    else if( k == health_key) {
                        FC_ASSERT(itr->second.which() == lua_types::tag<lua_int>::value, "attributes value type invalid (must be int)");
                        auto v = itr->second.get<lua_int>().v;
                        if(v != 0) {
                            _db.modify( actor, [&]( actor_object& obj ) {
                                obj.health_max = std::max<int16_t>(0, obj.health_max + v);
                                obj.last_update = now;
                            });
                        }
                    }
                }
            }
        }
        catch (fc::exception e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
        catch (std::exception e)
        {
            LUA_C_ERR_THROW(_context.mState, e.what());
        }
    }
    //=============================================================================
    void contract_nfa_handler::inject_material_from(nfa_id_type from, nfa_id_type to, double amount, const string& symbol_name, bool enable_logger)
    {
        try
        {
            asset_symbol_type symbol = s_get_symbol_type_from_string(symbol_name);
            auto token = asset(amount, symbol);
            inject_material_by_contract(from, to, token, _ch.result, enable_logger);
        }
        catch (fc::exception e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::inject_material_by_contract(nfa_id_type from, nfa_id_type to, asset token, contract_result &result, bool enable_logger)
    {
        try
        {
            FC_ASSERT(token.symbol.asset_num == TAIYI_ASSET_NUM_GOLD ||
                      token.symbol.asset_num == TAIYI_ASSET_NUM_FOOD ||
                      token.symbol.asset_num == TAIYI_ASSET_NUM_WOOD ||
                      token.symbol.asset_num == TAIYI_ASSET_NUM_FABRIC ||
                      token.symbol.asset_num == TAIYI_ASSET_NUM_HERB, "注入的材料不是有效的资源物质");
            FC_ASSERT(token.amount >= share_type(0), "resource amount must big than zero");

            const nfa_object &from_nfa = _db.get<nfa_object, by_id>(from);
            FC_ASSERT(from_nfa.owner_account == _caller_account.id || from_nfa.active_account == _caller_account.id, "caller account not the owner or active operator");
            const nfa_object &to_nfa = _db.get<nfa_object, by_id>(to);
            try
            {
                FC_ASSERT(_db.get_nfa_balance(from_nfa, token.symbol).amount >= token.amount, "Insufficient Balance: ${balance}, unable to inject material '${token}' from nfa #${a} to nfa #${t}", ("a", from_nfa.id)("t", to_nfa.id)("token", token)("balance", _db.get_nfa_balance(from_nfa, token.symbol)));
            }
            FC_RETHROW_EXCEPTIONS(error, "Unable to inject material ${a} from ${f} to ${t}", ("a", token)("f", from_nfa.id)("t", to_nfa.id));
            
            _db.adjust_nfa_balance(from_nfa, -token);
            
            const auto& material = _db.get<nfa_material_object, by_nfa_id>(to_nfa.id);
            _db.modify(material, [&](nfa_material_object& obj) {
                switch (token.symbol.asset_num) {
                    case TAIYI_ASSET_NUM_GOLD:
                        obj.gold += token;
                        break;
                    case TAIYI_ASSET_NUM_FOOD:
                        obj.food += token;
                        break;
                    case TAIYI_ASSET_NUM_WOOD:
                        obj.wood += token;
                        break;
                    case TAIYI_ASSET_NUM_FABRIC:
                        obj.fabric += token;
                        break;
                    case TAIYI_ASSET_NUM_HERB:
                        obj.herb += token;
                        break;
                    default:
                        FC_ASSERT(false, "can not be here");
                        break;
                }
            });
                                    
            if(enable_logger) {
                variant v;
                fc::to_variant(token, v);
                _ch.log(FORMAT_MESSAGE("nfa #${a} inject material ${token} to nfa #${b}", ("a", from_nfa.id)("token", fc::json::to_string(v))("b", to_nfa.id)));
            }
        }
        catch (fc::exception e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::separate_material_out(nfa_id_type from, nfa_id_type to, double amount, const string& symbol_name, bool enable_logger)
    {
        try
        {
            asset_symbol_type symbol = s_get_symbol_type_from_string(symbol_name);
            auto token = asset(amount, symbol);
            separate_material_by_contract(from, to, token, _ch.result, enable_logger);
        }
        catch (fc::exception e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::separate_material_by_contract(nfa_id_type from, nfa_id_type to, asset token, contract_result &result, bool enable_logger)
    {
        try
        {
            FC_ASSERT(token.symbol.asset_num == TAIYI_ASSET_NUM_GOLD ||
                      token.symbol.asset_num == TAIYI_ASSET_NUM_FOOD ||
                      token.symbol.asset_num == TAIYI_ASSET_NUM_WOOD ||
                      token.symbol.asset_num == TAIYI_ASSET_NUM_FABRIC ||
                      token.symbol.asset_num == TAIYI_ASSET_NUM_HERB, "析出的材料不是有效的资源物质");
            FC_ASSERT(token.amount >= share_type(0), "resource amount must big than zero");

            const nfa_object &from_nfa = _db.get<nfa_object, by_id>(from);
            FC_ASSERT(from_nfa.owner_account == _caller_account.id || from_nfa.active_account == _caller_account.id, "caller account not the owner or active operator");
            const nfa_object &to_nfa = _db.get<nfa_object, by_id>(to);

            const auto& material = _db.get<nfa_material_object, by_nfa_id>(from_nfa.id);
            try
            {
                switch (token.symbol.asset_num) {
                    case TAIYI_ASSET_NUM_GOLD:
                        FC_ASSERT(material.gold.amount >= token.amount, "Insufficient material: ${balance}, unable to inject material '${token}' from nfa #${a} to nfa #${t}", ("a", from_nfa.id)("t", to_nfa.id)("token", token)("balance", material.gold));
                        break;
                    case TAIYI_ASSET_NUM_FOOD:
                        FC_ASSERT(material.food.amount >= token.amount, "Insufficient material: ${balance}, unable to inject material '${token}' from nfa #${a} to nfa #${t}", ("a", from_nfa.id)("t", to_nfa.id)("token", token)("balance", material.food));
                        break;
                    case TAIYI_ASSET_NUM_WOOD:
                        FC_ASSERT(material.wood.amount >= token.amount, "Insufficient material: ${balance}, unable to inject material '${token}' from nfa #${a} to nfa #${t}", ("a", from_nfa.id)("t", to_nfa.id)("token", token)("balance", material.wood));
                        break;
                    case TAIYI_ASSET_NUM_FABRIC:
                        FC_ASSERT(material.fabric.amount >= token.amount, "Insufficient material: ${balance}, unable to inject material '${token}' from nfa #${a} to nfa #${t}", ("a", from_nfa.id)("t", to_nfa.id)("token", token)("balance", material.fabric));
                        break;
                    case TAIYI_ASSET_NUM_HERB:
                        FC_ASSERT(material.herb.amount >= token.amount, "Insufficient material: ${balance}, unable to inject material '${token}' from nfa #${a} to nfa #${t}", ("a", from_nfa.id)("t", to_nfa.id)("token", token)("balance", material.herb));
                        break;
                    default:
                        FC_ASSERT(false, "can not be here");
                        break;
                }
            }
            FC_RETHROW_EXCEPTIONS(error, "Unable to separate material ${a} out from ${f} to ${t}", ("a", token)("f", from_nfa.id)("t", to_nfa.id));

            _db.modify(material, [&](nfa_material_object& obj) {
                switch (token.symbol.asset_num) {
                    case TAIYI_ASSET_NUM_GOLD:
                        obj.gold -= token;
                        break;
                    case TAIYI_ASSET_NUM_FOOD:
                        obj.food -= token;
                        break;
                    case TAIYI_ASSET_NUM_WOOD:
                        obj.wood -= token;
                        break;
                    case TAIYI_ASSET_NUM_FABRIC:
                        obj.fabric -= token;
                        break;
                    case TAIYI_ASSET_NUM_HERB:
                        obj.herb -= token;
                        break;
                    default:
                        FC_ASSERT(false, "can not be here");
                        break;
                }
            });

            _db.adjust_nfa_balance(to_nfa, token);

            if(enable_logger) {
                variant v;
                fc::to_variant(token, v);
                _ch.log(FORMAT_MESSAGE("nfa #${a} separate meterial ${token} out to nfa ${b}", ("a", from_nfa.id)("token", fc::json::to_string(v))("b", to_nfa.id)));
            }
        }
        catch (fc::exception e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }

} } // namespace taiyi::chain

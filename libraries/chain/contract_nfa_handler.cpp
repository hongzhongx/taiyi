#include <chain/taiyi_fwd.hpp>

#include <chain/taiyi_evaluator.hpp>
#include <chain/database.hpp>

#include <chain/taiyi_objects.hpp>
#include <chain/block_summary_object.hpp>
#include <chain/account_object.hpp>
#include <chain/contract_objects.hpp>
#include <chain/nfa_objects.hpp>
#include <chain/actor_objects.hpp>
#include <chain/tiandao_property_object.hpp>

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

        main_contract = db.get<contract_object, by_id>(nfa.main_contract).name;
        if(nfa.is_miraged)
            mirage_contract = db.get<contract_object, by_id>(nfa.mirage_contract).name;
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
        catch (const fc::exception& e)
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
        catch (const fc::exception& e)
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
        catch (const fc::exception& e)
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
        catch (const fc::exception& e)
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
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=========================================================================
    void contract_nfa_handler::convert_qi_to_resource(int64_t amount, const string& resource_symbol_name)
    {
        string _err_str = "";
        try
        {
            _db.add_contract_handler_exe_point(2);
            
            FC_ASSERT(_caller.owner_account == _caller_account.id || _caller.active_account == _caller_account.id, "caller account not the owner or active operator");
            FC_ASSERT(amount > 0, "convert qi is zero");

            asset qi(amount, QI_SYMBOL);
            FC_ASSERT(_db.get_nfa_balance(_caller, QI_SYMBOL) >= qi, "实体真气不足");
            
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
            operation vop = nfa_convert_resources_operation( _caller.id, caller_owner.name, qi, new_resource, true );
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
        catch (const fc::exception& e)
        {
            _err_str = e.to_string();
            wdump((_err_str));
        }
        
        if (!_err_str.empty()) { LUA_C_ERR_THROW(_context.mState, _err_str); }
    }
    //=========================================================================
    void contract_nfa_handler::convert_resource_to_qi(int64_t amount, const string& resource_symbol_name)
    {
        try
        {
            _db.add_contract_handler_exe_point(2);
            
            FC_ASSERT(_caller.owner_account == _caller_account.id || _caller.active_account == _caller_account.id, "caller account not the owner or active operator");
            FC_ASSERT(amount > 0, "convert resource is zero");
            
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
                    FC_ASSERT(false, "NFA can not convert asset (${a}) which is not resource.", ("a", resource_symbol_name));
                    break;
            }
            
            asset resource(amount, symbol);
            FC_ASSERT(_db.get_nfa_balance(_caller, symbol) >= resource, "实体资源不足");

            asset new_qi = resource * p; //真气具有最小的价值粒度，所以这里不会有整型除法导致的误差
            
            const auto& caller_owner = _db.get<account_object, by_id>(_caller.owner_account);
            operation vop = nfa_convert_resources_operation( _caller.id, caller_owner.name, new_qi, resource, false );
            _db.pre_push_virtual_operation( vop );

            _db.adjust_nfa_balance(_caller, new_qi);
            _db.adjust_nfa_balance(_caller, -resource);
            
            _db.post_push_virtual_operation( vop );
            
            //更新统计自由真气量和统计资源
            _db.modify( _db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo ) {
                gpo.total_qi += new_qi;
                switch (symbol.asset_num) {
                    case TAIYI_ASSET_NUM_GOLD:
                        gpo.total_gold -= resource;
                        break;
                    case TAIYI_ASSET_NUM_FOOD:
                        gpo.total_food -= resource;
                        break;
                    case TAIYI_ASSET_NUM_WOOD:
                        gpo.total_wood -= resource;
                        break;
                    case TAIYI_ASSET_NUM_FABRIC:
                        gpo.total_fabric -= resource;
                        break;
                    case TAIYI_ASSET_NUM_HERB:
                        gpo.total_herb -= resource;
                        break;
                    default:
                        FC_ASSERT(false, "can not be here", ("a", resource_symbol_name));
                        break;
                }
            });
        }
        catch (const fc::exception& e)
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
            _db.add_contract_handler_exe_point(1);

            FC_ASSERT(_caller.owner_account == _caller_account.id || _caller.active_account == _caller_account.id, "caller account not the owner or active operator");

            const auto& child_nfa = _db.get<nfa_object, by_id>(nfa_id);
            FC_ASSERT(child_nfa.owner_account == _caller_account.id || child_nfa.active_account == _caller_account.id, "caller account not the input nfa's owner or active operator");
            FC_ASSERT(child_nfa.parent == nfa_id_type::max(), "input nfa already have parent");
                        
            _db.modify(child_nfa, [&]( nfa_object& obj ) {
                obj.parent = _caller.id;
            });
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::add_to_parent(int64_t parent_nfa_id)
    {
        try
        {
            _db.add_contract_handler_exe_point(1);

            FC_ASSERT(_caller.owner_account == _caller_account.id || _caller.active_account == _caller_account.id, "caller account not the owner or active operator");
            FC_ASSERT(_caller.parent == nfa_id_type::max(), "caller already have parent");

            const auto& parent_nfa = _db.get<nfa_object, by_id>(parent_nfa_id);
            FC_ASSERT(parent_nfa.owner_account == _caller_account.id || parent_nfa.active_account == _caller_account.id, "caller account not the parent nfa's owner or active operator");
                        
            _db.modify(_caller, [&]( nfa_object& obj ) {
                obj.parent = parent_nfa.id;
            });
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=========================================================================
    void contract_nfa_handler::add_child_from_contract_owner(int64_t nfa_id)
    {
        try
        {
            _db.add_contract_handler_exe_point(1);

            FC_ASSERT(_caller.owner_account == _caller_account.id || _caller.active_account == _caller_account.id, "caller account not the owner or active operator");
            
            const auto& child_nfa = _db.get<nfa_object, by_id>(nfa_id);
            FC_ASSERT(child_nfa.owner_account == _ch.contract.owner || child_nfa.active_account == _ch.contract.owner, "contract owner not the input nfa's owner or active operator");
            FC_ASSERT(child_nfa.parent == nfa_id_type::max(), "input nfa already have parent");
                        
            _db.modify(child_nfa, [&]( nfa_object& obj ) {
                obj.parent = _caller.id;
            });
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::add_to_parent_from_contract_owner(int64_t parent_nfa_id)
    {
        try
        {
            _db.add_contract_handler_exe_point(1);
            
            FC_ASSERT(_caller.owner_account == _caller_account.id || _caller.active_account == _caller_account.id, "caller account not the owner or active operator");
            FC_ASSERT(_caller.parent == nfa_id_type::max(), "caller already have parent");

            const auto& parent_nfa = _db.get<nfa_object, by_id>(parent_nfa_id);
            FC_ASSERT(parent_nfa.owner_account == _ch.contract.owner || parent_nfa.active_account == _ch.contract.owner, "contract owner not the parent nfa's owner or active operator");
                        
            _db.modify(_caller, [&]( nfa_object& obj ) {
                obj.parent = parent_nfa.id;
            });
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::remove_from_parent()
    {
        try
        {
            _db.add_contract_handler_exe_point(1);
            
            FC_ASSERT(_caller.owner_account == _caller_account.id || _caller.active_account == _caller_account.id, "caller account not the owner or active operator");
            FC_ASSERT(_caller.parent != nfa_id_type::max(), "caller have no parent");

            const auto& parent_nfa = _db.get<nfa_object, by_id>(_caller.parent);
            FC_ASSERT(parent_nfa.owner_account == _caller_account.id, "caller account not the parent nfa's owner");
                        
            _db.modify(_caller, [&]( nfa_object& obj ) {
                obj.parent = nfa_id_type::max();
            });
        }
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::transfer_from(nfa_id_type from, nfa_id_type to, double amount, const string& symbol_name, bool enable_logger)
    {
        try
        {
            _db.add_contract_handler_exe_point(1);
            
            asset_symbol_type symbol = s_get_symbol_type_from_string(symbol_name);
            auto token = asset(amount, symbol);
            transfer_by_contract(from, to, token, _ch.result, enable_logger);
        }
        catch (const fc::exception& e)
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
            _db.add_contract_handler_exe_point(2);
            
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

            operation vop = nfa_transfer_operation(from, _db.get<account_object, by_id>(from_nfa.owner_account).name, to, _db.get<account_object, by_id>(to_nfa.owner_account).name, token );
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
        catch (const fc::exception& e)
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
            _db.add_contract_handler_exe_point(1);
            
            asset_symbol_type symbol = s_get_symbol_type_from_string(symbol_name);
            auto token = asset(amount, symbol);
            withdraw_by_contract(from, _db.get_account(to).id, token, _ch.result, enable_logger);
        }
        catch (const fc::exception& e)
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
            _db.add_contract_handler_exe_point(2);
            
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
        catch (const fc::exception& e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::deposit_from(account_id_type from, nfa_id_type to, double amount, const string& symbol_name, bool enable_logger)
    {
        try
        {
            _db.add_contract_handler_exe_point(1);
            
            asset_symbol_type symbol = s_get_symbol_type_from_string(symbol_name);
            auto token = asset(amount, symbol);
            deposit_by_contract(from, to, token, _ch.result, enable_logger);
        }
        catch (const fc::exception& e)
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
            _db.add_contract_handler_exe_point(2);
            
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
        catch (const fc::exception& e)
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
        catch (const fc::exception& e)
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
        catch (const fc::exception& e)
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
            _db.add_contract_handler_exe_point(1);
            
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
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=============================================================================
    void contract_nfa_handler::modify_actor_attributes(const lua_map& values)
    {
        try
        {
            _db.add_contract_handler_exe_point(2);
            
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
        catch (const fc::exception& e)
        {
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
        catch (std::exception e)
        {
            LUA_C_ERR_THROW(_context.mState, e.what());
        }
    }
    //=============================================================================
    void contract_nfa_handler::talk_to_actor(const string& target_actor_name, const string& something)
    {
        try
        {
            _db.add_contract_handler_exe_point(2);
            
            FC_ASSERT(_caller.owner_account == _caller_account.id || _caller.active_account == _caller_account.id, "caller account not the owner or active operator");

            auto now = _db.head_block_time();
            
            const actor_object& actor_me = _db.get< actor_object, by_nfa_id >( _caller.id );
            FC_ASSERT( actor_me.born == true, "actor ${a} is not born.", ("a", actor_me.name));
            
            const actor_object& actor_target = _db.get< actor_object, by_name >( target_actor_name );
            FC_ASSERT( actor_target.born == true, "actor ${a} is not born.", ("a", actor_target.name));
            
            FC_ASSERT( actor_me.location == actor_target.location, "talk to actor not on the same zone location.");
            
            _db.prepare_actor_relations(actor_me, actor_target);
            
            _db.add_contract_handler_exe_point(3);

            //处理说话人
            int favor_delta_me = 0;
            {
                const auto& relation_me_to_target = _db.get< actor_relation_object, by_actor_target >( boost::make_tuple(actor_me.id, actor_target.id) );
                
                //以nfa视角回调nfa函数：on_actor_talking(target_actor_name, something)
                //该函数返回一个整数作为好感度变化值
                lua_map param;
                param[lua_key(lua_int(1))] = lua_types(lua_string(actor_target.name));
                param[lua_key(lua_int(2))] = lua_types(lua_string(something));
                lua_map result = _ch.call_nfa_function_with_caller(_caller_account, actor_me.nfa_id, "on_actor_talking", param, false);
                
                if (result.size() > 0) {
                    FC_ASSERT(result.size() == 1, "on_actor_talking should return one sigle integer.");
                    auto _itr = result.find(lua_types(lua_int(1)));
                    FC_ASSERT(_itr != result.end(), "on_actor_talking should return one sigle integer.");
                    auto a = _itr->second.which();
                    auto b = lua_types::tag<lua_int>::value;
                    FC_ASSERT(_itr->second.which() == lua_types::tag<lua_int>::value, "on_actor_talking should return one sigle integer.");
                    favor_delta_me = _itr->second.get<lua_int>().v;
                }
                
                if (favor_delta_me != 0) {
                    _db.modify(relation_me_to_target, [&]( actor_relation_object& rel ) {
                        rel.favor = std::min(30000, rel.favor + favor_delta_me);
                        rel.last_update = now;
                        //if(actor_me.name == s_debug_actor && rel.favor > 10000)
                        //    wlog("${y}年${m}月，${a}对${b}的好感达到${c}。", ("y", tiandao.v_years)("m", tiandao.v_months)("a", actor_me.name)("b", actor_target.name)("c", rel.favor));
                    });
                }
            }
            
            //处理听话人
            int favor_delta_target = 0;
            {
                const auto& relation_target_to_me = _db.get< actor_relation_object, by_actor_target >( boost::make_tuple(actor_target.id, actor_me.id) );

                //以nfa视角回调nfa函数：on_actor_listening(from_actor_name, something)
                //该函数返回一个整数作为好感度变化值
                lua_map param;
                param[lua_key(lua_int(1))] = lua_types(lua_string(actor_me.name));
                param[lua_key(lua_int(2))] = lua_types(lua_string(something));
                lua_map result = _ch.call_nfa_function_with_caller(_db.get_account(TAIYI_DANUO_ACCOUNT), actor_target.nfa_id, "on_actor_listening", param, false);

                if (result.size() > 0) {
                    FC_ASSERT(result.size() == 1, "on_actor_listening should return one sigle integer.");
                    auto _itr = result.find(lua_types(lua_int(1)));
                    FC_ASSERT(_itr != result.end(), "on_actor_listening should return one sigle integer.");
                    FC_ASSERT(_itr->second.which() == lua_types::tag<lua_int>::value, "on_actor_listening should return one sigle integer.");
                    favor_delta_target = _itr->second.get<lua_int>().v;
                }
                
                if (favor_delta_target != 0) {
                    _db.modify(relation_target_to_me, [&]( actor_relation_object& rel ) {
                        rel.favor = std::min(30000, rel.favor + favor_delta_target);
                        rel.last_update = now;
                        //if(actor_target.name == s_debug_actor && rel.favor > 10000)
                        //    wlog("${y}年${m}月，${a}对${b}的好感达到${c}。", ("y", tiandao.v_years)("m", tiandao.v_months)("a", actor_target.name)("b", actor_me.name)("c", rel.favor));
                    });
                }
            }
            
            const auto& tiandao = _db.get_tiandao_properties();
            const auto& actor_me_owner = _db.get<account_object, by_id>(_caller.owner_account);
            const auto& actor_target_owner = _db.get<account_object, by_id>(_db.get<nfa_object, by_id>( actor_target.nfa_id).owner_account);
            _db.push_virtual_operation( actor_talk_operation( tiandao.v_years, tiandao.v_months, tiandao.v_times, actor_me_owner.name, actor_me.nfa_id, actor_me.name, actor_target_owner.name, actor_target.nfa_id, actor_target.name, something, favor_delta_me, favor_delta_target ) );
            //if(actor_me.name == s_debug_actor)
            //    wlog("${y}年${m}月，${a}对${b}说道：\"${c}\"", ("y", tiandao.v_years)("m", tiandao.v_months)("a", actor_me.name)("b", actor_target.name)("c", talk_content));

        }
        catch (const fc::exception& e)
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
            _db.add_contract_handler_exe_point(1);
            
            asset_symbol_type symbol = s_get_symbol_type_from_string(symbol_name);
            auto token = asset(amount, symbol);
            inject_material_by_contract(from, to, token, _ch.result, enable_logger);
        }
        catch (const fc::exception& e)
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
            _db.add_contract_handler_exe_point(5);
            
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
        catch (const fc::exception& e)
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
            _db.add_contract_handler_exe_point(1);
            
            asset_symbol_type symbol = s_get_symbol_type_from_string(symbol_name);
            auto token = asset(amount, symbol);
            separate_material_by_contract(from, to, token, _ch.result, enable_logger);
        }
        catch (const fc::exception& e)
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
            _db.add_contract_handler_exe_point(5);
            
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
        catch (const fc::exception& e)
        {
            wdump((e.to_string()));
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=========================================================================
    lua_map contract_nfa_handler::eval_nfa_action(int64_t nfa_id, const string& action, const lua_map& params)
    {
        LuaContext nfa_context;
        
        if(lua_getdropsenabled(_context.mState)) {
            lua_enabledrops(nfa_context.mState, 1, 1);
            lua_setdrops(nfa_context.mState, lua_getdrops(_context.mState)); //上层虚拟机还剩下的drops可用
        }

        try
        {
            _db.add_contract_handler_exe_point(2);
            
            const nfa_object& nfa = _db.get<nfa_object, by_id>(nfa_id);
            
            //check existence and consequence type
            const auto* contract_ptr = _db.find<chain::contract_object, by_id>(nfa.is_miraged?nfa.mirage_contract:nfa.main_contract);
            if(contract_ptr == nullptr)
                return lua_map();
            
            auto abi_itr = contract_ptr->contract_ABI.find(lua_types(lua_string(action)));
            if(abi_itr == contract_ptr->contract_ABI.end())
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
            
            auto func_abi_itr = contract_ptr->contract_ABI.find(lua_types(lua_string(function_name)));
            FC_ASSERT(func_abi_itr != contract_ptr->contract_ABI.end(), "${f} not found, maybe a internal function", ("f", function_name));
            if(!func_abi_itr->second.get<lua_function>().is_var_arg)
                FC_ASSERT(value_list.size() == func_abi_itr->second.get<lua_function>().arglist.size(), "input values count is ${n}, but ${f}`s parameter list is ${p}...", ("n", value_list.size())("f", function_name)("p", func_abi_itr->second.get<lua_function>().arglist));
            FC_ASSERT(value_list.size() <= 20, "value list is greater than 20 limit");
            
            _db.add_contract_handler_exe_point(2);
            
            auto current_contract_name = _context.readVariable<string>("current_contract");
            auto current_cbi = _context.readVariable<contract_base_info*>(current_contract_name, "contract_base_info");
            auto current_ch = _context.readVariable<contract_handler*>(current_contract_name, "contract_helper");
            const auto &baseENV = _db.get<contract_bin_code_object, by_id>(0);
            
            const auto& nfa_contract = *contract_ptr;
            const auto& nfa_contract_owner = _db.get<account_object, by_id>(nfa_contract.owner).name;
            const auto& nfa_contract_code = _db.get<contract_bin_code_object, by_id>(nfa_contract.lua_code_b_id);
            
            contract_base_info cbi(_db, nfa_context, nfa_contract_owner, nfa_contract.name, current_cbi->caller, string(nfa_contract.creation_date), string(nfa_contract.contract_authority), current_cbi->invoker_contract_name);
            contract_handler ch(_db, current_ch->caller, &_caller, nfa_contract, current_ch->result, nfa_context, current_ch->sigkeys, current_ch->is_in_eval);
            contract_nfa_handler cnh(current_ch->caller, nfa, nfa_context, _db, ch);
            
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
        catch(const fc::exception& e)
        {
            if(lua_getdropsenabled(_context.mState)) {
                auto vm_drops = lua_getdrops(nfa_context.mState);
                lua_setdrops(_context.mState, vm_drops); //即使崩溃了也要将使用drops情况反馈到上层
            }
            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
    }
    //=========================================================================
    lua_map contract_nfa_handler::do_nfa_action(int64_t nfa_id, const string& action, const lua_map& params)
    {
        //TODO: do的权限以及产生的消耗
        LuaContext nfa_context;
        
        zone_id_type pre_contract_run_zone = _db.get_contract_run_zone();

        try
        {
            _db.add_contract_handler_exe_point(2);
            
            const nfa_object& nfa = _db.get<nfa_object, by_id>(nfa_id);
            
            //对actor要设置db的当前运行zone标记
            const auto* check_actor = _db.find_actor_with_parents(nfa);
            if (check_actor && pre_contract_run_zone == zone_id_type::max())
                _db.set_contract_run_zone(check_actor->location);

            //check material valid
            FC_ASSERT(_db.is_nfa_material_equivalent_qi_insufficient(nfa), "NFA material equivalent qi is insufficient(#t&&y#实体完整性不足#a&&i#)");
            _db.consume_nfa_material_random(nfa, _db.head_block_id()._hash[4] + 14071);

            //check existence and consequence type
            const auto* contract_ptr = _db.find<chain::contract_object, by_id>(nfa.is_miraged?nfa.mirage_contract:nfa.main_contract);
            if(contract_ptr == nullptr)
                return lua_map();
            
            FC_ASSERT(_db.is_contract_allowed_by_zone(*contract_ptr, _db.get_contract_run_zone()), "contract ${c} is not allowed by zone #${z}(#t&&y#所在区域禁止该天道运行#a&&i#)", ("c", contract_ptr->name)("z", _db.get_contract_run_zone()));
            
            auto abi_itr = contract_ptr->contract_ABI.find(lua_types(lua_string(action)));
            if(abi_itr == contract_ptr->contract_ABI.end())
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
                        
            auto func_abi_itr = contract_ptr->contract_ABI.find(lua_types(lua_string(function_name)));
            FC_ASSERT(func_abi_itr != contract_ptr->contract_ABI.end(), "${f} not found, maybe a internal function", ("f", function_name));
            if(!func_abi_itr->second.get<lua_function>().is_var_arg)
                FC_ASSERT(value_list.size() == func_abi_itr->second.get<lua_function>().arglist.size(), "input values count is ${n}, but ${f}`s parameter list is ${p}...", ("n", value_list.size())("f", function_name)("p", func_abi_itr->second.get<lua_function>().arglist));
            FC_ASSERT(value_list.size() <= 20, "value list is greater than 20 limit");
            
            _db.add_contract_handler_exe_point(2);
            
            auto current_contract_name = _context.readVariable<string>("current_contract");
            auto current_cbi = _context.readVariable<contract_base_info*>(current_contract_name, "contract_base_info");
            auto current_ch = _context.readVariable<contract_handler*>(current_contract_name, "contract_helper");
            const auto &baseENV = _db.get<contract_bin_code_object, by_id>(0);
            
            const auto& nfa_contract = *contract_ptr;
            const auto& nfa_contract_owner = _db.get<account_object, by_id>(nfa_contract.owner).name;
            const auto& nfa_contract_code = _db.get<contract_bin_code_object, by_id>(nfa_contract.lua_code_b_id);
            
            contract_base_info cbi(_db, nfa_context, nfa_contract_owner, nfa_contract.name, current_cbi->caller, string(nfa_contract.creation_date), string(nfa_contract.contract_authority), current_cbi->invoker_contract_name);
            contract_handler ch(_db, current_ch->caller, &_caller, nfa_contract, current_ch->result, nfa_context, current_ch->sigkeys, false);
            contract_nfa_handler cnh(current_ch->caller, nfa, nfa_context, _db, ch);
            
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
            
            _db.set_contract_run_zone(pre_contract_run_zone);

            return result_table.v;
        }
        catch (const fc::exception& e)
        {
            _db.set_contract_run_zone(pre_contract_run_zone);

            LUA_C_ERR_THROW(_context.mState, e.to_string());
        }
        
        _db.set_contract_run_zone(pre_contract_run_zone);
    }

} } // namespace taiyi::chain

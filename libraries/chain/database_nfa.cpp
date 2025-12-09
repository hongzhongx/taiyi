#include <chain/taiyi_fwd.hpp>

#include <protocol/taiyi_operations.hpp>

#include <chain/database.hpp>
#include <chain/database_exceptions.hpp>
#include <chain/global_property_object.hpp>
#include <chain/taiyi_objects.hpp>
#include <chain/transaction_object.hpp>
#include <chain/account_object.hpp>
#include <chain/contract_objects.hpp>
#include <chain/nfa_objects.hpp>
#include <chain/asset_objects/nfa_balance_object.hpp>

#include <chain/lua_context.hpp>
#include <chain/contract_worker.hpp>

#include <chain/util/uint256.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/uint128.hpp>

#include <fc/container/deque.hpp>

#include <fc/io/fstream.hpp>

#include <iostream>

#include <cstdint>
#include <deque>
#include <fstream>
#include <functional>


namespace taiyi { namespace chain {
    
    int64_t nfa_material_object::get_material_qi() const
    {
        asset mqi(0, QI_SYMBOL);
        mqi += gold * TAIYI_GOLD_QI_PRICE;
        mqi += food * TAIYI_FOOD_QI_PRICE;
        mqi += wood * TAIYI_WOOD_QI_PRICE;
        mqi += fabric * TAIYI_FABRIC_QI_PRICE;
        mqi += herb * TAIYI_HERB_QI_PRICE;
        return mqi.amount.value;
    }
    //=========================================================================
    bool database::is_nfa_material_equivalent_qi_insufficient(const nfa_object& nfa) const
    {
        const auto& nfa_symbol = get<nfa_symbol_object, by_id>(nfa.symbol_id);
        const auto& nfa_material = get<nfa_material_object, by_nfa_id>(nfa.id);
        
        int64_t material_equivalent_qi_value = nfa_material.get_material_qi();
        return material_equivalent_qi_value >= nfa_symbol.min_equivalent_qi;
    }
    //=========================================================================
    //注意，消耗掉的材料只能转移到财库
    void database::consume_nfa_material_random(const nfa_object& nfa, const uint32_t& seed)
    {
        const auto& nfa_symbol = get<nfa_symbol_object, by_id>(nfa.symbol_id);
        int64_t consume_qi_base_value = nfa_symbol.min_equivalent_qi / 50; //每次最多消耗基数的 1/50
        if (consume_qi_base_value == 0)
            return;

        const auto& nfa_material = get<nfa_material_object, by_nfa_id>(nfa.id);
        
        asset gold_qi = nfa_material.gold * TAIYI_GOLD_QI_PRICE;
        asset food_qi = nfa_material.food * TAIYI_FOOD_QI_PRICE;
        asset wood_qi = nfa_material.wood * TAIYI_WOOD_QI_PRICE;
        asset fabric_qi = nfa_material.fabric * TAIYI_FABRIC_QI_PRICE;
        asset herb_qi = nfa_material.herb * TAIYI_HERB_QI_PRICE;
        int64_t material_equivalent_qi_value = gold_qi.amount.value + food_qi.amount.value + wood_qi.amount.value + fabric_qi.amount.value + herb_qi.amount.value;
        if (material_equivalent_qi_value <= 0)
            return;
        
        asset consumed_gold(0, GOLD_SYMBOL);
        asset consumed_food(0, FOOD_SYMBOL);
        asset consumed_wood(0, WOOD_SYMBOL);
        asset consumed_fabric(0, FABRIC_SYMBOL);
        asset consumed_herb(0, HERB_SYMBOL);

        modify<nfa_material_object>(nfa_material, [&](nfa_material_object& _obj) {
            int64_t consume_qi_value = 0;

            consume_qi_value = consume_qi_base_value * gold_qi.amount.value / material_equivalent_qi_value;
            if (consume_qi_value > 0) {
                consume_qi_value = hasher::hash(seed + 30011) % (uint32_t)(consume_qi_value);
                consumed_gold = asset(consume_qi_value, QI_SYMBOL) * TAIYI_GOLD_QI_PRICE;
                consumed_gold.amount.value = std::min(_obj.gold.amount.value, consumed_gold.amount.value);
                if(consumed_gold.amount > 0) {
                    wlog("NFA #${n} consume material ${v}", ("n", nfa.id)("v", consumed_gold));
                    _obj.gold -= consumed_gold;
                }
            }

            consume_qi_value = consume_qi_base_value * food_qi.amount.value / material_equivalent_qi_value;
            if (consume_qi_value > 0) {
                consume_qi_value = hasher::hash(seed + 30223) % (uint32_t)(consume_qi_value);
                consumed_food = asset(consume_qi_value, QI_SYMBOL) * TAIYI_FOOD_QI_PRICE;
                consumed_food.amount.value = std::min(_obj.food.amount.value, consumed_food.amount.value);
                if(consumed_food.amount > 0) {
                    wlog("NFA #${n} consume material ${v}", ("n", nfa.id)("v", consumed_food));
                    _obj.food -= consumed_food;
                }
            }
            
            consume_qi_value = consume_qi_base_value * wood_qi.amount.value / material_equivalent_qi_value;
            if (consume_qi_value > 0) {
                consume_qi_value = hasher::hash(seed + 30253) % (uint32_t)(consume_qi_value);
                consumed_wood = asset(consume_qi_value, QI_SYMBOL) * TAIYI_WOOD_QI_PRICE;
                consumed_wood.amount.value = std::min(_obj.wood.amount.value, consumed_wood.amount.value);
                if(consumed_wood.amount > 0) {
                    wlog("NFA #${n} consume material ${v}", ("n", nfa.id)("v", consumed_wood));
                    _obj.wood -= consumed_wood;
                }
            }

            consume_qi_value = consume_qi_base_value * fabric_qi.amount.value / material_equivalent_qi_value;
            if (consume_qi_value > 0) {
                consume_qi_value = hasher::hash(seed + 30271) % (uint32_t)(consume_qi_value);
                consumed_fabric = asset(consume_qi_value, QI_SYMBOL) * TAIYI_FABRIC_QI_PRICE;
                consumed_fabric.amount.value = std::min(_obj.fabric.amount.value, consumed_fabric.amount.value);
                if(consumed_fabric.amount > 0) {
                    wlog("NFA #${n} consume material ${v}", ("n", nfa.id)("v", consumed_fabric));
                    _obj.fabric -= consumed_fabric;
                }
            }

            consume_qi_value = consume_qi_base_value * herb_qi.amount.value / material_equivalent_qi_value;
            if (consume_qi_value > 0) {
                consume_qi_value = hasher::hash(seed + 30341) % (uint32_t)(consume_qi_value);
                consumed_herb = asset(consume_qi_value, QI_SYMBOL) * TAIYI_HERB_QI_PRICE;
                consumed_herb.amount.value = std::min(_obj.herb.amount.value, consumed_herb.amount.value);
                if(consumed_herb.amount > 0) {
                    wlog("NFA #${n} consume material ${v}", ("n", nfa.id)("v", consumed_herb));
                    _obj.herb -= consumed_herb;
                }
            }
        });
        
        //传递到财库
        const auto& treasury_account = get<account_object, by_name>(TAIYI_DAO_ACCOUNT);
        if(consumed_gold.amount > 0) {
            operation vop = nfa_deposit_withdraw_operation(nfa.id, treasury_account.name, asset( 0, YANG_SYMBOL ), consumed_gold);
            pre_push_virtual_operation( vop );
            adjust_balance(treasury_account, consumed_gold);
            post_push_virtual_operation( vop );
        }
        if(consumed_food.amount > 0) {
            operation vop = nfa_deposit_withdraw_operation(nfa.id, treasury_account.name, asset( 0, YANG_SYMBOL ), consumed_food);
            pre_push_virtual_operation( vop );
            adjust_balance(treasury_account, consumed_food);
            post_push_virtual_operation( vop );
        }
        if(consumed_wood.amount > 0) {
            operation vop = nfa_deposit_withdraw_operation(nfa.id, treasury_account.name, asset( 0, YANG_SYMBOL ), consumed_wood);
            pre_push_virtual_operation( vop );
            adjust_balance(treasury_account, consumed_wood);
            post_push_virtual_operation( vop );
        }
        if(consumed_fabric.amount > 0) {
            operation vop = nfa_deposit_withdraw_operation(nfa.id, treasury_account.name, asset( 0, YANG_SYMBOL ), consumed_fabric);
            pre_push_virtual_operation( vop );
            adjust_balance(treasury_account, consumed_fabric);
            post_push_virtual_operation( vop );
        }
        if(consumed_herb.amount > 0) {
            operation vop = nfa_deposit_withdraw_operation(nfa.id, treasury_account.name, asset( 0, YANG_SYMBOL ), consumed_herb);
            pre_push_virtual_operation( vop );
            adjust_balance(treasury_account, consumed_herb);
            post_push_virtual_operation( vop );
        }
    }
    //=========================================================================
    void database::create_basic_nfa_objects()
    {
        //第一个心素
        const auto& nfa_symbol = get<nfa_symbol_object, by_symbol>(TAIYI_NFA_SYMBOL_NAME_XINSU_MARK);
        const auto& creator = get_account( TAIYI_DANUO_ACCOUNT );
        const auto& first_one = get_account( TAIYI_INIT_SIMING_NAME );
        LuaContext context;
        lua_setdrops(context.mState, 10000000);
        const auto& xinsu_mark = create_nfa(creator, nfa_symbol, true, context);
        modify(xinsu_mark, [&](nfa_object &obj) {
            obj.owner_account = first_one.id;
        });
    }
    //=========================================================================
    void database::create_basic_nfa_symbol_objects()
    {
        const auto& creator = get_account( TAIYI_DANUO_ACCOUNT );
        create_nfa_symbol_object(creator, TAIYI_NFA_SYMBOL_NAME_XINSU_MARK, "心素标记", "contract.nfa.xinsumark", 10000, 0, true);
        create_nfa_symbol_object(creator, TAIYI_NFA_SYMBOL_NAME_DEFAULT_ACTOR, "默认的角色", "contract.actor.default", 1000000000, 1000000, false);
        create_nfa_symbol_object(creator, TAIYI_NFA_SYMBOL_NAME_DEFAULT_ZONE, "默认的区域", "contract.zone.default", 1000000000, 10000000, false);
    }
    //=========================================================================
    size_t database::create_nfa_symbol_object(const account_object& creator, const string& symbol, const string& describe, const string& default_contract, const uint64_t& max_count, const uint64_t& min_equivalent_qi, const bool& is_sbt)
    {
        const auto* nfa_symbol = find<nfa_symbol_object, by_symbol>(symbol);
        FC_ASSERT(nfa_symbol == nullptr, "NFA symbol named \"${n}\" is already exist.", ("n", symbol));
        
        const auto& contract = find<contract_object, by_name>(default_contract);
        FC_ASSERT(contract != nullptr, "contract object named \"${n}\" is not exist.", ("n", default_contract));
        auto abi_itr = contract->contract_ABI.find(lua_types(lua_string(TAIYI_NFA_INIT_FUNC_NAME)));
        FC_ASSERT(abi_itr != contract->contract_ABI.end(), "contract ${c} has not init function named ${i}", ("c", contract->name)("i", TAIYI_NFA_INIT_FUNC_NAME));
        
        /*const auto& nfa_symbol_obj = */create<nfa_symbol_object>([&](nfa_symbol_object& obj) {
            obj.creator_account = creator.id;
            obj.authority_account = creator.id;
            obj.authority_nfa_symbol = nfa_symbol_id_type::max();
            obj.symbol = symbol;
            obj.describe = describe;
            obj.default_contract = contract->id;
            obj.count = 0;
            obj.max_count = max_count;
            obj.min_equivalent_qi = min_equivalent_qi;
            obj.is_sbt = is_sbt;
        });
        
        return TAIYI_NFA_SYMBOL_OBJ_STATE_BYTES; //fix size to avoid fc::raw::pack_size(nfa_symbol_obj) changing in future structure defination;
    }
    //=========================================================================
    const nfa_object& database::create_nfa(const account_object& creator, const nfa_symbol_object& nfa_symbol, bool reset_vm_memused, LuaContext& context, const nfa_object* caller_nfa)
    {
        const auto& caller = creator;
                
        //检查NFA最大数量限制
        FC_ASSERT(nfa_symbol.count < nfa_symbol.max_count, "The quantity of nfa with symbol \"${s}\" has reached the maximum limit ${c}", ("s", nfa_symbol.symbol)("c", nfa_symbol.max_count));

        modify<nfa_symbol_object>(nfa_symbol, [&](nfa_symbol_object& _obj) {
            _obj.count += 1;
        });
        
        const auto& nfa = create<nfa_object>([&](nfa_object& obj) {
            obj.creator_account = creator.id;
            obj.owner_account = creator.id;
            obj.active_account = creator.id;
            
            obj.symbol_id = nfa_symbol.id;
            obj.main_contract = nfa_symbol.default_contract;
            
            obj.created_time = head_block_time();
        });
        
        //运行主合约初始化nfa数据
        const auto& contract = get<contract_object, by_id>(nfa.main_contract);
        contract_worker worker;
        vector<lua_types> value_list;
        lua_table result_table;
        long long vm_drops = lua_getdrops(context.mState);
        result_table = worker.do_contract_function(caller, TAIYI_NFA_INIT_FUNC_NAME, value_list, contract, vm_drops, reset_vm_memused, context, *this);
        
        //init nfa from result table
        modify(nfa, [&](nfa_object& obj) {
            obj.contract_data = result_table.v;
        });
        
        //create material
        create<nfa_material_object>([&](nfa_material_object& obj) {
            obj.nfa = nfa.id;
        });
        
        //fix size to avoid fc::raw::pack_size(nfa_symbol_obj) changing in future structure defination
        size_t new_state_size = TAIYI_NFA_OBJ_STATE_BYTES + TAIYI_NFA_MATERIAL_STATE_BYTES + fc::raw::pack_size(result_table.v);
        add_contract_handler_exe_point(new_state_size + 100);
        
        return nfa;
    }
    //=============================================================================
    void database::process_nfa_tick()
    {
        auto now = head_block_num();

        //list NFAs this tick will sim
        const auto& nfa_idx = get_index< nfa_index, by_tick_time >();
        auto run_num = nfa_idx.size() / TAIYI_NFA_TICK_PERIOD_MAX_BLOCK_NUM + 1;
        auto itnfa = nfa_idx.begin();
        auto endnfa = nfa_idx.end();
        std::vector<const nfa_object*> tick_nfas;
        tick_nfas.reserve(run_num);
        while( itnfa != endnfa && run_num > 0  )
        {
            if(itnfa->next_tick_block > now)
                break;
            
            tick_nfas.push_back(&(*itnfa));
            run_num--;
            
            ++itnfa;
        }
        
        for(const auto* n : tick_nfas) {
            const auto& nfa = *n;

            //欠费限制
            if(nfa.debt_value > 0) {
                //try pay back
                if(nfa.debt_value > nfa.qi.amount.value) {
                    modify(nfa, [&]( nfa_object& obj ) { obj.next_tick_block = std::numeric_limits<uint32_t>::max(); }); //disable tick
                    continue;
                }
                
                wlog("NFA (${i}) pay debt ${v}", ("i", nfa.id)("v", nfa.debt_value));

                //reward contract owner
                const auto& to_contract = get<contract_object, by_id>(nfa.debt_contract);
                const auto& contract_owner = get<account_object, by_id>(to_contract.owner);
                reward_feigang(contract_owner, nfa, asset(nfa.debt_value, QI_SYMBOL));

                modify(nfa, [&](nfa_object& obj) {
                    obj.debt_value = 0;
                    obj.debt_contract = contract_id_type::max();
                });
            }
            
            const auto& contract = get<contract_object, by_id>(nfa.main_contract);
            auto abi_itr = contract.contract_ABI.find(lua_types(lua_string("on_heart_beat")));
            if(abi_itr == contract.contract_ABI.end()) {
                //主合约无心跳入口
                modify(nfa, [&]( nfa_object& obj ) { obj.next_tick_block = std::numeric_limits<uint32_t>::max(); }); //disable tick
                continue;
            }
            
            modify(nfa, [&]( nfa_object& obj ) {
                obj.next_tick_block = now + TAIYI_NFA_TICK_PERIOD_MAX_BLOCK_NUM;
            });

            vector<lua_types> value_list; //no params.
            contract_worker worker;

            LuaContext context;
            initialize_VM_baseENV(context);

            //qi可能在执行合约中被进一步使用，所以这里记录当前的qi来计算虚拟机的执行消耗
            long long old_drops = nfa.qi.amount.value / TAIYI_USEMANA_EXECUTION_SCALE;
            long long vm_drops = old_drops;
            int64_t api_exe_point = 0;
            bool beat_fail = false;
            try {
                auto session = start_undo_session();
                clear_contract_handler_exe_point(); //初始化api执行消耗统计
                const auto& caller = get<account_object, by_id>(nfa.owner_account);
                worker.do_nfa_contract_function(caller, nfa, "on_heart_beat", value_list, contract, vm_drops, true, context, *this, false);
                session.squash();
            }
            catch (const fc::exception& e) {
                //任何错误都不能造成核心循环崩溃
                beat_fail = true;
                wlog("NFA (${i}) process heart beat fail. err: ${e}", ("i", nfa.id)("e", e.to_string()));
            }
            catch (...) {
                //任何错误都不能造成核心循环崩溃
                beat_fail = true;
                wlog("NFA (${i}) process heart beat fail.", ("i", nfa.id));
            }

            int64_t used_drops = old_drops - vm_drops;
            api_exe_point = get_contract_handler_exe_point();

            int64_t used_qi = used_drops * TAIYI_USEMANA_EXECUTION_SCALE;
            int64_t exe_qi = (50 + api_exe_point) * TAIYI_USEMANA_EXECUTION_SCALE;
            int64_t debt_qi = 0;
            if((used_qi + exe_qi) > nfa.qi.amount.value) {
                debt_qi = (used_qi + exe_qi) - nfa.qi.amount.value;
                used_qi = nfa.qi.amount.value;
                exe_qi = 0; //欠款时，本次消耗的真气欠款全部算给合约创建者
                if(!beat_fail) {
                    beat_fail = true;
                    wlog("NFA (${i}) process heart beat successfully, but have not enough qi to next trigger.", ("i", nfa.id));
                }
                else {
                    wlog("NFA (${i}) process heart beat failed, and have not enough qi to next trigger.", ("i", nfa.id));
                }
            }
            
            //reward contract owner
            int64_t used_qi_for_treasury = used_qi * TAIYI_CONTRACT_USEMANA_REWARD_TREASURY_PERCENT / TAIYI_100_PERCENT;
            used_qi -= used_qi_for_treasury;
            if( used_qi > 0)
                reward_feigang(get<account_object, by_id>(contract.owner), nfa, asset(used_qi, QI_SYMBOL));
            
            //reward to treasury
            if( (exe_qi + used_qi_for_treasury) > 0)
                reward_feigang(get<account_object, by_name>(TAIYI_DAO_ACCOUNT), nfa, asset(exe_qi + used_qi_for_treasury, QI_SYMBOL));

            //执行错误不仅要扣费，还会将NFA重置为关闭心跳状态
            if(beat_fail)  {
                modify( nfa, [&]( nfa_object& obj ) {
                    if(debt_qi > 0) {
                        obj.debt_value = debt_qi;
                        obj.debt_contract = contract.id;
                    }
                    obj.next_tick_block = std::numeric_limits<uint32_t>::max();
                });
            }
        }
    }
    //=========================================================================
    asset database::get_nfa_balance( const nfa_object& nfa, asset_symbol_type symbol ) const
    {
        if(symbol.asset_num == TAIYI_ASSET_NUM_QI) {
            return nfa.qi;
        }
        else {
            auto key = boost::make_tuple( nfa.id, symbol );
            const nfa_regular_balance_object* rbo = find< nfa_regular_balance_object, by_nfa_liquid_symbol >( key );
            if( rbo == nullptr )
            {
                return asset(0, symbol);
            }
            else
            {
                return rbo->liquid;
            }
        }
    }
    //=============================================================================
    template< typename nfa_balance_object_type, class balance_operator_type >
    void database::adjust_nfa_balance( const nfa_id_type& nfa_id, const asset& delta, balance_operator_type balance_operator )
    {
        FC_ASSERT(!delta.symbol.is_qi(), "Qi is not go there.");
        
        asset_symbol_type liquid_symbol = delta.symbol;
        const nfa_balance_object_type* bo = find< nfa_balance_object_type, by_nfa_liquid_symbol >( boost::make_tuple( nfa_id, liquid_symbol ) );

        if( bo == nullptr )
        {
            // No balance object related to the FA means '0' balance. Check delta to avoid creation of negative balance.
            FC_ASSERT( delta.amount.value >= 0, "Insufficient FA ${a} funds", ("a", delta.symbol) );
            // No need to create object with '0' balance (see comment above).
            if( delta.amount.value == 0 )
                return;

            create< nfa_balance_object_type >( [&]( nfa_balance_object_type& nfa_balance ) {
                nfa_balance.clear_balance( liquid_symbol );
                nfa_balance.nfa = nfa_id;
                balance_operator.add_to_balance( nfa_balance );
            } );
        }
        else
        {
            bool is_all_zero = false;
            int64_t result = balance_operator.get_combined_balance( bo, &is_all_zero );
            // Check result to avoid negative balance storing.
            FC_ASSERT( result >= 0, "Insufficient Assets ${as} funds", ( "as", delta.symbol ) );

            // Exit if whole balance becomes zero.
            if( is_all_zero )
            {
                // Zero balance is the same as non object balance at all.
                // Remove balance object if liquid balances is zero.
                remove( *bo );
            }
            else
            {
                modify( *bo, [&]( nfa_balance_object_type& nfa_balance ) {
                    balance_operator.add_to_balance( nfa_balance );
                } );
            }
        }
    }
    //=========================================================================
    struct nfa_regular_balance_operator
    {
        nfa_regular_balance_operator( const asset& delta ) : delta(delta) {}

        void add_to_balance( nfa_regular_balance_object& balance )
        {
            balance.liquid += delta;
        }
        int64_t get_combined_balance( const nfa_regular_balance_object* bo, bool* is_all_zero )
        {
            asset result = bo->liquid + delta;
            *is_all_zero = result.amount.value == 0;
            return result.amount.value;
        }

        asset delta;
    };

    void database::adjust_nfa_balance( const nfa_object& nfa, const asset& delta )
    {
        if ( delta.amount < 0 )
        {
            asset available = get_nfa_balance( nfa, delta.symbol );
            FC_ASSERT( available >= -delta,
                      "NFA ${id} does not have sufficient assets for balance adjustment. Required: ${r}, Available: ${a}",
                      ("id", nfa.id)("r", delta)("a", available) );
        }
        
        if( delta.symbol.asset_num == TAIYI_ASSET_NUM_QI) {
            modify(nfa, [&](nfa_object& obj) {
                obj.qi += delta;
            });
        }
        else {
            nfa_regular_balance_operator balance_operator( delta );
            adjust_nfa_balance< nfa_regular_balance_object >( nfa.id, delta, balance_operator );
        }
    }
    //=========================================================================
    //递归修改nfa所有内含子nfa的所有者账号
    void database::modify_nfa_children_owner(const nfa_object& nfa, const account_object& new_owner, std::set<nfa_id_type>& recursion_loop_check)
    {
        vector<nfa_id_type> children;
        const auto& nfa_by_parent_idx = get_index< nfa_index >().indices().get< chain::by_parent >();
        auto itn = nfa_by_parent_idx.lower_bound( nfa.id );
        while(itn != nfa_by_parent_idx.end()) {
            if(itn->parent != nfa.id)
                break;
            children.push_back(itn->id);
            ++itn;
        }

        for (auto _id: children) {
            if(recursion_loop_check.find(_id) != recursion_loop_check.end())
                continue;

            const auto& child_nfa = get<nfa_object, by_id>(_id);
            modify(child_nfa, [&]( nfa_object& obj ) {
                obj.owner_account = new_owner.id;
            });
            
            recursion_loop_check.insert(_id);
            
            modify_nfa_children_owner(child_nfa, new_owner, recursion_loop_check);
        }
    }
    //=========================================================================
    int database::get_nfa_five_phase(const nfa_object& nfa) const
    {
        auto hblock_id = head_block_id();
        
        //金木织药食，分别对应金木水火土
        const auto& material = get<nfa_material_object, by_nfa_id>(nfa.id);
        int64_t phases[5] = {
            material.gold.amount.value,
            material.wood.amount.value,
            material.fabric.amount.value,
            material.herb.amount.value,
            material.food.amount.value
        };
        
        //最大量的一种材料决定五行，如果最大量同时有几种材料，则五行不稳，随机一种
        std::vector<int> maxones; maxones.reserve(5);
        maxones.push_back( 0 );
        for (int i=1; i<5; ++i) {
            if (phases[i] >= phases[maxones[0]]) {
                if (phases[i] > phases[maxones[0]])
                    maxones.clear();
                maxones.push_back(i);
            }
        }
        assert(maxones.size() > 0);
        
        uint32_t p = hasher::hash( hblock_id._hash[4] + nfa.id ) % maxones.size();
        return maxones[p];
    }
    //=========================================================================
    bool database::has_nfa_with_symbol(const account_object& account, const nfa_symbol_id_type& symbol_id) const
    {
        const auto& nfa_idx = get_index<nfa_index>().indices().get<by_owner_symbol>();
        auto found = nfa_idx.lower_bound( boost::make_tuple(account.id, symbol_id, 0) );
        if (found != nfa_idx.end() && found->owner_account == account.id && found->symbol_id == symbol_id)
            return true;
        else
            return false;
    }
    //=========================================================================
    bool database::is_xinsu(const account_object& account) const
    {
        return has_nfa_with_symbol(account, _xinsu_mark_nfa_symbol_id);
    }

} } //taiyi::chain

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

    void database::create_basic_nfa_symbol_objects()
    {
        const auto& creator = get_account( TAIYI_DANUO_ACCOUNT );
        create_nfa_symbol_object(creator, "nfa.actor.default", "默认的角色", "contract.actor.default");
        create_nfa_symbol_object(creator, "nfa.zone.default", "默认的区域", "contract.zone.default");
    }
    //=========================================================================
    size_t database::create_nfa_symbol_object(const account_object& creator, const string& symbol, const string& describe, const string& default_contract)
    {
        const auto* nfa_symbol = find<nfa_symbol_object, by_symbol>(symbol);
        FC_ASSERT(nfa_symbol == nullptr, "NFA symbol named \"${n}\" is already exist.", ("n", symbol));
        
        const auto& contract = find<contract_object, by_name>(default_contract);
        FC_ASSERT(contract != nullptr, "contract object named \"${n}\" is not exist.", ("n", default_contract));
        auto abi_itr = contract->contract_ABI.find(lua_types(lua_string(TAIYI_NFA_INIT_FUNC_NAME)));
        FC_ASSERT(abi_itr != contract->contract_ABI.end(), "contract ${c} has not init function named ${i}", ("c", contract->name)("i", TAIYI_NFA_INIT_FUNC_NAME));
        
        const auto& nfa_symbol_obj = create<nfa_symbol_object>([&](nfa_symbol_object& obj) {
            obj.creator = creator.name;
            obj.symbol = symbol;
            obj.describe = describe;
            obj.default_contract = contract->id;
            obj.count = 0;
        });
        
        return fc::raw::pack_size(nfa_symbol_obj);
    }
    //=========================================================================
    const nfa_object& database::create_nfa(const account_object& creator, const nfa_symbol_object& nfa_symbol, const flat_set<public_key_type>& sigkeys, bool reset_vm_memused, LuaContext& context)
    {
        const auto& caller = creator;
        
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
        
        //evaluate contract authority
        if (contract.check_contract_authority)
        {
            auto skip = node_properties().skip_flags;
            if (!(skip & (database::validation_steps::skip_transaction_signatures | database::validation_steps::skip_authority_check)))
            {
                auto key_itr = std::find(sigkeys.begin(), sigkeys.end(), contract.contract_authority);
                FC_ASSERT(key_itr != sigkeys.end(), "No contract related permissions were found in the signature, contract_authority:${c}", ("c", contract.contract_authority));
            }
        }
                
        contract_worker worker;
        vector<lua_types> value_list;
        
        //qi可能在执行合约中被进一步使用，所以这里记录当前的qi来计算虚拟机的执行消耗
        long long old_drops = caller.qi.amount.value / TAIYI_USEMANA_EXECUTION_SCALE;
        long long vm_drops = old_drops;
        lua_table result_table = worker.do_contract_function_return_table(caller, TAIYI_NFA_INIT_FUNC_NAME, value_list, sigkeys, contract, vm_drops, reset_vm_memused, context, *this);
        int64_t used_drops = old_drops - vm_drops;
        
        size_t new_state_size = fc::raw::pack_size(nfa);
        int64_t used_qi = used_drops * TAIYI_USEMANA_EXECUTION_SCALE + new_state_size * TAIYI_USEMANA_STATE_BYTES_SCALE + 100 * TAIYI_USEMANA_EXECUTION_SCALE;
        FC_ASSERT( caller.qi.amount.value >= used_qi, "Creator account does not have enough qi to create nfa." );
        
        //reward contract owner
        const auto& contract_owner = get<account_object, by_id>(contract.owner);
        reward_feigang(contract_owner, caller, asset(used_qi, QI_SYMBOL));
                        
        //init nfa from result table
        modify(nfa, [&](nfa_object& obj) {
            obj.contract_data = result_table.v;
        });
        
        return nfa;
    }
    //=============================================================================
    void database::process_nfa_tick()
    {
        auto now = head_block_time();

        //list NFAs this tick will sim
        const auto& nfa_idx = get_index< nfa_index, by_tick_time >();
        auto run_num = nfa_idx.size() / TAIYI_NFA_TICK_PERIOD_MAX_BLOCK_NUM + 1;
        auto itnfa = nfa_idx.begin();
        auto endnfa = nfa_idx.end();
        std::vector<const nfa_object*> tick_nfas;
        tick_nfas.reserve(run_num);
        while( itnfa != endnfa && run_num > 0  )
        {
            if(itnfa->next_tick_time > now)
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
                    modify(nfa, [&]( nfa_object& obj ) { obj.next_tick_time = time_point_sec::maximum(); }); //disable tick
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
            
            const auto* contract_ptr = find<contract_object, by_id>(nfa.main_contract);
            if(contract_ptr == nullptr)
                continue;
            auto abi_itr = contract_ptr->contract_ABI.find(lua_types(lua_string("on_heart_beat")));
            if(abi_itr == contract_ptr->contract_ABI.end()) {
                modify(nfa, [&]( nfa_object& obj ) { obj.next_tick_time = time_point_sec::maximum(); }); //disable tick
                continue;
            }

            modify(nfa, [&]( nfa_object& obj ) {
                obj.next_tick_time = now + TAIYI_NFA_TICK_PERIOD_MAX_BLOCK_NUM * TAIYI_BLOCK_INTERVAL;
            });

            vector<lua_types> value_list; //no params.
            contract_worker worker;

            LuaContext context;
            initialize_VM_baseENV(context);
            flat_set<public_key_type> sigkeys;

            //qi可能在执行合约中被进一步使用，所以这里记录当前的qi来计算虚拟机的执行消耗
            long long old_drops = nfa.qi.amount.value / TAIYI_USEMANA_EXECUTION_SCALE;
            long long vm_drops = old_drops;
            bool beat_fail = false;
            try {
                auto session = start_undo_session();
                worker.do_nfa_contract_function(nfa, "on_heart_beat", value_list, sigkeys, *contract_ptr, vm_drops, true, context, *this);
                session.push();
            }
            catch (fc::exception e) {
                //任何错误都不能照成核心循环崩溃
                beat_fail = true;
                wlog("NFA (${i}) process heart beat fail. err: ${e}", ("i", nfa.id)("e", e.to_string()));
            }
            catch (...) {
                //任何错误都不能照成核心循环崩溃
                beat_fail = true;
                wlog("NFA (${i}) process heart beat fail.", ("i", nfa.id));
            }
            int64_t used_drops = old_drops - vm_drops;

            int64_t used_qi = used_drops * TAIYI_USEMANA_EXECUTION_SCALE + 50 * TAIYI_USEMANA_EXECUTION_SCALE;
            int64_t debt_qi = 0;
            if(used_qi > nfa.qi.amount.value) {
                debt_qi = used_qi - nfa.qi.amount.value;
                used_qi = nfa.qi.amount.value;
                if(!beat_fail) {
                    beat_fail = true;
                    wlog("NFA (${i}) process heart beat successfully, but have not enough qi to next trigger.", ("i", nfa.id));
                }
                else {
                    wlog("NFA (${i}) process heart beat failed, and have not enough qi to next trigger.", ("i", nfa.id));
                }
            }
            
            //reward contract owner
            if( used_qi > 0) {
                const auto& contract_owner = get<account_object, by_id>(contract_ptr->owner);
                reward_feigang(contract_owner, nfa, asset(used_qi, QI_SYMBOL));
            }

            //执行错误不仅要扣费，还会将NFA重置为关闭心跳状态
            if(beat_fail)  {
                modify( nfa, [&]( nfa_object& obj ) {
                    if(debt_qi > 0) {
                        obj.debt_value = debt_qi;
                        obj.debt_contract = contract_ptr->id;
                    }
                    obj.next_tick_time = time_point_sec::maximum();
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

} } //taiyi::chain

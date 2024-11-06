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

#include <chain/lua_context.hpp>
#include <chain/contract_worker.hpp>

#include <chain/util/uint256.hpp>
#include <chain/util/manabar.hpp>

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

    const nfa_object& database::create_nfa(const account_object& creator, const nfa_symbol_object& nfa_symbol, const flat_set<public_key_type>& sigkeys, bool reset_vm_memused, LuaContext& context)
    {
        const auto& caller = creator;
        modify( caller, [&]( account_object& a ) {
            util::update_manabar( get_dynamic_global_properties(), a, true );
        });
        
        const auto& nfa = create<nfa_object>([&](nfa_object& obj) {
            obj.creator_account = creator.id;
            obj.owner_account = creator.id;
            
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
        
        const auto* op_acd = find<account_contract_data_object, by_account_contract>(boost::make_tuple(caller.id, contract.id));
        if(op_acd == nullptr) {
            create<account_contract_data_object>([&](account_contract_data_object &a) {
                a.owner = caller.id;
                a.contract_id = contract.id;
            });
            op_acd = find<account_contract_data_object, by_account_contract>(boost::make_tuple(caller.id, contract.id));
        }
        lua_map account_data = op_acd->contract_data;
        
        contract_result result;
        contract_worker worker;
        vector<lua_types> value_list;
        
        //mana可能在执行合约中被进一步使用，所以这里记录当前的mana来计算虚拟机的执行消耗
        long long old_drops = caller.manabar.current_mana / TAIYI_USEMANA_EXECUTION_SCALE;
        long long vm_drops = old_drops;
        lua_table result_table = worker.do_contract_function_return_table(caller, TAIYI_NFA_INIT_FUNC_NAME, value_list, account_data, sigkeys, result, contract, vm_drops, reset_vm_memused, context, *this);
        int64_t used_drops = old_drops - vm_drops;
        
        size_t new_state_size = fc::raw::pack_size(nfa);
        int64_t used_mana = used_drops * TAIYI_USEMANA_EXECUTION_SCALE + new_state_size * TAIYI_USEMANA_STATE_BYTES_SCALE + 100 * TAIYI_USEMANA_EXECUTION_SCALE;
        FC_ASSERT( caller.manabar.has_mana(used_mana), "Creator account does not have enough mana to create nfa." );
        modify( caller, [&]( account_object& a ) {
            a.manabar.use_mana( used_mana );
        });
        
        //reward contract owner
        const auto& contract_owner = get<account_object, by_id>(contract.owner);
        reward_contract_owner(contract_owner.name, asset(used_mana, QI_SYMBOL));
        
        uint64_t contract_private_data_size    = 3L * 1024;
        uint64_t contract_total_data_size      = 10L * 1024 * 1024;
        uint64_t contract_max_data_size        = 2L * 1024 * 1024 * 1024;
        FC_ASSERT(fc::raw::pack_size(account_data) <= contract_private_data_size, "the contract private data size is too large.");
        FC_ASSERT(fc::raw::pack_size(contract.contract_data) <= contract_total_data_size, "the contract total data size is too large.");
        
        modify(*op_acd, [&](account_contract_data_object &a) {
            a.contract_data = account_data;
        });
        
        //init nfa from result table
        modify(nfa, [&](nfa_object& obj) {
            obj.data = result_table.v;
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
            ++itnfa;
            run_num--;
        }
        
        for(const auto* n : tick_nfas) {
            const auto& nfa = *n;
            
            const auto* contract_ptr = find<contract_object, by_id>(nfa.main_contract);
            if(contract_ptr == nullptr)
                continue;
            auto abi_itr = contract_ptr->contract_ABI.find(lua_types(lua_string("heart_beat")));
            if(abi_itr == contract_ptr->contract_ABI.end()) {
                modify(nfa, [&]( nfa_object& obj ) { obj.next_tick_time = time_point_sec::maximum(); }); //disable tick
                continue;
            }

            modify(nfa, [&]( nfa_object& obj ) {
                obj.next_tick_time = now + TAIYI_NFA_TICK_PERIOD_MAX_BLOCK_NUM * TAIYI_BLOCK_INTERVAL;
                util::update_manabar( get_dynamic_global_properties(), obj, true );
            });

            vector<lua_types> value_list(1, lua_table()); //no params, so one empty table.
            contract_result cresult;
            contract_worker worker;

            LuaContext context;
            initialize_VM_baseENV(context);
            
            //mana可能在执行合约中被进一步使用，所以这里记录当前的mana来计算虚拟机的执行消耗
            long long old_drops = nfa.manabar.current_mana / TAIYI_USEMANA_EXECUTION_SCALE;
            long long vm_drops = old_drops;
            bool beat_fail = false;
            try {
                worker.do_nfa_contract_action(nfa, "heart_beat", value_list, cresult, vm_drops, true, context, *this);
            }
            catch (...) {
                //任何错误都不能照成核心循环崩溃
                beat_fail = true;
                wlog("NFA (${i}) process heart beat fail.", ("i", nfa.id));
            }
            int64_t used_drops = old_drops - vm_drops;

            int64_t used_mana = used_drops * TAIYI_USEMANA_EXECUTION_SCALE + 50 * TAIYI_USEMANA_EXECUTION_SCALE;
            modify( nfa, [&]( nfa_object& obj ) {
                if( obj.manabar.current_mana < used_mana )
                    obj.manabar.current_mana = 0;
                else
                    obj.manabar.current_mana -= used_mana;
                //执行错误不仅要扣费，还会将NFA重置为关闭心跳状态
                if(beat_fail)
                    obj.next_tick_time = time_point_sec::maximum();
            });
            
            //reward contract owner
            const auto& contract_owner = get<account_object, by_id>(contract_ptr->owner);
            reward_contract_owner(contract_owner.name, asset(used_mana, QI_SYMBOL));
        }
    }

} } //taiyi::chain

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

//=============================================================================
const nfa_object& database::create_nfa(const account_object& creator, const nfa_symbol_object& nfa_symbol, const flat_set<public_key_type>& sigkeys)
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

    //run contract function
    lua_settop (get_luaVM().mState, 0);
    
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
    long long vm_drops = caller.manabar.current_mana;
    lua_table result_table = worker.do_contract_function_return_table(caller, TAIYI_NFA_INIT_FUNC_NAME, value_list, account_data, sigkeys, result, contract, vm_drops, *this);
    int64_t used_drops = caller.manabar.current_mana - vm_drops;

    size_t new_state_size = fc::raw::pack_size(nfa);
    int64_t used_mana = used_drops + new_state_size * TAIYI_USEMANA_STATE_BYTES_SCALE + 100 * TAIYI_USEMANA_EXECUTION_SCALE;
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

} } //taiyi::chain

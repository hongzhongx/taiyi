#include <chain/taiyi_fwd.hpp>

#include <chain/taiyi_evaluator.hpp>
#include <chain/database.hpp>

#include <chain/taiyi_objects.hpp>
#include <chain/block_summary_object.hpp>
#include <chain/account_object.hpp>
#include <chain/contract_objects.hpp>

#include <chain/lua_context.hpp>
#include <chain/contract_worker.hpp>

#include <lua.hpp>

#include <fc/macros.hpp>

FC_TODO( "After we vendor fc, also vendor diff_match_patch and fix these warnings" )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wsign-compare"
#   pragma GCC diagnostic push
#   if !defined( __clang__ )
#       pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#   endif
#   include <diff_match_patch.h>
#   pragma GCC diagnostic pop
#   pragma GCC diagnostic pop
#   include <boost/locale/encoding_utf.hpp>

#include <limits>

extern std::wstring utf8_to_wstring(const std::string& str);
extern std::string wstring_to_utf8(const std::wstring& str);

namespace taiyi { namespace chain {

    operation_result create_contract_evaluator::do_apply( const create_contract_operation& o )
    { try {
        lua_settop (_db.get_luaVM().mState, 0);
        
        if(o.name=="contract.blacklist")
            FC_ASSERT(o.owner == TAIYI_COMMITTEE_ACCOUNT);
        
        long long vm_drops = 1000000; //TODO: fee
        _db.create_contract_objects( o.owner, o.name, o.data, o.contract_authority, vm_drops );
        
        return void_result();
    } FC_CAPTURE_AND_RETHROW( (o) ) }
    //=============================================================================
    operation_result revise_contract_evaluator::do_apply( const revise_contract_operation& o )
    { try {
        const auto &old_contract = _db.get<contract_object, by_name>(o.contract_name);
        const auto &contract_owner = _db.get<account_object, by_id>(old_contract.owner);
        
        if(o.reviser != TAIYI_COMMITTEE_ACCOUNT)
            FC_ASSERT(old_contract.can_do(_db), "The current contract may have been listed in the forbidden call list");
        
        FC_ASSERT(!old_contract.is_release," The current contract is release version cannot be change ");
        FC_ASSERT(contract_owner.name == o.reviser, "You do not have the authority to modify the contract, the contract owner is ${owner}", ("owner", contract_owner.name));
        
        lua_settop(_db.get_luaVM().mState, 0);
        
        vector<char> lua_code_b;
        contract_worker worker;
        long long vm_drops = 1000000; //TODO: fee
        lua_table aco = worker.do_contract(old_contract.id, old_contract.name, o.data, lua_code_b, vm_drops, _db.get_luaVM().mState, _db);
        const auto& old_code_bin_object = _db.get<contract_bin_code_object, by_id>(old_contract.lua_code_b_id);
        _db.modify(old_code_bin_object, [&](contract_bin_code_object&cbo) { cbo.lua_code_b = lua_code_b; });
        string previous_version = old_contract.current_version.str();
        _db.modify(old_contract, [&](contract_object &c) {
            c.current_version = _db.get_current_trx();
            c.contract_ABI = aco.v;
        });
        
        return logger_result(previous_version);
    } FC_CAPTURE_AND_RETHROW( (o) ) }
    //=============================================================================
    operation_result call_contract_function_evaluator::do_apply( const call_contract_function_operation& o )
    { try {
        
        const auto& caller = _db.get<account_object, by_name>(o.caller);
        const auto& contract = _db.get<contract_object, by_name>(o.contract_name);
        
        const auto* current_trx = _db.get_current_trx_ptr();
        FC_ASSERT(current_trx);
        const flat_set<public_key_type>& sigkeys = current_trx->get_signature_keys(_db.get_chain_id(), fc::ecc::fc_canonical);
        
        //evaluate contract authority
        if(o.caller != TAIYI_COMMITTEE_ACCOUNT)
            FC_ASSERT(contract.can_do(_db), "The current contract \"${n}\" may have been listed in the forbidden call list", ("n", o.contract_name));
        if (contract.check_contract_authority)
        {
            auto skip = _db.node_properties().skip_flags;
            if (!(skip & (database::validation_steps::skip_transaction_signatures | database::validation_steps::skip_authority_check)))
            {
                auto key_itr = std::find(sigkeys.begin(), sigkeys.end(), contract.contract_authority);
                FC_ASSERT(key_itr != sigkeys.end(), "No contract related permissions were found in the signature, contract_authority:${c}", ("c", contract.contract_authority));
            }
        }
        
        //run contract function
        lua_settop (_db.get_luaVM().mState, 0);
        
        const auto* op_acd = _db.find<account_contract_data_object, by_account_contract>(boost::make_tuple(caller.id, contract.id));
        if(op_acd == nullptr) {
            _db.create<account_contract_data_object>([&](account_contract_data_object &a) {
                a.owner = caller.id;
                a.contract_id = contract.id;
            });
            op_acd = _db.find<account_contract_data_object, by_account_contract>(boost::make_tuple(caller.id, contract.id));
        }
        lua_map account_data = op_acd->contract_data;
        
        contract_result result;
        contract_worker worker;
        worker.do_contract_function(caller, o.function_name, o.value_list, account_data, sigkeys, result, contract, _db);
        
        uint64_t contract_private_data_size    = 3L * 1024;
        uint64_t contract_total_data_size      = 10L * 1024 * 1024;
        uint64_t contract_max_data_size        = 2L * 1024 * 1024 * 1024;
        FC_ASSERT(fc::raw::pack_size(account_data) <= contract_private_data_size, "the contract private data size is too large.");
        FC_ASSERT(fc::raw::pack_size(contract.contract_data) <= contract_total_data_size, "the contract total data size is too large.");
        
        _db.modify(*op_acd, [&](account_contract_data_object &a) {
            a.contract_data = account_data;
        });
        
        return result;
    } FC_CAPTURE_AND_RETHROW( (o) ) }
    
} } // taiyi::chain


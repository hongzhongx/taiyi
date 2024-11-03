#pragma once

#include "types.hpp"
#include "asset.hpp"
#include "authority.hpp"
#include "version.hpp"

#include <fc/time.hpp>

namespace taiyi { namespace protocol {

    enum nfa_affected_type
    {
        transfer_from = 0,
        transfer_to = 1,
        modified = 2,
        create_by=3,
        create_for = 4,
        relate_nfa_symbol = 5
    };

    typedef struct nfa_affected
    {
        account_name_type affected_account;
        int64_t affected_item;
        nfa_affected_type action;
        optional<std::pair<string, string>> modified;
    } nfa_affected;

    struct base_result
    {
        optional<vector<asset>> fees;
        base_result() {};
    };

    typedef base_result void_result;

    typedef struct error_result : public base_result
    {
        int64_t error_code = 0;
        string message;
        error_result(int64_t error_code, string message) : error_code(error_code), message(message) {}
        error_result() {}
    } error_result;

    struct asset_result : public base_result
    {
        asset result;
        asset_result() {}
        asset_result(asset result) : result(result) {}
    };

    typedef struct logger_result : public base_result
    {
        string message;
        logger_result(string message) : message(message) {}
        logger_result() {}
    } logger_result;

    typedef struct asset_affected
    {
        account_name_type affected_account;
        asset affected_asset;
    } asset_affected;

    typedef struct contract_memo_message
    {
        account_name_type affected_account;
        protocol::memo_data memo;
    } contract_memo_message;

    typedef struct contract_logger
    {
        account_name_type affected_account;
        string message;
        contract_logger() {}
        contract_logger(account_name_type aft) : affected_account(aft) {}
    } contract_logger;

    struct contract_result;
    typedef fc::static_variant<
        asset_affected,
        nfa_affected,
        contract_memo_message,
        contract_logger,
        contract_result
    > contract_affected_type;

    typedef struct contract_result : public base_result
    {
    public:
        string contract_name;
        vector<contract_affected_type> contract_affecteds;
        bool existed_pv = false;
        vector<char> process_value;
        uint64_t relevant_datasize = 0;
    } contract_result;

    typedef fc::static_variant<
        void_result,
        error_result,
        asset_result,
        contract_result,
        logger_result
    > operation_result;

    struct contract_affected_type_visitor
    {
        typedef vector<account_name_type> result_type;
        template <typename contract_affected_type_op>
        result_type operator()(contract_affected_type_op &op)
        {
            return vector<account_name_type>{op.affected_account};
        }
        result_type operator()(contract_result &op)
        {
            vector<account_name_type> result;
            contract_affected_type_visitor visitor;
            for (auto var : op.contract_affecteds)
            {
                auto temp = var.visit(visitor);
                result.insert(result.end(), temp.begin(), temp.end());
            }
            return result;
        }
    };

    struct base_operation
    {
        void get_required_authorities( vector<authority>& )const {}
        void get_required_active_authorities( flat_set<account_name_type>& )const {}
        void get_required_posting_authorities( flat_set<account_name_type>& )const {}
        void get_required_owner_authorities( flat_set<account_name_type>& )const {}
        
        bool is_virtual()const { return false; }
        void validate()const {}
    };

    struct virtual_operation : public base_operation
    {
        bool is_virtual()const { return true; }
        void validate()const { FC_ASSERT( false, "This is a virtual operation" ); }
    };

    typedef static_variant<
        void_t
    > future_extensions;

    typedef flat_set<future_extensions> extensions_type;

} } // taiyi::protocol

FC_REFLECT_ENUM(taiyi::protocol::nfa_affected_type, (transfer_from)(transfer_to)(modified)(create_by)(create_for)(relate_nfa_symbol))
FC_REFLECT(taiyi::protocol::nfa_affected, (affected_account)(affected_item)(action)(modified))

FC_REFLECT_TYPENAME(taiyi::protocol::operation_result)
FC_REFLECT_TYPENAME(taiyi::protocol::contract_affected_type)
FC_REFLECT_TYPENAME(taiyi::protocol::future_extensions)

FC_REFLECT(taiyi::protocol::base_result, (fees))
FC_REFLECT_DERIVED(taiyi::protocol::asset_result, (taiyi::protocol::base_result), (result))
FC_REFLECT(taiyi::protocol::asset_affected, (affected_account)(affected_asset))
FC_REFLECT(taiyi::protocol::contract_memo_message, (affected_account)(memo))
FC_REFLECT(taiyi::protocol::contract_logger, (affected_account)(message))
FC_REFLECT_DERIVED(taiyi::protocol::contract_result, (taiyi::protocol::base_result), (contract_name)(contract_affecteds)(existed_pv)(process_value)(relevant_datasize))
FC_REFLECT_DERIVED(taiyi::protocol::logger_result, (taiyi::protocol::base_result), (message))
FC_REFLECT_DERIVED(taiyi::protocol::error_result, (taiyi::protocol::base_result), (error_code)(message))

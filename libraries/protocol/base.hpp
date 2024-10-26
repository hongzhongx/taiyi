#pragma once

#include "types.hpp"
#include "asset.hpp"
#include "authority.hpp"
#include "version.hpp"

#include <fc/time.hpp>

namespace taiyi { namespace protocol {

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

    typedef struct token_affected
    {
        account_name_type affected_account;
        asset affected_asset;
    } token_affected;

    typedef fc::static_variant<
        void_result,
        error_result,
        asset_result,
        logger_result
    > operation_result;

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

FC_REFLECT_TYPENAME(taiyi::protocol::operation_result)
FC_REFLECT_TYPENAME(taiyi::protocol::future_extensions)

FC_REFLECT(taiyi::protocol::base_result, (fees))
FC_REFLECT_DERIVED(taiyi::protocol::asset_result, (taiyi::protocol::base_result), (result))
FC_REFLECT(taiyi::protocol::token_affected, (affected_account)(affected_asset))
FC_REFLECT_DERIVED(taiyi::protocol::logger_result, (taiyi::protocol::base_result), (message))
FC_REFLECT_DERIVED(taiyi::protocol::error_result, (taiyi::protocol::base_result), (error_code)(message))

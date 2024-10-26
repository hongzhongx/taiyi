#pragma once

#include <fc/container/flat.hpp>
#include <protocol/operations.hpp>
#include <protocol/transaction.hpp>

#include <fc/string.hpp>

namespace taiyi { namespace chain {

    using namespace fc;
    
    void operation_get_impacted_accounts(const taiyi::protocol::operation& op, fc::flat_set<protocol::account_name_type>& result );
    
    void transaction_get_impacted_accounts(const taiyi::protocol::transaction& tx, fc::flat_set<protocol::account_name_type>& result);

} } // taiyi::chain

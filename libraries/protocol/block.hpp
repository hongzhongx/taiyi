#pragma once
#include "block_header.hpp"
#include "transaction.hpp"

namespace taiyi { namespace protocol {

    struct signed_block : public signed_block_header
    {
        checksum_type calculate_merkle_root()const;
        vector<signed_transaction> transactions;
    };

} } // taiyi::protocol

FC_REFLECT_DERIVED( taiyi::protocol::signed_block, (taiyi::protocol::signed_block_header), (transactions) )

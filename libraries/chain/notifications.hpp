#pragma once

#include <protocol/block.hpp>

namespace taiyi { namespace chain {

    struct block_notification
    {
        block_notification( const taiyi::protocol::signed_block& b ) : block(b)
        {
            block_id = b.id();
            block_num = taiyi::protocol::block_header::num_from_id( block_id );
        }
        
        taiyi::protocol::block_id_type          block_id;
        uint32_t                                block_num = 0;
        const taiyi::protocol::signed_block&    block;
    };

    struct transaction_notification
    {
        transaction_notification( const taiyi::protocol::signed_transaction& tx ) : transaction(tx)
        {
            transaction_id = tx.id();
        }
        
        taiyi::protocol::transaction_id_type          transaction_id;
        const taiyi::protocol::signed_transaction&    transaction;
    };
    
    struct operation_notification
    {
        operation_notification( const taiyi::protocol::operation& o ) : op(o) {}
        
        transaction_id_type trx_id;
        uint32_t            block = 0;
        uint32_t            trx_in_block = 0;
        uint32_t            op_in_trx = 0;
        uint32_t            virtual_op = 0;
        const taiyi::protocol::operation&    op;
    };

} } //taiyi::chain

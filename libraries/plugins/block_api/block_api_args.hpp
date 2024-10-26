#pragma once
#include <plugins/block_api/block_api_objects.hpp>

#include <protocol/types.hpp>
#include <protocol/transaction.hpp>
#include <protocol/block_header.hpp>

#include <plugins/json_rpc/utility.hpp>

namespace taiyi { namespace plugins { namespace block_api {

    /* get_block_header */
    
    struct get_block_header_args
    {
        uint32_t block_num;
    };
    
    struct get_block_header_return
    {
        optional< block_header > header;
    };
    
    /* get_block */
    struct get_block_args
    {
        uint32_t block_num;
    };
    
    struct get_block_return
    {
        optional< api_signed_block_object > block;
    };
    
} } } // taiyi::block_api

FC_REFLECT( taiyi::plugins::block_api::get_block_header_args, (block_num) )
FC_REFLECT( taiyi::plugins::block_api::get_block_header_return, (header) )
FC_REFLECT( taiyi::plugins::block_api::get_block_args, (block_num) )
FC_REFLECT( taiyi::plugins::block_api::get_block_return, (block) )


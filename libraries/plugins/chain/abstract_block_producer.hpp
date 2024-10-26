#pragma once

#include <fc/time.hpp>

#include <chain/database.hpp>

namespace taiyi { namespace plugins { namespace chain {
   
    class abstract_block_producer
    {
    public:
        virtual ~abstract_block_producer() = default;
        
        virtual taiyi::chain::signed_block generate_block( fc::time_point_sec when, const taiyi::chain::account_name_type& siming_owner, const fc::ecc::private_key& block_signing_private_key, uint32_t skip = taiyi::chain::database::skip_nothing) = 0;
    };
    
} } } // taiyi::plugins::chain

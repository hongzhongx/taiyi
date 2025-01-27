#pragma once
#include <chain/taiyi_fwd.hpp>

#include <protocol/authority.hpp>
#include <protocol/taiyi_operations.hpp>

#include <chain/taiyi_object_types.hpp>

namespace taiyi { namespace chain {
    
    struct nature_place_def
    {
        std::string kind;
        std::vector<E_ZONE_TYPE> types;
    };
    
    const std::vector<nature_place_def>& g_get_nature_places();

} } // taiyi::chain

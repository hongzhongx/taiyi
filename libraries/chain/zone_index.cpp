
#include <chain/taiyi_object_types.hpp>
#include <chain/index.hpp>

#include <chain/account_object.hpp>
#include <chain/tiandao_property_object.hpp>
#include <chain/contract_objects.hpp>
#include <chain/nfa_objects.hpp>
#include <chain/actor_objects.hpp>
#include <chain/zone_objects.hpp>

namespace taiyi { namespace chain {

    void initialize_zone_indexes( database& db )
    {
        TAIYI_ADD_CORE_INDEX(db, tiandao_property_index);

        TAIYI_ADD_CORE_INDEX(db, zone_index);
        TAIYI_ADD_CORE_INDEX(db, zone_connect_index);
        TAIYI_ADD_CORE_INDEX(db, zone_contract_permission_index);
        TAIYI_ADD_CORE_INDEX(db, cunzhuang_index);
    }

} }

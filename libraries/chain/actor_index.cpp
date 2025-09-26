
#include <chain/taiyi_object_types.hpp>
#include <chain/index.hpp>

#include <chain/account_object.hpp>
#include <chain/contract_objects.hpp>
#include <chain/nfa_objects.hpp>
#include <chain/zone_objects.hpp>
#include <chain/actor_objects.hpp>

namespace taiyi { namespace chain {

    void initialize_actor_indexes( database& db )
    {
        TAIYI_ADD_CORE_INDEX(db, actor_index);
        TAIYI_ADD_CORE_INDEX(db, actor_core_attributes_index);
        TAIYI_ADD_CORE_INDEX(db, actor_group_index);
        TAIYI_ADD_CORE_INDEX(db, actor_talent_rule_index);
        TAIYI_ADD_CORE_INDEX(db, actor_talents_index);
        TAIYI_ADD_CORE_INDEX(db, actor_relation_index);
        TAIYI_ADD_CORE_INDEX(db, actor_connection_index);
    }

} }

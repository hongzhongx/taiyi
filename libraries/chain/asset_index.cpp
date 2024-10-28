
#include <chain/taiyi_object_types.hpp>
#include <chain/index.hpp>

#include <chain/asset_objects/asset_objects.hpp>

namespace taiyi { namespace chain {

    void initialize_asset_indexes( database& db )
    {
        TAIYI_ADD_CORE_INDEX(db, account_regular_balance_index);
        TAIYI_ADD_CORE_INDEX(db, account_rewards_balance_index);
    }

} }

#include <chain/taiyi_object_types.hpp>
#include <chain/index.hpp>

#include <chain/account_object.hpp>
#include <chain/contract_objects.hpp>

namespace taiyi { namespace chain {

    void initialize_contract_indexes( database& db )
    {
        TAIYI_ADD_CORE_INDEX(db, contract_index);
        TAIYI_ADD_CORE_INDEX(db, account_contract_data_index);
        TAIYI_ADD_CORE_INDEX(db, contract_bin_code_index);
    }

} }

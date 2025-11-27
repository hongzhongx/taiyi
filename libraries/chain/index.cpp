#include <chain/taiyi_object_types.hpp>
#include <chain/index.hpp>

#include <chain/block_summary_object.hpp>
#include <chain/taiyi_objects.hpp>
#include <chain/account_object.hpp>
#include <chain/transaction_object.hpp>
#include <chain/siming_objects.hpp>
#include <chain/tps_objects.hpp>

namespace taiyi { namespace chain {

    void initialize_asset_indexes( database& db );
    void initialize_contract_indexes( database& db );
    void initialize_actor_indexes( database& db );
    void initialize_zone_indexes( database& db );

    void initialize_core_indexes( database& db )
    {
        TAIYI_ADD_CORE_INDEX(db, dynamic_global_property_index);
        TAIYI_ADD_CORE_INDEX(db, account_index);
        TAIYI_ADD_CORE_INDEX(db, account_metadata_index);
        TAIYI_ADD_CORE_INDEX(db, account_authority_index);
        TAIYI_ADD_CORE_INDEX(db, siming_index);
        TAIYI_ADD_CORE_INDEX(db, transaction_index);
        TAIYI_ADD_CORE_INDEX(db, block_summary_index);
        TAIYI_ADD_CORE_INDEX(db, siming_schedule_index);
        TAIYI_ADD_CORE_INDEX(db, siming_adore_index);
        TAIYI_ADD_CORE_INDEX(db, hardfork_property_index);
        TAIYI_ADD_CORE_INDEX(db, withdraw_qi_route_index);
        TAIYI_ADD_CORE_INDEX(db, owner_authority_history_index);
        TAIYI_ADD_CORE_INDEX(db, account_recovery_request_index);
        TAIYI_ADD_CORE_INDEX(db, change_recovery_account_request_index);
        TAIYI_ADD_CORE_INDEX(db, decline_adoring_rights_request_index);
        TAIYI_ADD_CORE_INDEX(db, reward_fund_index);
        TAIYI_ADD_CORE_INDEX(db, qi_delegation_index);
        TAIYI_ADD_CORE_INDEX(db, qi_delegation_expiration_index);

        initialize_asset_indexes(db);
        initialize_contract_indexes(db);
        initialize_actor_indexes(db);
        initialize_zone_indexes(db);
        
        TAIYI_ADD_CORE_INDEX(db, proposal_index);
        TAIYI_ADD_CORE_INDEX(db, proposal_vote_index);
    }
    
    index_info::index_info() {}
    index_info::~index_info() {}

} }

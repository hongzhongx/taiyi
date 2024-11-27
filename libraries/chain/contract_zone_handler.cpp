#include <chain/taiyi_fwd.hpp>

#include <chain/taiyi_evaluator.hpp>
#include <chain/database.hpp>

#include <chain/taiyi_objects.hpp>
#include <chain/block_summary_object.hpp>
#include <chain/account_object.hpp>
#include <chain/contract_objects.hpp>
#include <chain/nfa_objects.hpp>
#include <chain/zone_objects.hpp>

#include <chain/lua_context.hpp>

#include <chain/contract_handles.hpp>

namespace taiyi { namespace chain {

    contract_zone_base_info::contract_zone_base_info(const zone_object& z, database& db)
    : nfa_id(z.nfa_id), name(z.name)
    {
        type = get_zone_type_string(z.type);
    }

} } // namespace taiyi::chain

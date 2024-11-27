#include <chain/taiyi_fwd.hpp>

#include <chain/taiyi_evaluator.hpp>
#include <chain/database.hpp>

#include <chain/taiyi_objects.hpp>
#include <chain/block_summary_object.hpp>
#include <chain/account_object.hpp>
#include <chain/contract_objects.hpp>
#include <chain/nfa_objects.hpp>
#include <chain/actor_objects.hpp>
#include <chain/zone_objects.hpp>

#include <chain/lua_context.hpp>

#include <chain/contract_handles.hpp>

namespace taiyi { namespace chain {

    contract_actor_base_info::contract_actor_base_info(const actor_object& a, database& db)
    : nfa_id(a.nfa_id), name(a.name), age(a.age), health(a.health), health_max(a.health_max), born(a.born), born_vyears(a.born_vyears), born_vmonths(a.born_vmonths), born_vtimes(a.born_vtimes), five_phase(a.five_phase), gender(a.gender), standpoint_type(a.get_standpoint_type())
    {
        if(born) {
            location = db.get<zone_object, by_id>(a.location).name;
            base = db.get< zone_object, by_id >(a.base).name;
        }
    }

} } // namespace taiyi::chain

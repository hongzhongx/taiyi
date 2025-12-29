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
    : nfa_id(a.nfa_id), name(a.name), age(a.age), health(a.health), health_max(a.health_max), born(a.born), born_vyears(a.born_vyears), born_vmonths(a.born_vmonths), born_vdays(a.born_vdays), born_vtod(a.born_vtod), born_vtimes(a.born_vtimes), gender(a.gender), standpoint(a.standpoint), standpoint_type(a.get_standpoint_type())
    {
        if(born) {
            location = db.get<zone_object, by_id>(a.location).name;
            base = db.get< zone_object, by_id >(a.base).name;
        }
        
        five_phase = db.get_nfa_five_phase(db.get<nfa_object, by_id>(a.nfa_id));
        
        const actor_talents_object& talents_obj = db.get<actor_talents_object, by_actor>(a.id);
        for(auto itlt = talents_obj.talents.begin(); itlt != talents_obj.talents.end(); itlt++)
            talents.push_back(itlt->first);
    }
    //=========================================================================
    contract_actor_core_attributes::contract_actor_core_attributes(const actor_object & act, database& db)
    {
        const auto* maybe_core_attrs = db.find< actor_core_attributes_object, by_actor >( act.id );
        if( maybe_core_attrs ) {
            strength = maybe_core_attrs->strength;
            physique = maybe_core_attrs->physique;
            agility = maybe_core_attrs->agility;
            vitality = maybe_core_attrs->vitality;
            comprehension = maybe_core_attrs->comprehension;
            willpower = maybe_core_attrs->willpower;
            charm = maybe_core_attrs->charm;
            mood = maybe_core_attrs->mood;

            strength_max = maybe_core_attrs->strength_max;
            physique_max = maybe_core_attrs->physique_max;
            agility_max = maybe_core_attrs->agility_max;
            vitality_max = maybe_core_attrs->vitality_max;
            comprehension_max = maybe_core_attrs->comprehension_max;
            willpower_max = maybe_core_attrs->willpower_max;
            charm_max = maybe_core_attrs->charm_max;
            mood_max = maybe_core_attrs->mood_max;
        }
    }
    //=========================================================================
    contract_actor_talent_rule_info::contract_actor_talent_rule_info(const actor_talent_rule_object& rule, database& db)
    : id(rule.id), title(rule.title), description(rule.description), init_attribute_amount_modifier(rule.init_attribute_amount_modifier)
    {
        main_contract = db.get<contract_object, by_id>(rule.main_contract).name;
    }
    //=========================================================================
    contract_actor_relation_info::contract_actor_relation_info(const actor_relation_object& rel, database& db)
    : favor(rel.favor)
    {
        actor_name = db.get<actor_object, by_id>(rel.actor).name;
        target_actor_name = db.get<actor_object, by_id>(rel.target).name;
        favor_level = rel.get_favor_level();
    }

} } // namespace taiyi::chain

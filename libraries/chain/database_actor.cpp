#include <chain/taiyi_fwd.hpp>

#include <protocol/taiyi_operations.hpp>

#include <chain/block_summary_object.hpp>
#include <chain/database.hpp>
#include <chain/database_exceptions.hpp>
#include <chain/db_with.hpp>
#include <chain/evaluator_registry.hpp>
#include <chain/global_property_object.hpp>
#include <chain/taiyi_evaluator.hpp>
#include <chain/taiyi_objects.hpp>
#include <chain/transaction_object.hpp>
#include <chain/nfa_objects.hpp>
#include <chain/actor_objects.hpp>
#include <chain/contract_objects.hpp>

#include <chain/contract_worker.hpp>

#include <chain/util/uint256.hpp>
#include <chain/util/manabar.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/uint128.hpp>

#include <fc/container/deque.hpp>

#include <fc/io/fstream.hpp>

#include <iostream>

#include <cstdint>
#include <deque>
#include <fstream>
#include <functional>

namespace taiyi { namespace chain {

    void database::initialize_actor_object( actor_object& act, const std::string& name, const nfa_object& nfa)
    {
        act.nfa_id = nfa.id;
        act.name = name;
        act.standpoint = 500;
        act.loyalty = 300;
        act.location = zone_id_type::max();
        act.base = zone_id_type::max();
    }
    //=============================================================================
    // gender; 0=random, -1=男, 1=女, -2=男生女相, 2=女生男相
    // sexuality; 0=无性取向，1=喜欢男性，2=喜欢女性，3=双性恋
    void database::initialize_actor_talents( const actor_object& act, int gender, int sexuality )
    {
        auto now = head_block_time();
        auto hblock_id = head_block_id();

        const auto& talent_rule_idx = get_index< actor_talent_rule_index >().indices().get< by_id >();
        if(talent_rule_idx.size() == 0)
            return;
        
        int max_talent_num = 5;
        
        const auto& actor_talents = create< actor_talents_object >( [&]( actor_talents_object& tlts ) {
            tlts.actor = act.id;
            tlts.last_update = now;
            tlts.created = now;

            if(talent_rule_idx.size() > 0) {
                flat_set<uint32_t> exclusive_set;
                for(int i=0; i<max_talent_num; i++) {
                    const actor_talent_rule_object* talent_rule = 0;
                    if(i == 0) {
                        switch (gender) {
                            case -1: //男
                                talent_rule = find< actor_talent_rule_object, by_uuid >(1003);
                                break;
                            case -2: //男生女相
                                talent_rule = find< actor_talent_rule_object, by_uuid >(1200);
                                break;
                            case 1: //女
                                talent_rule = find< actor_talent_rule_object, by_uuid >(1004);
                                break;
                            case 2: //女生男相
                                talent_rule = find< actor_talent_rule_object, by_uuid >(1201);
                                break;
                            default:
                                break;
                        }
                    }
                    
                    if(i == 1) {
                        switch (sexuality) {
                            case 0: //无性取向
                                talent_rule = find< actor_talent_rule_object, by_uuid >(1027);
                                break;
                            case 1:
                            {
                                if(tlts.talents.find(1003) != tlts.talents.end() || tlts.talents.find(1200) != tlts.talents.end() )
                                    talent_rule = find< actor_talent_rule_object, by_uuid >(1026);
                                break;
                            }
                            case 2:
                            {
                                if(tlts.talents.find(1004) != tlts.talents.end() || tlts.talents.find(1201) != tlts.talents.end() )
                                    talent_rule = find< actor_talent_rule_object, by_uuid >(1026);
                                break;
                            }
                            default:
                                break;
                        }
                    }
                    
                    if(talent_rule == 0) {
                        uint32_t seed = hblock_id._hash[4] + act.id + i;
                        uint32_t tid = hasher::hash( seed ) % talent_rule_idx.size(); //0 to size-1, must match by_id
                        talent_rule = &(*talent_rule_idx.find(tid));
                    }
                    
                    if(tlts.talents.find(talent_rule->uuid) != tlts.talents.end())
                        continue; //already exist
                    
                    //search exclusive id in exist talent
                    bool is_exclusive = false;
                    for(auto uuid : talent_rule->exclude) {
                        if(tlts.talents.find(uuid) != tlts.talents.end()) {
                            //find one exclusive id already exist
                            is_exclusive = true;
                            break;
                        }
                    }
                    if(is_exclusive)
                        continue;
                    
                    //check if in all exclusive set of exist talent
                    if(exclusive_set.find(talent_rule->uuid) != exclusive_set.end())
                        continue;
                    
                    for(auto uuid : talent_rule->exclude) {
                        if(exclusive_set.find(uuid) == exclusive_set.end())
                            exclusive_set.insert(uuid);
                    }
                    
                    tlts.talents[talent_rule->uuid] = 0;
                }
            }
        });
        FC_UNUSED(actor_talents);
    }
    //=============================================================================
    void database::initialize_actor_attributes( const actor_object& act, const vector<uint16_t>& init_attrs )
    {
        auto now = head_block_time();
            
        //create actor attributes
        const auto& core_attrs = create< actor_core_attributes_object >( [&]( actor_core_attributes_object& attrs ) {
            attrs.actor = act.id;
            
            attrs.strength_max      = init_attrs[0];
            attrs.strength          = attrs.strength_max;
            attrs.physique_max      = init_attrs[1];
            attrs.physique          = attrs.physique_max;
            attrs.agility_max       = init_attrs[2];
            attrs.agility           = attrs.agility_max;
            attrs.vitality_max      = init_attrs[3];
            attrs.vitality          = attrs.vitality_max;
            attrs.comprehension_max = init_attrs[4];
            attrs.comprehension     = attrs.comprehension_max;
            attrs.willpower_max     = init_attrs[5];
            attrs.willpower         = attrs.willpower_max;
            attrs.charm_max         = init_attrs[6];
            attrs.charm             = attrs.charm_max;
            attrs.mood_max          = init_attrs[7];
            attrs.mood              = attrs.mood_max;
            
            attrs.last_update = now;
            attrs.created = now;
        });
        FC_UNUSED(core_attrs);
    }
    //=============================================================================
    const actor_object&  database::get_actor( const std::string& name )const
    { try {
        return get< actor_object, by_name >( name );
    } FC_CAPTURE_AND_RETHROW( (name) ) }
    //=============================================================================
    const actor_object*  database::find_actor( const std::string& name )const
    {
        return find< actor_object, by_name >( name );
    }
    //=============================================================================
    uint16_t database::get_actor_init_attribute_amount_max( const std::string& name )const {
        const auto& act = get_actor(name);
        return get_actor_init_attribute_amount_max(act);
    }
    //=============================================================================
    uint16_t database::get_actor_init_attribute_amount_max( const actor_object& actor )const {
        const actor_talents_object& actor_talents = get< actor_talents_object, by_actor >( actor.id );
        int max_init_attr_amount = TAIYI_ACTOR_INIT_ATTRIBUTE_AMOUNT;
        for(auto itlt = actor_talents.talents.begin(); itlt != actor_talents.talents.end(); itlt++) {
            const actor_talent_rule_object& tobj = get< actor_talent_rule_object, by_uuid >(itlt->first);
            max_init_attr_amount += tobj.status;
        }
        return (uint16_t)std::max(0, max_init_attr_amount);
    }


} } //taiyi::chain

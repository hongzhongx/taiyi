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
#include <chain/tiandao_property_object.hpp>
#include <chain/account_object.hpp>
#include <chain/nfa_objects.hpp>
#include <chain/actor_objects.hpp>
#include <chain/zone_objects.hpp>
#include <chain/contract_objects.hpp>

#include <chain/lua_context.hpp>
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
    void database::initialize_actor_talent_rule_object(const account_object& creator, actor_talent_rule_object& rule)
    {
        LuaContext context;
        initialize_VM_baseENV(context);
        flat_set<public_key_type> sigkeys;
        contract_worker worker;
        vector<lua_types> value_list;
        long long vm_drops = 1000000;
        //运行主合约获取初始化数据
        const auto& contract = get<contract_object, by_id>(rule.main_contract);
        lua_table result_table = worker.do_contract_function_return_table(creator, TAIYI_ACTOR_TALENT_RULE_INIT_FUNC_NAME, value_list, sigkeys, contract, vm_drops, true, context, *this);
        
        auto it_name = result_table.v.find(lua_types(lua_string("name")));
        FC_ASSERT(it_name != result_table.v.end(), "talent contract init data invalid, need \"name\"");
        rule.title = it_name->second.get<lua_string>().v;

        auto it_description = result_table.v.find(lua_types(lua_string("description")));
        FC_ASSERT(it_description != result_table.v.end(), "talent contract init data invalid, need \"description\"");
        rule.description = it_description->second.get<lua_string>().v;

        auto it_amodifier = result_table.v.find(lua_types(lua_string("init_attribute_amount_modifier")));
        if(it_amodifier != result_table.v.end())
            rule.init_attribute_amount_modifier = it_amodifier->second.get<lua_int>().v;
    }
    //=============================================================================
    void database::initialize_actor_talents( const actor_object& act )
    {
        auto now = head_block_time();
        auto hblock_id = head_block_id();

        const auto& talent_rule_idx = get_index< actor_talent_rule_index >().indices().get< by_id >();
        int max_talent_num = 5;
        
        const actor_talents_object& actor_talents = create< actor_talents_object >( [&]( actor_talents_object& tlts ) {
            tlts.actor = act.id;
            tlts.last_update = now;
            tlts.created = now;

            if(talent_rule_idx.size() > 0) {
                flat_set<uint32_t> exclusive_set;
                for(int i=0; i<max_talent_num; i++) {
                    
                    uint32_t seed = hblock_id._hash[4] + act.id + i;
                    uint32_t tid = hasher::hash( seed ) % talent_rule_idx.size(); //0 to size-1, must match by_id
                    const actor_talent_rule_object* talent_rule = &(*talent_rule_idx.find(tid));
                    
                    if(tlts.talents.find(talent_rule->id) != tlts.talents.end())
                        continue; //already exist
                                        
                    tlts.talents[talent_rule->id] = 0;
                }
            }
        });
        
        //根据分配的天赋初始化角色出生时能设置的属性总点数上限
        int max_init_attr_amount = TAIYI_ACTOR_INIT_ATTRIBUTE_AMOUNT;
        for(auto itlt = actor_talents.talents.begin(); itlt != actor_talents.talents.end(); itlt++) {
            const actor_talent_rule_object& tobj = get< actor_talent_rule_object, by_id >(itlt->first);
            max_init_attr_amount += tobj.init_attribute_amount_modifier;
        }
        modify(act, [&](actor_object& obj) {
            obj.init_attribute_amount_max = (uint16_t)std::max(0, max_init_attr_amount);
        });
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
    // gender; 0=random, -1=男, 1=女, -2=男生女相, 2=女生男相
    // sexuality; 0=无性取向，1=喜欢男性，2=喜欢女性，3=双性恋
    void database::born_actor( const actor_object& act, int gender, int sexuality, const zone_object& zone )
    {
        auto now = head_block_time();
        auto hblock_id = head_block_id();
        uint32_t seed = hblock_id._hash[4];

        const auto& tiandao = get_tiandao_properties();
        
        //update actor
        modify( act, [&]( actor_object& a ) {
            a.age = 0;
            
            a.location = zone.id;
            
            a.born = true;
            a.born_time = now;
            
            a.gender = (gender == 0 ? int(hasher::hash( seed + a.id + 2777) % 5) - 2 : gender);
            a.sexuality = sexuality;
            
            a.born_vyears = int(tiandao.v_years);
            a.born_vmonths = int(tiandao.v_months);
            a.born_vtimes = int(tiandao.v_times);
            a.five_phase = int(hasher::hash( seed + a.id + 3229) % 5);
            
            a.standpoint = (hasher::hash( seed + a.id + 1619) % 1000);

            a.last_update = now;
        });
                        
        push_virtual_operation( actor_born_operation( get<account_object, by_id>(get<nfa_object, by_id>(act.nfa_id).owner_account).name, act.name, zone.name, act.nfa_id ) );
    }
    //=============================================================================
    void database::born_actor( const actor_object& act, int gender, int sexuality, const string& zone_name )
    {
        const auto& zone = get_zone(zone_name);
        return born_actor(act, gender, sexuality, zone);
    }

} } //taiyi::chain

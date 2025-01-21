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
        lua_table result_table = worker.do_contract_function(creator, TAIYI_ACTOR_TALENT_RULE_INIT_FUNC_NAME, value_list, sigkeys, contract, vm_drops, true, context, *this);
        
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
            a.base = zone.id;
            
            a.born = true;
            a.born_time = now;
            
            a.gender = (gender == 0 ? int(hasher::hash( seed + a.id + 2777) % 5) - 2 : gender);
            a.sexuality = sexuality;
            
            a.born_vyears = int(tiandao.v_years);
            a.born_vmonths = int(tiandao.v_months);
            a.born_vtimes = int(tiandao.v_times);
            
            a.standpoint = (hasher::hash( seed + a.id + 1619) % 1000);

            a.last_update = now;
            a.next_tick_time = now; //will active actor tick
        });
                        
        //grow as first birthday
        try_trigger_actor_talents(act, 0);
        
        push_virtual_operation( actor_born_operation( get<account_object, by_id>(get<nfa_object, by_id>(act.nfa_id).owner_account).name, act.name, zone.name, act.nfa_id ) );
    }
    //=============================================================================
    void database::born_actor( const actor_object& act, int gender, int sexuality, const string& zone_name )
    {
        const auto& zone = get_zone(zone_name);
        return born_actor(act, gender, sexuality, zone);
    }
    //=============================================================================
    void database::process_actor_tick()
    {
        uint32_t hbn = head_block_num();
        const auto& tiandao = get_tiandao_properties();
        auto now = head_block_time();

        //list Actors this tick will sim
        const auto& actor_idx = get_index< actor_index, by_tick_time >();
        auto run_num = actor_idx.size() / TAIYI_ACTOR_TICK_PERIOD_MAX_BLOCK_NUM + 1;
        auto itactor = actor_idx.begin();
        auto endactor = actor_idx.end();
        std::vector<const actor_object*> tick_actors;
        tick_actors.reserve(run_num);
        while( itactor != endactor && run_num > 0  )
        {
            if(itactor->next_tick_time > now)
                break;
            
            tick_actors.push_back(&(*itactor));
            run_num--;
            
            ++itactor;
        }
        
        for(const auto* a : tick_actors) {
            const auto& actor = *a;
            
            modify(actor, [&]( actor_object& obj ) {
                obj.next_tick_time = now + TAIYI_ACTOR_TICK_PERIOD_MAX_BLOCK_NUM * TAIYI_BLOCK_INTERVAL;
            });
            
            if(actor.health <= 0) {
                //dead
                //wlog("actor ${a} is dead!", ("a", actor.name));
            }
            else if(actor.born) {
                //try grow
                int should_age = tiandao.v_years - actor.born_vyears;
                if(actor.age < should_age) {
                    if(tiandao.v_times >= actor.born_vtimes) {
                        uint16_t age = actor.age + 1;
                        //process talents
                        try_trigger_actor_talents(actor, age);
                                            
                        modify( actor, [&]( actor_object& act ) {
                            act.age = age; //note age may be changed in event processing
                            
                            if(act.age >= 30) {
                                //healthy down
                                act.health_max = std::max(0, act.health_max - 1);
                                act.health = std::min(act.health, act.health_max);
                            }
                            
                            act.last_update = now;
                        });

                        //trigger actor grow
                        try_trigger_actor_contract_grow(actor);
                        
                        //push event message
                        push_virtual_operation( actor_grown_operation( get<account_object, by_id>(get<nfa_object, by_id>(actor.nfa_id).owner_account).name, actor.name, actor.nfa_id, tiandao.v_years, tiandao.v_months, tiandao.v_times, actor.age, actor.health ) );
                    }
                }
                                        
                //procreate（生育）
                //try_procreate(npc, actor, *this);
            }
        }
    }
    //=============================================================================
    void database::try_trigger_actor_talents( const actor_object& act, uint16_t age )
    {
        const auto& nfa = get< nfa_object, by_id >( act.nfa_id );
        
        //欠费限制
        if(nfa.debt_value > 0) {
            //try pay back
            if(nfa.debt_value > nfa.qi.amount.value)
                return;
            
            wlog("NFA (${i}) pay debt ${v}", ("i", nfa.id)("v", nfa.debt_value));

            //reward contract owner
            const auto& to_contract = get<contract_object, by_id>(nfa.debt_contract);
            const auto& contract_owner = get<account_object, by_id>(to_contract.owner);
            reward_feigang(contract_owner, nfa, asset(nfa.debt_value, QI_SYMBOL));
            
            modify(nfa, [&](nfa_object& obj) {
                obj.debt_value = 0;
                obj.debt_contract = contract_id_type::max();
            });
        }

        const auto& owner_account = get< account_object, by_id >( nfa.owner_account );

        //process talents
        const actor_talents_object& actor_talents = get< actor_talents_object, by_actor >( act.id );
        
        std::map<int64_t, int> talent_trgger_numbers;
        for(auto itlt = actor_talents.talents.begin(); itlt != actor_talents.talents.end(); itlt++) {
            const actor_talent_rule_object& tobj = get< actor_talent_rule_object, by_id >(itlt->first);
            
            if(itlt->second >= tobj.max_triggers)
                continue;
            
            const auto* contract_ptr = find<contract_object, by_id>(tobj.main_contract);
            if(contract_ptr == nullptr)
                continue;
            auto abi_itr = contract_ptr->contract_ABI.find(lua_types(lua_string("trigger")));
            if(abi_itr == contract_ptr->contract_ABI.end())
                continue;
            
            //触发天赋合约函数
            {
                vector<lua_types> value_list; //no params.
                contract_worker worker;

                LuaContext context;
                initialize_VM_baseENV(context);
                flat_set<public_key_type> sigkeys;

                //qi可能在执行合约中被进一步使用，所以这里记录当前的qi来计算虚拟机的执行消耗
                long long old_drops = nfa.qi.amount.value / TAIYI_USEMANA_EXECUTION_SCALE;
                long long vm_drops = old_drops;
                bool trigger_fail = false;
                bool triggered = false;
                try {
                    auto session = start_undo_session();
                    lua_table result_table = worker.do_nfa_contract_function(nfa, "trigger", value_list, sigkeys, *contract_ptr, vm_drops, true, context, *this);
                    
                    auto it_triggered = result_table.v.find(lua_types(lua_string("triggered")));
                    if(it_triggered == result_table.v.end()) {
                        trigger_fail = true;
                        wlog("Actor (${a}) trigger talent #${t} return invalid result, need \"triggered\"", ("a", act.name)("t", tobj.id));
                    }
                    else
                        triggered = it_triggered->second.get<lua_bool>().v;
                    session.push();
                }
                catch (fc::exception e) {
                    //任何错误都不能照成核心循环崩溃
                    trigger_fail = true;
                    wlog("Actor (${a}) trigger talent #${t} fail. err: ${e}", ("a", act.name)("t", tobj.id)("e", e.to_string()));
                }
                catch (...) {
                    //任何错误都不能照成核心循环崩溃
                    trigger_fail = true;
                    wlog("Actor (${a}) trigger talent #${t} fail.", ("a", act.name)("t", tobj.id));
                }
                int64_t used_drops = old_drops - vm_drops;

                //执行错误仍然要扣费，但是不影响下次触发
                int64_t used_qi = used_drops * TAIYI_USEMANA_EXECUTION_SCALE + 50 * TAIYI_USEMANA_EXECUTION_SCALE;
                int64_t debt_qi = 0;
                if(used_qi > nfa.qi.amount.value) {
                    debt_qi = used_qi - nfa.qi.amount.value;
                    used_qi = nfa.qi.amount.value;
                    if(!trigger_fail) {
                        trigger_fail = true;
                        wlog("Actor (${a}) trigger talent #${t} successfully, but have not enough qi to next trigger.", ("a", act.name)("t", tobj.id));
                    }
                    else {
                        wlog("Actor (${a}) trigger talent #${t} failed, and have not enough qi to next trigger.", ("a", act.name)("t", tobj.id));
                    }
                }

                //reward contract owner
                if( used_qi > 0) {
                    const auto& contract_owner = get<account_object, by_id>(contract_ptr->owner);
                    reward_feigang(contract_owner, nfa, asset(used_qi, QI_SYMBOL));
                }
                
                if(debt_qi > 0) {
                    modify( nfa, [&]( nfa_object& obj ) {
                        obj.debt_value = debt_qi;
                        obj.debt_contract = contract_ptr->id;
                    });
                }
                
                if (!triggered)
                    continue;
            }
            
            //trigger number
            talent_trgger_numbers[itlt->first] = itlt->second + 1;
        }
        
        //update talent trigger numbers
        modify(actor_talents, [&]( actor_talents_object& tobj ) {
            for(auto p : talent_trgger_numbers)
                tobj.talents[p.first] = p.second;
        });
        
        //push talent trigger event message
        for(auto p : talent_trgger_numbers) {
            auto const& t = get< actor_talent_rule_object, by_id >(p.first);
            push_virtual_operation( actor_talent_trigger_operation( owner_account.name, act.name, act.nfa_id, p.first, t.title, t.description, age ) );
        }
    }
    //=============================================================================
    void database::try_trigger_actor_contract_grow( const actor_object& act )
    {
        const auto& nfa = get< nfa_object, by_id >( act.nfa_id );

        //欠费限制
        if(nfa.debt_value > 0) {
            //try pay back
            if(nfa.debt_value > nfa.qi.amount.value)
                return;
            
            wlog("NFA (${i}) pay debt ${v}", ("i", nfa.id)("v", nfa.debt_value));

            //reward contract owner
            const auto& to_contract = get<contract_object, by_id>(nfa.debt_contract);
            const auto& contract_owner = get<account_object, by_id>(to_contract.owner);
            reward_feigang(contract_owner, nfa, asset(nfa.debt_value, QI_SYMBOL));
            
            modify(nfa, [&](nfa_object& obj) {
                obj.debt_value = 0;
                obj.debt_contract = contract_id_type::max();
            });
        }

        const auto* contract_ptr = find<contract_object, by_id>(nfa.main_contract);
        if(contract_ptr == nullptr)
            return;
        auto abi_itr = contract_ptr->contract_ABI.find(lua_types(lua_string("on_grown")));
        if(abi_itr == contract_ptr->contract_ABI.end())
            return;

        const auto& owner_account = get< account_object, by_id >( nfa.owner_account );

        //回调合约生长函数
        {
            vector<lua_types> value_list; //no params.
            contract_worker worker;

            LuaContext context;
            initialize_VM_baseENV(context);
            flat_set<public_key_type> sigkeys;

            //qi可能在执行合约中被进一步使用，所以这里记录当前的qi来计算虚拟机的执行消耗
            long long old_drops = nfa.qi.amount.value / TAIYI_USEMANA_EXECUTION_SCALE;
            long long vm_drops = old_drops;
            bool trigger_fail = false;
            try {
                auto session = start_undo_session();
                worker.do_nfa_contract_function(nfa, "on_grown", value_list, sigkeys, *contract_ptr, vm_drops, true, context, *this);
                session.push();
            }
            catch (fc::exception e) {
                //任何错误都不能照成核心循环崩溃
                trigger_fail = true;
                wlog("Actor (${a}) trigger on_grown fail. err: ${e}", ("a", act.name)("e", e.to_string()));
            }
            catch (...) {
                //任何错误都不能照成核心循环崩溃
                trigger_fail = true;
                wlog("Actor (${a}) trigger on_grown fail.", ("a", act.name));
            }
            int64_t used_drops = old_drops - vm_drops;

            //执行错误仍然要扣费，但是不影响下次触发
            int64_t used_qi = used_drops * TAIYI_USEMANA_EXECUTION_SCALE + 50 * TAIYI_USEMANA_EXECUTION_SCALE;
            int64_t debt_qi = 0;
            if(used_qi > nfa.qi.amount.value) {
                debt_qi = used_qi - nfa.qi.amount.value;
                used_qi = nfa.qi.amount.value;
                if(!trigger_fail) {
                    trigger_fail = true;
                    wlog("Actor (${a}) trigger on_grown successfully, but have not enough qi to next trigger.", ("a", act.name));
                }
                else {
                    wlog("Actor (${a}) trigger on_grown failed, and have not enough qi to next trigger.", ("a", act.name));
                }
            }

            //reward contract owner
            if( used_qi > 0) {
                const auto& contract_owner = get<account_object, by_id>(contract_ptr->owner);
                reward_feigang(contract_owner, nfa, asset(used_qi, QI_SYMBOL));
            }
            
            if(debt_qi > 0) {
                modify( nfa, [&]( nfa_object& obj ) {
                    obj.debt_value = debt_qi;
                    obj.debt_contract = contract_ptr->id;
                });
            }
        }
    }

} } //taiyi::chain

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
#include <chain/cultivation_objects.hpp>

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

    const cultivation_object& database::create_cultivation(const nfa_object& manager_nfa, const chainbase::t_flat_map<nfa_id_type, uint>& beneficiaries, uint32_t prepare_time_blocks)
    {
        FC_ASSERT(prepare_time_blocks >= TAIYI_CULTIVATION_PREPARE_MIN_TIME_BLOCK_NUM, "prepare time not enough");
        
        uint total_share_check = 0;
        for (const auto& b : beneficiaries) {
            const auto* nfa = find<nfa_object, by_id>(b.first);
            FC_ASSERT(nfa != nullptr, "invalid beneficiary nfa id");
            FC_ASSERT(b.second > 0, "invalid beneficiary share weight");
            total_share_check += b.second;
        }
        FC_ASSERT(total_share_check == TAIYI_100_PERCENT, "sum of beneficiaries share is not 100%");
        
        auto now = head_block_num();
        const auto& cultivation = create<cultivation_object>([&](cultivation_object& obj) {
            obj.manager_nfa_id = manager_nfa.id;
            obj.beneficiary_map = beneficiaries;
            obj.create_time = now;
            obj.start_deadline = obj.create_time + prepare_time_blocks;
        });
        
        return cultivation;
    }
    //=========================================================================
    void database::dissolve_cultivation(const cultivation_object& cult)
    {
        asset total_cultivation_qi = asset(0, QI_SYMBOL);

        //return back cultivation value to qi
        for(const auto& p : cult.participants) {
            const nfa_object& nfa = get<nfa_object, by_id>(p);
            total_cultivation_qi.amount += nfa.cultivation_value;
            
            modify(nfa, [&](nfa_object& obj) {
                obj.qi.amount += obj.cultivation_value;
                obj.cultivation_value = 0;
            });
        }

        modify(cult, [&](cultivation_object& obj) {
            obj.participants.clear();
            obj.start_time = std::numeric_limits<uint32_t>::max();
            obj.last_update_time = obj.start_time;
            obj.qi_accumulated = 0;
        });

        if(total_cultivation_qi.amount > 0) {
            modify( get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo ) {
                gpo.total_qi += total_cultivation_qi;
                gpo.pending_cultivation_qi -= total_cultivation_qi;
            });
        }
    }
    //=========================================================================
    void database::participate_cultivation(const cultivation_object& cult, const nfa_object& nfa, uint64_t value)
    {
        FC_ASSERT(cult.start_time == std::numeric_limits<uint32_t>::max(), "can not participate in a started cultivation");
        FC_ASSERT(nfa.cultivation_value == 0, "nfa already in underway cultivation");
        FC_ASSERT(value > 0, "can not participate in a cultivation without any value");
        FC_ASSERT(nfa.qi.amount.value >= value, "nfa have not enough qi to participate");
        
        modify(cult, [&](cultivation_object& obj) {
            obj.participants.insert(nfa.id);
        });
        
        modify(nfa, [&](nfa_object& obj) {
            obj.cultivation_value = value;
            obj.qi.amount -= value;
        });

        modify( get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo ) {
            gpo.total_qi.amount -= value;
            gpo.pending_cultivation_qi.amount += value;
        });
    }
    //=========================================================================
    void database::start_cultivation(const cultivation_object& cult)
    {
        FC_ASSERT(cult.start_time == std::numeric_limits<uint32_t>::max(), "cultivation is already underway");
        FC_ASSERT(cult.participants.size() > 0, "cultivation can not start without any participants");
        
        modify(cult, [&](cultivation_object& obj) {
            obj.start_time = head_block_num();
            obj.last_update_time = obj.start_time;
            obj.qi_accumulated = 0;
        });
    }
    //=========================================================================
    void database::stop_cultivation(const cultivation_object& cult)
    {
        update_cultivation(cult);
        
        auto now = head_block_num();
        int64_t rv = cult.qi_accumulated;
        
        //检查基金池
        const reward_fund_object& rf = get< reward_fund_object, by_name >( TAIYI_CULTIVATION_REWARD_FUND_NAME );
        if(rf.reward_qi_balance.amount < rv)
            rv = rf.reward_qi_balance.amount.value;
            
        if(rv > 0) {
            //reward to each beneficiary with its share
            reward_cultivation_operation vop;
            const nfa_object* nfa_b = 0;
            int64_t rv_left = rv;
            for (const auto& b : cult.beneficiary_map) {
                nfa_b = find<nfa_object, by_id>(b.first);
                asset rn = asset(rv * b.second / TAIYI_100_PERCENT, QI_SYMBOL);
                
                adjust_nfa_balance(*nfa_b, rn);

                vop.account = get<account_object, by_id>(nfa_b->owner_account).name;
                vop.nfa = nfa_b->id;
                vop.qi = rn;
                push_virtual_operation( vop );
                
                rv_left -= rn.amount.value;
            }
            if (rv_left > 0) {
                //整数误差多出来的奖励给最后一个受益者
                asset rn = asset(rv_left, QI_SYMBOL);
                vop.account = get<account_object, by_id>(nfa_b->owner_account).name;
                vop.nfa = nfa_b->id;
                vop.qi = rn;
                pre_push_virtual_operation( vop );
                adjust_nfa_balance(*nfa_b, rn);
                post_push_virtual_operation( vop );
            }
            
            //更新修真基金池
            modify(rf, [&](reward_fund_object& rfo) {
                rfo.reward_qi_balance.amount -= rv;
            });
            
            //更新统计真气
            modify(get_dynamic_global_properties(), [&](dynamic_global_property_object& obj) {
                obj.pending_rewarded_qi.amount -= rv;
                obj.total_qi.amount += rv;
            });
        }
        
        //关闭并解散修真对象
        dissolve_cultivation(cult);
    }
    //=========================================================================
    void database::clean_cultivations()
    {
        auto now = head_block_num();
        std::set<const cultivation_object*> removed;

        //首先剔除截止开始时间还未开始的修真
        const auto& cidx_by_start_deadline = get_index< cultivation_index >().indices().get< by_start_deadline >();
        for (auto itr = cidx_by_start_deadline.begin(); itr != cidx_by_start_deadline.end(); ++itr) {
            if (itr->start_deadline > now)
                break;
            if (itr->start_time == std::numeric_limits<uint32_t>::max()) {
                dissolve_cultivation(*itr);
                removed.insert(&(*itr));
            }
        }
        
        //再剔除修真时间超过最大修真时间的
        const auto& cidx_by_start_time = get_index< cultivation_index >().indices().get< by_start_time >();
        for (auto itr = cidx_by_start_time.begin(); itr != cidx_by_start_time.end(); ++itr) {
            if (itr->start_time == std::numeric_limits<uint32_t>::max() || (itr->start_time + TAIYI_CULTIVATION_MAX_TIME_BLOCK_NUM) > now)
                break;
            stop_cultivation(*itr);
            removed.insert(&(*itr));
        }

        for (const auto* p : removed)
            remove(*p);        
    }
    //=========================================================================
    void database::update_cultivation(const cultivation_object& cult)
    {
        FC_ASSERT(cult.start_time != std::numeric_limits<uint32_t>::max(), "cultivation have not started");
        
        auto now = head_block_num();
        FC_ASSERT(cult.start_time < now, "invalid start time");
        if(cult.last_update_time >= now)
            return;
        
        uint32_t dt = now - cult.last_update_time;
        dt = std::min<uint32_t>(dt, TAIYI_CULTIVATION_MAX_TIME_BLOCK_NUM);
        
        /*
         修真获得真气上限和区域灵气浓度正相关，修真投入的所有真气、参与实体各自五行材料和区域五行相互的生克程度影响修真效率
         */
        
        // 所在区域灵气浓度
        const zone_object& zone = get<zone_object, by_id>(get<actor_object, by_nfa_id>(cult.manager_nfa_id).location);
        int64_t N = calculate_zone_spiritual_energy(zone);
        
        // 各个实体投入的所有真气，修真效率
        uint64_t Q = 0; // 投入真气总量
        int64_t Pw = 0;
        int64_t Pf = 0;
        int64_t Pe = 0;
        int64_t Pm = 0;
        int64_t Pa = 0;
        for (const auto& p : cult.participants) {
            const auto& nfa = get<nfa_object, by_id>(p);
            Q += nfa.cultivation_value;
            
            const auto& nfa_mat = get<nfa_material_object, by_nfa_id>(p); //各个实体材质
            Pw += nfa_mat.wood.amount.value;  // 木
            Pf += nfa_mat.herb.amount.value;  // 火
            Pe += nfa_mat.food.amount.value;  // 土
            Pm += nfa_mat.gold.amount.value;  // 金
            Pa += nfa_mat.fabric.amount.value;// 水
        }

        int64_t E = TAIYI_100_PERCENT; // 基础效率 100%
        int64_t Total_P = Pw + Pf + Pe + Pm + Pa;
        if (Total_P > 0) {
            // 1. 参与实体内部相互之间的生克 (Internal Harmony)
            // 相生：木生火，火生土，土生金，金生水，水生木 (Wood -> Fire -> Earth -> Metal -> Water -> Wood)
            int64_t HI_gen = Pw * Pf + Pf * Pe + Pe * Pm + Pm * Pa + Pa * Pw;
            // 相克：木克土，土克水，水克火，火克金，金克木 (Wood -> Earth -> Water -> Fire -> Metal -> Wood)
            int64_t HI_ovc = Pw * Pe + Pe * Pa + Pa * Pf + Pf * Pm + Pm * Pw;
            
            int64_t HI_bonus = 0;
            if (Total_P * Total_P / 5 > 0) {
                if (HI_gen > HI_ovc)
                    HI_bonus = (HI_gen - HI_ovc) * (TAIYI_100_PERCENT / 2) / (Total_P * Total_P / 5);
                else if (HI_ovc > HI_gen)
                    HI_bonus = -((HI_ovc - HI_gen) * (TAIYI_100_PERCENT / 2) / (Total_P * Total_P / 5));
            }
            E += HI_bonus;

            // 2. 实体与环境生克 (Environmental Harmony)
            const nfa_material_object& zone_mat = get<nfa_material_object, by_nfa_id>(zone.nfa_id); //区域材质
            int64_t Zw = zone_mat.wood.amount.value;
            int64_t Zf = zone_mat.herb.amount.value;
            int64_t Ze = zone_mat.food.amount.value;
            int64_t Zm = zone_mat.gold.amount.value;
            int64_t Za = zone_mat.fabric.amount.value;
            int64_t Total_Z = Zw + Zf + Ze + Zm + Za;
            
            if (Total_Z > 0) {
                // 环境生实体 (Zone generates Participant)
                int64_t HE_gen = Zw * Pf + Zf * Pe + Ze * Pm + Zm * Pa + Za * Pw;
                // 环境克实体 (Zone overcomes Participant)
                int64_t HE_ovc = Zw * Pe + Ze * Pa + Za * Pf + Zf * Pm + Zm * Pw;
                
                int64_t HE_bonus = 0;
                if (Total_Z * Total_P / 5 > 0) {
                    if (HE_gen > HE_ovc)
                        HE_bonus = (HE_gen - HE_ovc) * (TAIYI_100_PERCENT / 2) / (Total_Z * Total_P / 5);
                    else if (HE_ovc > HE_gen)
                        HE_bonus = -((HE_ovc - HE_gen) * (TAIYI_100_PERCENT / 2) / (Total_Z * Total_P / 5));
                }
                E += HE_bonus;
            }
        }
        
        // 确保效率不为负，有最低限度
        E = std::max<int64_t>(E, TAIYI_1_PERCENT * 10);

        auto hblock_id = head_block_id();
        uint32_t p = hasher::hash( hblock_id._hash[4] + cult.id ) % TAIYI_100_PERCENT;
        
        int64_t rv = N * Q * p * dt * E / TAIYI_CULTIVATION_MAX_TIME_BLOCK_NUM / TAIYI_100_PERCENT / TAIYI_100_PERCENT;
                
        if(rv > 0) {
            modify(cult, [&](cultivation_object& obj) {
                obj.qi_accumulated += rv;
                obj.last_update_time = now;
            });
        }
    }

} } //taiyi::chain

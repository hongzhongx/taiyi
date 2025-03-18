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

    const cultivation_object& database::create_cultivation(const nfa_object& manager_nfa, const chainbase::t_flat_map<nfa_id_type, uint>& beneficiaries, uint64_t prepare_time_seconds)
    {
        FC_ASSERT(prepare_time_seconds >= TAIYI_CULTIVATION_PREPARE_MIN_SECONDS, "prepare time not enough");
        
        uint total_share_check = 0;
        for (const auto& b : beneficiaries) {
            const auto* nfa = find<nfa_object, by_id>(b.first);
            FC_ASSERT(nfa != nullptr, "invalid beneficiary nfa id");
            FC_ASSERT(b.second > 0, "invalid beneficiary share weight");
            total_share_check += b.second;
        }
        FC_ASSERT(total_share_check == TAIYI_100_PERCENT, "sum of beneficiaries share is not 100%");
        
        auto now = head_block_time();
        const auto& cultivation = create<cultivation_object>([&](cultivation_object& obj) {
            obj.manager_nfa_id = manager_nfa.id;
            obj.beneficiary_map = beneficiaries;
            obj.create_time = now;
            obj.start_deadline = obj.create_time + prepare_time_seconds;
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
            obj.start_time = time_point_sec::maximum();
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
        FC_ASSERT(cult.start_time == time_point_sec::maximum(), "can not participate in a started cultivation");
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
        FC_ASSERT(cult.start_time == time_point_sec::maximum(), "cultivation is already underway");
        FC_ASSERT(cult.participants.size() > 0, "cultivation can not start without any participants");
        
        modify(cult, [&](cultivation_object& obj) {
            obj.start_time = head_block_time();
        });
    }
    //=========================================================================
    void database::stop_cultivation(const cultivation_object& cult)
    {
        FC_ASSERT(cult.start_time != time_point_sec::maximum(), "cultivation have not started");
        
        auto now = head_block_time();
        FC_ASSERT(cult.start_time < now, "invalid start time");
        uint64_t t = (now - cult.start_time).to_seconds();
        t = std::min<uint64_t>(t, TAIYI_CULTIVATION_MAX_SECONDS);
        
        if(t > 0) {
            //calculate reward qi
            auto hblock_id = head_block_id();
            uint32_t p = hasher::hash( hblock_id._hash[4] + cult.id ) % TAIYI_100_PERCENT;
            uint64_t k = 0;
            for (const auto& p : cult.participants)
                k += get<nfa_object, by_id>(p).cultivation_value;
            int64_t rv = k * p * t / TAIYI_CULTIVATION_MAX_SECONDS / TAIYI_100_PERCENT;
            
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
                    
                    vop.account = get<account_object, by_id>(nfa_b->owner_account).name;
                    vop.nfa = nfa_b->id;
                    vop.qi = rn;
                    pre_push_virtual_operation( vop );

                    adjust_nfa_balance(*nfa_b, rn);

#ifdef IS_TEST_NET
                    //测试网络修真可以起死回生
                    auto hbn = head_block_num();
                    const auto* test_actor = find< actor_object, by_nfa_id >( nfa_b->id );
                    if( test_actor != nullptr &&
                       (test_actor->health_max <= 0 || (hbn > 1601345 && test_actor->health_max <= 10) )
                       ) {
                        modify(*test_actor, [&](actor_object& obj) {
                            obj.health_max = 100;
                            obj.health = obj.health_max;
                        });
                    }
#endif

                    post_push_virtual_operation( vop );
                    
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
        }
        
        //关闭并解散修真对象
        dissolve_cultivation(cult);
    }
    //=========================================================================
    void database::process_cultivations()
    {
        auto now = head_block_time();
        std::set<const cultivation_object*> removed;

        //首先剔除截止开始时间还未开始的修真
        const auto& cidx_by_start_deadline = get_index< cultivation_index >().indices().get< by_start_deadline >();
        for (auto itr = cidx_by_start_deadline.begin(); itr != cidx_by_start_deadline.end(); ++itr) {
            if (itr->start_deadline > now)
                break;
            if (itr->start_time == time_point_sec::maximum()) {
                dissolve_cultivation(*itr);
                removed.insert(&(*itr));
            }
        }
        
        //再剔除修真时间超过最大修真时间的
        const auto& cidx_by_start_time = get_index< cultivation_index >().indices().get< by_start_time >();
        for (auto itr = cidx_by_start_time.begin(); itr != cidx_by_start_time.end(); ++itr) {
            if (itr->start_time == time_point_sec::maximum() || (itr->start_time + TAIYI_CULTIVATION_MAX_SECONDS) > now)
                break;
            stop_cultivation(*itr);
            removed.insert(&(*itr));
        }

        for (const auto* p : removed)
            remove(*p);
    }

} } //taiyi::chain

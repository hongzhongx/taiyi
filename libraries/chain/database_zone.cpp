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
#include <chain/tiandao_property_object.hpp>
#include <chain/zone_objects.hpp>
#include <chain/contract_objects.hpp>

#include <chain/contract_worker.hpp>

#include <chain/util/uint256.hpp>

#include <tyme/tyme.h>

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

    void g_init_tiandao_property_object( tiandao_property_object& tiandao )
    {
        //init gold grow speed
        tiandao.zone_grow_gold_speed_map.clear();
        tiandao.zone_grow_gold_speed_map.resize((int)_ZONE_TYPE_NUM, 100000000);
        tiandao.zone_grow_gold_speed_map[(int)SHANYUE]  = 200000000;
        tiandao.zone_grow_gold_speed_map[(int)DONGXUE]  = 200000000;
        tiandao.zone_grow_gold_speed_map[(int)SHILIN]   = 200000000;
        tiandao.zone_grow_gold_speed_map[(int)HAIYANG]  = 0;
        tiandao.zone_grow_gold_speed_map[(int)SHAMO]    = 0;
        tiandao.zone_grow_gold_speed_map[(int)HUANGYE]  = 0;
        tiandao.zone_grow_gold_speed_map[(int)ANYUAN]   = 0;

        //init food grow speed
        tiandao.zone_grow_food_speed_map.clear();
        tiandao.zone_grow_food_speed_map.resize((int)_ZONE_TYPE_NUM, 100000000);
        tiandao.zone_grow_food_speed_map[(int)YUANYE]   = 200000000;
        tiandao.zone_grow_food_speed_map[(int)HUPO]     = 200000000;
        tiandao.zone_grow_food_speed_map[(int)NONGTIAN] = 200000000;
        tiandao.zone_grow_food_speed_map[(int)HAIYANG]  = 0;
        tiandao.zone_grow_food_speed_map[(int)SHAMO]    = 0;
        tiandao.zone_grow_food_speed_map[(int)HUANGYE]  = 0;
        tiandao.zone_grow_food_speed_map[(int)ANYUAN]   = 0;

        //init wood grow speed
        tiandao.zone_grow_wood_speed_map.clear();
        tiandao.zone_grow_wood_speed_map.resize((int)_ZONE_TYPE_NUM, 100000000);
        tiandao.zone_grow_wood_speed_map[(int)LINDI]    = 200000000;
        tiandao.zone_grow_wood_speed_map[(int)MILIN]    = 200000000;
        tiandao.zone_grow_wood_speed_map[(int)YUANLIN]  = 200000000;
        tiandao.zone_grow_wood_speed_map[(int)HAIYANG]  = 0;
        tiandao.zone_grow_wood_speed_map[(int)SHAMO]    = 0;
        tiandao.zone_grow_wood_speed_map[(int)HUANGYE]  = 0;
        tiandao.zone_grow_wood_speed_map[(int)ANYUAN]   = 0;

        //init fabric grow speed
        tiandao.zone_grow_fabric_speed_map.clear();
        tiandao.zone_grow_fabric_speed_map.resize((int)_ZONE_TYPE_NUM, 100000000);
        tiandao.zone_grow_fabric_speed_map[(int)QIULIN]     = 200000000;
        tiandao.zone_grow_fabric_speed_map[(int)TAOYUAN]    = 200000000;
        tiandao.zone_grow_fabric_speed_map[(int)SANGYUAN]   = 200000000;
        tiandao.zone_grow_fabric_speed_map[(int)HAIYANG]    = 0;
        tiandao.zone_grow_fabric_speed_map[(int)SHAMO]      = 0;
        tiandao.zone_grow_fabric_speed_map[(int)HUANGYE]    = 0;
        tiandao.zone_grow_fabric_speed_map[(int)ANYUAN]     = 0;

        //init herb grow speed
        tiandao.zone_grow_herb_speed_map.clear();
        tiandao.zone_grow_herb_speed_map.resize((int)_ZONE_TYPE_NUM, 100000000);
        tiandao.zone_grow_herb_speed_map[(int)XIAGU]    = 200000000;
        tiandao.zone_grow_herb_speed_map[(int)ZAOZE]    = 200000000;
        tiandao.zone_grow_herb_speed_map[(int)YAOYUAN]  = 200000000;
        tiandao.zone_grow_herb_speed_map[(int)HAIYANG]  = 0;
        tiandao.zone_grow_herb_speed_map[(int)SHAMO]    = 0;
        tiandao.zone_grow_herb_speed_map[(int)HUANGYE]  = 0;
        tiandao.zone_grow_herb_speed_map[(int)ANYUAN]   = 0;
        
        //init assets max
        tiandao.zone_gold_max_map.clear();
        tiandao.zone_gold_max_map.resize((int)_ZONE_TYPE_NUM, 300000000);
        tiandao.zone_food_max_map.clear();
        tiandao.zone_food_max_map.resize((int)_ZONE_TYPE_NUM, 300000000);
        tiandao.zone_wood_max_map.clear();
        tiandao.zone_wood_max_map.resize((int)_ZONE_TYPE_NUM, 300000000);
        tiandao.zone_fabric_max_map.clear();
        tiandao.zone_fabric_max_map.resize((int)_ZONE_TYPE_NUM, 300000000);
        tiandao.zone_herb_max_map.clear();
        tiandao.zone_herb_max_map.resize((int)_ZONE_TYPE_NUM, 300000000);


        //init moving difficulty
        tiandao.zone_moving_difficulty_map.clear();
        tiandao.zone_moving_difficulty_map.resize((int)_ZONE_TYPE_NUM, 1);
        tiandao.zone_moving_difficulty_map[(int)HUPO]       = 2;
        tiandao.zone_moving_difficulty_map[(int)LINDI]      = 2;
        tiandao.zone_moving_difficulty_map[(int)MILIN]      = 3;
        tiandao.zone_moving_difficulty_map[(int)SHANYUE]    = 3;
        tiandao.zone_moving_difficulty_map[(int)DONGXUE]    = 2;
        tiandao.zone_moving_difficulty_map[(int)QIULIN]     = 2;
        tiandao.zone_moving_difficulty_map[(int)TAOYUAN]    = 2;
        tiandao.zone_moving_difficulty_map[(int)XIAGU]      = 2;
        tiandao.zone_moving_difficulty_map[(int)ZAOZE]      = 3;
        tiandao.zone_moving_difficulty_map[(int)HAIYANG]    = 2;
        tiandao.zone_moving_difficulty_map[(int)SHAMO]      = 2;
        tiandao.zone_moving_difficulty_map[(int)ANYUAN]     = 4;
        
        //init connection max number map
        for(int t = 0; t<_ZONE_TYPE_NUM; t++)
            tiandao.zone_type_connection_max_num_map[(E_ZONE_TYPE)t] = 4;
        tiandao.zone_type_connection_max_num_map[DONGXUE] = 1;
        tiandao.zone_type_connection_max_num_map[XIAGU] = 2;

        tiandao.zone_type_connection_max_num_map[CUNZHUANG] = 5;
        tiandao.zone_type_connection_max_num_map[GUANSAI] = 4;
        tiandao.zone_type_connection_max_num_map[SHIZHEN] = 6;
        tiandao.zone_type_connection_max_num_map[DUHUI] = 8;
    }
    //=============================================================================
    void database::initialize_zone_object( zone_object& zone, const std::string& name, const nfa_object& nfa, E_ZONE_TYPE type)
    {
        zone.nfa_id = nfa.id;
        zone.name = name;
        zone.type = type;
    }
    //=============================================================================
    const zone_object& database::get_zone(  const std::string& name )const
    { try {
        return get< zone_object, by_name >( name );
    } FC_CAPTURE_AND_RETHROW( (name) ) }
    //=============================================================================
    const zone_object* database::find_zone( const std::string& name )const
    {
        return find< zone_object, by_name >( name );
    }
    //=============================================================================
    const tiandao_property_object& database::get_tiandao_properties() const
    { try {
        return get< tiandao_property_object >();
    } FC_CAPTURE_AND_RETHROW() }
    //=============================================================================
    int database::calculate_moving_days_to_zone( const zone_object& zone )
    {
        const auto& tiandao = get_tiandao_properties();
        return tiandao.zone_moving_difficulty_map[(int)zone.type];
    }
    //=============================================================================
    // 区域灵气浓度N，由区域的五行材质决定
    int64_t database::calculate_zone_spiritual_energy( const zone_object& zone ) const
    {
        const auto& nfa_material = get<nfa_material_object, by_nfa_id>(zone.nfa_id);

        // 金木织药食，分别对应金木水火土 (Metal, Wood, Water, Fire, Earth)
        // 设计公式：Ni = sum(E_k) + mu * (sum(E_k * E_next_g(k)) - sum(E_k * E_next_l(k)))
        
        int64_t Ew = nfa_material.wood.amount.value;    // Wood (木)
        int64_t Ef = nfa_material.herb.amount.value;    // Fire (火)
        int64_t Ee = nfa_material.food.amount.value;    // Earth (土)
        int64_t Em = nfa_material.gold.amount.value;    // Metal (金)
        int64_t Ea = nfa_material.fabric.amount.value;  // Water (水)

        int64_t base_qi = Ew + Ef + Ee + Em + Ea;
        
        // 共鸣增益 mu = 0.5
        int64_t mu = TAIYI_1_PERCENT * 50;
        
        // 相生：木生火，火生土，土生金，金生水，水生木 (Wood -> Fire -> Earth -> Metal -> Water -> Wood)
        int64_t resonance_gain = Ew * Ef + Ef * Ee + Ee * Em + Em * Ea + Ea * Ew;
        
        // 相克：木克土，土克水，水克火，火克金，金克木 (Wood -> Earth -> Water -> Fire -> Metal -> Wood)
        int64_t suppression_loss = Ew * Ee + Ee * Ea + Ea * Ef + Ef * Em + Em * Ew;

        return base_qi + (int64_t)(mu * (resonance_gain - suppression_loss) / TAIYI_100_PERCENT);
    }
    //=============================================================================
    void database::process_tiandao()
    {
        uint32_t bn = head_block_num();

        //one day means one virtual month
        uint32_t yn = bn / (TAIYI_VMONTH_BLOCK_NUM * 12);
        bn -= yn * (TAIYI_VMONTH_BLOCK_NUM * 12);
        uint32_t mn = bn / TAIYI_VMONTH_BLOCK_NUM;
        bn -= mn * TAIYI_VMONTH_BLOCK_NUM;
        uint32_t dn = bn / TAIYI_VDAY_BLOCK_NUM;
        bn -= dn * TAIYI_VDAY_BLOCK_NUM;
        uint32_t tod = bn / (TAIYI_VDAY_BLOCK_NUM/4); //time on a day, 0=凌晨；1=上午；2=下午；3=夜晚
        //wlog("${y}年${m}月${d}日tod=${tod}, bn=${bn}", ("y", yn)("m", mn)("d", dn)("tod", tod)("bn", bn));
                
        //solar term number，注意虚拟公历中每个月都是30天
        uint32_t tn = tyme::SolarDay::from_ymd(yn+2, mn+1, dn+1).get_term().get_index();
        tn = (tn + 21) % 24; //序号0由冬至对齐到春分
        
        const auto& tiandao = get_tiandao_properties();
        FC_ASSERT(tiandao.v_years <= yn, "virtual year number (${tn}) bigger than now (${yn}).", ("tn", tiandao.v_years)("yn", yn));
        if(yn > tiandao.v_years) {
            //next year
            FC_ASSERT(mn == 0 && tn == 0, "both virtual month number (${mn}) and time number (${tn}) must be zero!", ("mn", mn)("tn", tn));
            
            //统计活人
            uint32_t live_num = 0;
            const auto& actor_by_health_idx = get_index< actor_index, by_health >();
            auto itr = actor_by_health_idx.lower_bound( std::numeric_limits<int16_t>::max() );
            auto end = actor_by_health_idx.end();
            while( itr != end )
            {
                if(itr->health <= 0)
                    break;
                ++live_num;
                ++itr;
            }
            uint32_t amount_actor = (uint32_t)actor_by_health_idx.size();
            uint32_t dead_num = amount_actor - live_num;
            
            uint32_t born_this_year = amount_actor - tiandao.amount_actor_last_vyear;
            uint32_t dead_this_year = dead_num - tiandao.dead_actor_last_vyear;
            
            modify(tiandao, [&]( tiandao_property_object& t ) {
                t.v_years = yn;
                t.v_months = mn;
                t.v_days = dn;
                t.v_timeonday = tod;
                t.v_times = tn;
                
                t.amount_actor_last_vyear = amount_actor;
                t.dead_actor_last_vyear = dead_num;
            });
            
            wlog("${y}年，世界人口数=${n}，出生=${m}，死亡=${d}", ("y", yn)("n", live_num)("m", born_this_year)("d", dead_this_year));

            push_virtual_operation( tiandao_year_change_operation( TAIYI_DANUO_ACCOUNT, yn, mn, tn, live_num, dead_num ) );
            push_virtual_operation( tiandao_month_change_operation( TAIYI_DANUO_ACCOUNT, yn, mn, tn ) );
            push_virtual_operation( tiandao_time_change_operation( TAIYI_DANUO_ACCOUNT, yn, mn, tn ) );
        }
        else {
            FC_ASSERT(tiandao.v_months <= mn, "virtual month number (${tn}) bigger than now (${mn}).", ("tn", tiandao.v_months)("mn", mn));
            if(mn > tiandao.v_months) {
                //next month
                FC_ASSERT(dn == 0, "virtual day number (${dn}) must be zero!", ("dn", dn));

                modify(tiandao, [&]( tiandao_property_object& t ) {
                    t.v_months = mn;
                    t.v_days = dn;
                    t.v_timeonday = tod;
                    t.v_times = tn;
                });

                push_virtual_operation( tiandao_month_change_operation( TAIYI_DANUO_ACCOUNT, yn, mn, tn ) );
                push_virtual_operation( tiandao_time_change_operation( TAIYI_DANUO_ACCOUNT, yn, mn, tn ) );
            }
            else {
                FC_ASSERT(tiandao.v_times <= tn, "virtual time number (${tn}) bigger than now (${tnn}).", ("tn", tiandao.v_times)("tnn", tn));
                if(tn > tiandao.v_times) {
                    //next time

                    modify(tiandao, [&]( tiandao_property_object& t ) {
                        t.v_days = dn;
                        t.v_timeonday = tod;
                        t.v_times = tn;
                    });

                    push_virtual_operation( tiandao_time_change_operation( TAIYI_DANUO_ACCOUNT, yn, mn, tn ) );
                }
                else {
                    FC_ASSERT(tiandao.v_days <= dn, "virtual day number (${dn}) bigger than now (${dnn}).", ("dn", tiandao.v_days)("dnn", dn));
                    if(dn > tiandao.v_days) {
                        //next day
                        FC_ASSERT(tod == 0, "virtual time on day number (${tod}) must be zero!", ("tod", tod));

                        modify(tiandao, [&]( tiandao_property_object& t ) {
                            t.v_days = dn;
                            t.v_timeonday = tod;
                        });
                    }
                    else {
                        modify(tiandao, [&]( tiandao_property_object& t ) {
                            t.v_timeonday = tod;
                        });
                    }
                }
            }
        }
    }
    //=============================================================================
    bool database::is_contract_allowed_by_zone(const contract_object& contract, const zone_id_type& zone_id) const
    {
        const auto* zone = find<zone_object, by_id>(zone_id);
        if (!zone)
            return true;
        
        const auto& permission_idx = get_index< zone_contract_permission_index >().indices().get< chain::by_zone >();

        //先检查区域有没有明确许可
        const auto* check1 = find< zone_contract_permission_object, by_zone >(boost::make_tuple( zone->id, contract.id ));
        if (check1)
            return check1->allowed;
        
        //再检查参考区域的直接许可项
        const auto* ref_zone = find< chain::zone_object, chain::by_id >( zone->ref_prohibited_contract_zone );
        if(ref_zone) {
            const auto* check2 = find< zone_contract_permission_object, by_zone >(boost::make_tuple( ref_zone->id, contract.id ));
            if (check2)
                return check2->allowed;
        }
        
        //默认是允许的
        return true;
    }
} } //taiyi::chain

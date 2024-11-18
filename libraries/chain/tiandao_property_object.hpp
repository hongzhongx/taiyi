#pragma once
#include <chain/taiyi_fwd.hpp>

#include <fc/uint128.hpp>

#include <chain/taiyi_object_types.hpp>

#include <protocol/asset.hpp>

namespace taiyi { namespace chain {

    using taiyi::protocol::asset;
    using taiyi::protocol::price;

    using chainbase::t_flat_map;

    //********* 24 solar terms, number range is 0 to 23 *******//
    //立春 the Beginning of Spring （1st solar term）Feb.3,4, or 5
    //雨水 Rain Water （2nd solar term）Feb.18,19 or 20
    //惊蛰 the Waking of Insects （3rd solar term）Mar.5,6, or 7
    //春分 the Spring Equinox （4th solar term）Mar.20,21 or 22
    //清明 Pure Brightness （5th solar term）Apr.4,5 or 6
    //谷雨 Grain Rain （6th solar teram）Apr.19,20 or 21
    //立夏 the Beginning of Summer （7th solar term）May 5,6 or 7
    //小满 Lesser Fullness of Grain （8th solar term）May 20,21 or 22
    //芒种 Grain in Beard （9th solar term）Jun.5,6 or 7
    //夏至 the Summer Solstice （10th solar term）Jun.21 or 22
    //小暑 Lesser Heat （11th solar term）Jul.6,7 or 8
    //大暑 Greater Heat （12th solar term）Jul.22,23 or 24
    //立秋 the Beginning of Autumn （13th solar term）Aug.7,8 or 9
    //处暑 the End of Heat （14th solar term）Aug.22,23 or 24
    //白露 White Dew （15th solar term）Sep.7,8 or 9
    //秋分 the Autumn Equinox （16th solar term）Sep.22,23 or 24
    //寒露 Cold Dew （17th solar term）Oct.8 or 9
    //霜降 Frost's Descent （18th solar term）Oct.23 or 24
    //立冬 the Beginning of Winter （19th solar term）Nov.7 or 8
    //小雪 Lesser Snow （20th solar term）Nov.22 or 23
    //大雪 Greater Snow （21st solar term）Dec.6,7 or 8
    //冬至 the Winter Solstice （22nd solar term）Dec.21,22 or 23
    //小寒 Lesser Cold （23rd solar term）Jan.5,6 or 7
    //大寒 Greater Cold （24th solar term）Jan.20 or 21

    //**********  five phases, number is 0 to 4 ********//
    //metal     金
    //wood      木
    //water     水
    //fire      火
    //earth     土

    /**
     * @class tiandao_property_object
     * @brief Maintains global nature law properties
     * @ingroup object
     * @ingroup implementation
     *
     * This is an implementation detail. The values here are calculated during normal game operations and reflect the
     * current values of global nature properties.
     */
    class tiandao_property_object : public object< tiandao_property_object_type, tiandao_property_object>
    {
    public:
        template< typename Constructor, typename Allocator >
        tiandao_property_object( Constructor&& c, allocator< Allocator > a )
        : zone_grow_gold_speed_map(a), zone_grow_food_speed_map(a), zone_grow_wood_speed_map(a), zone_grow_fabric_speed_map(a), zone_grow_herb_speed_map(a), zone_gold_max_map(a), zone_food_max_map(a), zone_wood_max_map(a), zone_fabric_max_map(a), zone_herb_max_map(a), zone_moving_difficulty_map(a), zone_type_connection_max_num_map(a)
        {
            c( *this );
        }

        tiandao_property_object(){}

        id_type     id;

        int64_t     cruelty;
        int64_t     enjoyment;
        int64_t     decay;
        int64_t     falsity;
        
        uint32_t    v_years;
        uint32_t    v_months;
        uint32_t    v_times; //same as solar term number

        time_point_sec    next_npc_born_time;
        
        std::vector<int64_t>  zone_grow_gold_speed_map;     // how many assets value grown per v_month, index is E_ZONE_TYPE
        std::vector<int64_t>  zone_grow_food_speed_map;     // how many assets value grown per v_month, index is E_ZONE_TYPE
        std::vector<int64_t>  zone_grow_wood_speed_map;     // how many assets value grown per v_month, index is E_ZONE_TYPE
        std::vector<int64_t>  zone_grow_fabric_speed_map;   // how many assets value grown per v_month, index is E_ZONE_TYPE
        std::vector<int64_t>  zone_grow_herb_speed_map;     // how many assets value grown per v_month, index is E_ZONE_TYPE

        std::vector<int64_t>  zone_gold_max_map;     // how many assets value max in certain tile, index is E_ZONE_TYPE
        std::vector<int64_t>  zone_food_max_map;     // how many assets value max in certain tile, index is E_ZONE_TYPE
        std::vector<int64_t>  zone_wood_max_map;     // how many assets value max in certain tile, index is E_ZONE_TYPE
        std::vector<int64_t>  zone_fabric_max_map;   // how many assets value max in certain tile, index is E_ZONE_TYPE
        std::vector<int64_t>  zone_herb_max_map;     // how many assets value max in certain tile, index is E_ZONE_TYPE

        std::vector<int>  zone_moving_difficulty_map;   // how many days need to pass through, index is E_ZONE_TYPE
        
        t_flat_map<E_ZONE_TYPE, uint> zone_type_connection_max_num_map; //max number of connections to different zone for certain type
        
        uint32_t    amount_actor_last_vyear = 0;
        uint32_t    dead_actor_last_vyear = 0;
    };

    typedef multi_index_container<
        tiandao_property_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< tiandao_property_object, tiandao_property_object::id_type, &tiandao_property_object::id > >
        >,
        allocator< tiandao_property_object >
    > tiandao_property_index;

} } // taiyi::chain

FC_REFLECT( taiyi::chain::tiandao_property_object, (id)(cruelty)(enjoyment)(decay)(falsity)(v_years)(v_months)(v_times)(next_npc_born_time)(zone_grow_gold_speed_map)(zone_grow_food_speed_map)(zone_grow_wood_speed_map)(zone_grow_fabric_speed_map)(zone_grow_herb_speed_map)(zone_gold_max_map)(zone_food_max_map)(zone_wood_max_map)(zone_fabric_max_map)(zone_herb_max_map)(zone_moving_difficulty_map)(zone_type_connection_max_num_map)(amount_actor_last_vyear)(dead_actor_last_vyear) )
CHAINBASE_SET_INDEX_TYPE( taiyi::chain::tiandao_property_object, taiyi::chain::tiandao_property_index )

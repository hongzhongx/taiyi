#pragma once
#include <chain/taiyi_fwd.hpp>

#include <protocol/authority.hpp>
#include <protocol/taiyi_operations.hpp>

#include <chain/taiyi_object_types.hpp>

namespace taiyi { namespace chain {
    
    using chainbase::t_flat_map;

    enum E_ACTOR_STANDPOINT_TYPE
    {
        UPRIGHT     = 0,//刚正
        KINDNESS    = 1,//仁善
        MIDDLEBROW  = 2,//中庸
        REBEL       = 3,//叛逆
        SOLIPSISM   = 4 //唯我
    };

    class actor_object : public object < actor_object_type, actor_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR(actor_object)

    public:
        template< typename Constructor, typename Allocator >
        actor_object(Constructor&& c, allocator< Allocator > a)
            : name(a), family_name(a), mid_name(a), last_name(a)
        {
            c(*this);
        }

        id_type             id;
        nfa_id_type         nfa_id      = nfa_id_type::max();
        
        std::string         name;
        std::string         family_name;
        std::string         mid_name;
        std::string         last_name;
        
        uint16_t            age         = 0;        //年龄
        int16_t             health      = 100;      //健康
        int16_t             health_max  = 100;
        uint16_t            init_attribute_amount_max = 0;  //出生时候角色属性初始化总点数上限，在创建角色时候决定

        bool                born        = false;    //是否出生
        int                 gender      = 0;        //-2=男生女相，-1=男，0=无性，1=女，2=女生男相, 3=双性
        bool is_male() const { return gender == -2 || gender == -1 || gender == 3; }
        bool is_female() const { return gender == 1 || gender == 2 || gender == 3; }

        int                 sexuality   = 0;        //0=无性取向，1=喜欢男性，2=喜欢女性，3=双性恋
        int                 fertility   = 100;      //基础生育能力
        
        time_point_sec      born_time;
        int                 born_vyears     = 0;
        int                 born_vmonths    = 0;
        int                 born_vdays      = 0;
        int                 born_vtod       = 0;
        int                 born_vtimes     = 0;    //solar term, 0-23
        uint                standpoint      = 500;  //立场, [0,1000]
        int32_t             loyalty         = 300;  //忠贞
        
        zone_id_type        location = zone_id_type::max();
        zone_id_type        base = zone_id_type::max();     //从属地
        
        actor_id_type       need_mating_target;             //春宵需求对象
        uint32_t            need_mating_end_block_num = 0;  //春宵需求截止时间
        
        actor_id_type       need_bullying_target;           //欺辱需求对象
        uint32_t            need_bullying_end_block_num = 0;//欺辱需求截止时间
        
        bool                is_pregnant                 = false;
        actor_id_type       pregnant_father             = actor_id_type::max();
        actor_id_type       pregnant_mother             = actor_id_type::max();
        uint32_t            pregnant_end_block_num      = 0;    //孕期结束时间
        uint32_t            pregnant_lock_end_block_num = 0;    //怀孕锁定期结束时间

        time_point_sec      last_update;
        uint32_t            next_tick_block = std::numeric_limits<uint32_t>::max();

        E_ACTOR_STANDPOINT_TYPE get_standpoint_type() const {
            if(standpoint <= 124)
                return UPRIGHT;     //"刚正" #范围 0~124，标准值 0
            else if(standpoint <= 374)
                return KINDNESS;    //"仁善" #范围 125~374，标准值 250
            else if(standpoint <= 624)
                return MIDDLEBROW;  //"中庸" #范围 375~624，标准值 500
            else if(standpoint <= 874)
                return REBEL;       //"叛逆" #范围 625~874，标准值 750
            else
                return SOLIPSISM;   //"唯我" #范围 875~1000，标准值 1000
        }
    };

    struct by_name;
    struct by_nfa_id;
    struct by_location;
    struct by_base;
    struct by_health;
    struct by_solor_term;
    struct by_tick_time;
    typedef multi_index_container<
        actor_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< actor_object, actor_id_type, &actor_object::id > >,
            ordered_unique< tag< by_name >, member< actor_object, std::string, &actor_object::name >, strcmp_less >,
            ordered_unique< tag< by_nfa_id >, member< actor_object, nfa_id_type, &actor_object::nfa_id> >,
            ordered_unique< tag< by_location >,
                composite_key< actor_object,
                    member< actor_object, zone_id_type, &actor_object::location >,
                    member< actor_object, actor_id_type, &actor_object::id >
                >
            >,
            ordered_unique< tag< by_base >,
                composite_key< actor_object,
                    member< actor_object, zone_id_type, &actor_object::base >,
                    member< actor_object, actor_id_type, &actor_object::id >
                >
            >,
            ordered_unique< tag< by_health >,
                composite_key< actor_object,
                    member< actor_object, int16_t, &actor_object::health >,
                    member< actor_object, actor_id_type, &actor_object::id >
                >,
                composite_key_compare< std::greater< int16_t >, std::less< actor_id_type > >
            >,
            ordered_unique< tag< by_solor_term >,
                composite_key< actor_object,
                    member< actor_object, int, &actor_object::born_vtimes >,
                    member< actor_object, actor_id_type, &actor_object::id >
                >
            >,
            ordered_unique< tag< by_tick_time >,
                composite_key< actor_object,
                    member< actor_object, uint32_t, &actor_object::next_tick_block >,
                    member< actor_object, actor_id_type, &actor_object::id >
                >
            >
        >,
        allocator< actor_object >
    > actor_index;

    //=========================================================================

    class actor_core_attributes_object : public object < actor_core_attributes_object_type, actor_core_attributes_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR(actor_core_attributes_object)

    public:
        template< typename Constructor, typename Allocator >
        actor_core_attributes_object(Constructor&& c, allocator< Allocator > a)
        {
            c(*this);
        }

        id_type             id;

        actor_id_type       actor;
                
        int16_t             strength        = 0;    //膂力
        int16_t             strength_max    = 0;
        int16_t             physique        = 0;    //体质
        int16_t             physique_max    = 0;
        int16_t             agility         = 0;    //灵敏
        int16_t             agility_max     = 0;
        int16_t             vitality        = 0;    //根骨
        int16_t             vitality_max    = 0;
        int16_t             comprehension   = 0;    //悟性
        int16_t             comprehension_max = 0;
        int16_t             willpower       = 0;    //定力
        int16_t             willpower_max   = 0;

        int16_t             charm           = 0;    //魅力
        int16_t             charm_max       = 0;
        int16_t             mood            = 0;    //心情
        int16_t             mood_max        = 0;

        time_point_sec      last_update;
        time_point_sec      created;
    };

    struct by_actor;
    typedef multi_index_container<
        actor_core_attributes_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< actor_core_attributes_object, actor_core_attributes_id_type, &actor_core_attributes_object::id > >,
            ordered_unique< tag< by_actor >, member< actor_core_attributes_object, actor_id_type, &actor_core_attributes_object::actor> >
        >,
        allocator< actor_core_attributes_object >
    > actor_core_attributes_index;

    //=========================================================================

    class actor_group_object : public object < actor_group_object_type, actor_group_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR(actor_group_object)

    public:
        template< typename Constructor, typename Allocator >
        actor_group_object(Constructor&& c, allocator< Allocator > a)
        {
            c(*this);
        }

        id_type             id;

        actor_id_type       actor;          //队员actor id
        actor_id_type       leader;         //队长actor id，也是组id
    };

    struct by_leader_member;
    typedef multi_index_container<
        actor_group_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< actor_group_object, actor_group_id_type, &actor_group_object::id > >,
            ordered_unique< tag< by_actor >, member< actor_group_object, actor_id_type, &actor_group_object::actor> >,
            ordered_unique< tag< by_leader_member >,
                composite_key< actor_group_object,
                    member< actor_group_object, actor_id_type, &actor_group_object::leader >,
                    member< actor_group_object, actor_id_type, &actor_group_object::actor >
                >
            >
        >,
        allocator< actor_group_object >
    > actor_group_index;

    //=========================================================================

    class actor_talent_rule_object : public object < actor_talent_rule_object_type, actor_talent_rule_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR(actor_talent_rule_object)

    public:
        template< typename Constructor, typename Allocator >
        actor_talent_rule_object(Constructor&& c, allocator< Allocator > a)
            : title(a), description(a)
        {
            c(*this);
        }

        id_type             id;
        contract_id_type    main_contract;

        std::string         title;
        std::string         description;
        int                 init_attribute_amount_modifier = 0; //对角色出生时候初始化属性的总点数加成
        int                 max_triggers = 1;   //最大触发次数
        bool                removed = false;
        
        time_point_sec      last_update;
        time_point_sec      created;
    };

    struct by_contract;
    typedef multi_index_container<
        actor_talent_rule_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< actor_talent_rule_object, actor_talent_rule_id_type, &actor_talent_rule_object::id > >,
            ordered_unique< tag< by_contract >, member< actor_talent_rule_object, contract_id_type, &actor_talent_rule_object::main_contract > >
        >,
        allocator< actor_talent_rule_object >
    > actor_talent_rule_index;

    //=========================================================================

    class actor_talents_object : public object < actor_talents_object_type, actor_talents_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR(actor_talents_object)

    public:
        template< typename Constructor, typename Allocator >
        actor_talents_object(Constructor&& c, allocator< Allocator > a)
            : talents(a)
        {
            c(*this);
        }

        id_type             id;

        actor_id_type       actor;

        t_flat_map< actor_talent_rule_id_type, uint16_t >   talents; //rule id -> trigger number

        time_point_sec      last_update;
        time_point_sec      created;
    };

    struct by_actor;
    typedef multi_index_container<
        actor_talents_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< actor_talents_object, actor_talents_id_type, &actor_talents_object::id > >,
            ordered_unique< tag< by_actor >, member< actor_talents_object, actor_id_type, &actor_talents_object::actor > >
        >,
        allocator< actor_talents_object >
    > actor_talents_index;
    
    //=========================================================================

    enum E_ACTOR_RELATION_TYPE
    {
        NC = 0,
        //亲友
        FRIEND          =1, //知心之交
        SWORN           =2, //义结金兰
        MENTOR          =3, //授业恩师
        DESCENDANT      =4, //嫡系传人
        BENEFACTOR      =5, //有恩于我（恩人）
        BENEFICIARY     =6, //恩深义重（受益者）
        ADORE           =7, //倾心爱慕
        ADORED          =8, //被慕倾心
        LOVE            =9, //两情相悦
        COUPLE          =10,//结发夫妻
        PARENT          =11,//亲父亲母
        ADOPTIVE_PARENT =12,//义父义母
        SIBLING         =13,//兄弟姐妹
        CHILD           =14,//儿女子嗣
        //仇敌
        IRRECONCILABLE  =15,//誓不两立
        BLOOD           =16 //血海深仇
    };

    class actor_connection_object : public object < actor_connection_object_type, actor_connection_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR(actor_connection_object)

    public:
        template< typename Constructor, typename Allocator >
        actor_connection_object(Constructor&& c, allocator< Allocator > a)
        {
            c(*this);
        }

        id_type                 id;

        actor_id_type           actor;
        E_ACTOR_RELATION_TYPE   type;
        
        flat_set<actor_id_type> people; //actor对他人的人物关系，即actor是“某某”的“type”者

        time_point_sec          last_update;
    };

    struct by_actor;
    struct by_relation_type;
    typedef multi_index_container<
        actor_connection_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< actor_connection_object, actor_connection_id_type, &actor_connection_object::id > >,
            ordered_unique< tag< by_actor >,
                composite_key< actor_connection_object,
                    member< actor_connection_object, actor_id_type, &actor_connection_object::actor >,
                    member< actor_connection_object, actor_connection_id_type, &actor_connection_object::id >
                >
            >,
            ordered_unique< tag< by_relation_type >,
                composite_key< actor_connection_object,
                    member< actor_connection_object, actor_id_type, &actor_connection_object::actor >,
                    member< actor_connection_object, E_ACTOR_RELATION_TYPE, &actor_connection_object::type >
                >
            >
        >,
        allocator< actor_connection_object >
    > actor_connection_index;

    class actor_relation_object : public object < actor_relation_object_type, actor_relation_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR(actor_relation_object)

    public:
        template< typename Constructor, typename Allocator >
        actor_relation_object(Constructor&& c, allocator< Allocator > a)
        {
            c(*this);
        }

        id_type             id;

        actor_id_type       actor;
        actor_id_type       target;
        
        int32_t             favor = 0;  //好感度

        time_point_sec      last_update;
        
        //-6 to +6, 13 levels
        int get_favor_level() const {
            if(favor <= -26000)
                return -6; //血仇, -30,000~-26,000
            else if(favor <= -22000)
                return -5; //痛恨, -25,999~-22,000
            else if(favor <= -18000)
                return -4; //憎恨, -21,999~-18,000
            else if(favor <= -14000)
                return -3; //仇视, -17,999~-14,000
            else if(favor <= -10000)
                return -2; //敌视, -13,999~-10,000
            else if(favor <= -6000)
                return -1; //鄙视, -9,999~-6,000
            else if(favor <= 6000)
                return 0; //陌路, -5,999~+6,000
            else if(favor <= 10000)
                return 1; //冷淡, +6,001~+10,000
            else if(favor <= 14000)
                return 2; //融洽, +10,001~+14,000
            else if(favor <= 18000)
                return 3; //热忱, +14,001~+18,000
            else if(favor <= 22000)
                return 4; //喜爱, +18,001~+22,000
            else if(favor <= 26000)
                return 5; //亲密, +22,001~+26,000
            else
                return 6; //不渝, +26,001~+30,000
        }
    };

    struct by_actor;
    struct by_target;
    struct by_actor_target;
    struct by_actor_favor;
    typedef multi_index_container<
        actor_relation_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< actor_relation_object, actor_relation_id_type, &actor_relation_object::id > >,
            ordered_unique< tag< by_actor >,
                composite_key< actor_relation_object,
                    member< actor_relation_object, actor_id_type, &actor_relation_object::actor >,
                    member< actor_relation_object, actor_relation_id_type, &actor_relation_object::id >
                >
            >,
            ordered_unique< tag< by_target >,
                composite_key< actor_relation_object,
                    member< actor_relation_object, actor_id_type, &actor_relation_object::target >,
                    member< actor_relation_object, actor_relation_id_type, &actor_relation_object::id >
                >
            >,
            ordered_unique< tag< by_actor_target >,
                composite_key< actor_relation_object,
                    member< actor_relation_object, actor_id_type, &actor_relation_object::actor >,
                    member< actor_relation_object, actor_id_type, &actor_relation_object::target >
                >
            >,
            ordered_unique< tag< by_actor_favor >,
                composite_key< actor_relation_object,
                    member< actor_relation_object, actor_id_type, &actor_relation_object::actor >,
                    member< actor_relation_object, int32_t, &actor_relation_object::favor >,
                    member< actor_relation_object, actor_relation_id_type, &actor_relation_object::id >
                >,
                composite_key_compare< std::less< actor_id_type >, std::greater< int32_t >, std::less< actor_relation_id_type > >
            >
        >,
        allocator< actor_relation_object >
    > actor_relation_index;

} } // taiyi::chain

namespace mira {
    template<> struct is_static_length< taiyi::chain::actor_core_attributes_object > : public boost::true_type {};
    template<> struct is_static_length< taiyi::chain::actor_group_object > : public boost::true_type {};
    template<> struct is_static_length< taiyi::chain::actor_relation_object > : public boost::true_type {};
} // mira

FC_REFLECT_ENUM( taiyi::chain::E_ACTOR_STANDPOINT_TYPE, (UPRIGHT)(KINDNESS)(MIDDLEBROW)(REBEL)(SOLIPSISM) )

FC_REFLECT(taiyi::chain::actor_object, (id)(nfa_id)(name)(family_name)(mid_name)(last_name)(age)(health)(health_max)(init_attribute_amount_max)(born)(gender)(sexuality)(fertility)(born_time)(born_vyears)(born_vmonths)(born_vdays)(born_vtod)(born_vtimes)(standpoint)(loyalty)(location)(base)(need_mating_target)(need_mating_end_block_num)(need_bullying_target)(need_bullying_end_block_num)(is_pregnant)(pregnant_father)(pregnant_mother)(pregnant_end_block_num)(pregnant_lock_end_block_num)(last_update)(next_tick_block))
CHAINBASE_SET_INDEX_TYPE(taiyi::chain::actor_object, taiyi::chain::actor_index)

FC_REFLECT(taiyi::chain::actor_core_attributes_object, (id)(actor)(strength)(strength_max)(physique)(physique_max)(agility)(agility_max)(vitality)(vitality_max)(comprehension)(comprehension_max)(willpower)(willpower_max)(charm)(charm_max)(mood)(mood_max)(last_update)(created))
CHAINBASE_SET_INDEX_TYPE(taiyi::chain::actor_core_attributes_object, taiyi::chain::actor_core_attributes_index)

FC_REFLECT(taiyi::chain::actor_group_object, (id)(actor)(leader))
CHAINBASE_SET_INDEX_TYPE(taiyi::chain::actor_group_object, taiyi::chain::actor_group_index)

FC_REFLECT(taiyi::chain::actor_talent_rule_object, (id)(main_contract)(title)(description)(init_attribute_amount_modifier)(max_triggers)(removed)(last_update)(created))
CHAINBASE_SET_INDEX_TYPE(taiyi::chain::actor_talent_rule_object, taiyi::chain::actor_talent_rule_index)

FC_REFLECT(taiyi::chain::actor_talents_object, (id)(actor)(talents)(last_update)(created))
CHAINBASE_SET_INDEX_TYPE(taiyi::chain::actor_talents_object, taiyi::chain::actor_talents_index)

FC_REFLECT_ENUM( taiyi::chain::E_ACTOR_RELATION_TYPE, (NC)(FRIEND)(SWORN)(MENTOR)(DESCENDANT)(BENEFACTOR)(BENEFICIARY)(ADORE)(ADORED)(LOVE)(COUPLE)(PARENT)(ADOPTIVE_PARENT)(SIBLING)(CHILD)(IRRECONCILABLE)(BLOOD))

FC_REFLECT(taiyi::chain::actor_relation_object, (id)(actor)(target)(favor)(last_update))
CHAINBASE_SET_INDEX_TYPE(taiyi::chain::actor_relation_object, taiyi::chain::actor_relation_index)

FC_REFLECT(taiyi::chain::actor_connection_object, (id)(actor)(type)(people)(last_update))
CHAINBASE_SET_INDEX_TYPE(taiyi::chain::actor_connection_object, taiyi::chain::actor_connection_index)

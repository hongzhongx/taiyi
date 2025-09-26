#pragma once
#include <chain/taiyi_fwd.hpp>

#include <plugins/database_api/database_api.hpp>
#include <plugins/block_api/block_api.hpp>
#include <plugins/account_history_api/account_history_api.hpp>
#include <plugins/account_by_key_api/account_by_key_api.hpp>
#include <plugins/network_broadcast_api/network_broadcast_api.hpp>
#include <plugins/baiyujing_api/baiyujing_api_legacy_objects.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/api.hpp>

namespace taiyi { namespace plugins { namespace baiyujing_api {

    using std::vector;
    using fc::variant;
    using fc::optional;
    
    using namespace chain;

    namespace detail{ class baiyujing_api_impl; }

    struct api_operation_object
    {
        api_operation_object() {}
        api_operation_object(const account_history::api_operation_object& obj, const legacy_operation& l_op) : trx_id(obj.trx_id), block(obj.block), trx_in_block(obj.trx_in_block), virtual_op(obj.virtual_op), timestamp(obj.timestamp), op(l_op)
        {}
        
        transaction_id_type  trx_id;
        uint32_t             block = 0;
        uint32_t             trx_in_block = 0;
        uint32_t             op_in_trx = 0;
        uint32_t             virtual_op = 0;
        fc::time_point_sec   timestamp;
        legacy_operation     op;
    };

    struct api_account_object
    {
        api_account_object(const database_api::api_account_object& a) : id(a.id), name(a.name), owner(a.owner), active(a.active), posting(a.posting), memo_key(a.memo_key), json_metadata(a.json_metadata), proxy(a.proxy), last_owner_update(a.last_owner_update), last_account_update(a.last_account_update), created(a.created), recovery_account(a.recovery_account), last_account_recovery(a.last_account_recovery), can_adore(a.can_adore), balance(legacy_asset::from_asset(a.balance)), reward_yang_balance(legacy_asset::from_asset(a.reward_yang_balance)), reward_qi_balance(legacy_asset::from_asset(a.reward_qi_balance)), reward_feigang_balance(legacy_asset::from_asset(a.reward_feigang_balance)), qi(legacy_asset::from_asset(a.qi)), delegated_qi(legacy_asset::from_asset(a.delegated_qi)), received_qi(legacy_asset::from_asset(a.received_qi)), qi_withdraw_rate(legacy_asset::from_asset(a.qi_withdraw_rate)), next_qi_withdrawal_time(a.next_qi_withdrawal_time), withdrawn(a.withdrawn), to_withdraw(a.to_withdraw), withdraw_routes(a.withdraw_routes), simings_adored_for(a.simings_adored_for), gold(legacy_asset::from_asset(a.gold)), food(legacy_asset::from_asset(a.food)), wood(legacy_asset::from_asset(a.wood)), fabric(legacy_asset::from_asset(a.fabric)), herb(legacy_asset::from_asset(a.herb))
        {
            proxied_vsf_adores.insert(proxied_vsf_adores.end(), a.proxied_vsf_adores.begin(), a.proxied_vsf_adores.end());
        }
        
        api_account_object(){}
        
        account_id_type   id;
        
        account_name_type name;
        authority         owner;
        authority         active;
        authority         posting;
        public_key_type   memo_key;
        string            json_metadata;
        account_name_type proxy;
        
        time_point_sec    last_owner_update;
        time_point_sec    last_account_update;
        
        time_point_sec    created;
        account_name_type recovery_account;
        time_point_sec    last_account_recovery;
        
        bool              can_adore = false;

        legacy_asset      balance;
        
        legacy_asset      reward_yang_balance;
        legacy_asset      reward_qi_balance;
        legacy_asset      reward_feigang_balance;

        legacy_asset      qi;
        legacy_asset      delegated_qi;
        legacy_asset      received_qi;
        legacy_asset      qi_withdraw_rate;
        time_point_sec    next_qi_withdrawal_time;
        share_type        withdrawn;
        share_type        to_withdraw;
        uint16_t          withdraw_routes = 0;
        
        vector< share_type > proxied_vsf_adores;
        
        uint16_t          simings_adored_for = 0;

        legacy_asset        gold;
        legacy_asset        food;
        legacy_asset        wood;
        legacy_asset        fabric;
        legacy_asset        herb;
    };

    struct extended_account : public api_account_object
    {
        extended_account(){}
        extended_account(const database_api::api_account_object& a) : api_account_object(a) {}
        
        legacy_asset                            qi_balance;  /// convert qi to qi taiyi
        map< uint64_t, api_operation_object >   transfer_history; /// transfer to/from qi
        map< uint64_t, api_operation_object >   other_history;
        set< string >                           siming_adores;
    };

    struct extended_dynamic_global_properties
    {
        extended_dynamic_global_properties() {}
        extended_dynamic_global_properties(const database_api::api_dynamic_global_property_object& o) : head_block_number(o.head_block_number), head_block_id(o.head_block_id), time(o.time), current_siming(o.current_siming), current_supply(legacy_asset::from_asset(o.current_supply)), total_qi(legacy_asset::from_asset(o.total_qi)), pending_rewarded_qi(legacy_asset::from_asset(o.pending_rewarded_qi)), pending_rewarded_feigang(legacy_asset::from_asset(o.pending_rewarded_feigang)), pending_cultivation_qi(legacy_asset::from_asset(o.pending_cultivation_qi)), total_gold(legacy_asset::from_asset(o.total_gold)), total_food(legacy_asset::from_asset(o.total_food)), total_wood(legacy_asset::from_asset(o.total_wood)), total_fabric(legacy_asset::from_asset(o.total_fabric)), total_herb(legacy_asset::from_asset(o.total_herb)), maximum_block_size(o.maximum_block_size), current_aslot(o.current_aslot), recent_slots_filled(o.recent_slots_filled), participation_count(o.participation_count), last_irreversible_block_num(o.last_irreversible_block_num), delegation_return_period(o.delegation_return_period), content_reward_yang_percent(o.content_reward_yang_percent), content_reward_qi_fund_percent(o.content_reward_qi_fund_percent),
            last_siming_production_reward(legacy_asset::from_asset(o.last_siming_production_reward))
        {}
        
        uint32_t          head_block_number = 0;
        block_id_type     head_block_id;
        time_point_sec    time;
        account_name_type current_siming;
        
        legacy_asset      current_supply;
        legacy_asset      total_qi;
        legacy_asset      pending_rewarded_qi;
        legacy_asset      pending_rewarded_feigang;
        legacy_asset      pending_cultivation_qi;
        
        legacy_asset      total_gold;
        legacy_asset      total_food;
        legacy_asset      total_wood;
        legacy_asset      total_fabric;
        legacy_asset      total_herb;
        
        uint32_t          maximum_block_size = 0;
        uint64_t          current_aslot = 0;
        fc::uint128_t     recent_slots_filled;
        uint8_t           participation_count = 0;
        
        uint32_t          last_irreversible_block_num = 0;
        
        uint32_t          delegation_return_period = TAIYI_DELEGATION_RETURN_PERIOD;
        
        uint16_t          content_reward_yang_percent = TAIYI_CONTENT_REWARD_YANG_PERCENT;
        uint16_t          content_reward_qi_fund_percent = TAIYI_CONTENT_REWARD_QI_FUND_PERCENT;
        
        legacy_asset      last_siming_production_reward;
    };

    struct api_siming_object
    {
        api_siming_object() {}
        api_siming_object(const database_api::api_siming_object& w) : id(w.id), owner(w.owner), created(w.created), url(w.url), total_missed(w.total_missed), last_aslot(w.last_aslot), last_confirmed_block_num(w.last_confirmed_block_num), signing_key(w.signing_key), props(w.props), adores(w.adores), virtual_last_update(w.virtual_last_update), virtual_position(w.virtual_position), virtual_scheduled_time(w.virtual_scheduled_time), running_version(w.running_version), hardfork_version_vote(w.hardfork_version_vote), hardfork_time_vote(w.hardfork_time_vote)
        {}
        
        siming_id_type  id;
        account_name_type       owner;
        time_point_sec          created;
        string                  url;
        uint32_t                total_missed = 0;
        uint64_t                last_aslot = 0;
        uint64_t                last_confirmed_block_num = 0;
        public_key_type         signing_key;
        api_chain_properties    props;
        share_type              adores;
        fc::uint128_t           virtual_last_update;
        fc::uint128_t           virtual_position;
        fc::uint128_t           virtual_scheduled_time = fc::uint128_t::max_value();
        version                 running_version;
        hardfork_version        hardfork_version_vote;
        time_point_sec          hardfork_time_vote = TAIYI_GENESIS_TIME;
    };

    struct api_siming_schedule_object
    {
        api_siming_schedule_object() {}
        api_siming_schedule_object(const database_api::api_siming_schedule_object& w) : id(w.id), current_virtual_time(w.current_virtual_time), next_shuffle_block_num(w.next_shuffle_block_num), num_scheduled_simings(w.num_scheduled_simings), elected_weight(w.elected_weight), timeshare_weight(w.timeshare_weight), miner_weight(w.miner_weight), siming_pay_normalization_factor(w.siming_pay_normalization_factor), median_props(w.median_props), majority_version(w.majority_version), max_adored_simings(w.max_adored_simings), hardfork_required_simings(w.hardfork_required_simings)
        {
            current_shuffled_simings.insert(current_shuffled_simings.begin(), w.current_shuffled_simings.begin(), w.current_shuffled_simings.end());
        }
        
        siming_schedule_id_type      id;
        fc::uint128_t                 current_virtual_time;
        uint32_t                      next_shuffle_block_num = 1;
        vector< account_name_type >   current_shuffled_simings;
        uint8_t                       num_scheduled_simings = 1;
        uint8_t                       elected_weight = 1;
        uint8_t                       timeshare_weight = 5;
        uint8_t                       miner_weight = 1;
        uint32_t                      siming_pay_normalization_factor = TAIYI_MAX_SIMINGS;
        api_chain_properties          median_props;
        version                       majority_version;
        uint8_t                       max_adored_simings           = TAIYI_MAX_SIMINGS;
        uint8_t                       hardfork_required_simings   = TAIYI_HARDFORK_REQUIRED_SIMINGS;        
    };

    struct api_reward_fund_object
    {
        api_reward_fund_object() {}
        api_reward_fund_object(const database_api::api_reward_fund_object& r) : id(r.id), name(r.name), reward_balance(legacy_asset::from_asset(r.reward_balance)), reward_qi_balance(legacy_asset::from_asset(r.reward_qi_balance)), last_update(r.last_update), percent_content_rewards(r.percent_content_rewards)
        {}
        
        reward_fund_id_type     id;
        reward_fund_name_type   name;
        legacy_asset            reward_balance;
        legacy_asset            reward_qi_balance;
        time_point_sec          last_update;
        uint16_t                percent_content_rewards = 0;
    };
    
    struct api_qi_delegation_object
    {
        api_qi_delegation_object() {}
        api_qi_delegation_object(const database_api::api_qi_delegation_object& v) : id(v.id), delegator(v.delegator), delegatee(v.delegatee), qi(legacy_asset::from_asset(v.qi)), min_delegation_time(v.min_delegation_time)
        {}
        
        qi_delegation_id_type id;
        account_name_type delegator;
        account_name_type delegatee;
        legacy_asset      qi;
        time_point_sec    min_delegation_time;
    };
    
    struct api_qi_delegation_expiration_object
    {
        api_qi_delegation_expiration_object() {}
        api_qi_delegation_expiration_object(const database_api::api_qi_delegation_expiration_object& v) : id(v.id), delegator(v.delegator), qi(legacy_asset::from_asset(v.qi)), expiration(v.expiration)
        {}
        
        qi_delegation_expiration_id_type id;
        account_name_type delegator;
        legacy_asset      qi;
        time_point_sec    expiration;
    };
        
    struct state
    {
        string                                              current_route;
        
        extended_dynamic_global_properties                  props;
        
        map< string, extended_account >                     accounts;
        
        map< string, api_siming_object >                    simings;
        api_siming_schedule_object                          siming_schedule;
        string                                              error;
    };

    struct scheduled_hardfork
    {
        hardfork_version     hf_version;
        fc::time_point_sec   live_time;
    };

    enum withdraw_route_type
    {
        incoming,
        outgoing,
        all
    };

    typedef vector< variant > get_version_args;
    struct get_version_return
    {
        get_version_return() {}
        get_version_return(std::string bc_v, std::string s_v, std::string fc_v) :blockchain_version(bc_v), taiyi_revision(s_v), fc_revision(fc_v) {}
        
        std::string blockchain_version;
        std::string taiyi_revision;
        std::string fc_revision;
    };

    typedef map< uint32_t, api_operation_object > get_account_history_return_type;
    typedef get_account_history_return_type get_nfa_history_return_type;
    typedef get_account_history_return_type get_actor_history_return_type;

    typedef vector< variant > broadcast_transaction_synchronous_args;
    struct broadcast_transaction_synchronous_return
    {
        broadcast_transaction_synchronous_return() {}
        broadcast_transaction_synchronous_return(transaction_id_type txid, int32_t bn, int32_t tn, bool ex) : id(txid), block_num(bn), trx_num(tn), expired(ex) {}
        
        transaction_id_type   id;
        int32_t               block_num = 0;
        int32_t               trx_num   = 0;
        bool                  expired   = false;
    };
    
    struct api_resource_assets
    {
        api_resource_assets(const database_api::resource_assets& r) : gold(legacy_asset::from_asset(r.gold)), food(legacy_asset::from_asset(r.food)), wood(legacy_asset::from_asset(r.wood)), fabric(legacy_asset::from_asset(r.fabric)), herb(legacy_asset::from_asset(r.herb))
        {}

        api_resource_assets() {}

        legacy_asset      gold;
        legacy_asset      food;
        legacy_asset      wood;
        legacy_asset      fabric;
        legacy_asset      herb;
    };
    
    struct api_nfa_object
    {
        api_nfa_object(const database_api::api_nfa_object& o) : id(o.id), creator_account(o.creator_account), owner_account(o.owner_account), active_account(o.active_account), symbol(o.symbol), parent(o.parent), children(o.children), main_contract(o.main_contract), contract_data(o.contract_data), qi(legacy_asset::from_asset(o.qi)), debt_value(o.debt_value), debt_contract(o.debt_contract), cultivation_value(o.cultivation_value), mirage_contract(o.mirage_contract), created_time(o.created_time), next_tick_time(o.next_tick_time), gold(legacy_asset::from_asset(o.gold)), food(legacy_asset::from_asset(o.food)), wood(legacy_asset::from_asset(o.wood)), fabric(legacy_asset::from_asset(o.fabric)), herb(legacy_asset::from_asset(o.herb)), material_gold(legacy_asset::from_asset(o.material_gold)), material_food(legacy_asset::from_asset(o.material_food)), material_wood(legacy_asset::from_asset(o.material_wood)), material_fabric(legacy_asset::from_asset(o.material_fabric)), material_herb(legacy_asset::from_asset(o.material_herb)), five_phase(o.five_phase)
        {}
        
        api_nfa_object(){}
        
        nfa_id_type         id;
        
        account_name_type   creator_account;
        account_name_type   owner_account;
        account_name_type   active_account;

        string              symbol;
        
        nfa_id_type         parent;
        vector<nfa_id_type> children;
        
        string              main_contract;
        lua_map             contract_data;
        
        legacy_asset        qi;
        int64_t             debt_value; /// 欠费的真气值
        string              debt_contract; /// 欠费的债主合约

        uint64_t            cultivation_value; ///参与修真的真气值，> 0表示正在参与修真

        string              mirage_contract; //幻觉状态下所处的合约剧情节点，为空表示正常状态

        time_point_sec      created_time;
        time_point_sec      next_tick_time;

        legacy_asset        gold;
        legacy_asset        food;
        legacy_asset        wood;
        legacy_asset        fabric;
        legacy_asset        herb;

        legacy_asset        material_gold;
        legacy_asset        material_food;
        legacy_asset        material_wood;
        legacy_asset        material_fabric;
        legacy_asset        material_herb;
        
        int                 five_phase;
    };

    struct api_contract_action_info
    {
        bool exist = false;
        bool consequence = false;
    };

    struct api_simple_actor_object
    {
        api_simple_actor_object(const account_name_type& o, const string& n) : owner(o), name(n) {}
        api_simple_actor_object() {}
        
        account_name_type   owner;
        string              name;
    };
    
    struct api_eval_action_return
    {
        vector<lua_types>   eval_result;
        vector<string>      narrate_logs;
        string              err;
    };
    
    struct api_actor_relation_data
    {
        api_actor_relation_data(const actor_relation_id_type& i, const account_name_type& o, const string& n, const int32_t& f, const int& l, const time_point_sec& u) : id(i), owner(o), name(n), favor(f), favor_level(l), last_update(u) {}
        api_actor_relation_data() {}
        
        actor_relation_id_type  id;
        
        account_name_type       owner;
        string                  name;
            
        int32_t                 favor = 0;
        int                     favor_level = 0;
        
        time_point_sec          last_update;
    };

    struct api_actor_friend_data
    {
        api_actor_friend_data(const account_name_type& o, const string& n, const int& ct, const bool& c, const bool& m) : owner(o), name(n), connection_type(ct), is_couple(c), is_mentor(m) {}
        api_actor_friend_data() {}
        
        account_name_type   owner;
        string              name;
        
        int connection_type = 0; //1=直接血亲或配偶, 2=非血亲家人或师父或徒弟, other=其他关系
        bool is_couple      = false; //夫妻关系
        bool is_mentor      = false; //师徒关系
    };

    //角色生理心理需求数据
    struct api_actor_needs_data
    {
        api_actor_needs_data(const account_name_type& mo, const string& mn, const uint32_t& me, const account_name_type& bo, const string& bn, const uint32_t& be) : mating_target_owner(mo), mating_target_name(mn), mating_end_block_num(me), bullying_target_owner(bo), bullying_target_name(bn), bullying_end_block_num(be) {}
        api_actor_needs_data() {}
        
        account_name_type   mating_target_owner;       //春宵需求对象
        string              mating_target_name;
        uint32_t            mating_end_block_num = 0;  //春宵需求截止时间
        
        account_name_type   bullying_target_owner;     //欺辱需求对象
        string              bullying_target_name;
        uint32_t            bullying_end_block_num = 0;//欺辱需求截止时间
    };

    struct api_people_stat_data
    {
        uint live_num = 0;
        uint dead_num = 0;
    };

#define DEFINE_API_ARGS( api_name, arg_type, return_type )  \
    typedef arg_type api_name ## _args;                     \
    typedef return_type api_name ## _return;

/*               API,                               args,              return */
DEFINE_API_ARGS( get_state,                         vector< variant >, state )
DEFINE_API_ARGS( get_active_simings,                vector< variant >, vector< account_name_type > )
DEFINE_API_ARGS( get_block_header,                  vector< variant >, optional< block_header > )
DEFINE_API_ARGS( get_block,                         vector< variant >, optional< legacy_signed_block > )
DEFINE_API_ARGS( get_ops_in_block,                  vector< variant >, vector< api_operation_object > )
DEFINE_API_ARGS( get_config,                        vector< variant >, fc::variant_object )
DEFINE_API_ARGS( get_dynamic_global_properties,     vector< variant >, extended_dynamic_global_properties )
DEFINE_API_ARGS( get_chain_properties,              vector< variant >, api_chain_properties )
DEFINE_API_ARGS( get_siming_schedule,               vector< variant >, api_siming_schedule_object )
DEFINE_API_ARGS( get_hardfork_version,              vector< variant >, hardfork_version )
DEFINE_API_ARGS( get_next_scheduled_hardfork,       vector< variant >, scheduled_hardfork )
DEFINE_API_ARGS( get_reward_fund,                   vector< variant >, api_reward_fund_object )
DEFINE_API_ARGS( get_key_references,                vector< variant >, vector< vector< account_name_type > > )
DEFINE_API_ARGS( get_accounts,                      vector< variant >, vector< extended_account > )
DEFINE_API_ARGS( lookup_account_names,              vector< variant >, vector< optional< api_account_object > > )
DEFINE_API_ARGS( lookup_accounts,                   vector< variant >, set< string > )
DEFINE_API_ARGS( get_account_count,                 vector< variant >, uint64_t )
DEFINE_API_ARGS( get_owner_history,                 vector< variant >, vector<database_api::api_owner_authority_history_object>)
DEFINE_API_ARGS( get_recovery_request,              vector< variant >, optional<database_api::api_account_recovery_request_object>)
DEFINE_API_ARGS( get_withdraw_routes,               vector< variant >, vector< database_api::api_withdraw_qi_route_object > )
DEFINE_API_ARGS( get_qi_delegations,                vector< variant >, vector< api_qi_delegation_object > )
DEFINE_API_ARGS( get_expiring_qi_delegations,       vector< variant >, vector< api_qi_delegation_expiration_object > )
DEFINE_API_ARGS( get_simings,                       vector< variant >, vector< optional< api_siming_object > > )
DEFINE_API_ARGS( get_siming_by_account,             vector< variant >, optional< api_siming_object > )
DEFINE_API_ARGS( get_simings_by_adore,              vector< variant >, vector< api_siming_object > )
DEFINE_API_ARGS( lookup_siming_accounts,            vector< variant >, vector< account_name_type > )
DEFINE_API_ARGS( get_siming_count,                  vector< variant >, uint64_t )
DEFINE_API_ARGS( get_transaction_hex,               vector< variant >, string )
DEFINE_API_ARGS( get_transaction,                   vector< variant >, legacy_signed_transaction )
DEFINE_API_ARGS( get_transaction_results,           vector< variant >, vector<operation_result> )
DEFINE_API_ARGS( get_required_signatures,           vector< variant >, set< public_key_type > )
DEFINE_API_ARGS( get_potential_signatures,          vector< variant >, set< public_key_type > )
DEFINE_API_ARGS( verify_authority,                  vector< variant >, bool )
DEFINE_API_ARGS( verify_account_authority,          vector< variant >, bool )
DEFINE_API_ARGS( get_account_history,               vector< variant >, get_account_history_return_type )
DEFINE_API_ARGS( broadcast_transaction,             vector< variant >, json_rpc::void_type )
DEFINE_API_ARGS( broadcast_block,                   vector< variant >, json_rpc::void_type )
DEFINE_API_ARGS( get_account_resources,             vector< variant >, vector< api_resource_assets > )
    
DEFINE_API_ARGS( find_nfa,                          vector< variant >, optional< api_nfa_object > )
DEFINE_API_ARGS( find_nfas,                         vector< variant >, vector< api_nfa_object > )
DEFINE_API_ARGS( list_nfas,                         vector< variant >, vector< api_nfa_object > )
DEFINE_API_ARGS( get_nfa_history,                   vector< variant >, get_nfa_history_return_type )
DEFINE_API_ARGS( get_nfa_action_info,               vector< variant >, api_contract_action_info )
DEFINE_API_ARGS( eval_nfa_action,                   vector< variant >, api_eval_action_return )
DEFINE_API_ARGS( eval_nfa_action_with_string_args,  vector< variant >, api_eval_action_return )

DEFINE_API_ARGS( find_actor,                        vector< variant >, optional< database_api::api_actor_object > )
DEFINE_API_ARGS( find_actors,                       vector< variant >, vector< database_api::api_actor_object > )
DEFINE_API_ARGS( list_actors,                       vector< variant >, vector< database_api::api_actor_object > )
DEFINE_API_ARGS( get_actor_history,                 vector< variant >, get_actor_history_return_type )
DEFINE_API_ARGS( list_actors_below_health,          vector< variant >, vector< database_api::api_actor_object > )
DEFINE_API_ARGS( find_actor_talent_rules,           vector< variant >, vector< database_api::api_actor_talent_rule_object > )
DEFINE_API_ARGS( list_actors_on_zone,               vector< variant >, vector< database_api::api_actor_object > )

DEFINE_API_ARGS( get_tiandao_properties,            vector< variant >, database_api::api_tiandao_property_object )
DEFINE_API_ARGS( find_zones,                        vector< variant >, vector< database_api::api_zone_object > )
DEFINE_API_ARGS( find_zones_by_name,                vector< variant >, vector< database_api::api_zone_object > )
DEFINE_API_ARGS( list_zones,                        vector< variant >, vector< database_api::api_zone_object > )
DEFINE_API_ARGS( list_zones_by_type,                vector< variant >, vector< database_api::api_zone_object > )
DEFINE_API_ARGS( list_to_zones_by_from,             vector< variant >, vector< database_api::api_zone_object > )
DEFINE_API_ARGS( list_from_zones_by_to,             vector< variant >, vector< database_api::api_zone_object > )
DEFINE_API_ARGS( find_way_to_zone,                  vector< variant >, database_api::find_way_to_zone_return )

DEFINE_API_ARGS( list_relations_from_actor,         vector< variant >, vector< api_actor_relation_data > )
DEFINE_API_ARGS( list_relations_to_actor,           vector< variant >, vector< api_actor_relation_data > )
DEFINE_API_ARGS( get_relation_from_to_actor,        vector< variant >, optional< api_actor_relation_data > )
DEFINE_API_ARGS( get_actor_connections,             vector< variant >, database_api::get_actor_connections_return )
DEFINE_API_ARGS( list_actor_groups,                 vector< variant >, database_api::list_actor_groups_return )
DEFINE_API_ARGS( find_actor_group,                  vector< variant >, database_api::find_actor_group_return )
DEFINE_API_ARGS( list_actor_friends,                vector< variant >, vector< api_actor_friend_data > )
DEFINE_API_ARGS( get_actor_needs,                   vector< variant >, api_actor_needs_data )
DEFINE_API_ARGS( list_actor_mating_targets_by_zone, vector< variant >, vector< api_simple_actor_object > )
DEFINE_API_ARGS( stat_people_by_zone,               vector< variant >, api_people_stat_data )
DEFINE_API_ARGS( stat_people_by_base,               vector< variant >, api_people_stat_data )

DEFINE_API_ARGS( get_contract_source_code,          vector< variant >, string )

#undef DEFINE_API_ARGS

    class baiyujing_api
    {
    public:
        baiyujing_api();
        ~baiyujing_api();
        
        DECLARE_API(
            (get_version)
            (get_state)
            (get_active_simings)
            (get_block_header)
            (get_block)
            (get_ops_in_block)
            (get_config)
            (get_dynamic_global_properties)
            (get_chain_properties)
            (get_siming_schedule)
            (get_hardfork_version)
            (get_next_scheduled_hardfork)
            (get_reward_fund)
            (get_key_references)
            (get_accounts)
            (lookup_account_names)
            (lookup_accounts)
            (get_account_count)
            (get_owner_history)
            (get_recovery_request)
            (get_withdraw_routes)
            (get_qi_delegations)
            (get_expiring_qi_delegations)
            (get_simings)
            (get_siming_by_account)
            (get_simings_by_adore)
            (lookup_siming_accounts)
            (get_siming_count)
            (get_transaction_hex)
            (get_transaction)
            (get_transaction_results)
            (get_required_signatures)
            (get_potential_signatures)
            (verify_authority)
            (verify_account_authority)
            (get_account_history)
            (broadcast_transaction)
            (broadcast_transaction_synchronous)
            (broadcast_block)
            (get_account_resources)
                    
            (find_nfa)
            (find_nfas)
            (list_nfas)
            (get_nfa_history)
            (eval_nfa_action)
            (eval_nfa_action_with_string_args)
            (get_nfa_action_info)
                    
            (find_actor)
            (find_actors)
            (list_actors)
            (get_actor_history)
            (list_actors_below_health)
            (find_actor_talent_rules)
            (list_actors_on_zone)

            (get_tiandao_properties)
            (find_zones)
            (find_zones_by_name)
            (list_zones)
            (list_zones_by_type)
            (list_to_zones_by_from)
            (list_from_zones_by_to)
            (find_way_to_zone)

            (list_relations_from_actor)
            (list_relations_to_actor)
            (get_relation_from_to_actor)
            (get_actor_connections)
            (list_actor_groups)
            (find_actor_group)
            (list_actor_friends)
            (get_actor_needs)
            (list_actor_mating_targets_by_zone)
            (stat_people_by_zone)
            (stat_people_by_base)

            (get_contract_source_code)
        )
        
    private:
        friend class baiyujing_api_plugin;
        void api_startup();
        
        std::unique_ptr< detail::baiyujing_api_impl > my;
    };

} } } // taiyi::plugins::baiyujing_api

FC_REFLECT( taiyi::plugins::baiyujing_api::state, (current_route)(props)(accounts)(simings)(siming_schedule)(error) )

FC_REFLECT( taiyi::plugins::baiyujing_api::api_operation_object, (trx_id)(block)(trx_in_block)(op_in_trx)(virtual_op)(timestamp)(op) )

FC_REFLECT( taiyi::plugins::baiyujing_api::api_account_object, (id)(name)(owner)(active)(posting)(memo_key)(json_metadata)(proxy)(last_owner_update)(last_account_update)(created)(recovery_account)(last_account_recovery)(can_adore)(balance)(reward_yang_balance)(reward_qi_balance)(reward_feigang_balance)(qi)(delegated_qi)(received_qi)(qi_withdraw_rate)(next_qi_withdrawal_time)(withdrawn)(to_withdraw)(withdraw_routes)(proxied_vsf_adores)(simings_adored_for)(gold)(food)(wood)(fabric)(herb) )

FC_REFLECT_DERIVED( taiyi::plugins::baiyujing_api::extended_account, (taiyi::plugins::baiyujing_api::api_account_object),(qi_balance)(transfer_history)(other_history)(siming_adores) )

FC_REFLECT( taiyi::plugins::baiyujing_api::extended_dynamic_global_properties, (head_block_number)(head_block_id)(time)(current_siming)(current_supply)(total_qi)(pending_rewarded_qi)(pending_rewarded_feigang)(pending_cultivation_qi)(total_gold)(total_food)(total_wood)(total_fabric)(total_herb)(maximum_block_size)(current_aslot)(recent_slots_filled)(participation_count)(last_irreversible_block_num)(delegation_return_period)(content_reward_yang_percent)(content_reward_qi_fund_percent)(last_siming_production_reward) )

FC_REFLECT( taiyi::plugins::baiyujing_api::api_siming_object, (id)(owner)(created)(url)(adores)(virtual_last_update)(virtual_position)(virtual_scheduled_time)(total_missed)(last_aslot)(last_confirmed_block_num)(signing_key)(props)(running_version)(hardfork_version_vote)(hardfork_time_vote) )

FC_REFLECT( taiyi::plugins::baiyujing_api::api_siming_schedule_object, (id)(current_virtual_time)(next_shuffle_block_num)(current_shuffled_simings)(num_scheduled_simings)(elected_weight)(timeshare_weight)(miner_weight)(siming_pay_normalization_factor)(median_props)(majority_version)(max_adored_simings)(hardfork_required_simings) )

FC_REFLECT( taiyi::plugins::baiyujing_api::api_reward_fund_object, (id)(name)(reward_balance)(reward_qi_balance)(last_update)(percent_content_rewards) )

FC_REFLECT( taiyi::plugins::baiyujing_api::api_qi_delegation_object, (id)(delegator)(delegatee)(qi)(min_delegation_time) )

FC_REFLECT( taiyi::plugins::baiyujing_api::api_qi_delegation_expiration_object, (id)(delegator)(qi)(expiration) )

FC_REFLECT( taiyi::plugins::baiyujing_api::scheduled_hardfork, (hf_version)(live_time) )

FC_REFLECT_ENUM( taiyi::plugins::baiyujing_api::withdraw_route_type, (incoming)(outgoing)(all) )

FC_REFLECT( taiyi::plugins::baiyujing_api::get_version_return, (blockchain_version)(taiyi_revision)(fc_revision) )

FC_REFLECT( taiyi::plugins::baiyujing_api::broadcast_transaction_synchronous_return, (id)(block_num)(trx_num)(expired) )

FC_REFLECT( taiyi::plugins::baiyujing_api::api_resource_assets, (gold)(food)(wood)(fabric)(herb) )

FC_REFLECT(taiyi::plugins::baiyujing_api::api_nfa_object, (id)(creator_account)(owner_account)(active_account)(symbol)(parent)(children)(main_contract)(contract_data)(qi)(debt_value)(debt_contract)(cultivation_value)(mirage_contract)(created_time)(next_tick_time)(gold)(food)(wood)(fabric)(herb)(material_gold)(material_food)(material_wood)(material_fabric)(material_herb)(five_phase))

FC_REFLECT( taiyi::plugins::baiyujing_api::api_contract_action_info, (exist)(consequence) )

FC_REFLECT( taiyi::plugins::baiyujing_api::api_simple_actor_object, (owner)(name) )

FC_REFLECT( taiyi::plugins::baiyujing_api::api_eval_action_return, (eval_result)(narrate_logs)(err) )

FC_REFLECT( taiyi::plugins::baiyujing_api::api_actor_relation_data, (id)(owner)(name)(favor)(favor_level)(last_update) )

FC_REFLECT( taiyi::plugins::baiyujing_api::api_actor_friend_data, (owner)(name)(connection_type)(is_couple)(is_mentor) )
FC_REFLECT( taiyi::plugins::baiyujing_api::api_actor_needs_data, (mating_target_owner)(mating_target_name)(mating_end_block_num)(bullying_target_owner)(bullying_target_name)(bullying_end_block_num) )
FC_REFLECT( taiyi::plugins::baiyujing_api::api_people_stat_data, (live_num)(dead_num) )

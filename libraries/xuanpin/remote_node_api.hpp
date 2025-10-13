#pragma once
#include <plugins/baiyujing_api/baiyujing_api.hpp>

namespace taiyi { namespace xuanpin {

    using std::vector;
    using fc::variant;
    using fc::optional;
    
    using namespace chain;
    using namespace plugins;

    /**
     * This is a dummy API so that the xuanpin can create properly formatted API calls
     */
    struct remote_node_api
    {
        baiyujing_api::get_version_return get_version() { FC_ASSERT( false ); }
        baiyujing_api::state get_state( string ) { FC_ASSERT( false ); }
        vector< account_name_type > get_active_simings() { FC_ASSERT( false ); }
        optional< block_header > get_block_header( uint32_t ) { FC_ASSERT( false ); }
        optional< baiyujing_api::legacy_signed_block > get_block( uint32_t ) { FC_ASSERT( false ); }
        vector< baiyujing_api::api_operation_object > get_ops_in_block( uint32_t, bool only_virtual = true ) { FC_ASSERT( false ); }
        fc::variant_object get_config() { FC_ASSERT( false ); }
        baiyujing_api::extended_dynamic_global_properties get_dynamic_global_properties() { FC_ASSERT( false ); }
        baiyujing_api::api_chain_properties get_chain_properties() { FC_ASSERT( false ); }
        baiyujing_api::api_siming_schedule_object get_siming_schedule() { FC_ASSERT( false ); }
        hardfork_version get_hardfork_version() { FC_ASSERT( false ); }
        baiyujing_api::scheduled_hardfork get_next_scheduled_hardfork() { FC_ASSERT( false ); }
        baiyujing_api::api_reward_fund_object get_reward_fund( string ) { FC_ASSERT( false ); }
        vector< vector< account_name_type > > get_key_references( vector< public_key_type > ) { FC_ASSERT( false ); }
        vector< baiyujing_api::extended_account > get_accounts( vector< account_name_type > ) { FC_ASSERT( false ); }
        vector< optional< baiyujing_api::api_account_object > > lookup_account_names( vector< account_name_type > ) { FC_ASSERT( false ); }
        vector< account_name_type > lookup_accounts( account_name_type, uint32_t ) { FC_ASSERT( false ); }
        uint64_t get_account_count() { FC_ASSERT( false ); }
        vector< database_api::api_owner_authority_history_object > get_owner_history( account_name_type ) { FC_ASSERT( false ); }
        optional< database_api::api_account_recovery_request_object > get_recovery_request( account_name_type ) { FC_ASSERT( false ); }
        vector< database_api::api_withdraw_qi_route_object > get_withdraw_routes( account_name_type, baiyujing_api::withdraw_route_type ) { FC_ASSERT( false ); }
        vector< baiyujing_api::api_qi_delegation_object > get_qi_delegations( account_name_type, account_name_type, uint32_t ) { FC_ASSERT( false ); }
        vector< baiyujing_api::api_qi_delegation_expiration_object > get_expiring_qi_delegations( account_name_type, time_point_sec, uint32_t) { FC_ASSERT( false ); }
        vector< optional< baiyujing_api::api_siming_object > > get_simings( vector< siming_id_type > ) { FC_ASSERT( false ); }
        optional< baiyujing_api::api_siming_object > get_siming_by_account( account_name_type ) { FC_ASSERT( false ); }
        vector< baiyujing_api::api_siming_object > get_simings_by_adore( account_name_type, uint32_t ) { FC_ASSERT( false ); }
        vector< account_name_type > lookup_siming_accounts( string, uint32_t ) { FC_ASSERT( false ); }
        uint64_t get_siming_count() { FC_ASSERT( false ); }
        string get_transaction_hex( baiyujing_api::legacy_signed_transaction ) { FC_ASSERT( false ); }
        baiyujing_api::legacy_signed_transaction get_transaction( transaction_id_type ) const { FC_ASSERT( false ); }
        vector<taiyi::chain::operation_result> get_transaction_results( transaction_id_type ) const  { FC_ASSERT( false ); }
        set< public_key_type > get_required_signatures( baiyujing_api::legacy_signed_transaction, flat_set< public_key_type > ) { FC_ASSERT( false ); }
        set< public_key_type > get_potential_signatures( baiyujing_api::legacy_signed_transaction ) { FC_ASSERT( false ); }
        bool verify_authority( baiyujing_api::legacy_signed_transaction ) { FC_ASSERT( false ); }
        bool verify_account_authority( string, flat_set< public_key_type > ) { FC_ASSERT( false ); }
        map< uint32_t, baiyujing_api::api_operation_object > get_account_history( account_name_type, uint64_t, uint32_t ) { FC_ASSERT( false ); }
        void broadcast_transaction( baiyujing_api::legacy_signed_transaction ) { FC_ASSERT( false ); }
        baiyujing_api::broadcast_transaction_synchronous_return broadcast_transaction_synchronous( baiyujing_api::legacy_signed_transaction ) { FC_ASSERT( false ); }
        void broadcast_block( signed_block ) { FC_ASSERT( false ); }
        vector< baiyujing_api::api_resource_assets > get_account_resources ( vector< account_name_type > ) { FC_ASSERT( false ); }
        optional< database_api::api_nfa_symbol_object > find_nfa_symbol( const string& ) { FC_ASSERT( false ); }
        optional< database_api::api_nfa_symbol_object > find_nfa_symbol_by_contract( const string& ) { FC_ASSERT( false ); }
        vector< baiyujing_api::api_nfa_object > find_nfas( vector< int64_t > ) { FC_ASSERT( false ); }
        optional< baiyujing_api::api_nfa_object > find_nfa( const int64_t& ) { FC_ASSERT( false ); }
        vector< baiyujing_api::api_nfa_object > list_nfas(const account_name_type&, uint32_t) const { FC_ASSERT( false ); }
        map< uint32_t, baiyujing_api::api_operation_object > get_nfa_history( int64_t, uint64_t, uint32_t ) { FC_ASSERT( false ); }
        baiyujing_api::api_contract_action_info get_nfa_action_info(int64_t, const string&) const { FC_ASSERT( false ); }
        baiyujing_api::api_eval_action_return eval_nfa_action(int64_t, const string&, const vector<lua_types>&) const { FC_ASSERT( false ); }
        optional< database_api::api_actor_object > find_actor( const string& ) { FC_ASSERT( false ); }
        vector< database_api::api_actor_object > find_actors( vector< int64_t > ) { FC_ASSERT( false ); }
        vector< database_api::api_actor_object > list_actors(const account_name_type&, uint32_t) const { FC_ASSERT( false ); }
        map< uint32_t, baiyujing_api::api_operation_object > get_actor_history(const string&, uint64_t, uint32_t ) { FC_ASSERT( false ); }
        vector< database_api::api_actor_object > list_actors_below_health(const int16_t&, uint32_t) const { FC_ASSERT( false ); }
        vector< database_api::api_actor_talent_rule_object > find_actor_talent_rules( vector< int64_t > ) { FC_ASSERT( false ); }
        vector< database_api::api_actor_object > list_actors_on_zone(const string&, uint32_t) const { FC_ASSERT( false ); }
        vector< database_api::api_zone_object > find_zones( vector< int64_t > ) { FC_ASSERT( false ); }
        vector< database_api::api_zone_object > list_zones(const account_name_type&, uint32_t) const { FC_ASSERT( false ); }
        vector< database_api::api_zone_object > list_zones_by_type(E_ZONE_TYPE, uint32_t) const { FC_ASSERT( false ); }
        vector< database_api::api_zone_object > list_to_zones_by_from(const string&, uint32_t) const { FC_ASSERT( false ); }
        vector< database_api::api_zone_object > list_from_zones_by_to(const string&, uint32_t) const { FC_ASSERT( false ); }
        database_api::find_way_to_zone_return find_way_to_zone(const string&, const string&) const { FC_ASSERT( false ); }
        vector< database_api::api_zone_object > find_zones_by_name( vector< string > ) { FC_ASSERT( false ); }
        database_api::api_tiandao_property_object get_tiandao_properties() const { FC_ASSERT( false ); }
        string get_contract_source_code(const string&) const { FC_ASSERT( false ); }

        vector< baiyujing_api::api_actor_relation_data > list_relations_from_actor(account_name_type, string) { FC_ASSERT( false ); }
        vector< baiyujing_api::api_actor_relation_data > list_relations_to_actor(account_name_type, string) { FC_ASSERT( false ); }
        optional< baiyujing_api::api_actor_relation_data > get_relation_from_to_actor(account_name_type, string, account_name_type, string) { FC_ASSERT( false ); }
        database_api::get_actor_connections_return get_actor_connections(const account_name_type&, const string&, const E_ACTOR_RELATION_TYPE&) const { FC_ASSERT( false ); }
        database_api::list_actor_groups_return list_actor_groups(const database_api::find_actor_args&, uint32_t) const { FC_ASSERT( false ); }
        database_api::find_actor_group_return find_actor_group(const database_api::find_actor_args&) const { FC_ASSERT( false ); }
        baiyujing_api::list_actor_friends_return list_actor_friends(const string&, int, bool) const { FC_ASSERT( false ); }
        baiyujing_api::get_actor_needs_return get_actor_needs(const string&) const { FC_ASSERT( false ); }
        baiyujing_api::list_actor_mating_targets_by_zone_return list_actor_mating_targets_by_zone(const string&, const string&) const { FC_ASSERT( false ); }
        baiyujing_api::api_people_stat_data stat_people_by_zone(const string&) const { FC_ASSERT( false ); }
        baiyujing_api::api_people_stat_data stat_people_by_base(const string&) const { FC_ASSERT( false ); }
    };

} } //taiyi::xuanpin

FC_API( taiyi::xuanpin::remote_node_api,
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
    (find_nfa_symbol)
    (find_nfa_symbol_by_contract)
    (find_nfa)
    (find_nfas)
    (list_nfas)
    (get_nfa_history)
    (get_nfa_action_info)
    (eval_nfa_action)
    (find_actor)
    (find_actors)
    (list_actors)
    (get_actor_history)
    (list_actors_below_health)
    (list_actors_on_zone)
    (find_actor_talent_rules)
    (find_zones)
    (list_zones)
    (list_zones_by_type)
    (list_to_zones_by_from)
    (list_from_zones_by_to)
    (find_way_to_zone)
    (find_zones_by_name)
    (get_tiandao_properties)
    (get_contract_source_code)
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
)

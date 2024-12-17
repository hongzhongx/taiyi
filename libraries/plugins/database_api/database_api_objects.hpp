#pragma once
#include <chain/block_summary_object.hpp>
#include <chain/database.hpp>
#include <chain/global_property_object.hpp>
#include <chain/taiyi_objects.hpp>
#include <chain/transaction_object.hpp>
#include <chain/siming_objects.hpp>
#include <chain/tiandao_property_object.hpp>
#include <chain/account_object.hpp>
#include <chain/nfa_objects.hpp>
#include <chain/actor_objects.hpp>
#include <chain/zone_objects.hpp>
#include <chain/contract_objects.hpp>

namespace taiyi { namespace plugins { namespace database_api {

    using namespace taiyi::chain;
    
    typedef change_recovery_account_request_object  api_change_recovery_account_request_object;
    typedef block_summary_object                    api_block_summary_object;
    typedef dynamic_global_property_object          api_dynamic_global_property_object;
    typedef withdraw_qi_route_object                api_withdraw_qi_route_object;
    typedef decline_adoring_rights_request_object   api_decline_adoring_rights_request_object;
    typedef siming_adore_object                     api_siming_adore_object;
    typedef qi_delegation_object                    api_qi_delegation_object;
    typedef qi_delegation_expiration_object         api_qi_delegation_expiration_object;
    typedef reward_fund_object                      api_reward_fund_object;
    typedef actor_talent_rule_object                api_actor_talent_rule_object;
    typedef tiandao_property_object                 api_tiandao_property_object;
    typedef zone_object                             api_zone_object;

    struct api_account_object
    {
        api_account_object( const account_object& a, const database& db ) : id(a.id), name(a.name), memo_key(a.memo_key), proxy(a.proxy), last_account_update(a.last_account_update), created(a.created), recovery_account(a.recovery_account), last_account_recovery(a.last_account_recovery), can_adore(a.can_adore), manabar(a.manabar), balance(a.balance), reward_yang_balance(a.reward_yang_balance), reward_qi_balance(a.reward_qi_balance), qi(a.qi), delegated_qi(a.delegated_qi), received_qi(a.received_qi), qi_withdraw_rate(a.qi_withdraw_rate), next_qi_withdrawal_time(a.next_qi_withdrawal_time), withdrawn(a.withdrawn), to_withdraw(a.to_withdraw), withdraw_routes(a.withdraw_routes), simings_adored_for(a.simings_adored_for)
        {
            size_t n = a.proxied_vsf_adores.size();
            proxied_vsf_adores.reserve( n );
            for( size_t i=0; i<n; i++ )
                proxied_vsf_adores.push_back( a.proxied_vsf_adores[i] );
            
            const auto& auth = db.get< account_authority_object, by_account >( name );
            owner = authority( auth.owner );
            active = authority( auth.active );
            posting = authority( auth.posting );
            last_owner_update = auth.last_owner_update;
#ifndef IS_LOW_MEM
            const auto* maybe_meta = db.find< account_metadata_object, by_account >( id );
            if( maybe_meta )
            {
                json_metadata = maybe_meta->json_metadata;
            }
#endif
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
        bool              mined = false;
        account_name_type recovery_account;
        time_point_sec    last_account_recovery;
        uint32_t          comment_count = 0;
        uint32_t          lifetime_adore_count = 0;
        uint32_t          post_count = 0;
        
        bool              can_adore = false;
        util::manabar     manabar;

        asset             balance;
        
        asset             reward_yang_balance;
        asset             reward_qi_balance;
        
        asset             qi;
        asset             delegated_qi;
        asset             received_qi;
        asset             qi_withdraw_rate;
        time_point_sec    next_qi_withdrawal_time;
        share_type        withdrawn;
        share_type        to_withdraw;
        uint16_t          withdraw_routes = 0;
        
        vector< share_type > proxied_vsf_adores;
        
        uint16_t          simings_adored_for = 0;
    };

    struct api_owner_authority_history_object
    {
        api_owner_authority_history_object( const owner_authority_history_object& o ) : id(o.id), account(o.account), previous_owner_authority(authority(o.previous_owner_authority)), last_valid_time(o.last_valid_time)
        {}
        
        api_owner_authority_history_object() {}
        
        owner_authority_history_id_type  id;
        
        account_name_type                account;
        authority                        previous_owner_authority;
        time_point_sec                   last_valid_time;
    };

    struct api_account_recovery_request_object
    {
        api_account_recovery_request_object( const account_recovery_request_object& o ) : id(o.id), account_to_recover(o.account_to_recover), new_owner_authority(authority(o.new_owner_authority)), expires(o.expires)
        {}
        
        api_account_recovery_request_object() {}
        
        account_recovery_request_id_type id;
        account_name_type                account_to_recover;
        authority                        new_owner_authority;
        time_point_sec                   expires;
    };

    struct api_siming_object
    {
        api_siming_object( const siming_object& w ) : id(w.id), owner(w.owner), created(w.created), url(w.url), total_missed(w.total_missed), last_aslot(w.last_aslot), last_confirmed_block_num(w.last_confirmed_block_num), signing_key(w.signing_key), props(w.props), adores(w.adores), virtual_last_update(w.virtual_last_update), virtual_position(w.virtual_position), virtual_scheduled_time(w.virtual_scheduled_time), running_version(w.running_version), hardfork_version_vote(w.hardfork_version_vote), hardfork_time_vote(w.hardfork_time_vote)
        {}
        
        api_siming_object() {}
        
        siming_id_type   id;
        account_name_type owner;
        time_point_sec    created;
        string            url;
        uint32_t          total_missed = 0;
        uint64_t          last_aslot = 0;
        uint64_t          last_confirmed_block_num = 0;
        public_key_type   signing_key;
        chain_properties  props;
        share_type        adores;
        fc::uint128       virtual_last_update;
        fc::uint128       virtual_position;
        fc::uint128       virtual_scheduled_time;
        version           running_version;
        hardfork_version  hardfork_version_vote;
        time_point_sec    hardfork_time_vote;
    };

    struct api_siming_schedule_object
    {
        api_siming_schedule_object() {}
        
        api_siming_schedule_object( const siming_schedule_object& wso) : id(wso.id), current_virtual_time(wso.current_virtual_time), next_shuffle_block_num(wso.next_shuffle_block_num), num_scheduled_simings(wso.num_scheduled_simings), elected_weight(wso.elected_weight), timeshare_weight(wso.timeshare_weight), miner_weight(wso.miner_weight), siming_pay_normalization_factor(wso.siming_pay_normalization_factor), median_props(wso.median_props), majority_version(wso.majority_version), max_adored_simings(wso.max_adored_simings), hardfork_required_simings(wso.hardfork_required_simings)
        {
            size_t n = wso.current_shuffled_simings.size();
            current_shuffled_simings.reserve( n );
            std::transform(wso.current_shuffled_simings.begin(), wso.current_shuffled_simings.end(), std::back_inserter(current_shuffled_simings), [](const account_name_type& s) -> std::string { return s; } );
            // ^ fixed_string std::string operator used here.
        }
        
        siming_schedule_id_type   id;
        
        fc::uint128                current_virtual_time;
        uint32_t                   next_shuffle_block_num;
        vector<string>             current_shuffled_simings;   // fc::array<account_name_type,...> -> vector<string>
        uint8_t                    num_scheduled_simings;
        uint8_t                    elected_weight;
        uint8_t                    timeshare_weight;
        uint8_t                    miner_weight;
        uint32_t                   siming_pay_normalization_factor;
        chain_properties           median_props;
        version                    majority_version;
        
        uint8_t                    max_adored_simings;
        uint8_t                    hardfork_required_simings;        
    };

    struct api_signed_block_object : public signed_block
    {
        api_signed_block_object( const signed_block& block ) : signed_block(block)
        {
            block_id = id();
            signing_key = signee();
            transaction_ids.reserve( transactions.size() );
            for( const signed_transaction& tx : transactions )
                transaction_ids.push_back( tx.id() );
        }
        api_signed_block_object() {}
        
        block_id_type                 block_id;
        public_key_type               signing_key;
        vector< transaction_id_type > transaction_ids;
    };

    struct api_hardfork_property_object
    {
        api_hardfork_property_object( const hardfork_property_object& h ) : id(h.id), last_hardfork(h.last_hardfork), current_hardfork_version(h.current_hardfork_version), next_hardfork(h.next_hardfork), next_hardfork_time(h.next_hardfork_time)
        {
            size_t n = h.processed_hardforks.size();
            processed_hardforks.reserve( n );
            
            for( size_t i = 0; i < n; i++ )
                processed_hardforks.push_back( h.processed_hardforks[i] );
        }
        
        api_hardfork_property_object() {}
        
        hardfork_property_id_type     id;
        vector< fc::time_point_sec >  processed_hardforks;
        uint32_t                      last_hardfork;
        protocol::hardfork_version    current_hardfork_version;
        protocol::hardfork_version    next_hardfork;
        fc::time_point_sec            next_hardfork_time;
    };
    
    struct api_nfa_object
    {
        api_nfa_object( const nfa_object& o, const database& db ) : id(o.id), parent(o.parent), contract_data(o.contract_data), qi(o.qi), manabar(o.manabar), created_time(o.created_time), next_tick_time(o.next_tick_time)
        {
            creator_account = db.get<account_object, by_id>(o.creator_account).name;
            owner_account = db.get<account_object, by_id>(o.owner_account).name;
            active_account = db.get<account_object, by_id>(o.active_account).name;
            
            symbol = db.get<nfa_symbol_object, by_id>(o.symbol_id).symbol;
            
            main_contract = db.get<contract_object, by_id>(o.main_contract).name;
            
            gold = db.get_nfa_balance(o, GOLD_SYMBOL);
            food = db.get_nfa_balance(o, FOOD_SYMBOL);
            wood = db.get_nfa_balance(o, WOOD_SYMBOL);
            fabric = db.get_nfa_balance(o, FABRIC_SYMBOL);
            herb = db.get_nfa_balance(o, HERB_SYMBOL);
            
            const auto& nfa_by_parent_idx = db.get_index< nfa_index >().indices().get< chain::by_parent >();
            auto itn = nfa_by_parent_idx.lower_bound( id );
            while(itn != nfa_by_parent_idx.end()) {
                if(itn->parent != id)
                    break;
                children.push_back(itn->id);
                ++itn;
            }
        }
        
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
        
        asset               qi;
        util::manabar       manabar;

        time_point_sec      created_time;
        time_point_sec      next_tick_time;

        asset               gold;
        asset               food;
        asset               wood;
        asset               fabric;
        asset               herb;
    };
        
    struct api_actor_object
    {
        api_actor_object( const actor_object& a, const database& db ) : id(a.id), nfa_id(a.nfa_id), name(a.name), age(a.age), health(a.health), health_max(a.health_max), init_attribute_amount_max(a.init_attribute_amount_max), born(a.born), gender(a.gender), sexuality(a.sexuality), fertility(a.fertility), born_time(a.born_time), born_vyears(a.born_vyears), born_vmonths(a.born_vmonths), born_vtimes(a.born_vtimes), five_phase(a.five_phase), standpoint(a.standpoint), loyalty(a.loyalty), last_update(a.last_update), next_tick_time(a.next_tick_time)
        {
            const auto& talents_obj = db.get< actor_talents_object, by_actor >( id );
            for(auto it = talents_obj.talents.begin(); it!=talents_obj.talents.end(); it++)
                talents.push_back(it->first);

            const auto* maybe_core_attrs = db.find< actor_core_attributes_object, by_actor >( id );
            if( maybe_core_attrs ) {
                strength = maybe_core_attrs->strength;
                physique = maybe_core_attrs->physique;
                agility = maybe_core_attrs->agility;
                vitality = maybe_core_attrs->vitality;
                comprehension = maybe_core_attrs->comprehension;
                willpower = maybe_core_attrs->willpower;
                charm = maybe_core_attrs->charm;
                mood = maybe_core_attrs->mood;

                strength_max = maybe_core_attrs->strength_max;
                physique_max = maybe_core_attrs->physique_max;
                agility_max = maybe_core_attrs->agility_max;
                vitality_max = maybe_core_attrs->vitality_max;
                comprehension_max = maybe_core_attrs->comprehension_max;
                willpower_max = maybe_core_attrs->willpower_max;
                charm_max = maybe_core_attrs->charm_max;
                mood_max = maybe_core_attrs->mood_max;
            }
            
            standpoint_type = a.get_standpoint_type();
                    
            if(born) {
                location = db.get<zone_object, by_id>(a.location).name;
                base_name = db.get<zone_object, by_id>(a.base).name;
            }
        }
        
        api_actor_object(){}
        
        actor_id_type       id;
        nfa_id_type         nfa_id;
        
        string              name;
        
        uint16_t            age;
        int16_t             health;
        int16_t             health_max;
        uint16_t            init_attribute_amount_max;
        
        //core attributes
        int16_t             strength        = 0;
        int16_t             strength_max    = 0;
        int16_t             physique        = 0;
        int16_t             physique_max    = 0;
        int16_t             agility         = 0;
        int16_t             agility_max     = 0;
        int16_t             vitality        = 0;
        int16_t             vitality_max    = 0;
        int16_t             comprehension   = 0;
        int16_t             comprehension_max   = 0;
        int16_t             willpower       = 0;
        int16_t             willpower_max   = 0;
        int16_t             charm           = 0;
        int16_t             charm_max       = 0;
        int16_t             mood            = 0;
        int16_t             mood_max        = 0;

        vector<actor_talent_rule_id_type>  talents;
        
        bool                born;
        int                 gender;
        int                 sexuality;
        int                 fertility;
                
        time_point_sec      born_time;
        int                 born_vyears;
        int                 born_vmonths;
        int                 born_vtimes;
        int                 five_phase;
        uint                standpoint;
        E_ACTOR_STANDPOINT_TYPE standpoint_type;
        int32_t             loyalty;
        
        string              location;   //所在区域名称
        string              base_name;  //从属地名称
        
        time_point_sec      last_update;
        time_point_sec      next_tick_time;
    };

    typedef uint64_t api_id_type;
    
} } } // taiyi::plugins::database_api

FC_REFLECT( taiyi::plugins::database_api::api_account_object, (id)(name)(owner)(active)(posting)(memo_key)(json_metadata)(proxy)(last_owner_update)(last_account_update)(created)(mined)(recovery_account)(last_account_recovery)(can_adore)(manabar)(balance)(reward_yang_balance)(reward_qi_balance)(qi)(delegated_qi)(received_qi)(qi_withdraw_rate)(next_qi_withdrawal_time)(withdrawn)(to_withdraw)(withdraw_routes)(proxied_vsf_adores)(simings_adored_for) )

FC_REFLECT( taiyi::plugins::database_api::api_owner_authority_history_object, (id)(account)(previous_owner_authority)(last_valid_time) )

FC_REFLECT( taiyi::plugins::database_api::api_account_recovery_request_object, (id)(account_to_recover)(new_owner_authority)(expires) )

FC_REFLECT( taiyi::plugins::database_api::api_siming_object, (id)(owner)(created)(url)(adores)(virtual_last_update)(virtual_position)(virtual_scheduled_time)(total_missed)(last_aslot)(last_confirmed_block_num)(signing_key)(props)(running_version)(hardfork_version_vote)(hardfork_time_vote) )

FC_REFLECT( taiyi::plugins::database_api::api_siming_schedule_object, (id)(current_virtual_time)(next_shuffle_block_num)(current_shuffled_simings)(num_scheduled_simings)(elected_weight)(timeshare_weight)(miner_weight)(siming_pay_normalization_factor)(median_props)(majority_version)(max_adored_simings)(hardfork_required_simings) )

FC_REFLECT_DERIVED( taiyi::plugins::database_api::api_signed_block_object, (taiyi::protocol::signed_block), (block_id)(signing_key)(transaction_ids) )

FC_REFLECT( taiyi::plugins::database_api::api_hardfork_property_object, (id)(processed_hardforks)(last_hardfork)(current_hardfork_version)(next_hardfork)(next_hardfork_time) )

FC_REFLECT(taiyi::plugins::database_api::api_nfa_object, (id)(creator_account)(owner_account)(active_account)(symbol)(parent)(children)(main_contract)(contract_data)(qi)(manabar)(created_time)(next_tick_time)(gold)(food)(wood)(fabric)(herb))

FC_REFLECT( taiyi::plugins::database_api::api_actor_object, (id)(name)(nfa_id)(age)(health)(health_max)(init_attribute_amount_max)(strength)(strength_max)(physique)(physique_max)(agility)(agility_max)(vitality)(vitality_max)(comprehension)(comprehension_max)(willpower)(willpower_max)(charm)(charm_max)(mood)(mood_max)(talents)(born)(gender)(sexuality)(fertility)(born_time)(born_vyears)(born_vmonths)(born_vtimes)(five_phase)(standpoint)(standpoint_type)(loyalty)(location)(base_name)(last_update)(next_tick_time) )

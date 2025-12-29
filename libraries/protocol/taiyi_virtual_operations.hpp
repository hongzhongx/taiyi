#pragma once
#include <protocol/base.hpp>
#include <protocol/block_header.hpp>
#include <protocol/asset.hpp>

#include <fc/utf8.hpp>

namespace taiyi { namespace protocol {
    
    struct fill_qi_withdraw_operation : public virtual_operation
    {
        fill_qi_withdraw_operation(){}
        fill_qi_withdraw_operation( const string& f, const string& t, const asset& w, const asset& d )
            : from_account(f), to_account(t), withdrawn(w), deposited(d) {}
        
        account_name_type from_account;
        account_name_type to_account;
        asset             withdrawn;
        asset             deposited;
    };
        
    struct hardfork_operation : public virtual_operation
    {
        hardfork_operation() {}
        hardfork_operation( uint32_t hf_id )
            : hardfork_id( hf_id ) {}
        
        uint32_t         hardfork_id = 0;
    };
        
    struct return_qi_delegation_operation : public virtual_operation
    {
        return_qi_delegation_operation() {}
        return_qi_delegation_operation( const account_name_type& a, const asset& v )
            : account( a ), qi( v ) {}
        
        account_name_type account;
        asset             qi;
    };
        
    struct producer_reward_operation : public virtual_operation
    {
        producer_reward_operation(){}
        producer_reward_operation( const string& p, const asset& v )
            : producer( p ), reward( v ) {}
        
        account_name_type producer;
        asset             reward;
        
    };
    
    struct shutdown_siming_operation : public virtual_operation
    {
        shutdown_siming_operation(){}
        shutdown_siming_operation( const string& o ) : owner(o) {}

        account_name_type owner;
    };
    
    struct nfa_symbol_create_operation : public virtual_operation
    {
        nfa_symbol_create_operation() {}
        nfa_symbol_create_operation(const account_name_type& creator_, const string& symbol_, const string& describe_, const string& default_contract_, const uint64_t& max_count_, const uint64_t& min_equivalent_qi_, const bool& is_sbt_)
        : creator(creator_), symbol(symbol_), describe(describe_), default_contract(default_contract_), max_count(max_count_), min_equivalent_qi(min_equivalent_qi_), is_sbt(is_sbt_) {}
        
        account_name_type   creator;
        string              symbol;
        string              describe;
        string              default_contract;
        uint64_t            max_count;
        uint64_t            min_equivalent_qi;
        bool                is_sbt;
    };
    
    struct nfa_symbol_authority_change_operation : public virtual_operation
    {
        nfa_symbol_authority_change_operation() {}
        nfa_symbol_authority_change_operation(const account_name_type& creator_, const string& symbol_, const string& authority_account_, const string& authority_nfa_symbol_)
        : creator(creator_), symbol(symbol_), authority_account(authority_account_), authority_nfa_symbol(authority_nfa_symbol_) {}
        
        account_name_type   creator;
        string              symbol;
        string              authority_account;
        string              authority_nfa_symbol;
    };
    
    struct nfa_create_operation : public virtual_operation
    {
        nfa_create_operation() {}
        nfa_create_operation(const account_name_type& creator_, const string& symbol_) : creator(creator_), symbol(symbol_) {}

        account_name_type   creator;
        string              symbol;
    };
    
    struct nfa_transfer_operation : public virtual_operation
    {
        nfa_transfer_operation() {}
        nfa_transfer_operation(const account_name_type& from_, const account_name_type& to_, const int64_t& id_) : from(from_), to(to_), id(id_) {}
        
        account_name_type       from;
        account_name_type       to;
        int64_t                 id; ///nfa id
    };
    
    struct nfa_active_approve_operation : public virtual_operation
    {
        nfa_active_approve_operation() {}
        nfa_active_approve_operation(const account_name_type& owner_, const account_name_type& active_account_, const int64_t& id_) : owner(owner_), active_account(active_account_), id(id_) {}

        account_name_type       owner;
        account_name_type       active_account;
        int64_t                 id; ///nfa id
    };
    
    struct nfa_convert_resources_operation : public virtual_operation
    {
        nfa_convert_resources_operation(){}
        nfa_convert_resources_operation(const int64_t& n, const account_name_type& o, const asset& q, const asset& r, const bool& _isqtor)
            : nfa( n ), owner( o ), qi( q ), resource( r ), is_qi_to_resource(_isqtor) {}
        
        int64_t             nfa;
        account_name_type   owner;
        asset               qi;
        asset               resource;
        bool                is_qi_to_resource;
    };

    struct nfa_asset_transfer_operation : public virtual_operation
    {
        nfa_asset_transfer_operation(){}
        nfa_asset_transfer_operation( const int64_t& f, const account_name_type& fo, const int64_t& t, const account_name_type& to, const asset& a )
            : from(f), from_owner(fo), to(t), to_owner(to), amount(a) {}
        
        int64_t             from;
        account_name_type   from_owner;
        int64_t             to;
        account_name_type   to_owner;

        asset               amount;
    };
    
    struct nfa_deposit_withdraw_operation : public virtual_operation
    {
        nfa_deposit_withdraw_operation(){}
        nfa_deposit_withdraw_operation( const int64_t& n, const string& a, const asset& d, const asset& w )
            : nfa(n), account(a), deposited(d), withdrawn(w)  {}
        
        int64_t             nfa;
        account_name_type   account;
        
        asset               deposited;
        asset               withdrawn;
    };

    struct reward_feigang_operation : public virtual_operation
    {
        reward_feigang_operation(){}
        reward_feigang_operation( const account_name_type& f, const int64_t& fn, const account_name_type& t, const asset& q )
            : from( f ), from_nfa(fn), to( t ), qi(q) {}
        
        account_name_type   from;
        int64_t             from_nfa;
        account_name_type   to;
        asset               qi;
    };
    
    struct reward_cultivation_operation : public virtual_operation
    {
        reward_cultivation_operation(){}
        reward_cultivation_operation( const account_name_type& a, const int64_t& n, const asset& q )
            : account( a ), nfa(n), qi( q ) {}
        
        account_name_type   account;
        int64_t             nfa;
        asset               qi;
    };

    struct tiandao_year_change_operation : public virtual_operation
    {
        tiandao_year_change_operation() {}
        tiandao_year_change_operation( const account_name_type& a, const uint32_t& y, const uint32_t& m, const uint32_t& t, const uint32_t& n, const uint32_t& d)
            : messager(a), years(y), months(m), times(t), live_num(n), dead_num(d) {}
        
        account_name_type           messager;
        
        uint32_t                    years;
        uint32_t                    months;
        uint32_t                    times;
        
        uint32_t                    live_num;   //人口数（多少活人）
        uint32_t                    dead_num;   //总去世人数
        uint32_t                    born_this_year;
        uint32_t                    dead_this_year;
    };
    
    struct tiandao_month_change_operation : public virtual_operation
    {
        tiandao_month_change_operation() {}
        tiandao_month_change_operation( const account_name_type& a, const uint32_t& y, const uint32_t& m, const uint32_t& t)
            : messager(a), years(y), months(m), times(t) {}
        
        account_name_type           messager;
        
        uint32_t                    years;
        uint32_t                    months;
        uint32_t                    times;
    };
    
    struct tiandao_time_change_operation : public virtual_operation
    {
        tiandao_time_change_operation() {}
        tiandao_time_change_operation( const account_name_type& a, const uint32_t& y, const uint32_t& m, const uint32_t& t)
            : messager(a), years(y), months(m), times(t) {}
        
        account_name_type           messager;
        
        uint32_t                    years;
        uint32_t                    months;
        uint32_t                    times;
    };
    
    struct actor_talent_rule_create_operation : public virtual_operation
    {
        actor_talent_rule_create_operation() {}
        actor_talent_rule_create_operation(const account_name_type& creator_, const string& contract_) : creator(creator_), contract(contract_) {}
        
        account_name_type           creator;
        string                      contract;
    };
    
    struct actor_create_operation : public virtual_operation
    {
        actor_create_operation() {}
        actor_create_operation(const account_name_type& creator_, const string& family_name_, const string& last_name_, const int64_t& nfa_) : creator(creator_), family_name(family_name_), last_name(last_name_), nfa(nfa_) {}
        
        account_name_type           creator;
        string                      family_name;
        string                      last_name;
        int64_t                     nfa;
    };

    struct actor_born_operation : public virtual_operation
    {
        actor_born_operation() {}
        actor_born_operation( const account_name_type& a, const string& n, const string& z, const int64_t& nf)
            : owner(a), name(n), zone(z), nfa(nf) {}
        
        account_name_type           owner;
        string                      name;
        string                      zone;
        int64_t                     nfa;
    };
        
    struct actor_movement_operation : public virtual_operation
    {
        actor_movement_operation() {}
        actor_movement_operation( const account_name_type& a, const string& n, const string& fz, const string& tz, const int64_t& nf)
            : owner(a), name(n), from_zone(fz), to_zone(tz), nfa(nf) {}
        
        account_name_type           owner;
        string                      name;        
        string                      from_zone;
        string                      to_zone;
        int64_t                     nfa;
    };

    struct narrate_log_operation : public virtual_operation
    {
        narrate_log_operation() {}
        narrate_log_operation( const account_name_type& a, const int64_t& nf, const uint32_t& y, const uint32_t& m, const uint32_t& d, const uint32_t&_tod, const uint32_t& t, const string& l)
            : narrator(a), nfa(nf), years(y), months(m), days(d), tod(_tod), times(t), log(l) {}
        
        account_name_type           narrator;
        int64_t                     nfa;

        uint32_t                    years;
        uint32_t                    months;
        uint32_t                    days;
        uint32_t                    tod;
        uint32_t                    times;

        string                      log;
    };
    
    struct actor_talk_operation : public virtual_operation
    {
        actor_talk_operation() {}
        actor_talk_operation( const uint32_t& vy, const uint32_t& vm, const uint32_t& vd, const uint32_t& tod, const uint32_t& vt, const account_name_type& ao, const int64_t& anfa, const string& an, const account_name_type& to, const int64_t& tnfa, const string& tn, const string& c)
            : v_years(vy), v_months(vm), v_days(vd), v_tod(tod), v_times(vt), actor_owner(ao), actor_nfa(anfa), actor_name(an), target_owner(to), target_nfa(tnfa), target_name(tn), content(c) {}
        
        uint32_t                    v_years;
        uint32_t                    v_months;
        uint32_t                    v_days;
        uint32_t                    v_tod;
        uint32_t                    v_times; //same as solar term number
        
        account_name_type           actor_owner;
        int64_t                     actor_nfa;
        string                      actor_name;
        account_name_type           target_owner;
        int64_t                     target_nfa;
        string                      target_name;
        
        string                      content;
    };
    
    struct zone_create_operation : public virtual_operation
    {
        zone_create_operation() {}
        zone_create_operation(const account_name_type& creator_, const string& name_, const int64_t& nfa_) : creator(creator_), name(name_), nfa(nfa_) {}
        
        account_name_type creator;
        string            name;
        int64_t           nfa;
    };
    
    struct zone_type_change_operation : public virtual_operation
    {
        zone_type_change_operation() {}
        zone_type_change_operation(const account_name_type& creator_, const string& name_, const int64_t& nfa_, const string& type_) : creator(creator_), name(name_), nfa(nfa_), type(type_) {}
        
        account_name_type creator;
        string            name;
        int64_t           nfa;
        string            type;
    };
    
    struct zone_connect_operation : public virtual_operation
    {
        zone_connect_operation() {}
        zone_connect_operation(const account_name_type& account_, const string& zone1_, const int64_t& zone1_nfa_, const string& zone2_, const int64_t& zone2_nfa_) : account(account_), zone1(zone1_), zone1_nfa(zone1_nfa_), zone2(zone2_), zone2_nfa(zone2_nfa_) {}
        
        account_name_type account;
        string            zone1;
        int64_t           zone1_nfa;
        string            zone2;
        int64_t           zone2_nfa;
    };
    
    struct proposal_execute_operation : public virtual_operation
    {
        proposal_execute_operation() = default;
        proposal_execute_operation(const string& contract_name_, const string& function_name_, const vector<lua_types> value_list_, const string subject_)
        : contract_name(contract_name_), function_name(function_name_), value_list(value_list_), subject(subject_) {}
                
        string              contract_name;  // 执行函数的合约名
        string              function_name;  // 执行的目标函数名
        vector<lua_types>   value_list;     // 执行函数的参数列表

        string              subject;
    };
    
    struct create_proposal_operation : public virtual_operation
    {
        create_proposal_operation() {}
        create_proposal_operation(const account_name_type& creator_, const string& contract_name_, const string& function_name_, const vector<lua_types>& value_list_, const time_point_sec& end_date_, const string& subject_)
        : creator(creator_), contract_name(contract_name_), function_name(function_name_), value_list(value_list_), end_date(end_date_), subject(subject_) {}
        
        account_name_type   creator;

        string              contract_name;  // 执行函数的合约名
        string              function_name;  // 执行的目标函数名
        vector<lua_types>   value_list;     // 执行函数的参数列表

        time_point_sec      end_date;
        string              subject;
    };
    
    struct update_proposal_votes_operation : public virtual_operation
    {
        update_proposal_votes_operation() {}
        update_proposal_votes_operation(const account_name_type& voter_, const flat_set_ex<int64_t>& proposal_ids_, const bool& approve_)
        : voter(voter_), proposal_ids(proposal_ids_), approve(approve_) {}
        
        account_name_type       voter;
        flat_set_ex<int64_t>    proposal_ids; // IDs of proposals to vote for/against. Nonexisting IDs are ignored.
        bool                    approve = false;
    };
    
    // Allows to remove proposals specified by given IDs. Operation can be performed only by proposal owner.
    struct remove_proposal_operation : public virtual_operation
    {
        remove_proposal_operation() {}
        remove_proposal_operation(const account_name_type& proposal_owner_, const flat_set_ex<int64_t>& proposal_ids_)
        : proposal_owner(proposal_owner_), proposal_ids(proposal_ids_) {}
        
        account_name_type       proposal_owner;
        flat_set_ex<int64_t>    proposal_ids; // IDs of proposals to be removed. Nonexisting IDs are ignored.
    };


} } //taiyi::protocol

FC_REFLECT( taiyi::protocol::fill_qi_withdraw_operation, (from_account)(to_account)(withdrawn)(deposited) )
FC_REFLECT( taiyi::protocol::hardfork_operation, (hardfork_id) )
FC_REFLECT( taiyi::protocol::return_qi_delegation_operation, (account)(qi) )
FC_REFLECT( taiyi::protocol::producer_reward_operation, (producer)(reward) )
FC_REFLECT( taiyi::protocol::shutdown_siming_operation, (owner) )
FC_REFLECT( taiyi::protocol::nfa_symbol_create_operation, (creator)(symbol)(describe)(default_contract)(max_count)(min_equivalent_qi)(is_sbt) )
FC_REFLECT( taiyi::protocol::nfa_symbol_authority_change_operation, (creator)(symbol)(authority_account)(authority_nfa_symbol) )
FC_REFLECT( taiyi::protocol::nfa_create_operation, (creator)(symbol) )
FC_REFLECT( taiyi::protocol::nfa_convert_resources_operation, (nfa)(owner)(qi)(resource)(is_qi_to_resource) )
FC_REFLECT( taiyi::protocol::nfa_asset_transfer_operation, (from)(from_owner)(to)(to_owner)(amount) )
FC_REFLECT( taiyi::protocol::nfa_transfer_operation, (from)(to)(id) )
FC_REFLECT( taiyi::protocol::nfa_active_approve_operation, (owner)(active_account)(id) )
FC_REFLECT( taiyi::protocol::nfa_deposit_withdraw_operation, (nfa)(account)(deposited)(withdrawn) )
FC_REFLECT( taiyi::protocol::reward_feigang_operation, (from)(from_nfa)(to)(qi) )
FC_REFLECT( taiyi::protocol::reward_cultivation_operation, (account)(nfa)(qi) )
FC_REFLECT( taiyi::protocol::tiandao_year_change_operation, (messager)(years)(months)(times)(live_num)(dead_num)(born_this_year)(dead_this_year) )
FC_REFLECT( taiyi::protocol::tiandao_month_change_operation, (messager)(years)(months)(times) )
FC_REFLECT( taiyi::protocol::tiandao_time_change_operation, (messager)(years)(months)(times) )
FC_REFLECT( taiyi::protocol::actor_talent_rule_create_operation, (creator)(contract) )
FC_REFLECT( taiyi::protocol::actor_create_operation, (creator)(family_name)(last_name)(nfa) )
FC_REFLECT( taiyi::protocol::actor_born_operation, (owner)(name)(zone)(nfa) )
FC_REFLECT( taiyi::protocol::actor_movement_operation, (owner)(name)(from_zone)(to_zone)(nfa) )
FC_REFLECT( taiyi::protocol::narrate_log_operation, (narrator)(nfa)(years)(months)(days)(tod)(times)(log) )
FC_REFLECT( taiyi::protocol::actor_talk_operation, (v_years)(v_months)(v_days)(v_tod)(v_times)(actor_owner)(actor_nfa)(actor_name)(target_owner)(target_nfa)(target_name)(content) )
FC_REFLECT( taiyi::protocol::zone_create_operation, (creator)(name)(nfa) )
FC_REFLECT( taiyi::protocol::zone_type_change_operation, (creator)(name)(nfa)(type) )
FC_REFLECT( taiyi::protocol::zone_connect_operation, (account)(zone1)(zone1_nfa)(zone2)(zone2_nfa) )

FC_REFLECT( taiyi::protocol::proposal_execute_operation, (contract_name)(function_name)(value_list)(subject) )
FC_REFLECT( taiyi::protocol::create_proposal_operation, (creator)(contract_name)(function_name)(value_list)(end_date)(subject) )
FC_REFLECT( taiyi::protocol::update_proposal_votes_operation, (voter)(proposal_ids)(approve) )
FC_REFLECT( taiyi::protocol::remove_proposal_operation, (proposal_owner)(proposal_ids) )

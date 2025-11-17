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
        nfa_symbol_create_operation(const account_name_type& creator_, const string& symbol_, const string& describe_, const string& default_contract_, const uint64_t& max_count_, const uint64_t& min_equivalent_qi_)
        : creator(creator_), symbol(symbol_), describe(describe_), default_contract(default_contract_), max_count(max_count_), min_equivalent_qi(min_equivalent_qi_) {}
        
        account_name_type   creator;
        string              symbol;
        string              describe;
        string              default_contract;
        uint64_t            max_count;
        uint64_t            min_equivalent_qi;
    };
    
    struct nfa_create_operation : public virtual_operation
    {
        nfa_create_operation() {}
        nfa_create_operation(const account_name_type& creator_, const string& symbol_) : creator(creator_), symbol(symbol_) {}

        account_name_type   creator;
        string              symbol;
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

    struct nfa_transfer_operation : public virtual_operation
    {
        nfa_transfer_operation(){}
        nfa_transfer_operation( const int64_t& f, const account_name_type& fo, const int64_t& t, const account_name_type& to, const asset& a )
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
    
    struct actor_talent_trigger_operation : public virtual_operation
    {
        actor_talent_trigger_operation() {}
        actor_talent_trigger_operation( const account_name_type& a, const string& n, const int64_t& nf, const int64_t& ti, const string& t, const string& d, const uint32_t& ag )
            : owner(a), name(n), nfa(nf), tid(ti), title(t), desc(d), age(ag) {}
        
        account_name_type           owner;
        string                      name;
        int64_t                     nfa;

        int64_t                     tid;
        string                      title;
        string                      desc;
        
        uint32_t                    age;
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

    struct actor_grown_operation : public virtual_operation
    {
        actor_grown_operation() {}
        actor_grown_operation( const account_name_type& a, const string& n, const int64_t& nf, const uint32_t& y, const uint32_t& m, const uint32_t& d, const uint32_t& _tod, const uint32_t& t, const uint32_t& ag, const int32_t& h)
            : owner(a), name(n), nfa(nf), years(y), months(m), days(d), tod(_tod), times(t), age(ag), health(h) {}
        
        account_name_type           owner;
        string                      name;
        int64_t                     nfa;

        uint32_t                    years;
        uint32_t                    months;
        uint32_t                    days;
        uint32_t                    tod;
        uint32_t                    times;

        uint32_t                    age;
        int32_t                     health;
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
        actor_talk_operation( const uint32_t& vy, const uint32_t& vm, const uint32_t& vd, const uint32_t& tod, const uint32_t& vt, const account_name_type& ao, const int64_t& anfa, const string& an, const account_name_type& to, const int64_t& tnfa, const string& tn, const string& c, const int& fdm, const int& fdt)
            : v_years(vy), v_months(vm), v_days(vd), v_tod(tod), v_times(vt), actor_owner(ao), actor_nfa(anfa), actor_name(an), target_owner(to), target_nfa(tnfa), target_name(tn), content(c), favor_delta_actor(fdm), favor_delta_target(fdt) {}
        
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
        int                         favor_delta_actor;
        int                         favor_delta_target;
    };

} } //taiyi::protocol

FC_REFLECT( taiyi::protocol::fill_qi_withdraw_operation, (from_account)(to_account)(withdrawn)(deposited) )
FC_REFLECT( taiyi::protocol::hardfork_operation, (hardfork_id) )
FC_REFLECT( taiyi::protocol::return_qi_delegation_operation, (account)(qi) )
FC_REFLECT( taiyi::protocol::producer_reward_operation, (producer)(reward) )
FC_REFLECT( taiyi::protocol::shutdown_siming_operation, (owner) )
FC_REFLECT( taiyi::protocol::nfa_symbol_create_operation, (creator)(symbol)(describe)(default_contract)(max_count)(min_equivalent_qi) )
FC_REFLECT( taiyi::protocol::nfa_create_operation, (creator)(symbol) )
FC_REFLECT( taiyi::protocol::nfa_convert_resources_operation, (nfa)(owner)(qi)(resource)(is_qi_to_resource) )
FC_REFLECT( taiyi::protocol::nfa_transfer_operation, (from)(from_owner)(to)(to_owner)(amount) )
FC_REFLECT( taiyi::protocol::nfa_deposit_withdraw_operation, (nfa)(account)(deposited)(withdrawn) )
FC_REFLECT( taiyi::protocol::reward_feigang_operation, (from)(from_nfa)(to)(qi) )
FC_REFLECT( taiyi::protocol::reward_cultivation_operation, (account)(nfa)(qi) )
FC_REFLECT( taiyi::protocol::tiandao_year_change_operation, (messager)(years)(months)(times)(live_num)(dead_num)(born_this_year)(dead_this_year) )
FC_REFLECT( taiyi::protocol::tiandao_month_change_operation, (messager)(years)(months)(times) )
FC_REFLECT( taiyi::protocol::tiandao_time_change_operation, (messager)(years)(months)(times) )
FC_REFLECT( taiyi::protocol::actor_born_operation, (owner)(name)(zone)(nfa) )
FC_REFLECT( taiyi::protocol::actor_talent_trigger_operation, (owner)(name)(nfa)(tid)(title)(desc)(age) )
FC_REFLECT( taiyi::protocol::actor_movement_operation, (owner)(name)(from_zone)(to_zone)(nfa) )
FC_REFLECT( taiyi::protocol::actor_grown_operation, (owner)(name)(nfa)(years)(months)(days)(tod)(times)(age)(health) )
FC_REFLECT( taiyi::protocol::narrate_log_operation, (narrator)(nfa)(years)(months)(days)(tod)(times)(log) )
FC_REFLECT( taiyi::protocol::actor_talk_operation, (v_years)(v_months)(v_days)(v_tod)(v_times)(actor_owner)(actor_nfa)(actor_name)(target_owner)(target_nfa)(target_name)(content)(favor_delta_actor)(favor_delta_target) )

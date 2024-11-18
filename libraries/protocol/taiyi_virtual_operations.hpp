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
            : producer( p ), qi( v ) {}
        
        account_name_type producer;
        asset             qi;
        
    };
    
    struct nfa_convert_qi_to_resources_operation : public virtual_operation
    {
        nfa_convert_qi_to_resources_operation(){}
        nfa_convert_qi_to_resources_operation( const int64_t& n, const account_name_type& o, const asset& q, const asset& r )
            : nfa( n ), owner( o ), qi_converted( q ), new_resource( r ) {}
        
        int64_t             nfa;
        account_name_type   owner;
        asset               qi_converted;
        asset               new_resource;
    };

    struct reward_qi_operation : public virtual_operation
    {
        reward_qi_operation(){}
        reward_qi_operation( const account_name_type& a, const asset& q )
            : account( a ), qi( q ) {}
        
        account_name_type   account;
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

} } //taiyi::protocol

FC_REFLECT( taiyi::protocol::fill_qi_withdraw_operation, (from_account)(to_account)(withdrawn)(deposited) )
FC_REFLECT( taiyi::protocol::hardfork_operation, (hardfork_id) )
FC_REFLECT( taiyi::protocol::return_qi_delegation_operation, (account)(qi) )
FC_REFLECT( taiyi::protocol::producer_reward_operation, (producer)(qi) )
FC_REFLECT( taiyi::protocol::nfa_convert_qi_to_resources_operation, (nfa)(owner)(qi_converted)(new_resource) )
FC_REFLECT( taiyi::protocol::reward_qi_operation, (account)(qi) )
FC_REFLECT( taiyi::protocol::tiandao_year_change_operation, (messager)(years)(months)(times)(live_num)(dead_num)(born_this_year)(dead_this_year) )
FC_REFLECT( taiyi::protocol::tiandao_month_change_operation, (messager)(years)(months)(times) )
FC_REFLECT( taiyi::protocol::tiandao_time_change_operation, (messager)(years)(months)(times) )

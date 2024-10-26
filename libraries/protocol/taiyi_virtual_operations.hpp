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
            : account( a ), qi_shares( v ) {}
        
        account_name_type account;
        asset             qi_shares;
    };
        
    struct producer_reward_operation : public virtual_operation
    {
        producer_reward_operation(){}
        producer_reward_operation( const string& p, const asset& v )
            : producer( p ), qi_shares( v ) {}
        
        account_name_type producer;
        asset             qi_shares;
        
    };
                        
} } //taiyi::protocol

FC_REFLECT( taiyi::protocol::fill_qi_withdraw_operation, (from_account)(to_account)(withdrawn)(deposited) )
FC_REFLECT( taiyi::protocol::hardfork_operation, (hardfork_id) )
FC_REFLECT( taiyi::protocol::return_qi_delegation_operation, (account)(qi_shares) )
FC_REFLECT( taiyi::protocol::producer_reward_operation, (producer)(qi_shares) )

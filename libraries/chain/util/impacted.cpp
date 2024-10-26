#include <protocol/authority.hpp>

#include <chain/util/impacted.hpp>

#include <fc/utility.hpp>

namespace taiyi { namespace chain {

    using namespace fc;
    using namespace taiyi::protocol;

    struct get_impacted_account_visitor
    {
        flat_set<account_name_type>& _impacted;
        
        get_impacted_account_visitor( flat_set<account_name_type>& impact ):_impacted( impact ) {}
        typedef void result_type;
        
        template<typename T>
        void operator()( const T& op )
        {
            op.get_required_posting_authorities( _impacted );
            op.get_required_active_authorities( _impacted );
            op.get_required_owner_authorities( _impacted );
        }
    
        // ops
        void operator()( const account_create_operation& op )
        {
            _impacted.insert( op.new_account_name );
            _impacted.insert( op.creator );
        }
        
        void operator()( const transfer_operation& op )
        {
            _impacted.insert( op.from );
            _impacted.insert( op.to );
        }
        
        void operator()( const transfer_to_qi_operation& op )
        {
            _impacted.insert( op.from );
            
            if ( op.to != account_name_type() && op.to != op.from )
            {
                _impacted.insert( op.to );
            }
        }
        
        void operator()( const set_withdraw_qi_route_operation& op )
        {
            _impacted.insert( op.from_account );
            _impacted.insert( op.to_account );
        }
        
        void operator()( const account_siming_adore_operation& op )
        {
            _impacted.insert( op.account );
            _impacted.insert( op.siming );
        }
        
        void operator()( const account_siming_proxy_operation& op )
        {
            _impacted.insert( op.account );
            _impacted.insert( op.proxy );
        }
        
        void operator()( const request_account_recovery_operation& op )
        {
            _impacted.insert( op.account_to_recover );
            _impacted.insert( op.recovery_account );
        }
        
        void operator()( const recover_account_operation& op )
        {
            _impacted.insert( op.account_to_recover );
        }
        
        void operator()( const change_recovery_account_operation& op )
        {
            _impacted.insert( op.account_to_recover );
        }
        
        void operator()( const delegate_qi_shares_operation& op )
        {
            _impacted.insert( op.delegator );
            _impacted.insert( op.delegatee );
        }
        
        void operator()( const siming_set_properties_operation& op )
        {
            _impacted.insert( op.owner );
        }
                
        // vops
        
        void operator()( const fill_qi_withdraw_operation& op )
        {
            _impacted.insert( op.from_account );
            _impacted.insert( op.to_account );
        }
        
        void operator()( const return_qi_delegation_operation& op )
        {
            _impacted.insert( op.account );
        }
        
        void operator()( const producer_reward_operation& op )
        {
            _impacted.insert( op.producer );
        }
                        
        void operator()( const hardfork_operation& op )
        {
            _impacted.insert( TAIYI_INIT_MINER_NAME );
        }
        
        //void operator()( const operation& op ){}
    };

    void operation_get_impacted_accounts( const operation& op, flat_set<account_name_type>& result )
    {
        get_impacted_account_visitor vtor = get_impacted_account_visitor( result );
        op.visit( vtor );
    }
    
    void transaction_get_impacted_accounts( const transaction& tx, flat_set<account_name_type>& result )
    {
        for( const auto& op : tx.operations )
            operation_get_impacted_accounts( op, result );
    }

} } //taiyi::chain

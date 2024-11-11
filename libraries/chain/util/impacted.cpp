#include <protocol/authority.hpp>

#include <chain/util/impacted.hpp>

#include <fc/utility.hpp>

namespace taiyi { namespace chain {

    using namespace fc;
    using namespace taiyi::protocol;

    struct get_impacted_account_visitor
    {
        flat_set<account_name_type>& _impacted;
        flat_set<int64_t>& _impacted_nfas;
        
        get_impacted_account_visitor( flat_set<account_name_type>& impact, flat_set<int64_t>& impact_nfas ):_impacted( impact ), _impacted_nfas(impact_nfas) {}
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
        
        void operator()( const delegate_qi_operation& op )
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
            _impacted.insert( TAIYI_INIT_SIMING_NAME );
        }

        void operator()( const create_contract_operation& op )
        {
            _impacted.insert( op.owner );
        }

        void operator()( const call_contract_function_operation& op )
        {
            _impacted.insert( op.caller );
        }

        void operator()( const revise_contract_operation& op )
        {
            _impacted.insert( op.reviser );
        }

        void operator()( const create_nfa_symbol_operation& op )
        {
            _impacted.insert( op.creator );
        }
        
        void operator()( const create_nfa_operation& op )
        {
            _impacted.insert( op.creator );
        }

        void operator()( const transfer_nfa_operation& op )
        {
            _impacted.insert( op.from );
            _impacted.insert( op.to );

            _impacted_nfas.insert( op.id );
        }

        void operator()( const deposit_qi_to_nfa_operation& op )
        {
            _impacted.insert( op.account );

            _impacted_nfas.insert( op.id );
        }

        void operator()( const withdraw_qi_from_nfa_operation& op )
        {
            _impacted.insert( op.owner );

            _impacted_nfas.insert( op.id );
        }

        void operator()( const action_nfa_operation& op )
        {
            _impacted.insert( op.owner );

            _impacted_nfas.insert( op.id );
        }

        void operator()( const nfa_convert_qi_to_resources_operation& op )
        {
            _impacted.insert( op.owner ); //至少要有一个受影响的账号，否则历史记录不会记录
            _impacted_nfas.insert( op.nfa );
        }

        void operator()( const reward_qi_operation& op )
        {
            _impacted.insert( op.account );
        }

        //void operator()( const operation& op ){}
    };

    void operation_get_impacted_accounts( const operation& op, flat_set<account_name_type>& result )
    {
        fc::flat_set<int64_t> impact_nfas;
        get_impacted_account_visitor vtor = get_impacted_account_visitor( result, impact_nfas );
        op.visit( vtor );
    }
    
    void operation_get_impacted_nfas(const taiyi::protocol::operation& op, fc::flat_set<int64_t>& result )
    {
        flat_set<account_name_type> impact_accounts;
        get_impacted_account_visitor vtor = get_impacted_account_visitor( impact_accounts, result );
        op.visit( vtor );
    }

    void transaction_get_impacted_accounts( const transaction& tx, flat_set<account_name_type>& result )
    {
        for( const auto& op : tx.operations )
            operation_get_impacted_accounts( op, result );
    }

} } //taiyi::chain

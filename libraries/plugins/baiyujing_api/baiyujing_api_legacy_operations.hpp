#pragma once
#include <chain/taiyi_fwd.hpp>

#include <protocol/operations.hpp>

#include <chain/siming_objects.hpp>

#include <plugins/baiyujing_api/baiyujing_api_legacy_asset.hpp>

namespace taiyi { namespace plugins { namespace baiyujing_api {

    template< typename T >
    struct convert_to_legacy_static_variant
    {
        convert_to_legacy_static_variant( T& l_sv ) : legacy_sv( l_sv ) {}

        T& legacy_sv;

        typedef void result_type;

        template< typename V >
        void operator()( const V& v ) const
        {
            legacy_sv = v;
        }
    };

    using namespace taiyi::protocol;

    typedef account_update_operation                legacy_account_update_operation;
    typedef set_withdraw_qi_route_operation         legacy_set_withdraw_qi_route_operation;
    typedef siming_set_properties_operation         legacy_siming_set_properties_operation;
    typedef account_siming_adore_operation          legacy_account_siming_adore_operation;
    typedef account_siming_proxy_operation          legacy_account_siming_proxy_operation;
    typedef custom_operation                        legacy_custom_operation;
    typedef custom_json_operation                   legacy_custom_json_operation;
    typedef request_account_recovery_operation      legacy_request_account_recovery_operation;
    typedef recover_account_operation               legacy_recover_account_operation;
    typedef change_recovery_account_operation       legacy_change_recovery_account_operation;
    typedef decline_adoring_rights_operation        legacy_decline_adoring_rights_operation;
    typedef hardfork_operation                      legacy_hardfork_operation;
    typedef create_contract_operation               legacy_create_contract_operation;
    typedef revise_contract_operation               legacy_revise_contract_operation;
    typedef call_contract_function_operation        legacy_call_contract_function_operation;
    typedef create_nfa_symbol_operation             legacy_create_nfa_symbol_operation;
    typedef create_nfa_operation                    legacy_create_nfa_operation;
    typedef transfer_nfa_operation                  legacy_transfer_nfa_operation;
    typedef action_nfa_operation                    legacy_action_nfa_operation;

    struct api_chain_properties
    {
        api_chain_properties() {}
        api_chain_properties( const chain::chain_properties& c ) : account_creation_fee( legacy_asset::from_asset( c.account_creation_fee ) ), maximum_block_size( c.maximum_block_size )
        {}

        operator legacy_chain_properties() const
        {
            legacy_chain_properties props;
            props.account_creation_fee = legacy_taiyi_asset::from_asset( asset( account_creation_fee ) );
            props.maximum_block_size = maximum_block_size;
            return props;
        }

        legacy_asset   account_creation_fee;
        uint32_t       maximum_block_size = TAIYI_MIN_BLOCK_SIZE_LIMIT * 2;
    };

    struct legacy_account_create_operation
    {
        legacy_account_create_operation() {}
        legacy_account_create_operation( const account_create_operation& op ) : fee( legacy_asset::from_asset( op.fee ) ), creator( op.creator ), new_account_name( op.new_account_name ), owner( op.owner ), active( op.active ), posting( op.posting ), memo_key( op.memo_key ), json_metadata( op.json_metadata )
        {}

        operator account_create_operation()const
        {
            account_create_operation op;
            op.fee = fee;
            op.creator = creator;
            op.new_account_name = new_account_name;
            op.owner = owner;
            op.active = active;
            op.posting = posting;
            op.memo_key = memo_key;
            op.json_metadata = json_metadata;
            return op;
        }

        legacy_asset      fee;
        account_name_type creator;
        account_name_type new_account_name;
        authority         owner;
        authority         active;
        authority         posting;
        public_key_type   memo_key;
        string            json_metadata;
    };

    struct legacy_transfer_operation
    {
        legacy_transfer_operation() {}
        legacy_transfer_operation( const transfer_operation& op ) : from( op.from ), to( op.to ), amount( legacy_asset::from_asset( op.amount ) ), memo( op.memo )
        {}

        operator transfer_operation()const
        {
            transfer_operation op;
            op.from = from;
            op.to = to;
            op.amount = amount;
            op.memo = memo;
            return op;
        }

        account_name_type from;
        account_name_type to;
        legacy_asset      amount;
        string            memo;
    };

    struct legacy_transfer_to_qi_operation
    {
        legacy_transfer_to_qi_operation() {}
        legacy_transfer_to_qi_operation( const transfer_to_qi_operation& op ) : from( op.from ), to( op.to ), amount(legacy_asset::from_asset( op.amount ))
        {}

        operator transfer_to_qi_operation()const
        {
            transfer_to_qi_operation op;
            op.from = from;
            op.to = to;
            op.amount = amount;
            return op;
        }

        account_name_type from;
        account_name_type to;
        legacy_asset      amount;
    };

    struct legacy_withdraw_qi_operation
    {
        legacy_withdraw_qi_operation() {}
        legacy_withdraw_qi_operation( const withdraw_qi_operation& op ) : account( op.account ), qi_shares( legacy_asset::from_asset(op.qi_shares) )
        {}

        operator withdraw_qi_operation()const
        {
            withdraw_qi_operation op;
            op.account = account;
            op.qi_shares = qi_shares;
            return op;
        }

        account_name_type account;
        legacy_asset      qi_shares;
    };

    struct legacy_siming_update_operation
    {
        legacy_siming_update_operation() {}
        legacy_siming_update_operation( const siming_update_operation& op ) : owner( op.owner ), url( op.url ), block_signing_key(op.block_signing_key ), fee( legacy_asset::from_asset( op.fee ) )
        {
            props.account_creation_fee = legacy_asset::from_asset( op.props.account_creation_fee.to_asset< false >() );
            props.maximum_block_size = op.props.maximum_block_size;
        }

        operator siming_update_operation()const
        {
            siming_update_operation op;
            op.owner = owner;
            op.url = url;
            op.block_signing_key = block_signing_key;
            op.props.account_creation_fee = legacy_taiyi_asset::from_asset( asset( props.account_creation_fee ) );
            op.props.maximum_block_size = props.maximum_block_size;
            op.fee = fee;
            return op;
        }

        account_name_type       owner;
        string                  url;
        public_key_type         block_signing_key;
        api_chain_properties    props;
        legacy_asset            fee;
    };

    struct legacy_claim_reward_balance_operation
    {
        legacy_claim_reward_balance_operation() {}
        legacy_claim_reward_balance_operation( const claim_reward_balance_operation& op ) : account( op.account ), reward_yang(legacy_asset::from_asset( op.reward_yang ) ), reward_qi(legacy_asset::from_asset( op.reward_qi ) )
        {}

        operator claim_reward_balance_operation()const
        {
            claim_reward_balance_operation op;
            op.account = account;
            op.reward_yang = reward_yang;
            op.reward_qi = reward_qi;
            return op;
        }

        account_name_type account;
        legacy_asset      reward_yang;
        legacy_asset      reward_qi;
    };

    struct legacy_delegate_qi_shares_operation
    {
        legacy_delegate_qi_shares_operation() {}
        legacy_delegate_qi_shares_operation( const delegate_qi_shares_operation& op ) : delegator( op.delegator ), delegatee(op.delegatee), qi_shares( legacy_asset::from_asset( op.qi_shares ) )
        {}

        operator delegate_qi_shares_operation()const
        {
            delegate_qi_shares_operation op;
            op.delegator = delegator;
            op.delegatee = delegatee;
            op.qi_shares = qi_shares;
            return op;
        }

        account_name_type delegator;
        account_name_type delegatee;
        legacy_asset      qi_shares;
    };

    struct legacy_fill_qi_withdraw_operation
    {
        legacy_fill_qi_withdraw_operation() {}
        legacy_fill_qi_withdraw_operation( const fill_qi_withdraw_operation& op ) : from_account( op.from_account ), to_account(op.to_account ), withdrawn( legacy_asset::from_asset( op.withdrawn ) ), deposited( legacy_asset::from_asset( op.deposited ) )
        {}

        operator fill_qi_withdraw_operation()const
        {
            fill_qi_withdraw_operation op;
            op.from_account = from_account;
            op.to_account = to_account;
            op.withdrawn = withdrawn;
            op.deposited = deposited;
            return op;
        }

        account_name_type from_account;
        account_name_type to_account;
        legacy_asset      withdrawn;
        legacy_asset      deposited;
    };

    struct legacy_return_qi_delegation_operation
    {
        legacy_return_qi_delegation_operation() {}
        legacy_return_qi_delegation_operation( const return_qi_delegation_operation& op ) : account( op.account ), qi_shares(legacy_asset::from_asset( op.qi_shares ) )
        {}

        operator return_qi_delegation_operation()const
        {
            return_qi_delegation_operation op;
            op.account = account;
            op.qi_shares = qi_shares;
            return op;
        }

        account_name_type account;
        legacy_asset      qi_shares;
    };

    struct legacy_producer_reward_operation
    {
        legacy_producer_reward_operation() {}
        legacy_producer_reward_operation( const producer_reward_operation& op ) : producer( op.producer ), qi_shares(legacy_asset::from_asset(op.qi_shares ) )
        {}

        operator producer_reward_operation()const
        {
            producer_reward_operation op;
            op.producer = producer;
            op.qi_shares = qi_shares;
            return op;
        }

        account_name_type producer;
        legacy_asset      qi_shares;
    };
    
    struct legacy_deposit_qi_to_nfa_operation
    {
        legacy_deposit_qi_to_nfa_operation() {}
        legacy_deposit_qi_to_nfa_operation( const deposit_qi_to_nfa_operation& op ) : account( op.account ), id( op.id ), amount(legacy_asset::from_asset( op.amount ))
        {}

        operator deposit_qi_to_nfa_operation()const
        {
            deposit_qi_to_nfa_operation op;
            op.account = account;
            op.id = id;
            op.amount = amount;
            return op;
        }

        account_name_type account;
        int64_t           id;
        legacy_asset      amount;
    };

    struct legacy_withdraw_qi_from_nfa_operation
    {
        legacy_withdraw_qi_from_nfa_operation() {}
        legacy_withdraw_qi_from_nfa_operation( const withdraw_qi_from_nfa_operation& op ) : owner( op.owner ), id( op.id ), amount(legacy_asset::from_asset( op.amount ))
        {}

        operator withdraw_qi_from_nfa_operation()const
        {
            withdraw_qi_from_nfa_operation op;
            op.owner = owner;
            op.id = id;
            op.amount = amount;
            return op;
        }

        account_name_type owner;
        int64_t           id;
        legacy_asset      amount;
    };

    typedef fc::static_variant<
        legacy_transfer_operation,
        legacy_transfer_to_qi_operation,
        legacy_withdraw_qi_operation,
        legacy_account_create_operation,
        legacy_account_update_operation,
        legacy_siming_update_operation,
        legacy_account_siming_adore_operation,
        legacy_account_siming_proxy_operation,
        legacy_custom_operation,
        legacy_custom_json_operation,
        legacy_set_withdraw_qi_route_operation,
        legacy_request_account_recovery_operation,
        legacy_recover_account_operation,
        legacy_change_recovery_account_operation,
        legacy_decline_adoring_rights_operation,
        legacy_claim_reward_balance_operation,
        legacy_delegate_qi_shares_operation,
        legacy_siming_set_properties_operation,

        legacy_create_contract_operation,
        legacy_revise_contract_operation,
        legacy_call_contract_function_operation,

        legacy_create_nfa_symbol_operation,
        legacy_create_nfa_operation,
        legacy_transfer_nfa_operation,
        legacy_deposit_qi_to_nfa_operation,
        legacy_withdraw_qi_from_nfa_operation,
        legacy_action_nfa_operation,

        /// virtual operations below this point
        legacy_fill_qi_withdraw_operation,
        legacy_hardfork_operation,
        legacy_return_qi_delegation_operation,
        legacy_producer_reward_operation

    > legacy_operation;

    struct legacy_operation_conversion_visitor
    {
        legacy_operation_conversion_visitor( legacy_operation& legacy_op ) : l_op( legacy_op ) {}

        typedef bool result_type;

        legacy_operation& l_op;

        bool operator()( const account_update_operation& op )const                  { l_op = op; return true; }
        bool operator()( const set_withdraw_qi_route_operation& op )const           { l_op = op; return true; }
        bool operator()( const siming_set_properties_operation& op )const           { l_op = op; return true; }
        bool operator()( const account_siming_adore_operation& op )const            { l_op = op; return true; }
        bool operator()( const account_siming_proxy_operation& op )const            { l_op = op; return true; }
        bool operator()( const custom_operation& op )const                          { l_op = op; return true; }
        bool operator()( const custom_json_operation& op )const                     { l_op = op; return true; }
        bool operator()( const request_account_recovery_operation& op )const        { l_op = op; return true; }
        bool operator()( const recover_account_operation& op )const                 { l_op = op; return true; }
        bool operator()( const change_recovery_account_operation& op )const         { l_op = op; return true; }
        bool operator()( const decline_adoring_rights_operation& op )const          { l_op = op; return true; }
        bool operator()( const hardfork_operation& op )const                        { l_op = op; return true; }
        bool operator()( const create_contract_operation& op )const                 { l_op = op; return true; }
        bool operator()( const revise_contract_operation& op )const                 { l_op = op; return true; }
        bool operator()( const call_contract_function_operation& op )const          { l_op = op; return true; }
        bool operator()( const create_nfa_symbol_operation& op )const               { l_op = op; return true; }
        bool operator()( const create_nfa_operation& op )const                      { l_op = op; return true; }
        bool operator()( const transfer_nfa_operation& op )const                    { l_op = op; return true; }
        bool operator()( const action_nfa_operation& op )const                      { l_op = op; return true; }

        bool operator()( const transfer_operation& op )const
        {
            l_op = legacy_transfer_operation( op );
            return true;
        }

        bool operator()( const transfer_to_qi_operation& op )const
        {
            l_op = legacy_transfer_to_qi_operation( op );
            return true;
        }

        bool operator()( const withdraw_qi_operation& op )const
        {
            l_op = legacy_withdraw_qi_operation( op );
            return true;
        }

        bool operator()( const account_create_operation& op )const
        {
            l_op = legacy_account_create_operation( op );
            return true;
        }

        bool operator()( const siming_update_operation& op )const
        {
            l_op = legacy_siming_update_operation( op );
            return true;
        }

        bool operator()( const claim_reward_balance_operation& op )const
        {
            l_op = legacy_claim_reward_balance_operation( op );
            return true;
        }

        bool operator()( const delegate_qi_shares_operation& op )const
        {
            l_op = legacy_delegate_qi_shares_operation( op );
            return true;
        }

        bool operator()( const fill_qi_withdraw_operation& op )const
        {
            l_op = legacy_fill_qi_withdraw_operation( op );
            return true;
        }

        bool operator()( const return_qi_delegation_operation& op )const
        {
            l_op = legacy_return_qi_delegation_operation( op );
            return true;
        }

        bool operator()( const producer_reward_operation& op )const
        {
            l_op = legacy_producer_reward_operation( op );
            return true;
        }
        
        bool operator()( const deposit_qi_to_nfa_operation& op )const
        {
            l_op = legacy_deposit_qi_to_nfa_operation( op );
            return true;
        }

        bool operator()( const withdraw_qi_from_nfa_operation& op )const
        {
            l_op = legacy_withdraw_qi_from_nfa_operation( op );
            return true;
        }

        // Should only be SGT ops
        template< typename T >
        bool operator()( const T& )const { return false; }
    };

    struct convert_from_legacy_operation_visitor
    {
        convert_from_legacy_operation_visitor() {}

        typedef operation result_type;

        operation operator()( const legacy_transfer_operation& op )const
        {
            return operation( transfer_operation( op ) );
        }

        operation operator()( const legacy_transfer_to_qi_operation& op )const
        {
            return operation( transfer_to_qi_operation( op ) );
        }

        operation operator()( const legacy_withdraw_qi_operation& op )const
        {
            return operation( withdraw_qi_operation( op ) );
        }

        operation operator()( const legacy_account_create_operation& op )const
        {
            return operation( account_create_operation( op ) );
        }

        operation operator()( const legacy_siming_update_operation& op )const
        {
            return operation( siming_update_operation( op ) );
        }

        operation operator()( const legacy_claim_reward_balance_operation& op )const
        {
            return operation( claim_reward_balance_operation( op ) );
        }

        operation operator()( const legacy_delegate_qi_shares_operation& op )const
        {
            return operation( delegate_qi_shares_operation( op ) );
        }

        operation operator()( const legacy_fill_qi_withdraw_operation& op )const
        {
            return operation( fill_qi_withdraw_operation( op ) );
        }

        operation operator()( const legacy_return_qi_delegation_operation& op )const
        {
            return operation( return_qi_delegation_operation( op ) );
        }

        operation operator()( const legacy_producer_reward_operation& op )const
        {
            return operation( producer_reward_operation( op ) );
        }
        
        operation operator()( const legacy_deposit_qi_to_nfa_operation& op )const
        {
            return operation( deposit_qi_to_nfa_operation( op ) );
        }

        operation operator()( const legacy_withdraw_qi_from_nfa_operation& op )const
        {
            return operation( withdraw_qi_from_nfa_operation( op ) );
        }

        template< typename T >
        operation operator()( const T& t )const
        {
            return operation( t );
        }
    };

} } } // taiyi::plugins::baiyujing_api

namespace fc {

    void to_variant( const taiyi::plugins::baiyujing_api::legacy_operation&, fc::variant& );
    void from_variant( const fc::variant&, taiyi::plugins::baiyujing_api::legacy_operation& );
    
    struct from_old_static_variant
    {
        variant& var;
        from_old_static_variant( variant& dv ):var(dv){}
        
        typedef void result_type;
        template<typename T> void operator()( const T& v )const
        {
            to_variant( v, var );
        }
    };
    
    struct to_old_static_variant
    {
        const variant& var;
        to_old_static_variant( const variant& dv ):var(dv){}
        
        typedef void result_type;
        template<typename T> void operator()( T& v )const
        {
            from_variant( var, v );
        }
    };
    
    template< typename T >
    void old_sv_to_variant( const T& sv, fc::variant& v )
    {
        variant tmp;
        variants vars(2);
        vars[0] = sv.which();
        sv.visit( from_old_static_variant(vars[1]) );
        v = std::move(vars);
    }

    template< typename T >
    void old_sv_from_variant( const fc::variant& v, T& sv )
    {
        auto ar = v.get_array();
        if( ar.size() < 2 ) return;
        sv.set_which( static_cast< int64_t >( ar[0].as_uint64() ) );
        sv.visit( to_old_static_variant(ar[1]) );
    }

} //fc

FC_REFLECT( taiyi::plugins::baiyujing_api::api_chain_properties, (account_creation_fee)(maximum_block_size) )

FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_account_create_operation, (fee)(creator)(new_account_name)(owner)(active)(posting)(memo_key)(json_metadata) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_transfer_operation, (from)(to)(amount)(memo) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_transfer_to_qi_operation, (from)(to)(amount) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_withdraw_qi_operation, (account)(qi_shares) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_siming_update_operation, (owner)(url)(block_signing_key)(props)(fee) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_claim_reward_balance_operation, (account)(reward_yang)(reward_qi) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_delegate_qi_shares_operation, (delegator)(delegatee)(qi_shares) );
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_fill_qi_withdraw_operation, (from_account)(to_account)(withdrawn)(deposited) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_return_qi_delegation_operation, (account)(qi_shares) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_producer_reward_operation, (producer)(qi_shares) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_deposit_qi_to_nfa_operation, (account)(id)(amount) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_withdraw_qi_from_nfa_operation, (owner)(id)(amount) )

FC_REFLECT_TYPENAME( taiyi::plugins::baiyujing_api::legacy_operation )

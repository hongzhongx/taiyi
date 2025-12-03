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
    typedef release_contract_operation              legacy_release_contract_operation;
    typedef call_contract_function_operation        legacy_call_contract_function_operation;
    typedef nfa_symbol_create_operation             legacy_nfa_symbol_create_operation;
    typedef nfa_symbol_authority_change_operation   legacy_nfa_symbol_authority_change_operation;
    typedef nfa_create_operation                    legacy_nfa_create_operation;
    typedef nfa_transfer_operation                  legacy_nfa_transfer_operation;
    typedef nfa_active_approve_operation            legacy_nfa_active_approve_operation;
    typedef action_nfa_operation                    legacy_action_nfa_operation;
    typedef tiandao_year_change_operation           legacy_tiandao_year_change_operation;
    typedef tiandao_month_change_operation          legacy_tiandao_month_change_operation;
    typedef tiandao_time_change_operation           legacy_tiandao_time_change_operation;
    typedef actor_talent_rule_create_operation      legacy_actor_talent_rule_create_operation;
    typedef actor_create_operation                  legacy_actor_create_operation;
    typedef actor_born_operation                    legacy_actor_born_operation;
    typedef actor_talent_trigger_operation          legacy_actor_talent_trigger_operation;
    typedef actor_movement_operation                legacy_actor_movement_operation;
    typedef actor_grown_operation                   legacy_actor_grown_operation;
    typedef narrate_log_operation                   legacy_narrate_log_operation;
    typedef actor_talk_operation                    legacy_actor_talk_operation;
    typedef zone_create_operation                   legacy_zone_create_operation;
    typedef shutdown_siming_operation               legacy_shutdown_siming_operation;

    struct api_chain_properties
    {
        api_chain_properties() {}
        api_chain_properties( const chain::chain_properties& c ) : account_creation_fee( legacy_asset::from_asset( c.account_creation_fee ) ), maximum_block_size( c.maximum_block_size ), proposal_adopted_votes_threshold( c.proposal_adopted_votes_threshold )
        {}

        operator legacy_chain_properties() const
        {
            legacy_chain_properties props;
            props.account_creation_fee = legacy_taiyi_asset::from_asset( asset( account_creation_fee ) );
            props.maximum_block_size = maximum_block_size;
            props.proposal_adopted_votes_threshold = proposal_adopted_votes_threshold;
            return props;
        }

        legacy_asset   account_creation_fee = legacy_asset::from_asset( asset(TAIYI_MIN_ACCOUNT_CREATION_FEE, YANG_SYMBOL) );
        uint32_t       maximum_block_size = TAIYI_MIN_BLOCK_SIZE_LIMIT * 2;
        uint32_t       proposal_adopted_votes_threshold = 1;
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
        legacy_withdraw_qi_operation( const withdraw_qi_operation& op ) : account( op.account ), qi( legacy_asset::from_asset(op.qi) )
        {}

        operator withdraw_qi_operation()const
        {
            withdraw_qi_operation op;
            op.account = account;
            op.qi = qi;
            return op;
        }

        account_name_type account;
        legacy_asset      qi;
    };

    struct legacy_siming_update_operation
    {
        legacy_siming_update_operation() {}
        legacy_siming_update_operation( const siming_update_operation& op ) : owner( op.owner ), url( op.url ), block_signing_key(op.block_signing_key )
        {
            props.account_creation_fee = legacy_asset::from_asset( op.props.account_creation_fee.to_asset< false >() );
            props.maximum_block_size = op.props.maximum_block_size;
            props.proposal_adopted_votes_threshold = op.props.proposal_adopted_votes_threshold;
        }

        operator siming_update_operation()const
        {
            siming_update_operation op;
            op.owner = owner;
            op.url = url;
            op.block_signing_key = block_signing_key;
            op.props.account_creation_fee = legacy_taiyi_asset::from_asset( asset( props.account_creation_fee ) );
            op.props.maximum_block_size = props.maximum_block_size;
            op.props.proposal_adopted_votes_threshold = props.proposal_adopted_votes_threshold;
            return op;
        }

        account_name_type       owner;
        string                  url;
        public_key_type         block_signing_key;
        api_chain_properties    props;
    };

    struct legacy_claim_reward_balance_operation
    {
        legacy_claim_reward_balance_operation() {}
        legacy_claim_reward_balance_operation( const claim_reward_balance_operation& op ) : account( op.account ), reward_yang(legacy_asset::from_asset( op.reward_yang ) ), reward_qi(legacy_asset::from_asset( op.reward_qi ) ), reward_feigang(legacy_asset::from_asset( op.reward_feigang ) )
        {}

        operator claim_reward_balance_operation()const
        {
            claim_reward_balance_operation op;
            op.account = account;
            op.reward_yang = reward_yang;
            op.reward_feigang = reward_feigang;
            return op;
        }

        account_name_type account;
        legacy_asset      reward_yang;
        legacy_asset      reward_qi;
        legacy_asset      reward_feigang;
    };

    struct legacy_delegate_qi_operation
    {
        legacy_delegate_qi_operation() {}
        legacy_delegate_qi_operation( const delegate_qi_operation& op ) : delegator( op.delegator ), delegatee(op.delegatee), qi( legacy_asset::from_asset( op.qi ) )
        {}

        operator delegate_qi_operation()const
        {
            delegate_qi_operation op;
            op.delegator = delegator;
            op.delegatee = delegatee;
            op.qi = qi;
            return op;
        }

        account_name_type delegator;
        account_name_type delegatee;
        legacy_asset      qi;
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
        legacy_return_qi_delegation_operation( const return_qi_delegation_operation& op ) : account( op.account ), qi(legacy_asset::from_asset( op.qi ) )
        {}

        operator return_qi_delegation_operation()const
        {
            return_qi_delegation_operation op;
            op.account = account;
            op.qi = qi;
            return op;
        }

        account_name_type account;
        legacy_asset      qi;
    };

    struct legacy_producer_reward_operation
    {
        legacy_producer_reward_operation() {}
        legacy_producer_reward_operation( const producer_reward_operation& op ) : producer( op.producer ), reward(legacy_asset::from_asset(op.reward ) )
        {}

        operator producer_reward_operation()const
        {
            producer_reward_operation op;
            op.producer = producer;
            op.reward = reward;
            return op;
        }

        account_name_type producer;
        legacy_asset      reward;
    };
        
    struct legacy_nfa_convert_resources_operation
    {
        legacy_nfa_convert_resources_operation() {}
        legacy_nfa_convert_resources_operation( const nfa_convert_resources_operation& op ) : nfa( op.nfa ), owner( op.owner ), qi(legacy_asset::from_asset( op.qi )), resource(legacy_asset::from_asset( op.resource )), is_qi_to_resource( op.is_qi_to_resource )
        {}

        operator nfa_convert_resources_operation()const
        {
            nfa_convert_resources_operation op;
            op.nfa = nfa;
            op.owner = owner;
            op.qi = qi;
            op.resource = resource;
            op.is_qi_to_resource = is_qi_to_resource;
            return op;
        }

        int64_t             nfa;
        account_name_type   owner;
        legacy_asset        qi;
        legacy_asset        resource;
        bool                is_qi_to_resource;
    };

    struct legacy_nfa_asset_transfer_operation
    {
        legacy_nfa_asset_transfer_operation(){}
        legacy_nfa_asset_transfer_operation( const nfa_asset_transfer_operation& op )
            : from(op.from), from_owner(op.from_owner), to(op.to), to_owner(op.to_owner), amount(legacy_asset::from_asset(op.amount))
        {}

        operator nfa_asset_transfer_operation()const
        {
            nfa_asset_transfer_operation op;
            op.from = from;
            op.from_owner = from_owner;
            op.to = to;
            op.to_owner = to_owner;
            op.amount = amount;
            return op;
        }

        int64_t             from;
        account_name_type   from_owner;
        int64_t             to;
        account_name_type   to_owner;
        legacy_asset        amount;
    };
    
    struct legacy_nfa_deposit_withdraw_operation
    {
        legacy_nfa_deposit_withdraw_operation(){}
        legacy_nfa_deposit_withdraw_operation( const nfa_deposit_withdraw_operation& op )
            : nfa(op.nfa), account(op.account), deposited(legacy_asset::from_asset(op.deposited)), withdrawn(legacy_asset::from_asset(op.withdrawn))
        {}

        operator nfa_deposit_withdraw_operation()const
        {
            nfa_deposit_withdraw_operation op;
            op.nfa = nfa;
            op.account = account;
            op.deposited = deposited;
            op.withdrawn = withdrawn;
            return op;
        }

        int64_t             nfa;
        account_name_type   account;
        
        legacy_asset        deposited;
        legacy_asset        withdrawn;
    };


    struct legacy_reward_feigang_operation
    {
        legacy_reward_feigang_operation() {}
        legacy_reward_feigang_operation( const reward_feigang_operation& op ) : from(op.from), from_nfa(op.from_nfa), to(op.to), qi(legacy_asset::from_asset(op.qi))
        {}

        operator reward_feigang_operation()const
        {
            reward_feigang_operation op;
            op.from = from;
            op.from_nfa = from_nfa;
            op.to = to;
            op.qi = qi;
            return op;
        }

        account_name_type   from;
        int64_t             from_nfa;
        account_name_type   to;
        legacy_asset        qi;
    };
    
    struct legacy_reward_cultivation_operation
    {
        legacy_reward_cultivation_operation() {}
        legacy_reward_cultivation_operation( const reward_cultivation_operation& op ) : account( op.account ), nfa( op.nfa ), qi(legacy_asset::from_asset( op.qi ))
        {}

        operator reward_cultivation_operation()const
        {
            reward_cultivation_operation op;
            op.account = account;
            op.nfa = nfa;
            op.qi = qi;
            return op;
        }

        account_name_type   account;
        int64_t             nfa;
        legacy_asset        qi;
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
        legacy_delegate_qi_operation,
        legacy_siming_set_properties_operation,

        legacy_create_contract_operation,
        legacy_revise_contract_operation,
        legacy_release_contract_operation,
        legacy_call_contract_function_operation,

        legacy_action_nfa_operation,
    
        //**** virtual operations below this point
        legacy_fill_qi_withdraw_operation,
        legacy_hardfork_operation,
        legacy_return_qi_delegation_operation,
        legacy_producer_reward_operation,

        legacy_nfa_symbol_create_operation,
        legacy_nfa_symbol_authority_change_operation,
        legacy_nfa_create_operation,
        legacy_nfa_transfer_operation,
        legacy_nfa_active_approve_operation,
        legacy_nfa_convert_resources_operation,
        legacy_nfa_asset_transfer_operation,
        legacy_nfa_deposit_withdraw_operation,
    
        legacy_reward_feigang_operation,
        legacy_reward_cultivation_operation,

        legacy_tiandao_year_change_operation,
        legacy_tiandao_month_change_operation,
        legacy_tiandao_time_change_operation,
    
        legacy_actor_talent_rule_create_operation,
        legacy_actor_create_operation,
        legacy_actor_born_operation,
        legacy_actor_talent_trigger_operation,
        legacy_actor_movement_operation,
        legacy_actor_grown_operation,
        legacy_narrate_log_operation,
        legacy_actor_talk_operation,
        
        legacy_zone_create_operation,
    
        legacy_shutdown_siming_operation

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
        bool operator()( const release_contract_operation& op )const                { l_op = op; return true; }
        bool operator()( const call_contract_function_operation& op )const          { l_op = op; return true; }
        bool operator()( const nfa_symbol_create_operation& op )const               { l_op = op; return true; }
        bool operator()( const nfa_symbol_authority_change_operation& op )const     { l_op = op; return true; }
        bool operator()( const nfa_create_operation& op )const                      { l_op = op; return true; }
        bool operator()( const nfa_transfer_operation& op )const                    { l_op = op; return true; }
        bool operator()( const nfa_active_approve_operation& op )const              { l_op = op; return true; }
        bool operator()( const action_nfa_operation& op )const                      { l_op = op; return true; }
        bool operator()( const tiandao_year_change_operation& op )const             { l_op = op; return true; }
        bool operator()( const tiandao_month_change_operation& op )const            { l_op = op; return true; }
        bool operator()( const tiandao_time_change_operation& op )const             { l_op = op; return true; }
        bool operator()( const actor_talent_rule_create_operation& op )const        { l_op = op; return true; }
        bool operator()( const actor_create_operation& op )const                    { l_op = op; return true; }
        bool operator()( const actor_born_operation& op )const                      { l_op = op; return true; }
        bool operator()( const actor_talent_trigger_operation& op )const            { l_op = op; return true; }
        bool operator()( const actor_movement_operation& op )const                  { l_op = op; return true; }
        bool operator()( const actor_grown_operation& op )const                     { l_op = op; return true; }
        bool operator()( const narrate_log_operation& op )const                     { l_op = op; return true; }
        bool operator()( const actor_talk_operation& op )const                      { l_op = op; return true; }
        bool operator()( const zone_create_operation& op )const                     { l_op = op; return true; }
        bool operator()( const shutdown_siming_operation& op )const                 { l_op = op; return true; }

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

        bool operator()( const delegate_qi_operation& op )const
        {
            l_op = legacy_delegate_qi_operation( op );
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
        
        bool operator()( const nfa_convert_resources_operation& op )const
        {
            l_op = legacy_nfa_convert_resources_operation( op );
            return true;
        }
        
        bool operator()( const nfa_asset_transfer_operation& op )const
        {
            l_op = legacy_nfa_asset_transfer_operation( op );
            return true;
        }

        bool operator()( const nfa_deposit_withdraw_operation& op )const
        {
            l_op = legacy_nfa_deposit_withdraw_operation( op );
            return true;
        }

        bool operator()( const reward_feigang_operation& op )const
        {
            l_op = legacy_reward_feigang_operation( op );
            return true;
        }
        
        bool operator()( const reward_cultivation_operation& op )const
        {
            l_op = legacy_reward_cultivation_operation( op );
            return true;
        }

        // Should only be FA ops
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

        operation operator()( const legacy_delegate_qi_operation& op )const
        {
            return operation( delegate_qi_operation( op ) );
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
        
        template< typename T >
        operation operator()( const T& t )const
        {
            return operation( t );
        }
            
        operation operator()( const legacy_nfa_convert_resources_operation& op )const
        {
            return operation( nfa_convert_resources_operation( op ) );
        }
        
        operation operator()( const legacy_nfa_asset_transfer_operation& op )const
        {
            return operation( nfa_asset_transfer_operation( op ) );
        }

        operation operator()( const legacy_nfa_deposit_withdraw_operation& op )const
        {
            return operation( nfa_deposit_withdraw_operation( op ) );
        }

        operation operator()( const legacy_reward_feigang_operation& op )const
        {
            return operation( reward_feigang_operation( op ) );
        }
        
        operation operator()( const legacy_reward_cultivation_operation& op )const
        {
            return operation( reward_cultivation_operation( op ) );
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

FC_REFLECT( taiyi::plugins::baiyujing_api::api_chain_properties, (account_creation_fee)(maximum_block_size)(proposal_adopted_votes_threshold) )

FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_account_create_operation, (fee)(creator)(new_account_name)(owner)(active)(posting)(memo_key)(json_metadata) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_transfer_operation, (from)(to)(amount)(memo) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_transfer_to_qi_operation, (from)(to)(amount) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_withdraw_qi_operation, (account)(qi) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_siming_update_operation, (owner)(url)(block_signing_key)(props) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_claim_reward_balance_operation, (account)(reward_yang)(reward_qi)(reward_feigang) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_delegate_qi_operation, (delegator)(delegatee)(qi) );
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_fill_qi_withdraw_operation, (from_account)(to_account)(withdrawn)(deposited) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_return_qi_delegation_operation, (account)(qi) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_producer_reward_operation, (producer)(reward) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_nfa_convert_resources_operation, (nfa)(owner)(qi)(resource)(is_qi_to_resource) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_nfa_asset_transfer_operation, (from)(from_owner)(to)(to_owner)(amount) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_nfa_deposit_withdraw_operation, (nfa)(account)(deposited)(withdrawn) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_reward_feigang_operation, (from)(from_nfa)(to)(qi) )
FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_reward_cultivation_operation, (account)(nfa)(qi) )

FC_REFLECT_TYPENAME( taiyi::plugins::baiyujing_api::legacy_operation )

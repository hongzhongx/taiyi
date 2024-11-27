#pragma once
#include <protocol/base.hpp>
#include <protocol/block_header.hpp>
#include <protocol/asset.hpp>
#include <protocol/validation.hpp>
#include <protocol/legacy_asset.hpp>
#include <protocol/lua_types.hpp>

namespace taiyi { namespace protocol {

    void validate_auth_size( const authority& a );

    struct account_create_operation : public base_operation
    {
        asset             fee;
        account_name_type creator;
        account_name_type new_account_name;
        authority         owner;
        authority         active;
        authority         posting;
        public_key_type   memo_key;
        string            json_metadata;

        void validate()const;
        void get_required_active_authorities( flat_set<account_name_type>& a )const { a.insert(creator); }
    };

    struct account_update_operation : public base_operation
    {
        account_name_type             account;
        optional< authority >         owner;
        optional< authority >         active;
        optional< authority >         posting;
        public_key_type               memo_key;
        string                        json_metadata;

        void validate()const;
        void get_required_owner_authorities( flat_set<account_name_type>& a )const { if( owner ) a.insert( account ); }
        void get_required_active_authorities( flat_set<account_name_type>& a )const { if( !owner ) a.insert( account ); }
    };

    /**
     * @ingroup operations
     *
     * @brief Transfers YANG from one account to another.
     */
    struct transfer_operation : public base_operation
    {
        account_name_type from;
        /// Account to transfer asset to
        account_name_type to;
        /// The amount of asset to transfer from @ref from to @ref to
        asset             amount;

        /// The memo is plain-text, any encryption on the memo is up to
        /// a higher level protocol.
        string            memo;

        void validate()const;
        void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(from); }
    };

    /**
     *  This operation converts liquid token (YANG) into VFS (Vesting Fund Shares,
     *  QI) at the current exchange rate. With this operation it is possible to
     *  give another account qi shares so that faucets can pre-fund new accounts with qi shares.
     */
    struct transfer_to_qi_operation : public base_operation
    {
        account_name_type from;
        account_name_type to;      ///< if null, then same as from
        asset             amount;  ///< must be YANG

        void validate()const;
        void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(from); }
    };

    /**
     * At any given point in time an account can be withdrawing from their
     * qi shares. A user may change the number of shares they wish to
     * cash out at any time between 0 and their total qi stake.
     *
     * After applying this operation, qi will be withdrawn
     * at a rate of qi/104 per week for two years starting
     * one week after this operation is included in the blockchain.
     *
     * This operation is not valid if the user has no qi shares.
     */
    struct withdraw_qi_operation : public base_operation
    {
        account_name_type account;
        asset             qi;

        void validate()const;
        void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(account); }
    };

    /**
     * Allows an account to setup a qi withdraw but with the additional
     * request for the funds to be transferred directly to another account's
     * balance rather than the withdrawing account. In addition, those funds
     * can be immediately vested again, circumventing the conversion from
     * qi to taiyi and back, guaranteeing they maintain their value.
     */
    struct set_withdraw_qi_route_operation : public base_operation
    {
        account_name_type from_account;
        account_name_type to_account;
        uint16_t          percent = 0;
        bool              auto_vest = false;

        void validate()const;
        void get_required_active_authorities( flat_set<account_name_type>& a )const { a.insert( from_account ); }
    };

    /**
     * Simings must vote on how to set certain chain properties to ensure a smooth
     * and well functioning network.  Any time @owner is in the active set of simings these
     * properties will be used to control the blockchain configuration.
     */
    struct legacy_chain_properties
    {
        /**
         *  This fee, paid in TAIYI, is converted into QI SHARES for the new account. Accounts
         *  without qi shares cannot earn usage rations and therefore are powerless. This minimum
         *  fee requires all accounts to have some kind of commitment to the network that includes the
         *  ability to vote and make transactions.
         */
        legacy_taiyi_asset account_creation_fee = legacy_taiyi_asset::from_amount( TAIYI_MIN_ACCOUNT_CREATION_FEE );

        /**
         *  This simings vote for the maximum_block_size which is used by the network
         *  to tune rate limiting and capacity
         */
        uint32_t          maximum_block_size = TAIYI_MIN_BLOCK_SIZE_LIMIT * 2;

        template< bool force_canon >
        void validate()const
        {
            if( force_canon )
            {
                FC_ASSERT( account_creation_fee.symbol.is_canon() );
            }
            FC_ASSERT( account_creation_fee.amount >= TAIYI_MIN_ACCOUNT_CREATION_FEE);
            FC_ASSERT( maximum_block_size >= TAIYI_MIN_BLOCK_SIZE_LIMIT);
        }
    };

    /**
     *  Users who wish to become a siming must pay a fee acceptable to
     *  the current simings to apply for the position and allow adoring
     *  to begin.
     *
     *  If the owner isn't a siming they will become a siming.  Simings
     *  are charged a fee equal to 1 weeks worth of siming pay which in
     *  turn is derived from the current share supply.  The fee is
     *  only applied if the owner is not already a siming.
     *
     *  If the block_signing_key is null then the siming is removed from
     *  contention.  The network will pick the top 21 simings for
     *  producing blocks.
     */
    struct siming_update_operation : public base_operation
    {
        account_name_type owner;
        string            url;
        public_key_type   block_signing_key;
        legacy_chain_properties  props;
        asset             fee; ///< the fee paid to register a new siming, should be 10x current block production pay

        void validate()const;
        void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(owner); }
    };

    struct siming_set_properties_operation : public base_operation
    {
        account_name_type                   owner;
        flat_map< string, vector< char > >  props;
        extensions_type                     extensions;

        void validate()const;
        void get_required_authorities( vector< authority >& a )const
        {
            auto key_itr = props.find( "key" );

            if( key_itr != props.end() )
            {
                public_key_type signing_key;
                fc::raw::unpack_from_vector( key_itr->second, signing_key );
                a.push_back( authority( 1, signing_key, 1 ) );
            }
            else
                a.push_back( authority( 1, TAIYI_NULL_ACCOUNT, 1 ) ); // The null account auth is impossible to satisfy
        }
    };

    /**
     * All accounts with a VFS can adore for or against any siming.
     *
     * If a proxy is specified then all existing adores are removed.
     */
    struct account_siming_adore_operation : public base_operation
    {
        account_name_type account;
        account_name_type siming;
        bool              approve = true;

        void validate() const;
        void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(account); }
    };

    struct account_siming_proxy_operation : public base_operation
    {
        account_name_type account;
        account_name_type proxy;

        void validate()const;
        void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(account); }
    };

    /**
     * @brief provides a generic way to add higher level protocols on top of siming consensus
     * @ingroup operations
     *
     * There is no validation for this operation other than that required auths are valid
     */
    struct custom_operation : public base_operation
    {
        flat_set< account_name_type > required_auths;
        uint16_t                      id = 0;
        vector< char >                data;

        void validate()const;
        void get_required_active_authorities( flat_set<account_name_type>& a )const{ for( const auto& i : required_auths ) a.insert(i); }
    };

    /** serves the same purpose as custom_operation but also supports required posting authorities. Unlike custom_operation,
     * this operation is designed to be human readable/developer friendly.
     **/
    struct custom_json_operation : public base_operation
    {
        flat_set< account_name_type > required_auths;
        flat_set< account_name_type > required_posting_auths;
        custom_id_type                id; ///< must be less than 32 characters long
        string                        json; ///< must be proper utf8 / JSON string.

        void validate()const;
        void get_required_active_authorities( flat_set<account_name_type>& a )const{ for( const auto& i : required_auths ) a.insert(i); }
        void get_required_posting_authorities( flat_set<account_name_type>& a )const{ for( const auto& i : required_posting_auths ) a.insert(i); }
    };

    /**
     * All account recovery requests come from a listed recovery account. This
     * is secure based on the assumption that only a trusted account should be
     * a recovery account. It is the responsibility of the recovery account to
     * verify the identity of the account holder of the account to recover by
     * whichever means they have agreed upon. The blockchain assumes identity
     * has been verified when this operation is broadcast.
     *
     * This operation creates an account recovery request which the account to
     * recover has 24 hours to respond to before the request expires and is
     * invalidated.
     *
     * There can only be one active recovery request per account at any one time.
     * Pushing this operation for an account to recover when it already has
     * an active request will either update the request to a new new owner authority
     * and extend the request expiration to 24 hours from the current head block
     * time or it will delete the request. To cancel a request, simply set the
     * weight threshold of the new owner authority to 0, making it an open authority.
     *
     * Additionally, the new owner authority must be satisfiable. In other words,
     * the sum of the key weights must be greater than or equal to the weight
     * threshold.
     *
     * This operation only needs to be signed by the the recovery account.
     * The account to recover confirms its identity to the blockchain in
     * the recover account operation.
     */
    struct request_account_recovery_operation : public base_operation
    {
        account_name_type recovery_account;       ///< The recovery account is listed as the recovery account on the account to recover.

        account_name_type account_to_recover;     ///< The account to recover. This is likely due to a compromised owner authority.

        authority         new_owner_authority;    ///< The new owner authority the account to recover wishes to have. This is secret
                                                  ///< known by the account to recover and will be confirmed in a recover_account_operation

        extensions_type   extensions;             ///< Extensions. Not currently used.

        void validate() const;
        void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert( recovery_account ); }
    };

    /**
    * Recover an account to a new authority using a previous authority and verification
    * of the recovery account as proof of identity. This operation can only succeed
    * if there was a recovery request sent by the account's recover account.
    *
    * In order to recover the account, the account holder must provide proof
    * of past ownership and proof of identity to the recovery account. Being able
    * to satisfy an owner authority that was used in the past 30 days is sufficient
    * to prove past ownership. The get_owner_history function in the database API
    * returns past owner authorities that are valid for account recovery.
    *
    * Proving identity is an off chain contract between the account holder and
    * the recovery account. The recovery request contains a new authority which
    * must be satisfied by the account holder to regain control. The actual process
    * of verifying authority may become complicated, but that is an application
    * level concern, not a blockchain concern.
    *
    * This operation requires both the past and future owner authorities in the
    * operation because neither of them can be derived from the current chain state.
    * The operation must be signed by keys that satisfy both the new owner authority
    * and the recent owner authority. Failing either fails the operation entirely.
    *
    * If a recovery request was made inadvertantly, the account holder should
    * contact the recovery account to have the request deleted.
    *
    * The two setp combination of the account recovery request and recover is
    * safe because the recovery account never has access to secrets of the account
    * to recover. They simply act as an on chain endorsement of off chain identity.
    * In other systems, a fork would be required to enforce such off chain state.
    * Additionally, an account cannot be permanently recovered to the wrong account.
    * While any owner authority from the past 30 days can be used, including a compromised
    * authority, the account can be continually recovered until the recovery account
    * is confident a combination of uncompromised authorities were used to
    * recover the account. The actual process of verifying authority may become
    * complicated, but that is an application level concern, not the blockchain's
    * concern.
    */
    struct recover_account_operation : public base_operation
    {
        account_name_type account_to_recover;        ///< The account to be recovered

        authority         new_owner_authority;       ///< The new owner authority as specified in the request account recovery operation.

        authority         recent_owner_authority;    ///< A previous owner authority that the account holder will use to prove past ownership of the account to be recovered.

        extensions_type   extensions;                ///< Extensions. Not currently used.

        void validate() const;
        void get_required_authorities( vector< authority >& a )const
        {
            a.push_back( new_owner_authority );
            a.push_back( recent_owner_authority );
        }
    };

    /**
    * Each account lists another account as their recovery account.
    * The recovery account has the ability to create account_recovery_requests
    * for the account to recover. An account can change their recovery account
    * at any time with a 30 day delay. This delay is to prevent
    * an attacker from changing the recovery account to a malicious account
    * during an attack. These 30 days match the 30 days that an
    * owner authority is valid for recovery purposes.
    *
    * On account creation the recovery account is set to the creator of
    * the account (The account that pays the creation fee and is a signer on 
    * the transaction). An account with no recovery
    * has the top adored siming as a recovery account, at the time the recover
    * request is created. Note: This does mean the effective recovery account
    * of an account with no listed recovery account can change at any time as
    * siming adore weights. The top adored siming is explicitly the most trusted
    * siming according to stake.
    */
    struct change_recovery_account_operation : public base_operation
    {
        account_name_type account_to_recover;     ///< The account that would be recovered in case of compromise
        account_name_type new_recovery_account;   ///< The account that creates the recover request
        extensions_type   extensions;             ///< Extensions. Not currently used.

        void get_required_owner_authorities( flat_set<account_name_type>& a )const{ a.insert( account_to_recover ); }
        void validate() const;
    };

    struct decline_adoring_rights_operation : public base_operation
    {
        account_name_type account;
        bool              decline = true;

        void get_required_owner_authorities( flat_set<account_name_type>& a )const{ a.insert( account ); }
        void validate() const;
    };

    struct claim_reward_balance_operation : public base_operation
    {
        account_name_type account;
        asset             reward_yang;
        asset             reward_qi;

        void get_required_posting_authorities( flat_set< account_name_type >& a )const{ a.insert( account ); }
        void validate() const;
    };

    /**
    * Delegate qi shares from one account to the other. The qi shares are still owned
    * by the original account, but content voting rights and bandwidth allocation are transferred
    * to the receiving account. This sets the delegation to `qi`, increasing it or
    * decreasing it as needed. (i.e. a delegation of 0 removes the delegation)
    *
    * When a delegation is removed the shares are placed in limbo for a week to prevent a satoshi
    * of QI from voting on the same content twice.
    */
    struct delegate_qi_operation : public base_operation
    {
        account_name_type delegator;        ///< The account delegating qi shares
        account_name_type delegatee;        ///< The account receiving qi shares
        asset             qi;   ///< The amount of qi shares delegated

        void get_required_active_authorities( flat_set< account_name_type >& a ) const { a.insert( delegator ); }
        void validate() const;
    };

    // contracts
    struct create_contract_operation : public base_operation
    {
        account_name_type   owner;      // 合约创建者
        string              name;       // 合约名字
        string              data;       // 合约内容
        public_key_type     contract_authority;//合约权限
        vector<string>      extensions;
       
        void validate()const;
        void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(owner); }
    };

    struct revise_contract_operation : public base_operation
    {
        account_name_type   reviser;        // 合约创建者
        string              contract_name;  // 合约名字
        string              data;           // 合约内容
        vector<string>      extensions;
        
        void validate()const;
        void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(reviser); }
    };

    struct call_contract_function_operation : public base_operation
    {
        account_name_type   caller;        // 合约调用者
        account_name_type   creator;
        string              contract_name; // 合约名
        string              function_name; // 目标函数名
        vector<lua_types>   value_list;    // 参数列表
        vector<string>      extensions;
        
        void validate()const;
        void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(caller); }
    };

    struct create_nfa_symbol_operation : public base_operation
    {
        account_name_type   creator;
        string              symbol;
        string              describe;
        string              default_contract;

        void validate()const;
        void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(creator); }
    };

    struct create_nfa_operation : public base_operation
    {
        account_name_type   creator;
        string              symbol;
        
        void validate()const;
        void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(creator); }
    };

    struct transfer_nfa_operation : public base_operation
    {
        account_name_type       from;
        account_name_type       to;

        int64_t                 id; ///nfa id
        
        void validate()const;
        void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(from); }
    };
        
    struct action_nfa_operation : public base_operation
    {
        account_name_type   owner;      ///nfa owner
        int64_t             id;         ///nfa id
        string              action;     ///action name
        vector<lua_types>   value_list; ///action function parameter value list
        
        void validate()const;
        void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(owner); }
    };
    
    struct create_actor_talent_rule_operation : public base_operation
    {
        account_name_type   creator;
        string              contract;

        void validate()const;
        void get_required_posting_authorities( flat_set<account_name_type>& a )const{ a.insert(creator); }
    };
    
    struct create_actor_operation : public base_operation
    {
        asset             fee;          ///< must be QI
        account_name_type creator;
        string            family_name;
        string            last_name;

        void validate()const;
        void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(creator); }
    };

    struct create_zone_operation : public base_operation
    {
        asset             fee;          ///< must be QI
        account_name_type creator;
        string            name;

        void validate()const;
        void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(creator); }
    };

} } // taiyi::protocol

FC_REFLECT( taiyi::protocol::legacy_chain_properties, (account_creation_fee)(maximum_block_size))
FC_REFLECT( taiyi::protocol::account_create_operation, (fee)(creator)(new_account_name)(owner)(active)(posting)(memo_key)(json_metadata) )
FC_REFLECT( taiyi::protocol::account_update_operation, (account)(owner)(active)(posting)(memo_key)(json_metadata) )
FC_REFLECT( taiyi::protocol::transfer_operation, (from)(to)(amount)(memo) )
FC_REFLECT( taiyi::protocol::transfer_to_qi_operation, (from)(to)(amount) )
FC_REFLECT( taiyi::protocol::withdraw_qi_operation, (account)(qi) )
FC_REFLECT( taiyi::protocol::set_withdraw_qi_route_operation, (from_account)(to_account)(percent)(auto_vest) )
FC_REFLECT( taiyi::protocol::siming_update_operation, (owner)(url)(block_signing_key)(props)(fee) )
FC_REFLECT( taiyi::protocol::siming_set_properties_operation, (owner)(props)(extensions) )
FC_REFLECT( taiyi::protocol::account_siming_adore_operation, (account)(siming)(approve) )
FC_REFLECT( taiyi::protocol::account_siming_proxy_operation, (account)(proxy) )
FC_REFLECT( taiyi::protocol::custom_operation, (required_auths)(id)(data) )
FC_REFLECT( taiyi::protocol::custom_json_operation, (required_auths)(required_posting_auths)(id)(json) )
FC_REFLECT( taiyi::protocol::request_account_recovery_operation, (recovery_account)(account_to_recover)(new_owner_authority)(extensions) );
FC_REFLECT( taiyi::protocol::recover_account_operation, (account_to_recover)(new_owner_authority)(recent_owner_authority)(extensions) );
FC_REFLECT( taiyi::protocol::change_recovery_account_operation, (account_to_recover)(new_recovery_account)(extensions) );
FC_REFLECT( taiyi::protocol::decline_adoring_rights_operation, (account)(decline) );
FC_REFLECT( taiyi::protocol::claim_reward_balance_operation, (account)(reward_yang)(reward_qi) )
FC_REFLECT( taiyi::protocol::delegate_qi_operation, (delegator)(delegatee)(qi) );

FC_REFLECT( taiyi::protocol::create_contract_operation, (owner)(name)(data)(contract_authority)(extensions) )
FC_REFLECT( taiyi::protocol::revise_contract_operation, (reviser)(contract_name)(data)(extensions) )
FC_REFLECT( taiyi::protocol::call_contract_function_operation, (caller)(creator)(contract_name)(function_name)(value_list)(extensions) )

FC_REFLECT( taiyi::protocol::create_nfa_symbol_operation, (creator)(symbol)(describe)(default_contract) )
FC_REFLECT( taiyi::protocol::create_nfa_operation, (creator)(symbol) )
FC_REFLECT( taiyi::protocol::transfer_nfa_operation, (from)(to)(id) )
FC_REFLECT( taiyi::protocol::action_nfa_operation, (owner)(id)(action)(value_list) )

FC_REFLECT( taiyi::protocol::create_actor_talent_rule_operation, (creator)(contract) )
FC_REFLECT( taiyi::protocol::create_actor_operation, (fee)(creator)(family_name)(last_name) )
FC_REFLECT( taiyi::protocol::create_zone_operation, (fee)(creator)(name) )

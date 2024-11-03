#pragma once

#include <plugins/baiyujing_api/baiyujing_api.hpp>

#include <xuanpin/remote_node_api.hpp>

#include <utilities/key_conversion.hpp>

#include <fc/macros.hpp>
#include <fc/real128.hpp>
#include <fc/api.hpp>

namespace taiyi { namespace xuanpin {

    using namespace std;
    
    using namespace taiyi::utilities;
    using namespace taiyi::protocol;
    
    typedef uint16_t transaction_handle_type;
    
    struct brain_key_info
    {
        string              brain_priv_key;
        public_key_type     pub_key;
        string              wif_priv_key;
    };
    
    struct xuanpin_data
    {
        vector<char>        cipher_keys; /** encrypted keys */
        
        string              ws_server = "ws://localhost:8090";
        string              ws_user;
        string              ws_password;
    };
    
    enum authority_type
    {
        owner,
        active,
        posting
    };
    
    struct plain_keys
    {
        fc::sha512                  checksum;
        map<public_key_type,string> keys;
    };

    namespace detail {
        class xuanpin_api_impl;
    }

    /**
     * This xuanpin assumes it is connected to the database server with a high-bandwidth, low-latency connection and
     * performs minimal caching. This API could be provided locally to be used by a web interface.
     */
    class xuanpin_api
    {
    public:
        fc::signal<void(bool)> lock_changed;
        std::shared_ptr<detail::xuanpin_api_impl> my;
        
    public:
        xuanpin_api( const xuanpin_data& initial_data, const taiyi::protocol::chain_id_type& _taiyi_chain_id, fc::api< remote_node_api > rapi );
        virtual ~xuanpin_api();
        
        bool copy_xuanpin_file( string destination_filename );
        
    
        /** Returns a list of all commands supported by the xuanpin API.
         *
         * This lists each command, along with its arguments and return types.
         * For more detailed help on a single command, use \c get_help()
         *
         * @returns a multi-line string suitable for displaying on a terminal
         */
        string help() const;
        
        /**
         * Returns info about the current state of the blockchain
         */
        variant info();
        
        /** Returns info such as client version, git version of taiyi/fc, version of boost, openssl.
         * @returns compile time info and client and dependencies versions
         */
        variant_object about() const;
        
        /** Returns the information about a block
         *
         * @param num Block num
         *
         * @returns Public block data on the blockchain
         */
        optional< baiyujing_api::legacy_signed_block > get_block( uint32_t num );
        
        /** Returns sequence of operations included/generated in a specified block
         *
         * @param block_num Block height of specified block
         * @param only_virtual Whether to only return virtual operations
         */
        vector< baiyujing_api::api_operation_object > get_ops_in_block( uint32_t block_num, bool only_virtual = true );
        
        /**
         * Returns the list of simings producing blocks in the current round (21 Blocks)
         */
        vector< account_name_type > get_active_simings() const;
        
        /**
         * Returns the state info associated with the URL
         */
        baiyujing_api::state get_state( string url );
        
        /**
         * Returns qi withdraw routes for an account.
         *
         * @param account Account to query routes
         * @param type Withdraw type type [incoming, outgoing, all]
         */
        vector< database_api::api_withdraw_qi_route_object > get_withdraw_routes( string account, baiyujing_api::withdraw_route_type type = baiyujing_api::all ) const;
        
        /**
         *  Gets the account information for all accounts for which this xuanpin has a private key
         */
        vector< baiyujing_api::api_account_object > list_my_accounts();
        
        /** Lists all accounts registered in the blockchain.
         * This returns a list of all account names and their account ids, sorted by account name.
         *
         * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all accounts,
         * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
         * the last account name returned as the \c lowerbound for the next \c list_accounts() call.
         *
         * @param lowerbound the name of the first account to return.  If the named account does not exist,
         *                   the list will start at the account that comes after \c lowerbound
         * @param limit the maximum number of accounts to return (max: 1000)
         * @returns a list of accounts mapping account names to account ids
         */
        vector< account_name_type > list_accounts(const string& lowerbound, uint32_t limit);
        
        /** Returns the block chain's rapidly-changing properties.
         * The returned object contains information that changes every block interval
         * such as the head block number, the next siming, etc.
         * @see \c get_global_properties() for less-frequently changing properties
         * @returns the dynamic global properties
         */
        baiyujing_api::extended_dynamic_global_properties get_dynamic_global_properties() const;
        
        /** Returns information about the given account.
         *
         * @param account_name the name of the account to provide information about
         * @returns the public account data stored in the blockchain
         */
        baiyujing_api::api_account_object get_account( string account_name ) const;
        
        /** Returns the current xuanpin filename.
         *
         * This is the filename that will be used when automatically saving the xuanpin.
         *
         * @see set_xuanpin_filename()
         * @return the xuanpin filename
         */
        string get_xuanpin_filename() const;
        
        /**
         * Get the WIF private key corresponding to a public key.  The
         * private key must already be in the xuanpin.
         */
        string get_private_key( public_key_type pubkey ) const;
        
        /**
         *  @param account  - the name of the account to retrieve key for
         *  @param role     - active | owner | posting | memo
         *  @param password - the password to be used at key generation
         *  @return public key corresponding to generated private key, and private key in WIF format.
         */
        pair<public_key_type,string> get_private_key_from_password( string account, string role, string password ) const;
        
        
        /**
         * Returns transaction by ID.
         */
        baiyujing_api::legacy_signed_transaction get_transaction( transaction_id_type trx_id ) const;
        vector<taiyi::chain::operation_result> get_transaction_results( transaction_id_type trx_id ) const;
        
        /** Checks whether the xuanpin has just been created and has not yet had a password set.
         *
         * Calling \c set_password will transition the xuanpin to the locked state.
         * @return true if the xuanpin is new
         * @ingroup Xuanpin Management
         */
        bool is_new() const;
        
        /** Checks whether the xuanpin is locked (is unable to use its private keys).
         *
         * This state can be changed by calling \c lock() or \c unlock().
         * @return true if the xuanpin is locked
         * @ingroup Xuanpin Management
         */
        bool is_locked() const;
        
        /** Locks the xuanpin immediately.
         * @ingroup Xuanpin Management
         */
        void lock();
        
        /** Unlocks the xuanpin.
         *
         * The xuanpin remain unlocked until the \c lock is called
         * or the program exits.
         * @param password the password previously set with \c set_password()
         * @ingroup Xuanpin Management
         */
        void unlock(string password);
        
        /** Sets a new password on the xuanpin.
         *
         * The xuanpin must be either 'new' or 'unlocked' to
         * execute this command.
         * @ingroup Xuanpin Management
         */
        void set_password(string password);
        
        /** Dumps all private keys owned by the xuanpin.
         *
         * The keys are printed in WIF format.  You can import these keys into another xuanpin
         * using \c import_key()
         * @returns a map containing the private keys, indexed by their public key
         */
        map<public_key_type, string> list_keys();
        
        /** Returns detailed help on a single API command.
         * @param method the name of the API command you want help with
         * @returns a multi-line string suitable for displaying on a terminal
         */
        string gethelp(const string& method) const;
        
        /** Loads a specified Taiyi xuanpin.
         *
         * The current xuanpin is closed before the new xuanpin is loaded.
         *
         * @warning This does not change the filename that will be used for future
         * xuanpin writes, so this may cause you to overwrite your original
         * xuanpin unless you also call \c set_xuanpin_filename()
         *
         * @param xuanpin_filename the filename of the xuanpin JSON file to load.
         *                        If \c xuanpin_filename is empty, it reloads the
         *                        existing xuanpin file
         * @returns true if the specified xuanpin is loaded
         */
        bool load_xuanpin_file(string xuanpin_filename = "");
        
        /** Saves the current xuanpin to the given filename.
         *
         * @warning This does not change the xuanpin filename that will be used for future
         * writes, so think of this function as 'Save a Copy As...' instead of
         * 'Save As...'.  Use \c set_xuanpin_filename() to make the filename
         * persist.
         * @param xuanpin_filename the filename of the new xuanpin JSON file to create
         *                        or overwrite.  If \c xuanpin_filename is empty,
         *                        save to the current filename.
         */
        void save_xuanpin_file(string xuanpin_filename = "");
        
        /** Sets the xuanpin filename used for future writes.
         *
         * This does not trigger a save, it only changes the default filename
         * that will be used the next time a save is triggered.
         *
         * @param xuanpin_filename the new filename to use for future saves
         */
        void set_xuanpin_filename(string xuanpin_filename);
        
        /** Suggests a safe brain key to use for creating your account.
         * \c create_account_with_brain_key() requires you to specify a 'brain key',
         * a long passphrase that provides enough entropy to generate cyrptographic
         * keys.  This function will suggest a suitably random string that should
         * be easy to write down (and, with effort, memorize).
         * @returns a suggested brain_key
         */
        brain_key_info suggest_brain_key() const;
        
        /** Converts a signed_transaction in JSON form to its binary representation.
         *
         * TODO: I don't see a broadcast_transaction() function, do we need one?
         *
         * @param tx the transaction to serialize
         * @returns the binary form of the transaction.  It will not be hex encoded,
         *          this returns a raw string that may have null characters embedded
         *          in it
         */
        string serialize_transaction(signed_transaction tx) const;
        
        /** Imports a WIF Private Key into the xuanpin to be used to sign transactions by an account.
         *
         * example: import_key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
         *
         * @param wif_key the WIF Private Key to import
         */
        bool import_key( string wif_key );
        
        /** Transforms a brain key to reduce the chance of errors when re-entering the key from memory.
         *
         * This takes a user-supplied brain key and normalizes it into the form used
         * for generating private keys.  In particular, this upper-cases all ASCII characters
         * and collapses multiple spaces into one.
         * @param s the brain key as supplied by the user
         * @returns the brain key in its normalized form
         */
        string normalize_brain_key(string s) const;
        
        /**
         *  This method will genrate new owner, active, and memo keys for the new account which
         *  will be controlable by this xuanpin. There is a fee associated with account creation
         *  that is paid by the creator. The current account creation fee can be found with the
         *  'info' xuanpin command.
         *
         *  @param creator The account creating the new account
         *  @param new_account_name The name of the new account
         *  @param json_meta JSON Metadata associated with the new account
         *  @param broadcast true if you wish to broadcast the transaction
         */
        baiyujing_api::legacy_signed_transaction create_account( string creator, string new_account_name, string json_meta, bool broadcast );
        
        /**
         * This method is used by faucets to create new accounts for other users which must
         * provide their desired keys. The resulting account may not be controllable by this
         * xuanpin. There is a fee associated with account creation that is paid by the creator.
         * The current account creation fee can be found with the 'info' xuanpin command.
         *
         * @param creator The account creating the new account
         * @param newname The name of the new account
         * @param json_meta JSON Metadata associated with the new account
         * @param owner public owner key of the new account
         * @param active public active key of the new account
         * @param posting public posting key of the new account
         * @param memo public memo key of the new account
         * @param broadcast true if you wish to broadcast the transaction
         */
        baiyujing_api::legacy_signed_transaction create_account_with_keys(string creator, string newname, string json_meta, public_key_type owner, public_key_type active, public_key_type posting, public_key_type memo, bool broadcast ) const;
        
        /**
         * This method updates the keys of an existing account.
         *
         * @param accountname The name of the account
         * @param json_meta New JSON Metadata to be associated with the account
         * @param owner New public owner key for the account
         * @param active New public active key for the account
         * @param posting New public posting key for the account
         * @param memo New public memo key for the account
         * @param broadcast true if you wish to broadcast the transaction
         */
        baiyujing_api::legacy_signed_transaction update_account(string accountname, string json_meta, public_key_type owner, public_key_type active, public_key_type posting, public_key_type memo, bool broadcast ) const;
        
        /**
         * This method updates the key of an authority for an exisiting account.
         * Warning: You can create impossible authorities using this method. The method
         * will fail if you create an impossible owner authority, but will allow impossible
         * active and posting authorities.
         *
         * @param account_name The name of the account whose authority you wish to update
         * @param type The authority type. e.g. owner, active, or posting
         * @param key The public key to add to the authority
         * @param weight The weight the key should have in the authority. A weight of 0 indicates the removal of the key.
         * @param broadcast true if you wish to broadcast the transaction.
         */
        baiyujing_api::legacy_signed_transaction update_account_auth_key(string account_name, authority_type type, public_key_type key, weight_type weight, bool broadcast );
        
        /**
         * This method updates the account of an authority for an exisiting account.
         * Warning: You can create impossible authorities using this method. The method
         * will fail if you create an impossible owner authority, but will allow impossible
         * active and posting authorities.
         *
         * @param account_name The name of the account whose authority you wish to update
         * @param type The authority type. e.g. owner, active, or posting
         * @param auth_account The account to add the the authority
         * @param weight The weight the account should have in the authority. A weight of 0 indicates the removal of the account.
         * @param broadcast true if you wish to broadcast the transaction.
         */
        baiyujing_api::legacy_signed_transaction update_account_auth_account(string account_name, authority_type type, string auth_account, weight_type weight, bool broadcast );
        
        /**
         * This method updates the weight threshold of an authority for an account.
         * Warning: You can create impossible authorities using this method as well
         * as implicitly met authorities. The method will fail if you create an implicitly
         * true authority and if you create an impossible owner authoroty, but will allow
         * impossible active and posting authorities.
         *
         * @param account_name The name of the account whose authority you wish to update
         * @param type The authority type. e.g. owner, active, or posting
         * @param threshold The weight threshold required for the authority to be met
         * @param broadcast true if you wish to broadcast the transaction
         */
        baiyujing_api::legacy_signed_transaction update_account_auth_threshold(string account_name, authority_type type, uint32_t threshold, bool broadcast );
        
        /**
         * This method updates the account JSON metadata
         *
         * @param account_name The name of the account you wish to update
         * @param json_meta The new JSON metadata for the account. This overrides existing metadata
         * @param broadcast ture if you wish to broadcast the transaction
         */
        baiyujing_api::legacy_signed_transaction update_account_meta(string account_name, string json_meta, bool broadcast );
        
        /**
         * This method updates the memo key of an account
         *
         * @param account_name The name of the account you wish to update
         * @param key The new memo public key
         * @param broadcast true if you wish to broadcast the transaction
         */
        baiyujing_api::legacy_signed_transaction update_account_memo_key(string account_name, public_key_type key, bool broadcast );
        
        /**
         * This method delegates QI from one account to another.
         *
         * @param delegator The name of the account delegating QI
         * @param delegatee The name of the account receiving QI
         * @param qi_shares The amount of QI to delegate
         * @param broadcast true if you wish to broadcast the transaction
         */
        baiyujing_api::legacy_signed_transaction delegate_qi_shares(string delegator, string delegatee, baiyujing_api::legacy_asset qi_shares, bool broadcast );
        
        /**
         *  This method is used to convert a JSON transaction to its transaction ID.
         */
        transaction_id_type get_transaction_id( const signed_transaction& trx )const { return trx.id(); }
        
        /** Lists all simings registered in the blockchain.
         * This returns a list of all account names that own simings, and the associated siming id,
         * sorted by name.  This lists simings whether they are currently adored in or not.
         *
         * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all simings,
         * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
         * the last siming name returned as the \c lowerbound for the next \c list_simings() call.
         *
         * @param lowerbound the name of the first siming to return.  If the named siming does not exist,
         *                   the list will start at the siming that comes after \c lowerbound
         * @param limit the maximum number of simings to return (max: 1000)
         * @returns a list of simings mapping siming names to siming ids
         */
        vector< account_name_type > list_simings(const string& lowerbound, uint32_t limit);
        
        /** Returns information about the given siming.
         * @param owner_account the name or id of the siming account owner, or the id of the siming
         * @returns the information about the siming stored in the block chain
         */
        optional< baiyujing_api::api_siming_object > get_siming(string owner_account);
        
        /**
         * Update a siming object owned by the given account.
         *
         * @param siming_name The name of the siming account.
         * @param url A URL containing some information about the siming.  The empty string makes it remain the same.
         * @param block_signing_key The new block signing public key.  The empty string disables block production.
         * @param props The chain properties the siming is voting on.
         * @param broadcast true if you wish to broadcast the transaction.
         */
        baiyujing_api::legacy_signed_transaction update_siming(string siming_name, string url, public_key_type block_signing_key, const baiyujing_api::api_chain_properties& props, bool broadcast = false);
        
        /** Set the adoring proxy for an account.
         *
         * If a user does not wish to take an active part in adoring, they can choose
         * to allow another account to adore their stake.
         *
         * Setting a adore proxy does not remove your previous adores from the blockchain,
         * they remain there but are ignored.  If you later null out your adore proxy,
         * your previous adores will take effect again.
         *
         * This setting can be changed at any time.
         *
         * @param account_to_modify the name or id of the account to update
         * @param proxy the name of account that should proxy to, or empty string to have no proxy
         * @param broadcast true if you wish to broadcast the transaction
         */
        baiyujing_api::legacy_signed_transaction set_adoring_proxy(string account_to_modify, string proxy, bool broadcast = false);
        
        /**
         * Vote for a siming to become a block producer. By default an account has not adored
         * positively or negatively for a siming. The account can either adore for with positively
         * adores or against with negative adores. The adore will remain until updated with another
         * adore. Vote strength is determined by the accounts qi shares.
         *
         * @param account_to_adore_with The account adoring for a siming
         * @param siming_to_adore_for The siming that is being adored for
         * @param approve true if the account is adoring for the account to be able to be a block produce
         * @param broadcast true if you wish to broadcast the transaction
         */
        baiyujing_api::legacy_signed_transaction adore_for_siming(string account_to_adore_with, string siming_to_adore_for, bool approve = true, bool broadcast = false);
        
        /**
         * Transfer funds from one account to another. YANG can be transferred.
         *
         * @param from The account the funds are coming from
         * @param to The account the funds are going to
         * @param amount The funds being transferred. i.e. "100.000 YANG"
         * @param memo A memo for the transactionm, encrypted with the to account's public memo key
         * @param broadcast true if you wish to broadcast the transaction
         */
        baiyujing_api::legacy_signed_transaction transfer(string from, string to, baiyujing_api::legacy_asset amount, string memo, bool broadcast = false);
        
        /**
         * Transfer YANG into a qi fund represented by qi shares (QI). QI are required to qi
         * for a minimum of one coin year and can be withdrawn once a week over a two year withdraw period.
         * QI are protected against dilution up until 90% of YANG is qi.
         *
         * @param from The account the YANG is coming from
         * @param to The account getting the QI
         * @param amount The amount of YANG to vest i.e. "100.00 YANG"
         * @param broadcast true if you wish to broadcast the transaction
         */
        baiyujing_api::legacy_signed_transaction transfer_to_qi(string from, string to, baiyujing_api::legacy_asset amount, bool broadcast = false);
        
        /**
         * Set up a qi withdraw request. The request is fulfilled once a week over the next two year (104 weeks).
         *
         * @param from The account the QI are withdrawn from
         * @param qi_shares The amount of QI to withdraw over the next two years. Each week (amount/104) shares are
         *    withdrawn and deposited back as TAIYI. i.e. "10.000000 QI"
         * @param broadcast true if you wish to broadcast the transaction
         */
        baiyujing_api::legacy_signed_transaction withdraw_qi(string from, baiyujing_api::legacy_asset qi_shares, bool broadcast = false );
        
        /**
         * Set up a qi withdraw route. When qi shares are withdrawn, they will be routed to these accounts
         * based on the specified weights.
         *
         * @param from The account the QI are withdrawn from.
         * @param to   The account receiving either QI or TAIYI.
         * @param percent The percent of the withdraw to go to the 'to' account. This is denoted in hundreths of a percent.
         *    i.e. 100 is 1% and 10000 is 100%. This value must be between 1 and 100000
         * @param auto_vest Set to true if the from account should receive the QI as QI, or false if it should receive
         *    them as TAIYI.
         * @param broadcast true if you wish to broadcast the transaction.
         */
        baiyujing_api::legacy_signed_transaction set_withdraw_qi_route(string from, string to, uint16_t percent, bool auto_vest, bool broadcast = false );
        
        /** Signs a transaction.
         *
         * Given a fully-formed transaction that is only lacking signatures, this signs
         * the transaction with the necessary keys and optionally broadcasts the transaction
         * @param tx the unsigned transaction
         * @param broadcast true if you wish to broadcast the transaction
         * @return the signed version of the transaction
         */
        baiyujing_api::legacy_signed_transaction sign_transaction(baiyujing_api::legacy_signed_transaction tx, bool broadcast = false);
        
        /** Returns an uninitialized object representing a given blockchain operation.
         *
         * This returns a default-initialized object of the given type; it can be used
         * during early development of the xuanpin when we don't yet have custom commands for
         * creating all of the operations the blockchain supports.
         *
         * Any operation the blockchain supports can be created using the transaction builder's
         * \c add_operation_to_builder_transaction() , but to do that from the CLI you need to
         * know what the JSON form of the operation looks like.  This will give you a template
         * you can fill in.  It's better than nothing.
         *
         * @param operation_type the type of operation to return, must be one of the operations defined in `chain/operations.hpp` (e.g., "global_parameters_update_operation")
         * @return a default-constructed operation of the given type
         */
        operation get_prototype_operation(string operation_type);
        
        /**
         * Sets the amount of time in the future until a transaction expires.
         */
        void set_transaction_expiration(uint32_t seconds);
        
        /**
         * Create an account recovery request as a recover account. The syntax for this command contains a serialized authority object
         * so there is an example below on how to pass in the authority.
         *
         * request_account_recovery "your_account" "account_to_recover" {"weight_threshold": 1,"account_auths": [], "key_auths": [["new_public_key",1]]} true
         *
         * @param recovery_account The name of your account
         * @param account_to_recover The name of the account you are trying to recover
         * @param new_authority The new owner authority for the recovered account. This should be given to you by the holder of the compromised or lost account.
         * @param broadcast true if you wish to broadcast the transaction
         */
        baiyujing_api::legacy_signed_transaction request_account_recovery(string recovery_account, string account_to_recover, authority new_authority, bool broadcast );
        
        /**
         * Recover your account using a recovery request created by your recovery account. The syntax for this commain contains a serialized
         * authority object, so there is an example below on how to pass in the authority.
         *
         * recover_account "your_account" {"weight_threshold": 1,"account_auths": [], "key_auths": [["old_public_key",1]]} {"weight_threshold": 1,"account_auths": [], "key_auths": [["new_public_key",1]]} true
         *
         * @param account_to_recover The name of your account
         * @param recent_authority A recent owner authority on your account
         * @param new_authority The new authority that your recovery account used in the account recover request.
         * @param broadcast true if you wish to broadcast the transaction
         */
        baiyujing_api::legacy_signed_transaction recover_account(string account_to_recover, authority recent_authority, authority new_authority, bool broadcast );
        
        /**
         * Change your recovery account after a 30 day delay.
         *
         * @param owner The name of your account
         * @param new_recovery_account The name of the recovery account you wish to have
         * @param broadcast true if you wish to broadcast the transaction
         */
        baiyujing_api::legacy_signed_transaction change_recovery_account(string owner, string new_recovery_account, bool broadcast );
        
        vector< database_api::api_owner_authority_history_object > get_owner_history( string account ) const;
        
        /**
         *  Account operations have sequence numbers from 0 to N where N is the most recent operation. This method
         *  returns operations in the range [from-limit, from]
         *
         *  @param account - account whose history will be returned
         *  @param from - the absolute sequence number, -1 means most recent, limit is the number of operations before from.
         *  @param limit - the maximum number of items that can be queried (0 to 1000], must be less than from
         */
        map< uint32_t, baiyujing_api::api_operation_object > get_account_history( string account, uint32_t from, uint32_t limit );
        
        std::map<string,std::function<string(fc::variant,const fc::variants&)>> get_result_formatters() const;
        
        void encrypt_keys();
        
        /**
         * Checks memos against private keys on account and imported in xuanpin
         */
        void check_memo( const string& memo, const baiyujing_api::api_account_object& account )const;
        
        /**
         *  Returns the encrypted memo if memo starts with '#' otherwise returns memo
         */
        string get_encrypted_memo( string from, string to, string memo );
        
        /**
         * Returns the decrypted memo if possible given xuanpin's known private keys
         */
        string decrypt_memo( string memo );
        
        baiyujing_api::legacy_signed_transaction decline_adoring_rights( string account, bool decline, bool broadcast );
        
        baiyujing_api::legacy_signed_transaction claim_reward_balance(string account, baiyujing_api::legacy_asset reward_yang, baiyujing_api::legacy_asset reward_qi, bool broadcast );
        
        /** get resources of accounts
         * This returns a list of all resource assets owned by account names.
         *
         * @param names the names of accounts.
         * @returns a list of resources
         */
        vector< baiyujing_api::api_resource_assets > get_account_resources ( vector< account_name_type > names );
                    
        //contracts
        baiyujing_api::legacy_signed_transaction create_contract(const account_name_type& owner, const string& name, const public_key_type& contract_authority, const string& data, bool broadcast);
        baiyujing_api::legacy_signed_transaction create_contract_from_file(const account_name_type& owner, const string& name, const public_key_type&  contract_authority, const string& filename, bool broadcast);
        
        baiyujing_api::legacy_signed_transaction revise_contract(const account_name_type& reviser, const string& name, const string& data, bool broadcast);
        baiyujing_api::legacy_signed_transaction revise_contract_from_file(const account_name_type& reviser, const string& name, const string& filename, bool broadcast);
        
        //value_list 目前仅支持string, bool, double, int64
        baiyujing_api::legacy_signed_transaction call_contract_function(const account_name_type& account, const string& contract_name, const string& function_name, const vector<fc::variant>& value_list, bool broadcast);
                
        baiyujing_api::legacy_signed_transaction create_nfa_symbol( const account_name_type& creator, const string& symbol, const string& describe, bool broadcast );
        baiyujing_api::legacy_signed_transaction create_nfa( const account_name_type& creator, const string& symbol, bool broadcast );
        baiyujing_api::legacy_signed_transaction transfer_nfa( const account_name_type& from, const account_name_type& to, int64_t nfa_id, bool broadcast );
        
    };
    
} }

FC_REFLECT( taiyi::xuanpin::xuanpin_data, (cipher_keys)(ws_server)(ws_user)(ws_password) )
FC_REFLECT( taiyi::xuanpin::brain_key_info, (brain_priv_key)(wif_priv_key) (pub_key) )
FC_REFLECT( taiyi::xuanpin::plain_keys, (checksum)(keys) )

FC_REFLECT_ENUM( taiyi::xuanpin::authority_type, (owner)(active)(posting) )

FC_API( taiyi::xuanpin::xuanpin_api,
    /// app api
    (help)(gethelp)
    (about)(is_new)(is_locked)(lock)(unlock)(set_password)
    (load_xuanpin_file)(save_xuanpin_file)

    /// key api
    (import_key)
    (suggest_brain_key)
    (list_keys)
    (get_private_key)
    (get_private_key_from_password)
    (normalize_brain_key)

    /// query api
    (info)
    (list_my_accounts)
    (list_accounts)
    (list_simings)
    (get_siming)
    (get_account)
    (get_block)
    (get_ops_in_block)
    (get_account_history)
    (get_state)
    (get_withdraw_routes)
    (get_account_resources)
    (get_owner_history)
    (get_active_simings)
    (get_transaction)
    (get_transaction_results)

    /// transaction api
    (create_account)
    (create_account_with_keys)
    (update_account)
    (update_account_auth_key)
    (update_account_auth_account)
    (update_account_auth_threshold)
    (update_account_meta)
    (update_account_memo_key)
    (delegate_qi_shares)
    (update_siming)
    (set_adoring_proxy)
    (adore_for_siming)
    (transfer)
    (transfer_to_qi)
    (withdraw_qi)
    (set_withdraw_qi_route)
    (request_account_recovery)
    (recover_account)
    (change_recovery_account)
    (decline_adoring_rights)
    (claim_reward_balance)
       
    //contracts
    (create_contract)
    (create_contract_from_file)
    (revise_contract)
    (revise_contract_from_file)
    (call_contract_function)
       
    //nfa
    (create_nfa_symbol)
    (create_nfa)
    (transfer_nfa)
       
    /// helper api
    (get_prototype_operation)
    (serialize_transaction)
    (sign_transaction)
    (set_transaction_expiration)
    (decrypt_memo)
    (get_encrypted_memo)
)

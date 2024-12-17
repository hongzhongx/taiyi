#pragma once

#include <plugins/baiyujing_api/baiyujing_api.hpp>

#include <danuo/remote_node_api.hpp>

#include <utilities/key_conversion.hpp>

#include <fc/macros.hpp>
#include <fc/real128.hpp>
#include <fc/api.hpp>

namespace fc {
    template<> struct get_typename<variant_object>  { static const char* name()  { return "variant_object";  } };
}

namespace taiyi { namespace danuo {
    
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
    
    struct danuo_data
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
        class danuo_api_impl;
    }

    /**
     * This danuo assumes it is connected to the database server with a high-bandwidth, low-latency connection and
     * performs minimal caching. This API could be provided locally to be used by a web interface.
     */
    class danuo_api
    {
    public:
        fc::signal<void(bool)> lock_changed;
        fc::signal<void(bool, string, string)> start_game_changed;
        
        std::shared_ptr<detail::danuo_api_impl> my;
        
    public:
        danuo_api( const danuo_data& initial_data, const taiyi::protocol::chain_id_type& _taiyi_chain_id, fc::api< remote_node_api > rapi );
        virtual ~danuo_api();
        
        bool copy_danuo_file( string destination_filename );
        
        
        /** Returns a list of all commands supported by the danuo API.
         *
         * This lists each command, along with its arguments and return types.
         * For more detailed help on a single command, use \c get_help()
         *
         * @returns a multi-line string suitable for displaying on a terminal
         */
        string help() const;
        
        /** Returns info such as client version, git version of taiyi/fc, version of boost, openssl.
         * @returns compile time info and client and dependencies versions
         */
        variant_object about() const;
        
        /** Returns the current danuo filename.
         *
         * This is the filename that will be used when automatically saving the danuo.
         *
         * @see set_danuo_filename()
         * @return the danuo filename
         */
        string get_danuo_filename() const;
        
        /**
         * Get the WIF private key corresponding to a public key.  The
         * private key must already be in the danuo.
         */
        string get_private_key( public_key_type pubkey ) const;
        
        /** Checks whether the danuo has just been created and has not yet had a password set.
         *
         * Calling \c set_password will transition the danuo to the locked state.
         * @return true if the danuo is new
         * @ingroup danuo Management
         */
        bool is_new() const;
        
        /** Checks whether the danuo is locked (is unable to use its private keys).
         *
         * This state can be changed by calling \c lock() or \c unlock().
         * @return true if the danuo is locked
         * @ingroup danuo Management
         */
        bool is_locked() const;
        
        /** Locks the danuo immediately.
         * @ingroup danuo Management
         */
        void lock();
        
        /** Unlocks the danuo.
         *
         * The danuo remain unlocked until the \c lock is called
         * or the program exits.
         * @param password the password previously set with \c set_password()
         * @ingroup danuo Management
         */
        void unlock(string password);
        
        /** Sets a new password on the danuo.
         *
         * The danuo must be either 'new' or 'unlocked' to
         * execute this command.
         * @ingroup danuo Management
         */
        void set_password(string password);
        
        /** Dumps all private keys owned by the danuo.
         *
         * The keys are printed in WIF format.  You can import these keys into another danuo
         * using \c import_key()
         * @returns a map containing the private keys, indexed by their public key
         */
        map<public_key_type, string> list_keys();
        
        /** Returns detailed help on a single API command.
         * @param method the name of the API command you want help with
         * @returns a multi-line string suitable for displaying on a terminal
         */
        string  gethelp(const string& method) const;
        
        /** Loads a specified Taiyi danuo.
         *
         * The current danuo is closed before the new danuo is loaded.
         *
         * @warning This does not change the filename that will be used for future
         * danuo writes, so this may cause you to overwrite your original
         * danuo unless you also call \c set_danuo_filename()
         *
         * @param danuo_filename the filename of the danuo JSON file to load.
         *                        If \c danuo_filename is empty, it reloads the
         *                        existing danuo file
         * @returns true if the specified danuo is loaded
         */
        bool load_danuo_file(string danuo_filename = "");
        
        /** Saves the current danuo to the given filename.
         *
         * @warning This does not change the danuo filename that will be used for future
         * writes, so think of this function as 'Save a Copy As...' instead of
         * 'Save As...'.  Use \c set_danuo_filename() to make the filename
         * persist.
         * @param danuo_filename the filename of the new danuo JSON file to create
         *                        or overwrite.  If \c danuo_filename is empty,
         *                        save to the current filename.
         */
        void save_danuo_file(string danuo_filename = "");
        
        /** Sets the danuo filename used for future writes.
         *
         * This does not trigger a save, it only changes the default filename
         * that will be used the next time a save is triggered.
         *
         * @param danuo_filename the new filename to use for future saves
         */
        
        void set_danuo_filename(string danuo_filename);
        /** Imports a WIF Private Key into the danuo to be used to sign transactions by an account.
         *
         * example: import_key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
         *
         * @param wif_key the WIF Private Key to import
         */
        bool import_key( string wif_key );
        
        std::map<string, std::function<string(fc::variant, const fc::variants&)>> get_result_formatters() const;
                
        /** Signs a transaction.
         *
         * Given a fully-formed transaction that is only lacking signatures, this signs
         * the transaction with the necessary keys and optionally broadcasts the transaction
         * @param tx the unsigned transaction
         * @param broadcast true if you wish to broadcast the transaction
         * @return the signed version of the transaction
         */
        baiyujing_api::legacy_signed_transaction sign_transaction(baiyujing_api::legacy_signed_transaction tx, bool broadcast = false);
        
        /**
         * Sets the amount of time in the future until a transaction expires.
         */
        void set_transaction_expiration(uint32_t seconds);
        
        void encrypt_keys();
        
        /**
         * Checks memos against private keys on account and imported in danuo
         */
        void check_memo( const string& memo, const baiyujing_api::api_account_object& account )const;
        
        /**
         *  Returns the encrypted memo if memo starts with '#' otherwise returns memo
         */
        string get_encrypted_memo( string from, string to, string memo );
        
        /**
         * Returns the decrypted memo if possible given danuo's known private keys
         */
        string decrypt_memo( string memo );
        
        string start_game(const account_name_type& account, const string& actor_name);
        
        baiyujing_api::api_contract_action_info get_nfa_action_info(int64_t nfa_id, const string& action);
        baiyujing_api::find_actor_return find_actor( const string& name );
        
        vector<taiyi::chain::operation_result> get_transaction_results( transaction_id_type trx_id )const;
        
        //value_list 目前仅支持string, bool, double, int64, array
        string action_actor(const account_name_type& account, const string& actor_name, const string& action, const vector<fc::variant>& value_list);
        baiyujing_api::legacy_signed_transaction action_nfa_consequence(const account_name_type& caller, int64_t nfa_id, const string& action, const vector<fc::variant>& value_list, bool broadcast);
    };
    
} }

FC_REFLECT( taiyi::danuo::danuo_data, (cipher_keys)(ws_server)(ws_user)(ws_password) )
FC_REFLECT( taiyi::danuo::brain_key_info, (brain_priv_key)(wif_priv_key)(pub_key) )
FC_REFLECT( taiyi::danuo::plain_keys, (checksum)(keys) )

FC_REFLECT_ENUM( taiyi::danuo::authority_type, (owner)(active)(posting) )

FC_API( taiyi::danuo::danuo_api,
       /// danuo api
       (help)(gethelp)
       (about)(is_new)(is_locked)(lock)(unlock)(set_password)
       (load_danuo_file)(save_danuo_file)
       
       /// key api
       (import_key)
       (list_keys)
       
       /// danuo api
       (start_game)
       (action_actor)
       )

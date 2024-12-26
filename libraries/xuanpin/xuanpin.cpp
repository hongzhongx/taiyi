#include <chain/taiyi_fwd.hpp>

#include <utilities/git_revision.hpp>
#include <utilities/key_conversion.hpp>
#include <utilities/words.hpp>

#include <protocol/base.hpp>
#include <xuanpin/xuanpin.hpp>
#include <xuanpin/api_documentation.hpp>
#include <xuanpin/reflect_util.hpp>
#include <xuanpin/remote_node_api.hpp>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <list>

#include <boost/version.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm/unique.hpp>
#include <boost/range/algorithm/sort.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <fc/container/deque.hpp>
#include <fc/git_revision.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>
#include <fc/io/stdio.hpp>
#include <fc/macros.hpp>
#include <fc/network/http/websocket.hpp>
#include <fc/rpc/cli.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>
#include <fc/smart_ref_impl.hpp>

#ifndef WIN32
# include <sys/types.h>
# include <sys/stat.h>
#endif

#include <xuanpin/ansi.hpp>

#define BRAIN_KEY_WORD_COUNT 16

namespace taiyi { namespace xuanpin {

    using taiyi::plugins::baiyujing_api::legacy_asset;
    
    namespace detail {
        
        template<class T>
        optional<T> maybe_id( const string& name_or_id )
        {
            if( std::isdigit( name_or_id.front() ) )
            {
                try
                {
                    return fc::variant(name_or_id).as<T>();
                }
                catch (const fc::exception&)
                {
                }
            }
            return optional<T>();
        }

        string pubkey_to_shorthash( const public_key_type& key )
        {
            uint32_t x = fc::sha256::hash(key)._hash[0];
            static const char hd[] = "0123456789abcdef";
            string result;
            
            result += hd[(x >> 0x1c) & 0x0f];
            result += hd[(x >> 0x18) & 0x0f];
            result += hd[(x >> 0x14) & 0x0f];
            result += hd[(x >> 0x10) & 0x0f];
            result += hd[(x >> 0x0c) & 0x0f];
            result += hd[(x >> 0x08) & 0x0f];
            result += hd[(x >> 0x04) & 0x0f];
            result += hd[(x        ) & 0x0f];
            
            return result;
        }
        
        fc::ecc::private_key derive_private_key( const std::string& prefix_string, int sequence_number )
        {
            std::string sequence_string = std::to_string(sequence_number);
            fc::sha512 h = fc::sha512::hash(prefix_string + " " + sequence_string);
            fc::ecc::private_key derived_key = fc::ecc::private_key::regenerate(fc::sha256::hash(h));
            return derived_key;
        }

        string normalize_brain_key( string s )
        {
            size_t i = 0, n = s.length();
            std::string result;
            char c;
            result.reserve( n );
            
            bool preceded_by_whitespace = false;
            bool non_empty = false;
            while( i < n )
            {
                c = s[i++];
                switch( c )
                {
                    case ' ':  case '\t': case '\r': case '\n': case '\v': case '\f':
                        preceded_by_whitespace = true;
                        continue;
                        
                    case 'a': c = 'A'; break;
                    case 'b': c = 'B'; break;
                    case 'c': c = 'C'; break;
                    case 'd': c = 'D'; break;
                    case 'e': c = 'E'; break;
                    case 'f': c = 'F'; break;
                    case 'g': c = 'G'; break;
                    case 'h': c = 'H'; break;
                    case 'i': c = 'I'; break;
                    case 'j': c = 'J'; break;
                    case 'k': c = 'K'; break;
                    case 'l': c = 'L'; break;
                    case 'm': c = 'M'; break;
                    case 'n': c = 'N'; break;
                    case 'o': c = 'O'; break;
                    case 'p': c = 'P'; break;
                    case 'q': c = 'Q'; break;
                    case 'r': c = 'R'; break;
                    case 's': c = 'S'; break;
                    case 't': c = 'T'; break;
                    case 'u': c = 'U'; break;
                    case 'v': c = 'V'; break;
                    case 'w': c = 'W'; break;
                    case 'x': c = 'X'; break;
                    case 'y': c = 'Y'; break;
                    case 'z': c = 'Z'; break;
                        
                    default:
                        break;
                }
                if( preceded_by_whitespace && non_empty )
                    result.push_back(' ');
                result.push_back(c);
                preceded_by_whitespace = false;
                non_empty = true;
            }
            return result;
        }

        vector<lua_types> from_variants_to_lua_types(const vector<fc::variant>& value_list) {
            vector<lua_types> result;
            for(size_t i=0; i<value_list.size(); i++) {
                const auto& v = value_list[i];
                if(v.is_string())
                    result.push_back(lua_string(v.as_string()));
                else if(v.is_bool())
                    result.push_back(lua_bool(v.as_bool()));
                else if(v.is_double())
                    result.push_back(lua_number(v.as_double()));
                else if(v.is_int64())
                    result.push_back(lua_int(v.as_int64()));
                else if(v.is_uint64())
                    result.push_back(lua_int(v.as_uint64()));
                else if(v.is_array()) {
                    const auto sub_list = v.as<vector<fc::variant>>();
                    vector<lua_types> values = from_variants_to_lua_types(sub_list);
                    lua_table vt;
                    for(size_t k=0; k<values.size(); k++)
                        vt.v[lua_key(lua_int(k+1))] = values[k]; //lua 数组下表从1开始
                    result.push_back(vt);
                }
                else
                    FC_ASSERT(false, "#${i} value type in value list is not supported.", ("i", i));
            }
            return result;
        }

        struct op_prototype_visitor
        {
            typedef void result_type;
            
            int t = 0;
            flat_map< std::string, operation >& name2op;
            
            op_prototype_visitor(int _t, flat_map< std::string, operation >& _prototype_ops) : t(_t), name2op(_prototype_ops) {}
            
            template<typename Type>
            result_type operator()( const Type& op )const
            {
                string name = fc::get_typename<Type>::name();
                size_t p = name.rfind(':');
                if( p != string::npos )
                    name = name.substr( p+1 );
                name2op[ name ] = Type();
            }
        };

        class xuanpin_api_impl
        {
        public:
            api_documentation method_documentation;
        private:
            void enable_umask_protection() {
#ifdef __unix__
                _old_umask = umask( S_IRWXG | S_IRWXO );
#endif
            }
            
            void disable_umask_protection() {
#ifdef __unix__
                umask( _old_umask );
#endif
            }
            
            void init_prototype_ops()
            {
                operation op;
                for( int t=0; t<op.count(); t++ )
                {
                    op.set_which( t );
                    op.visit( op_prototype_visitor(t, _prototype_ops) );
                }
                return;
            }

        public:
            xuanpin_api& self;
            
            xuanpin_api_impl( xuanpin_api& s, const xuanpin_data& initial_data, const taiyi::protocol::chain_id_type& _taiyi_chain_id, fc::api< remote_node_api > rapi ) :
                self( s ),
                _remote_api( rapi )
            {
                init_prototype_ops();
                
                _xuanpin.ws_server = initial_data.ws_server;
                taiyi_chain_id = _taiyi_chain_id;
            }
            
            virtual ~xuanpin_api_impl()
            {}
            
            void encrypt_keys()
            {
                if( !is_locked() )
                {
                    plain_keys data;
                    data.keys = _keys;
                    data.checksum = _checksum;
                    auto plain_txt = fc::raw::pack_to_vector(data);
                    _xuanpin.cipher_keys = fc::aes_encrypt( data.checksum, plain_txt );
                }
            }
            
            bool copy_xuanpin_file( string destination_filename )
            {
                fc::path src_path = get_xuanpin_filename();
                if( !fc::exists( src_path ) )
                    return false;
                fc::path dest_path = destination_filename + _xuanpin_filename_extension;
                int suffix = 0;
                while( fc::exists(dest_path) )
                {
                    ++suffix;
                    dest_path = destination_filename + "-" + std::to_string( suffix ) + _xuanpin_filename_extension;
                }
                wlog( "backing up xuanpin ${src} to ${dest}",
                     ("src", src_path)
                     ("dest", dest_path) );
                
                fc::path dest_parent = fc::absolute(dest_path).parent_path();
                try
                {
                    enable_umask_protection();
                    if( !fc::exists( dest_parent ) )
                        fc::create_directories( dest_parent );
                    fc::copy( src_path, dest_path );
                    disable_umask_protection();
                }
                catch(...)
                {
                    disable_umask_protection();
                    throw;
                }
                return true;
            }
            
            bool is_locked() const
            {
                return _checksum == fc::sha512();
            }
            
            variant info() const
            {
                auto dynamic_props = _remote_api->get_dynamic_global_properties();
                fc::mutable_variant_object result(fc::variant(dynamic_props).get_object());
                result["siming_majority_version"] = std::string( _remote_api->get_siming_schedule().majority_version );
                result["hardfork_version"] = std::string( _remote_api->get_hardfork_version() );
                result["head_block_num"] = dynamic_props.head_block_number;
                result["head_block_id"] = dynamic_props.head_block_id;
                result["head_block_age"] = fc::get_approximate_relative_time_string(dynamic_props.time,
                                                                                    time_point_sec(time_point::now()),
                                                                                    " old");
                result["participation"] = (100*dynamic_props.recent_slots_filled.popcount()) / 128.0;
                result["account_creation_fee"] = _remote_api->get_chain_properties().account_creation_fee;
                result["content_reward_fund"] = fc::variant(_remote_api->get_reward_fund( TAIYI_CULTIVATION_REWARD_FUND_NAME )).get_object();
                return result;
            }
            
            variant_object about() const
            {
                string client_version( taiyi::utilities::git_revision_description );
                const size_t pos = client_version.find( '/' );
                if( pos != string::npos && client_version.size() > pos )
                    client_version = client_version.substr( pos + 1 );
                
                fc::mutable_variant_object result;
                result["blockchain_version"]       = TAIYI_BLOCKCHAIN_VERSION;
                result["client_version"]           = client_version;
                result["taiyi_revision"]           = taiyi::utilities::git_revision_sha;
                result["taiyi_revision_age"]       = fc::get_approximate_relative_time_string( fc::time_point_sec( taiyi::utilities::git_revision_unix_timestamp ) );
                result["fc_revision"]              = fc::git_revision_sha;
                result["fc_revision_age"]          = fc::get_approximate_relative_time_string( fc::time_point_sec( fc::git_revision_unix_timestamp ) );
                result["compile_date"]             = "compiled on " __DATE__ " at " __TIME__;
                result["boost_version"]            = boost::replace_all_copy(std::string(BOOST_LIB_VERSION), "_", ".");
                result["openssl_version"]          = OPENSSL_VERSION_TEXT;
                
                std::string bitness = boost::lexical_cast<std::string>(8 * sizeof(int*)) + "-bit";
#if defined(__APPLE__)
                std::string os = "osx";
#elif defined(__linux__)
                std::string os = "linux";
#elif defined(_MSC_VER)
                std::string os = "win32";
#else
                std::string os = "other";
#endif
                result["build"] = os + " " + bitness;
                
                try
                {
                    auto v = _remote_api->get_version();
                    result["server_blockchain_version"] = v.blockchain_version;
                    result["server_taiyi_revision"] = v.taiyi_revision;
                    result["server_fc_revision"] = v.fc_revision;
                }
                catch( fc::exception& )
                {
                    result["server"] = "could not retrieve server version information";
                }
                
                return result;
            }
            
            baiyujing_api::api_account_object get_account( string account_name ) const
            {
                auto accounts = _remote_api->get_accounts( { account_name } );
                FC_ASSERT( !accounts.empty(), "Unknown account" );
                return accounts.front();
            }
            
            string get_xuanpin_filename() const { return _xuanpin_filename; }
            
            optional<fc::ecc::private_key>  try_get_private_key(const public_key_type& id)const
            {
                auto it = _keys.find(id);
                if( it != _keys.end() )
                    return wif_to_key( it->second );
                return optional<fc::ecc::private_key>();
            }
            
            fc::ecc::private_key get_private_key(const public_key_type& id)const
            {
                auto has_key = try_get_private_key( id );
                FC_ASSERT( has_key );
                return *has_key;
            }
            

            fc::ecc::private_key get_private_key_for_account(const baiyujing_api::api_account_object& account)const
            {
                vector<public_key_type> active_keys = account.active.get_keys();
                if (active_keys.size() != 1)
                    FC_THROW("Expecting a simple authority with one active key");
                return get_private_key(active_keys.front());
            }
            
            // imports the private key into the xuanpin, and associate it in some way (?) with the
            // given account name.
            // @returns true if the key matches a current active/owner/memo key for the named
            //          account, false otherwise (but it is stored either way)
            bool import_key(string wif_key)
            {
                fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
                if (!optional_private_key)
                    FC_THROW("Invalid private key");
                taiyi::chain::public_key_type wif_pub_key = optional_private_key->get_public_key();
                
                _keys[wif_pub_key] = wif_key;
                return true;
            }
            
            bool load_xuanpin_file(string xuanpin_filename = "")
            {
                // TODO:  Merge imported xuanpin with existing xuanpin,
                //        instead of replacing it
                if( xuanpin_filename == "" )
                    xuanpin_filename = _xuanpin_filename;
                
                if( ! fc::exists( xuanpin_filename ) )
                    return false;
                
                _xuanpin = fc::json::from_file( xuanpin_filename ).as< xuanpin_data >();
                
                return true;
            }
            
            void save_xuanpin_file(string xuanpin_filename = "")
            {
                //
                // Serialize in memory, then save to disk
                //
                // This approach lessens the risk of a partially written xuanpin
                // if exceptions are thrown in serialization
                //
                
                encrypt_keys();
                
                if( xuanpin_filename == "" )
                    xuanpin_filename = _xuanpin_filename;
                
                wlog( "saving xuanpin to file ${fn}", ("fn", xuanpin_filename) );
                
                string data = fc::json::to_pretty_string( _xuanpin );
                try
                {
                    enable_umask_protection();
                    //
                    // Parentheses on the following declaration fails to compile,
                    // due to the Most Vexing Parse.  Thanks, C++
                    //
                    // http://en.wikipedia.org/wiki/Most_vexing_parse
                    //
                    fc::ofstream outfile{ fc::path( xuanpin_filename ) };
                    outfile.write( data.c_str(), data.length() );
                    outfile.flush();
                    outfile.close();
                    disable_umask_protection();
                }
                catch(...)
                {
                    disable_umask_protection();
                    throw;
                }
            }
            
            // This function generates derived keys starting with index 0 and keeps incrementing
            // the index until it finds a key that isn't registered in the block chain.  To be
            // safer, it continues checking for a few more keys to make sure there wasn't a short gap
            // caused by a failed registration or the like.
            int find_first_unused_derived_key_index(const fc::ecc::private_key& parent_key)
            {
                int first_unused_index = 0;
                int number_of_consecutive_unused_keys = 0;
                for (int key_index = 0; ; ++key_index)
                {
                    fc::ecc::private_key derived_private_key = derive_private_key(key_to_wif(parent_key), key_index);
                    taiyi::chain::public_key_type derived_public_key = derived_private_key.get_public_key();
                    if( _keys.find(derived_public_key) == _keys.end() )
                    {
                        if (number_of_consecutive_unused_keys)
                        {
                            ++number_of_consecutive_unused_keys;
                            if (number_of_consecutive_unused_keys > 5)
                                return first_unused_index;
                        }
                        else
                        {
                            first_unused_index = key_index;
                            number_of_consecutive_unused_keys = 1;
                        }
                    }
                    else
                    {
                        // key_index is used
                        first_unused_index = 0;
                        number_of_consecutive_unused_keys = 0;
                    }
                }
            }
            
            signed_transaction create_account_with_private_key(fc::ecc::private_key owner_privkey, string account_name, string creator_account_name, bool broadcast = false, bool save_xuanpin = true)
            { try {
                int active_key_index = find_first_unused_derived_key_index(owner_privkey);
                fc::ecc::private_key active_privkey = derive_private_key( key_to_wif(owner_privkey), active_key_index);
                
                int memo_key_index = find_first_unused_derived_key_index(active_privkey);
                fc::ecc::private_key memo_privkey = derive_private_key( key_to_wif(active_privkey), memo_key_index);
                
                taiyi::chain::public_key_type owner_pubkey = owner_privkey.get_public_key();
                taiyi::chain::public_key_type active_pubkey = active_privkey.get_public_key();
                taiyi::chain::public_key_type memo_pubkey = memo_privkey.get_public_key();
                
                account_create_operation account_create_op;
                
                account_create_op.creator = creator_account_name;
                account_create_op.new_account_name = account_name;
                account_create_op.fee = _remote_api->get_chain_properties().account_creation_fee;
                account_create_op.owner = authority(1, owner_pubkey, 1);
                account_create_op.active = authority(1, active_pubkey, 1);
                account_create_op.memo_key = memo_pubkey;
                
                signed_transaction tx;
                
                tx.operations.push_back( account_create_op );
                tx.validate();
                
                if( save_xuanpin )
                    save_xuanpin_file();
                if( broadcast )
                {
                    //_remote_api->broadcast_transaction( tx );
                    auto result = _remote_api->broadcast_transaction_synchronous( baiyujing_api::legacy_signed_transaction( tx ) );
                    FC_UNUSED(result);
                }
                return tx;
            } FC_CAPTURE_AND_RETHROW( (account_name)(creator_account_name)(broadcast) ) }

            signed_transaction set_adoring_proxy(string account_to_modify, string proxy, bool broadcast /* = false */)
            { try {
                account_siming_proxy_operation op;
                op.account = account_to_modify;
                op.proxy = proxy;
                
                signed_transaction tx;
                tx.operations.push_back( op );
                tx.validate();
                
                return sign_transaction( tx, broadcast );
            } FC_CAPTURE_AND_RETHROW( (account_to_modify)(proxy)(broadcast) ) }
            
            optional< baiyujing_api::api_siming_object > get_siming( string owner_account )
            {
                return _remote_api->get_siming_by_account( owner_account );
            }
            
            void set_transaction_expiration( uint32_t tx_expiration_seconds )
            {
                FC_ASSERT( tx_expiration_seconds < TAIYI_MAX_TIME_UNTIL_EXPIRATION );
                _tx_expiration_seconds = tx_expiration_seconds;
            }
            
            annotated_signed_transaction sign_transaction(signed_transaction tx, bool broadcast = false )
            {
                static const authority null_auth( 1, public_key_type(), 0 );
                flat_set< account_name_type >   req_active_approvals;
                flat_set< account_name_type >   req_owner_approvals;
                flat_set< account_name_type >   req_posting_approvals;
                vector< authority >  other_auths;
                
                tx.get_required_authorities( req_active_approvals, req_owner_approvals, req_posting_approvals, other_auths );
                
                for( const auto& auth : other_auths )
                    for( const auto& a : auth.account_auths )
                        req_active_approvals.insert(a.first);
                
                // std::merge lets us de-duplicate account_id's that occur in both
                //   sets, and dump them into a vector (as required by remote_db api)
                //   at the same time
                vector< account_name_type > v_approving_account_names;
                std::merge(req_active_approvals.begin(), req_active_approvals.end(),
                           req_owner_approvals.begin() , req_owner_approvals.end(),
                           std::back_inserter( v_approving_account_names ) );
                
                for( const auto& a : req_posting_approvals )
                    v_approving_account_names.push_back(a);
                
                /// TODO: fetch the accounts specified via other_auths as well.
                
                auto approving_account_objects = _remote_api->get_accounts( v_approving_account_names );
                
                /// TODO: recursively check one layer deeper in the authority tree for keys
                
                FC_ASSERT( approving_account_objects.size() == v_approving_account_names.size(), "", ("aco.size:", approving_account_objects.size())("acn",v_approving_account_names.size()) );
                
                flat_map< string, baiyujing_api::api_account_object > approving_account_lut;
                size_t i = 0;
                for( const optional< baiyujing_api::api_account_object >& approving_acct : approving_account_objects )
                {
                    if( !approving_acct.valid() )
                    {
                        wlog( "operation_get_required_auths said approval of non-existing account ${name} was needed",
                             ("name", v_approving_account_names[i]) );
                        i++;
                        continue;
                    }
                    approving_account_lut[ approving_acct->name ] =  *approving_acct;
                    i++;
                }
                auto get_account_from_lut = [&]( const std::string& name ) -> fc::optional< const baiyujing_api::api_account_object* >
                {
                    fc::optional< const baiyujing_api::api_account_object* > result;
                    auto it = approving_account_lut.find( name );
                    if( it != approving_account_lut.end() )
                    {
                        result = &(it->second);
                    }
                    else
                    {
                        elog( "Tried to access authority for account ${a}.", ("a", name) );
                        elog( "Is it possible you are using an account authority? Signing with an account authority is currently not supported." );
                    }
                    
                    return result;
                };
                
                flat_set<public_key_type> approving_key_set;
                for( account_name_type& acct_name : req_active_approvals )
                {
                    const auto it = approving_account_lut.find( acct_name );
                    if( it == approving_account_lut.end() )
                        continue;
                    const baiyujing_api::api_account_object& acct = it->second;
                    vector<public_key_type> v_approving_keys = acct.active.get_keys();
                    wdump((v_approving_keys));
                    for( const public_key_type& approving_key : v_approving_keys )
                    {
                        wdump((approving_key));
                        approving_key_set.insert( approving_key );
                    }
                }
                
                for( account_name_type& acct_name : req_posting_approvals )
                {
                    const auto it = approving_account_lut.find( acct_name );
                    if( it == approving_account_lut.end() )
                        continue;
                    const baiyujing_api::api_account_object& acct = it->second;
                    vector<public_key_type> v_approving_keys = acct.posting.get_keys();
                    wdump((v_approving_keys));
                    for( const public_key_type& approving_key : v_approving_keys )
                    {
                        wdump((approving_key));
                        approving_key_set.insert( approving_key );
                    }
                }
                
                for( const account_name_type& acct_name : req_owner_approvals )
                {
                    const auto it = approving_account_lut.find( acct_name );
                    if( it == approving_account_lut.end() )
                        continue;
                    const baiyujing_api::api_account_object& acct = it->second;
                    vector<public_key_type> v_approving_keys = acct.owner.get_keys();
                    for( const public_key_type& approving_key : v_approving_keys )
                    {
                        wdump((approving_key));
                        approving_key_set.insert( approving_key );
                    }
                }
                
                for( const authority& a : other_auths )
                {
                    for( const auto& k : a.key_auths )
                    {
                        wdump((k.first));
                        approving_key_set.insert( k.first );
                    }
                }
                
                auto dyn_props = _remote_api->get_dynamic_global_properties();
                tx.set_reference_block( dyn_props.head_block_id );
                tx.set_expiration( dyn_props.time + fc::seconds(_tx_expiration_seconds) );
                tx.signatures.clear();
                
                //idump((_keys));
                flat_set< public_key_type > available_keys;
                flat_map< public_key_type, fc::ecc::private_key > available_private_keys;
                for( const public_key_type& key : approving_key_set )
                {
                    auto it = _keys.find(key);
                    if( it != _keys.end() )
                    {
                        fc::optional<fc::ecc::private_key> privkey = wif_to_key( it->second );
                        FC_ASSERT( privkey.valid(), "Malformed private key in _keys" );
                        available_keys.insert(key);
                        available_private_keys[key] = *privkey;
                    }
                }
                
                auto minimal_signing_keys = tx.minimize_required_signatures(
                    taiyi_chain_id,
                    available_keys,
                    [&]( const string& account_name ) -> const authority&
                    {
                        auto maybe_account = get_account_from_lut( account_name );
                        if( maybe_account.valid() )
                            return (*maybe_account)->active;
                        
                        return null_auth;
                    },
                    [&]( const string& account_name ) -> const authority&
                    {
                        auto maybe_account = get_account_from_lut( account_name );
                        if( maybe_account.valid() )
                            return (*maybe_account)->owner;
                        
                        return null_auth;
                    },
                    [&]( const string& account_name ) -> const authority&
                    {
                        auto maybe_account = get_account_from_lut( account_name );
                        if( maybe_account.valid() )
                            return (*maybe_account)->posting;
                        
                        return null_auth;
                    },
                    TAIYI_MAX_SIG_CHECK_DEPTH,
                    TAIYI_MAX_AUTHORITY_MEMBERSHIP,
                    TAIYI_MAX_SIG_CHECK_ACCOUNTS,
                    fc::ecc::fc_canonical
                );
                
                for( const public_key_type& k : minimal_signing_keys )
                {
                    auto it = available_private_keys.find(k);
                    FC_ASSERT( it != available_private_keys.end() );
                    tx.sign( it->second, taiyi_chain_id, fc::ecc::fc_canonical );
                }
                
                if( broadcast )
                {
                    try
                    {
                        auto result = _remote_api->broadcast_transaction_synchronous( baiyujing_api::legacy_signed_transaction( tx ) );
                        annotated_signed_transaction rtrx(tx);
                        rtrx.block_num = result.block_num;
                        rtrx.transaction_num = result.trx_num;
                        return rtrx;
                    }
                    catch (const fc::exception& e)
                    {
                        elog("Caught exception while broadcasting tx ${id}:  ${e}", ("id", tx.id().str())("e", e.to_detail_string()) );
                        throw;
                    }
                }
                return tx;
            }
            
            std::map<string, std::function<string(fc::variant, const fc::variants&)>> get_result_formatters() const
            {
                std::map<string, std::function<string(fc::variant, const fc::variants&)> > m;
                m["help"] = [](variant result, const fc::variants& a)
                {
                    return result.get_string();
                };
                
                m["gethelp"] = [](variant result, const fc::variants& a)
                {
                    return result.get_string();
                };
                
                m["list_my_accounts"] = [](variant result, const fc::variants& a ) {
                    std::stringstream out;
                    
                    auto accounts = result.as<vector<baiyujing_api::api_account_object>>();
                    asset total_yang;
                    asset total_vest(0, QI_SYMBOL );
                    for( const auto& a : accounts ) {
                        total_yang += a.balance.to_asset();
                        total_vest  += a.qi.to_asset();
                        out << std::left << std::setw( 17 ) << std::string(a.name)
                        << std::right << std::setw(18) << fc::variant(a.balance).as_string() <<" "
                        << std::right << std::setw(26) << fc::variant(a.qi).as_string() <<"\n";
                    }
                    out << "-------------------------------------------------------------------------\n";
                    out << std::left << std::setw( 17 ) << "TOTAL"
                    << std::right << std::setw(18) << legacy_asset::from_asset(total_yang).to_string() <<" "
                    << std::right << std::setw(26) << legacy_asset::from_asset(total_vest).to_string() <<"\n";
                    return out.str();
                };
                
                //      m["get_account_history"] = []( variant result, const fc::variants& a ) {
                //         std::stringstream ss;
                //         ss << std::left << std::setw( 5 )  << "#" << " ";
                //         ss << std::left << std::setw( 10 ) << "BLOCK #" << " ";
                //         ss << std::left << std::setw( 15 ) << "TRX ID" << " ";
                //         ss << std::left << std::setw( 20 ) << "OPERATION" << " ";
                //         ss << std::left << std::setw( 50 ) << "DETAILS" << "\n";
                //         ss << "-------------------------------------------------------------------------------\n";
                //         const auto& results = result.get_array();
                //         for( const auto& item : results ) {
                //            ss << std::left << std::setw(5) << item.get_array()[0].as_string() << " ";
                //            const auto& op = item.get_array()[1].get_object();
                //            ss << std::left << std::setw(10) << op["block"].as_string() << " ";
                //            ss << std::left << std::setw(15) << op["trx_id"].as_string() << " ";
                //            const auto& opop = op["op"].get_array();
                //            ss << std::left << std::setw(20) << opop[0].as_string() << " ";
                //            ss << std::left << std::setw(50) << fc::json::to_string(opop[1]) << "\n ";
                //         }
                //         return ss.str();
                //      };
                
                m["get_withdraw_routes"] = []( variant result, const fc::variants& a )
                {
                    auto routes = result.as< vector< database_api::api_withdraw_qi_route_object > >();
                    std::stringstream ss;
                    
                    ss << ' ' << std::left << std::setw( 20 ) << "From";
                    ss << ' ' << std::left << std::setw( 20 ) << "To";
                    ss << ' ' << std::right << std::setw( 8 ) << "Percent";
                    ss << ' ' << std::right << std::setw( 9 ) << "Auto-Vest";
                    ss << "\n==============================================================\n";
                    
                    for( auto& r : routes )
                    {
                        ss << ' ' << std::left << std::setw( 20 ) << std::string( r.from_account );
                        ss << ' ' << std::left << std::setw( 20 ) << std::string( r.to_account );
                        ss << ' ' << std::right << std::setw( 8 ) << std::setprecision( 2 ) << std::fixed << double( r.percent ) / 100;
                        ss << ' ' << std::right << std::setw( 9 ) << ( r.auto_vest ? "true" : "false" ) << std::endl;
                    }
                    
                    return ss.str();
                };

                m["action_actor"] = []( variant result, const fc::variants& a )
                {
                    vector<string> results = result.as<vector<string>>();
                    
                    string ss;
                    for(const string& s : results)
                        ss += s + "\n" + NOR;
                    
                    ansi(ss);
                    
                    return ss;
                };

                return m;
            }
            
            operation get_prototype_operation( string operation_name )
            {
                auto it = _prototype_ops.find( operation_name );
                if( it == _prototype_ops.end() )
                    FC_THROW("Unsupported operation: \"${operation_name}\"", ("operation_name", operation_name));
                return it->second;
            }
            
            string                                  _xuanpin_filename;
            xuanpin_data                             _xuanpin;
            taiyi::protocol::chain_id_type          taiyi_chain_id;
            
            map<public_key_type,string>             _keys;
            fc::sha512                              _checksum;
            fc::api< remote_node_api >              _remote_api;
            uint32_t                                _tx_expiration_seconds = 30;
            
            flat_map<string, operation>             _prototype_ops;
            
            static_variant_map _operation_which_map = create_static_variant_map< operation >();
            
#ifdef __unix__
            mode_t                  _old_umask;
#endif
            const string _xuanpin_filename_extension = ".xuanpin";
        };
        
    } } } // taiyi::xuanpin::detail


namespace taiyi { namespace xuanpin {
    
    xuanpin_api::xuanpin_api(const xuanpin_data& initial_data, const taiyi::protocol::chain_id_type& _taiyi_chain_id, fc::api< remote_node_api > rapi)
        : my(new detail::xuanpin_api_impl(*this, initial_data, _taiyi_chain_id, rapi))
    {}

    xuanpin_api::~xuanpin_api(){}
    
    bool xuanpin_api::copy_xuanpin_file(string destination_filename)
    {
        return my->copy_xuanpin_file(destination_filename);
    }
    
    optional< baiyujing_api::legacy_signed_block > xuanpin_api::get_block(uint32_t num)
    {
        return my->_remote_api->get_block( num );
    }
    
    vector< baiyujing_api::api_operation_object > xuanpin_api::get_ops_in_block(uint32_t block_num, bool only_virtual)
    {
        return my->_remote_api->get_ops_in_block( block_num, only_virtual );
    }
    
    vector< baiyujing_api::api_account_object > xuanpin_api::list_my_accounts()
    {
        FC_ASSERT( !is_locked(), "Xuanpin must be unlocked to list accounts" );
        vector<baiyujing_api::api_account_object> result;
        
        vector<public_key_type> pub_keys;
        pub_keys.reserve( my->_keys.size() );
        
        for( const auto& item : my->_keys )
            pub_keys.push_back(item.first);
        
        auto refs = my->_remote_api->get_key_references( pub_keys );
        set<string> names;
        for( const auto& item : refs )
            for( const auto& name : item )
                names.insert( name );
        
        
        result.reserve( names.size() );
        for( const auto& name : names )
            result.emplace_back( get_account( name ) );
        
        return result;
    }
    
    vector< account_name_type > xuanpin_api::list_accounts(const string& lowerbound, uint32_t limit)
    {
        return my->_remote_api->lookup_accounts( lowerbound, limit );
    }
    
    vector< account_name_type > xuanpin_api::get_active_simings()const {
        return my->_remote_api->get_active_simings();
    }
    
    brain_key_info xuanpin_api::suggest_brain_key()const
    {
        brain_key_info result;
        // create a private key for secure entropy
        fc::sha256 sha_entropy1 = fc::ecc::private_key::generate().get_secret();
        fc::sha256 sha_entropy2 = fc::ecc::private_key::generate().get_secret();
        fc::bigint entropy1( sha_entropy1.data(), sha_entropy1.data_size() );
        fc::bigint entropy2( sha_entropy2.data(), sha_entropy2.data_size() );
        fc::bigint entropy(entropy1);
        entropy <<= 8*sha_entropy1.data_size();
        entropy += entropy2;
        string brain_key = "";
        
        for( int i=0; i<BRAIN_KEY_WORD_COUNT; i++ )
        {
            fc::bigint choice = entropy % taiyi::words::word_list_size;
            entropy /= taiyi::words::word_list_size;
            if( i > 0 )
                brain_key += " ";
            brain_key += taiyi::words::word_list[ choice.to_int64() ];
        }
        
        brain_key = normalize_brain_key(brain_key);
        fc::ecc::private_key priv_key = detail::derive_private_key( brain_key, 0 );
        result.brain_priv_key = brain_key;
        result.wif_priv_key = key_to_wif( priv_key );
        result.pub_key = priv_key.get_public_key();
        return result;
    }
    
    string xuanpin_api::serialize_transaction( signed_transaction tx )const
    {
        return fc::to_hex(fc::raw::pack_to_vector(tx));
    }
    
    string xuanpin_api::get_xuanpin_filename() const
    {
        return my->get_xuanpin_filename();
    }
    
    
    baiyujing_api::api_account_object xuanpin_api::get_account( string account_name ) const
    {
        return my->get_account( account_name );
    }
    
    bool xuanpin_api::import_key(string wif_key)
    {
        FC_ASSERT(!is_locked());
        // backup xuanpin
        fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
        if (!optional_private_key)
            FC_THROW("Invalid private key");
        //   string shorthash = detail::pubkey_to_shorthash( optional_private_key->get_public_key() );
        //   copy_xuanpin_file( "before-import-key-" + shorthash );
        
        if( my->import_key(wif_key) )
        {
            save_xuanpin_file();
            //     copy_xuanpin_file( "after-import-key-" + shorthash );
            return true;
        }
        return false;
    }
    
    string xuanpin_api::normalize_brain_key(string s) const
    {
        return detail::normalize_brain_key( s );
    }
    
    variant xuanpin_api::info()
    {
        return my->info();
    }
    
    variant_object xuanpin_api::about() const
    {
        return my->about();
    }
    
    /*
     fc::ecc::private_key xuanpin_api::derive_private_key(const std::string& prefix_string, int sequence_number) const
     {
        return detail::derive_private_key( prefix_string, sequence_number );
     }
     */
    
    vector< account_name_type > xuanpin_api::list_simings(const string& lowerbound, uint32_t limit)
    {
        return my->_remote_api->lookup_siming_accounts( lowerbound, limit );
    }
    
    optional< baiyujing_api::api_siming_object > xuanpin_api::get_siming(string owner_account)
    {
        return my->get_siming(owner_account);
    }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::set_adoring_proxy(string account_to_modify, string adoring_account, bool broadcast /* = false */)
    {
        return my->set_adoring_proxy(account_to_modify, adoring_account, broadcast);
    }
    
    void xuanpin_api::set_xuanpin_filename(string xuanpin_filename)
    {
        my->_xuanpin_filename = xuanpin_filename;
    }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::sign_transaction(baiyujing_api::legacy_signed_transaction tx, bool broadcast)
    { try {
        signed_transaction appbase_tx( tx );
        baiyujing_api::annotated_signed_transaction result = my->sign_transaction( appbase_tx, broadcast);
        return baiyujing_api::legacy_signed_transaction( result );
    } FC_CAPTURE_AND_RETHROW( (tx) ) }
    
    operation xuanpin_api::get_prototype_operation(string operation_name) {
        return my->get_prototype_operation( operation_name );
    }
    
    string xuanpin_api::help()const
    {
        std::vector<std::string> method_names = my->method_documentation.get_method_names();
        std::stringstream ss;
        for (const std::string& method_name : method_names)
        {
            try
            {
                ss << my->method_documentation.get_brief_description(method_name);
            }
            catch (const fc::key_not_found_exception&)
            {
                ss << method_name << " (no help available)\n";
            }
        }
        return ss.str();
    }
    
    string xuanpin_api::gethelp(const string& method)const
    {
        fc::api<xuanpin_api> tmp;
        std::stringstream ss;
        ss << "\n";
        
        std::string doxygenHelpString = my->method_documentation.get_detailed_description(method);
        if (!doxygenHelpString.empty())
            ss << doxygenHelpString;
        else
            ss << "No help defined for method " << method << "\n";
        
        return ss.str();
    }
    
    bool xuanpin_api::load_xuanpin_file( string xuanpin_filename )
    {
        return my->load_xuanpin_file( xuanpin_filename );
    }
    
    void xuanpin_api::save_xuanpin_file( string xuanpin_filename )
    {
        my->save_xuanpin_file( xuanpin_filename );
    }
    
    std::map<string,std::function<string(fc::variant,const fc::variants&)> > xuanpin_api::get_result_formatters() const
    {
        return my->get_result_formatters();
    }
    
    bool xuanpin_api::is_locked()const
    {
        return my->is_locked();
    }
    bool xuanpin_api::is_new()const
    {
        return my->_xuanpin.cipher_keys.size() == 0;
    }
    
    void xuanpin_api::encrypt_keys()
    {
        my->encrypt_keys();
    }
    
    void xuanpin_api::lock()
    { try {
        FC_ASSERT( !is_locked() );
        encrypt_keys();
        for( auto& key : my->_keys )
            key.second = key_to_wif(fc::ecc::private_key());
        my->_keys.clear();
        my->_checksum = fc::sha512();
        my->self.lock_changed(true);
    } FC_CAPTURE_AND_RETHROW() }
    
    void xuanpin_api::unlock(string password)
    { try {
        FC_ASSERT(password.size() > 0);
        auto pw = fc::sha512::hash(password.c_str(), password.size());
        vector<char> decrypted = fc::aes_decrypt(pw, my->_xuanpin.cipher_keys);
        auto pk = fc::raw::unpack_from_vector<plain_keys>(decrypted);
        FC_ASSERT(pk.checksum == pw);
        my->_keys = std::move(pk.keys);
        my->_checksum = pk.checksum;
        my->self.lock_changed(false);
    } FC_CAPTURE_AND_RETHROW() }
    
    void xuanpin_api::set_password( string password )
    {
        if( !is_new() )
            FC_ASSERT( !is_locked(), "The xuanpin must be unlocked before the password can be set" );
        my->_checksum = fc::sha512::hash( password.c_str(), password.size() );
        lock();
    }
    
    map<public_key_type, string> xuanpin_api::list_keys()
    {
        FC_ASSERT(!is_locked());
        return my->_keys;
    }
    
    string xuanpin_api::get_private_key( public_key_type pubkey )const
    {
        return key_to_wif( my->get_private_key( pubkey ) );
    }
    
    pair<public_key_type,string> xuanpin_api::get_private_key_from_password( string account, string role, string password )const {
        auto seed = account + role + password;
        FC_ASSERT( seed.size() );
        auto secret = fc::sha256::hash( seed.c_str(), seed.size() );
        auto priv = fc::ecc::private_key::regenerate( secret );
        return std::make_pair( public_key_type( priv.get_public_key() ), key_to_wif( priv ) );
    }
    
    /**
     * This method is used by faucets to create new accounts for other users which must
     * provide their desired keys. The resulting account may not be controllable by this
     * xuanpin.
     */
    baiyujing_api::legacy_signed_transaction xuanpin_api::create_account_with_keys( string creator, string new_account_name, string json_meta, public_key_type owner, public_key_type active, public_key_type posting, public_key_type memo, bool broadcast ) const
    { try {
        FC_ASSERT( !is_locked() );
        account_create_operation op;
        op.creator = creator;
        op.new_account_name = new_account_name;
        op.owner = authority( 1, owner, 1 );
        op.active = authority( 1, active, 1 );
        op.posting = authority( 1, posting, 1 );
        op.memo_key = memo;
        op.json_metadata = json_meta;
        op.fee = my->_remote_api->get_chain_properties().account_creation_fee;
        
        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();
        
        return my->sign_transaction( tx, broadcast );
    } FC_CAPTURE_AND_RETHROW( (creator)(new_account_name)(json_meta)(owner)(active)(memo)(broadcast) ) }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::request_account_recovery( string recovery_account, string account_to_recover, authority new_authority, bool broadcast )
    {
        FC_ASSERT( !is_locked() );
        request_account_recovery_operation op;
        op.recovery_account = recovery_account;
        op.account_to_recover = account_to_recover;
        op.new_owner_authority = new_authority;
        
        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();
        
        return my->sign_transaction( tx, broadcast );
    }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::recover_account( string account_to_recover, authority recent_authority, authority new_authority, bool broadcast ) {
        FC_ASSERT( !is_locked() );
        
        recover_account_operation op;
        op.account_to_recover = account_to_recover;
        op.new_owner_authority = new_authority;
        op.recent_owner_authority = recent_authority;
        
        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();
        
        return my->sign_transaction( tx, broadcast );
    }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::change_recovery_account( string owner, string new_recovery_account, bool broadcast ) {
        FC_ASSERT( !is_locked() );
        
        change_recovery_account_operation op;
        op.account_to_recover = owner;
        op.new_recovery_account = new_recovery_account;
        
        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();
        
        return my->sign_transaction( tx, broadcast );
    }
    
    vector< database_api::api_owner_authority_history_object > xuanpin_api::get_owner_history( string account )const
    {
        return my->_remote_api->get_owner_history( account );
    }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::update_account( string account_name, string json_meta, public_key_type owner, public_key_type active, public_key_type posting, public_key_type memo, bool broadcast ) const
    {
        try
        {
            FC_ASSERT( !is_locked() );
            
            account_update_operation op;
            op.account = account_name;
            op.owner  = authority( 1, owner, 1 );
            op.active = authority( 1, active, 1);
            op.posting = authority( 1, posting, 1);
            op.memo_key = memo;
            op.json_metadata = json_meta;
            
            signed_transaction tx;
            tx.operations.push_back(op);
            tx.validate();
            
            return my->sign_transaction( tx, broadcast );
        }
        FC_CAPTURE_AND_RETHROW( (account_name)(json_meta)(owner)(active)(memo)(broadcast) )
    }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::update_account_auth_key( string account_name, authority_type type, public_key_type key, weight_type weight, bool broadcast )
    {
        FC_ASSERT( !is_locked() );
        
        auto accounts = my->_remote_api->get_accounts( { account_name } );
        FC_ASSERT( accounts.size() == 1, "Account does not exist" );
        FC_ASSERT( account_name == accounts[0].name, "Account name doesn't match?" );
        
        account_update_operation op;
        op.account = account_name;
        op.memo_key = accounts[0].memo_key;
        op.json_metadata = accounts[0].json_metadata;
        
        authority new_auth;
        
        switch( type )
        {
            case( owner ):
                new_auth = accounts[0].owner;
                break;
            case( active ):
                new_auth = accounts[0].active;
                break;
            case( posting ):
                new_auth = accounts[0].posting;
                break;
        }
        
        if( weight == 0 ) // Remove the key
        {
            new_auth.key_auths.erase( key );
        }
        else
        {
            new_auth.add_authority( key, weight );
        }
        
        if( new_auth.is_impossible() )
        {
            if ( type == owner )
            {
                FC_ASSERT( false, "Owner authority change would render account irrecoverable." );
            }
            
            wlog( "Authority is now impossible." );
        }
        
        switch( type )
        {
            case( owner ):
                op.owner = new_auth;
                break;
            case( active ):
                op.active = new_auth;
                break;
            case( posting ):
                op.posting = new_auth;
                break;
        }
        
        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();
        
        return my->sign_transaction( tx, broadcast );
    }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::update_account_auth_account( string account_name, authority_type type, string auth_account, weight_type weight, bool broadcast )
    {
        FC_ASSERT( !is_locked() );
        
        auto accounts = my->_remote_api->get_accounts( { account_name } );
        FC_ASSERT( accounts.size() == 1, "Account does not exist" );
        FC_ASSERT( account_name == accounts[0].name, "Account name doesn't match?" );
        
        account_update_operation op;
        op.account = account_name;
        op.memo_key = accounts[0].memo_key;
        op.json_metadata = accounts[0].json_metadata;
        
        authority new_auth;
        
        switch( type )
        {
            case( owner ):
                new_auth = accounts[0].owner;
                break;
            case( active ):
                new_auth = accounts[0].active;
                break;
            case( posting ):
                new_auth = accounts[0].posting;
                break;
        }
        
        if( weight == 0 ) // Remove the key
        {
            new_auth.account_auths.erase( auth_account );
        }
        else
        {
            new_auth.add_authority( auth_account, weight );
        }
        
        if( new_auth.is_impossible() )
        {
            if ( type == owner )
            {
                FC_ASSERT( false, "Owner authority change would render account irrecoverable." );
            }
            
            wlog( "Authority is now impossible." );
        }
        
        switch( type )
        {
            case( owner ):
                op.owner = new_auth;
                break;
            case( active ):
                op.active = new_auth;
                break;
            case( posting ):
                op.posting = new_auth;
                break;
        }
        
        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();
        
        return my->sign_transaction( tx, broadcast );
    }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::update_account_auth_threshold(string account_name, authority_type type, uint32_t threshold, bool broadcast )
    {
        FC_ASSERT( !is_locked() );
        
        auto accounts = my->_remote_api->get_accounts( { account_name } );
        FC_ASSERT( accounts.size() == 1, "Account does not exist" );
        FC_ASSERT( account_name == accounts[0].name, "Account name doesn't match?" );
        FC_ASSERT( threshold != 0, "Authority is implicitly satisfied" );
        
        account_update_operation op;
        op.account = account_name;
        op.memo_key = accounts[0].memo_key;
        op.json_metadata = accounts[0].json_metadata;
        
        authority new_auth;
        
        switch( type )
        {
            case( owner ):
                new_auth = accounts[0].owner;
                break;
            case( active ):
                new_auth = accounts[0].active;
                break;
            case( posting ):
                new_auth = accounts[0].posting;
                break;
        }
        
        new_auth.weight_threshold = threshold;
        
        if( new_auth.is_impossible() )
        {
            if ( type == owner )
            {
                FC_ASSERT( false, "Owner authority change would render account irrecoverable." );
            }
            
            wlog( "Authority is now impossible." );
        }
        
        switch( type )
        {
            case( owner ):
                op.owner = new_auth;
                break;
            case( active ):
                op.active = new_auth;
                break;
            case( posting ):
                op.posting = new_auth;
                break;
        }
        
        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();
        
        return my->sign_transaction( tx, broadcast );
    }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::update_account_meta( string account_name, string json_meta, bool broadcast )
    {
        FC_ASSERT( !is_locked() );
        
        auto accounts = my->_remote_api->get_accounts( { account_name } );
        FC_ASSERT( accounts.size() == 1, "Account does not exist" );
        FC_ASSERT( account_name == accounts[0].name, "Account name doesn't match?" );
        
        account_update_operation op;
        op.account = account_name;
        op.memo_key = accounts[0].memo_key;
        op.json_metadata = json_meta;
        
        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();
        
        return my->sign_transaction( tx, broadcast );
    }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::update_account_memo_key( string account_name, public_key_type key, bool broadcast )
    {
        FC_ASSERT( !is_locked() );
        
        auto accounts = my->_remote_api->get_accounts( { account_name } );
        FC_ASSERT( accounts.size() == 1, "Account does not exist" );
        FC_ASSERT( account_name == accounts[0].name, "Account name doesn't match?" );
        
        account_update_operation op;
        op.account = account_name;
        op.memo_key = key;
        op.json_metadata = accounts[0].json_metadata;
        
        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();
        
        return my->sign_transaction( tx, broadcast );
    }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::delegate_qi( string delegator, string delegatee, baiyujing_api::legacy_asset qi, bool broadcast )
    {
        FC_ASSERT( !is_locked() );
        
        auto accounts = my->_remote_api->get_accounts( { delegator, delegatee } );
        FC_ASSERT( accounts.size() == 2 , "One or more of the accounts specified do not exist." );
        FC_ASSERT( delegator == accounts[0].name, "Delegator account is not right?" );
        FC_ASSERT( delegatee == accounts[1].name, "Delegator account is not right?" );
        
        delegate_qi_operation op;
        op.delegator = delegator;
        op.delegatee = delegatee;
        op.qi = qi.to_asset();
        
        signed_transaction tx;
        tx.operations.push_back( op );
        tx.validate();
        
        return my->sign_transaction( tx, broadcast );
    }
    
    /**
     *  This method will genrate new owner, active, and memo keys for the new account which
     *  will be controlable by this xuanpin.
     */
    baiyujing_api::legacy_signed_transaction xuanpin_api::create_account( string creator, string new_account_name, string json_meta, bool broadcast )
    { try {
        FC_ASSERT( !is_locked() );
        auto owner = suggest_brain_key();
        auto active = suggest_brain_key();
        auto posting = suggest_brain_key();
        auto memo = suggest_brain_key();
        import_key( owner.wif_priv_key );
        import_key( active.wif_priv_key );
        import_key( posting.wif_priv_key );
        import_key( memo.wif_priv_key );
        return create_account_with_keys( creator, new_account_name, json_meta, owner.pub_key, active.pub_key, posting.pub_key, memo.pub_key, broadcast );
    } FC_CAPTURE_AND_RETHROW( (creator)(new_account_name)(json_meta) ) }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::update_siming( string siming_account_name, string url, public_key_type block_signing_key, const baiyujing_api::api_chain_properties& props, bool broadcast  )
    {
        FC_ASSERT( !is_locked() );
        
        siming_update_operation op;
        
        optional< baiyujing_api::api_siming_object > wit = my->_remote_api->get_siming_by_account( siming_account_name );
        if( !wit.valid() )
        {
            op.url = url;
        }
        else
        {
            FC_ASSERT( wit->owner == siming_account_name );
            if( url != "" )
                op.url = url;
            else
                op.url = wit->url;
        }
        op.owner = siming_account_name;
        op.block_signing_key = block_signing_key;
        op.props = props;
        
        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();
        
        return my->sign_transaction( tx, broadcast );
    }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::adore_for_siming( string adoring_account, string siming_to_adore_for, bool approve, bool broadcast )
    { try {
        FC_ASSERT( !is_locked() );
        account_siming_adore_operation op;
        op.account = adoring_account;
        op.siming = siming_to_adore_for;
        op.approve = approve;
        
        signed_transaction tx;
        tx.operations.push_back( op );
        tx.validate();
        
        return my->sign_transaction( tx, broadcast );
    } FC_CAPTURE_AND_RETHROW( (adoring_account)(siming_to_adore_for)(approve)(broadcast) ) }
    
    void xuanpin_api::check_memo( const string& memo, const baiyujing_api::api_account_object& account ) const
    {
        vector< public_key_type > keys;
        
        try
        {
            // Check if memo is a private key
            keys.push_back( fc::ecc::extended_private_key::from_base58( memo ).get_public_key() );
        }
        catch( fc::parse_error_exception& ) {}
        catch( fc::assert_exception& ) {}
        
        // Get possible keys if memo was an account password
        string owner_seed = account.name + "owner" + memo;
        auto owner_secret = fc::sha256::hash( owner_seed.c_str(), owner_seed.size() );
        keys.push_back( fc::ecc::private_key::regenerate( owner_secret ).get_public_key() );
        
        string active_seed = account.name + "active" + memo;
        auto active_secret = fc::sha256::hash( active_seed.c_str(), active_seed.size() );
        keys.push_back( fc::ecc::private_key::regenerate( active_secret ).get_public_key() );
        
        string posting_seed = account.name + "posting" + memo;
        auto posting_secret = fc::sha256::hash( posting_seed.c_str(), posting_seed.size() );
        keys.push_back( fc::ecc::private_key::regenerate( posting_secret ).get_public_key() );
        
        // Check keys against public keys in authorites
        for( auto& key_weight_pair : account.owner.key_auths )
        {
            for( auto& key : keys )
                FC_ASSERT( key_weight_pair.first != key, "Detected private owner key in memo field. Cancelling transaction." );
        }
        
        for( auto& key_weight_pair : account.active.key_auths )
        {
            for( auto& key : keys )
                FC_ASSERT( key_weight_pair.first != key, "Detected private active key in memo field. Cancelling transaction." );
        }
        
        for( auto& key_weight_pair : account.posting.key_auths )
        {
            for( auto& key : keys )
                FC_ASSERT( key_weight_pair.first != key, "Detected private posting key in memo field. Cancelling transaction." );
        }
        
        const auto& memo_key = account.memo_key;
        for( auto& key : keys )
            FC_ASSERT( memo_key != key, "Detected private memo key in memo field. Cancelling transaction." );
        
        // Check against imported keys
        for( auto& key_pair : my->_keys )
        {
            for( auto& key : keys )
                FC_ASSERT( key != key_pair.first, "Detected imported private key in memo field. Cancelling trasanction." );
        }
    }
    
    string xuanpin_api::get_encrypted_memo(string from, string to, string memo )
    {
        if( memo.size() > 0 && memo[0] == '#' )
        {
            auto from_account = get_account( from );
            auto to_account   = get_account( to );
            
            memo_data m;
            
            auto from_priv = my->get_private_key( from_account.memo_key );
            m.set_message(from_priv, to_account.memo_key, memo.substr(1), fc::time_point::now().time_since_epoch().count());
            
            return m;
        }
        else
        {
            return memo;
        }
    }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::transfer( string from, string to, baiyujing_api::legacy_asset amount, string memo, bool broadcast )
    { try {
        FC_ASSERT( !is_locked() );
        check_memo( memo, get_account( from ) );
        transfer_operation op;
        op.from = from;
        op.to = to;
        op.amount = amount.to_asset();
        
        op.memo = get_encrypted_memo( from, to, memo );
        
        signed_transaction tx;
        tx.operations.push_back( op );
        tx.validate();
        
        return my->sign_transaction( tx, broadcast );
    } FC_CAPTURE_AND_RETHROW( (from)(to)(amount)(memo)(broadcast) ) }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::transfer_to_qi( string from, string to, baiyujing_api::legacy_asset amount, bool broadcast )
    {
        FC_ASSERT( !is_locked() );
        transfer_to_qi_operation op;
        op.from = from;
        op.to = (to == from ? "" : to);
        op.amount = amount.to_asset();
        
        signed_transaction tx;
        tx.operations.push_back( op );
        tx.validate();
        
        return my->sign_transaction( tx, broadcast );
    }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::withdraw_qi( string from, baiyujing_api::legacy_asset qi, bool broadcast )
    {
        FC_ASSERT( !is_locked() );
        withdraw_qi_operation op;
        op.account = from;
        op.qi = qi.to_asset();
        
        signed_transaction tx;
        tx.operations.push_back( op );
        tx.validate();
        
        return my->sign_transaction( tx, broadcast );
    }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::set_withdraw_qi_route( string from, string to, uint16_t percent, bool auto_vest, bool broadcast )
    {
        FC_ASSERT( !is_locked() );
        set_withdraw_qi_route_operation op;
        op.from_account = from;
        op.to_account = to;
        op.percent = percent;
        op.auto_vest = auto_vest;
        
        signed_transaction tx;
        tx.operations.push_back( op );
        tx.validate();
        
        return my->sign_transaction( tx, broadcast );
    }
    
    string xuanpin_api::decrypt_memo( string encrypted_memo )
    {
        if( is_locked() )
            return encrypted_memo;
        
        if( encrypted_memo.size() && encrypted_memo[0] == '#' )
        {
            auto m = memo_data::from_string( encrypted_memo );
            if( m )
            {
                auto from_key = my->try_get_private_key( m->from );
                if( !from_key )
                {
                    auto to_key   = my->try_get_private_key( m->to );
                    if( !to_key )
                        return encrypted_memo;
                    return m->get_message(*to_key, m->from);
                }
                else
                    return m->get_message(*from_key, m->to);
            }
        }
        return encrypted_memo;
    }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::decline_adoring_rights( string account, bool decline, bool broadcast )
    {
        FC_ASSERT( !is_locked() );
        decline_adoring_rights_operation op;
        op.account = account;
        op.decline = decline;
        
        signed_transaction tx;
        tx.operations.push_back( op );
        tx.validate();
        
        return my->sign_transaction( tx, broadcast );
    }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::claim_reward_balance( string account, baiyujing_api::legacy_asset reward_yang, baiyujing_api::legacy_asset reward_qi, baiyujing_api::legacy_asset reward_feigang, bool broadcast )
    {
        FC_ASSERT( !is_locked() );
        claim_reward_balance_operation op;
        op.account = account;
        op.reward_yang = reward_yang.to_asset();
        op.reward_qi = reward_qi.to_asset();
        op.reward_feigang = reward_feigang.to_asset();
        
        signed_transaction tx;
        tx.operations.push_back( op );
        tx.validate();
        
        return my->sign_transaction( tx, broadcast );
    }
    
    map< uint32_t, baiyujing_api::api_operation_object > xuanpin_api::get_account_history( string account, uint32_t from, uint32_t limit )
    {
        auto result = my->_remote_api->get_account_history( account, from, limit );
        if( !is_locked() )
        {
            for( auto& item : result )
            {
                if( item.second.op.which() == baiyujing_api::legacy_operation::tag<baiyujing_api::legacy_transfer_operation>::value )
                {
                    auto& top = item.second.op.get<baiyujing_api::legacy_transfer_operation>();
                    top.memo = decrypt_memo( top.memo );
                }
            }
        }
        return result;
    }
    
    baiyujing_api::state xuanpin_api::get_state( string url )
    {
        return my->_remote_api->get_state( url );
    }
    
    vector< database_api::api_withdraw_qi_route_object > xuanpin_api::get_withdraw_routes( string account, baiyujing_api::withdraw_route_type type )const
    {
        return my->_remote_api->get_withdraw_routes( account, type );
    }
    
    void xuanpin_api::set_transaction_expiration(uint32_t seconds)
    {
        my->set_transaction_expiration(seconds);
    }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::get_transaction( transaction_id_type id )const
    {
        return my->_remote_api->get_transaction( id );
    }
    
    vector<taiyi::chain::operation_result> xuanpin_api::get_transaction_results( transaction_id_type trx_id )const
    {
        return my->_remote_api->get_transaction_results( trx_id );
    }    
    
    vector< baiyujing_api::api_resource_assets > xuanpin_api::get_account_resources ( vector< account_name_type > names )
    {
        return my->_remote_api->get_account_resources( names );
    }

    baiyujing_api::legacy_signed_transaction xuanpin_api::create_contract(const account_name_type& owner, const string& name, const public_key_type& contract_authority, const string& data, bool broadcast)
    { try {
        FC_ASSERT( !is_locked() );

        create_contract_operation op;
        op.owner = owner;
        op.name = name;
        op.contract_authority = contract_authority;
        op.data = data;

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        return my->sign_transaction( tx, broadcast );
    } FC_CAPTURE_AND_RETHROW( (owner)(name)(contract_authority)(data) ) }

    baiyujing_api::legacy_signed_transaction xuanpin_api::create_contract_from_file(const account_name_type& owner, const string& name, const public_key_type&  contract_authority, const string& filename, bool broadcast)
    { try {
        FC_ASSERT( !is_locked() );

        FC_ASSERT(filename != "" && fc::exists(filename));
        std::string contract_data;
        fc::read_file_contents(filename, contract_data);

        return create_contract(owner, name, contract_authority, contract_data, broadcast);
    } FC_CAPTURE_AND_RETHROW( (owner)(name)(contract_authority)(filename) ) }

    baiyujing_api::legacy_signed_transaction xuanpin_api::call_contract_function(const account_name_type& account, const string& contract_name, const string& function_name, const vector<fc::variant>& value_list, bool broadcast)
    { try {
        FC_ASSERT( !is_locked() );

        call_contract_function_operation op;

        op.caller = account;
        op.contract_name = contract_name;
        op.function_name = function_name;
        op.value_list = detail::from_variants_to_lua_types(value_list);

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        return my->sign_transaction( tx, broadcast );
    } FC_CAPTURE_AND_RETHROW( (account)(contract_name)(function_name)(value_list) ) }

    baiyujing_api::legacy_signed_transaction xuanpin_api::revise_contract(const account_name_type& reviser, const string& name, const string& data, bool broadcast)
    { try {
        FC_ASSERT( !is_locked() );

        revise_contract_operation op;

        op.reviser = reviser;
        op.contract_name = name;
        op.data = data;

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        return my->sign_transaction( tx, broadcast );
    } FC_CAPTURE_AND_RETHROW( (reviser)(name)(data) ) }

    baiyujing_api::legacy_signed_transaction xuanpin_api::revise_contract_from_file(const account_name_type& reviser, const string& name, const string& filename, bool broadcast)
    { try {
        FC_ASSERT(filename != "" && fc::exists(filename));
        std::string contract_data;
        fc::read_file_contents(filename, contract_data);
        return revise_contract(reviser, name, contract_data, broadcast);
    } FC_CAPTURE_AND_RETHROW( (reviser)(name)(filename) ) }

    baiyujing_api::legacy_signed_transaction xuanpin_api::create_nfa_symbol( const account_name_type& creator, const string& symbol, const string& describe, const string& contract, bool broadcast )
    { try {
        FC_ASSERT( !is_locked() );
        create_nfa_symbol_operation op;
        op.creator = creator;
        op.symbol = symbol;
        op.describe = describe;
        op.default_contract = contract;

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        auto transaction = baiyujing_api::legacy_signed_transaction(my->sign_transaction( tx, broadcast ));
        transaction.operation_results = get_transaction_results(transaction.transaction_id);
        return transaction;
    } FC_CAPTURE_AND_RETHROW( (creator)(symbol)(describe)(broadcast) ) }

    baiyujing_api::legacy_signed_transaction xuanpin_api::create_nfa( const account_name_type& creator, const string& symbol, bool broadcast )
    { try {
        FC_ASSERT( !is_locked() );
        create_nfa_operation op;
        op.creator = creator;
        op.symbol = symbol;

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        auto transaction = baiyujing_api::legacy_signed_transaction(my->sign_transaction( tx, broadcast ));
        transaction.operation_results = get_transaction_results(transaction.transaction_id);
        return transaction;
    } FC_CAPTURE_AND_RETHROW( (creator)(symbol)(broadcast) ) }

    baiyujing_api::legacy_signed_transaction xuanpin_api::transfer_nfa( const account_name_type& from, const account_name_type& to, int64_t nfa_id, bool broadcast )
    { try {
        FC_ASSERT( !is_locked() );
        transfer_nfa_operation op;
        op.from = from;
        op.to = to;
        op.id = nfa_id;

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        return my->sign_transaction( tx, broadcast );
    } FC_CAPTURE_AND_RETHROW( (from)(to)(nfa_id)(broadcast) ) }

    baiyujing_api::legacy_signed_transaction xuanpin_api::approve_nfa_active( const account_name_type& owner, const account_name_type& active_account, int64_t nfa_id, bool broadcast )
    { try {
        FC_ASSERT( !is_locked() );
        approve_nfa_active_operation op;
        op.owner = owner;
        op.active_account = active_account;
        op.id = nfa_id;

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        return my->sign_transaction( tx, broadcast );
    } FC_CAPTURE_AND_RETHROW( (owner)(active_account)(nfa_id)(broadcast) ) }

    baiyujing_api::api_contract_action_info xuanpin_api::get_nfa_action_info(int64_t nfa_id, const string& action)
    {
        return my->_remote_api->get_nfa_action_info( nfa_id, action );
    }

    vector<string> xuanpin_api::action_nfa(const account_name_type& owner, int64_t nfa_id, const string& action, const vector<fc::variant>& value_list)
    {
        auto info = get_nfa_action_info(nfa_id, action);
        if(!info.consequence)
        {
            vector<lua_types> lua_value_list = detail::from_variants_to_lua_types(value_list);
            return my->_remote_api->eval_nfa_action( nfa_id, action, lua_value_list );
        }
        else
        {
            auto transaction = action_nfa_consequence(owner, nfa_id, action, value_list, true);
            auto transaction_results = get_transaction_results(transaction.transaction_id);
            vector<string> logs;
            for(const auto& result : transaction_results) {
                if(result.which() == operation_result::tag<contract_result>::value) {
                    auto cresult = result.get<contract_result>();
                    for(auto& temp : cresult.contract_affecteds) {
                        if(temp.which() == contract_affected_type::tag<contract_logger>::value) {
                            logs.push_back(temp.get<contract_logger>().message);
                        }
                    }
                }
            }
            return logs;
        }
    }
    
    baiyujing_api::legacy_signed_transaction xuanpin_api::action_nfa_consequence(const account_name_type& caller, int64_t nfa_id, const string& action, const vector<fc::variant>& value_list, bool broadcast)
    { try {
        FC_ASSERT( !is_locked() );
        action_nfa_operation op;
        op.caller = caller;
        op.id = nfa_id;
        op.action = action;
        op.value_list = detail::from_variants_to_lua_types(value_list);

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        auto transaction = baiyujing_api::legacy_signed_transaction(my->sign_transaction( tx, broadcast ));
        transaction.operation_results = get_transaction_results(transaction.transaction_id);
        return transaction;
    } FC_CAPTURE_AND_RETHROW( (caller)(nfa_id)(action)(value_list) ) }

    baiyujing_api::find_nfas_return xuanpin_api::find_nfas( vector< int64_t > ids )
    {
        return my->_remote_api->find_nfas( ids );
    }

    baiyujing_api::find_nfa_return xuanpin_api::find_nfa( const int64_t& id )
    {
        return my->_remote_api->find_nfa( id );
    }

    vector< baiyujing_api::api_nfa_object > xuanpin_api::list_nfas(const account_name_type& owner, uint32_t limit)
    {
       return my->_remote_api->list_nfas( owner, limit );
    }

    map< uint32_t, baiyujing_api::api_operation_object > xuanpin_api::get_nfa_history( const int64_t& nfa_id, uint32_t from, uint32_t limit )
    {
       return my->_remote_api->get_nfa_history( nfa_id, from, limit );
    }

    baiyujing_api::legacy_signed_transaction xuanpin_api::create_actor_talent_rule(account_name_type creator, const string& contract, bool broadcast )
    { try {
        FC_ASSERT( !is_locked() );
        create_actor_talent_rule_operation op;
        op.creator = creator;
        op.contract = contract;

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        return my->sign_transaction( tx, broadcast );
    } FC_CAPTURE_AND_RETHROW( (creator)(contract) ) }

    baiyujing_api::legacy_signed_transaction xuanpin_api::create_actor( const account_name_type& creator, const string& family_name, const string& last_name, bool broadcast )
    { try {
        FC_ASSERT( !is_locked() );
        create_actor_operation op;
        op.creator = creator;
        op.family_name = family_name;
        op.last_name = last_name;
        op.fee = my->_remote_api->get_chain_properties().account_creation_fee * TAIYI_QI_SHARE_PRICE;

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        auto transaction = baiyujing_api::legacy_signed_transaction(my->sign_transaction( tx, broadcast ));
        transaction.operation_results = get_transaction_results(transaction.transaction_id);
        return transaction;
    } FC_CAPTURE_AND_RETHROW( (creator)(family_name)(last_name) ) }

    baiyujing_api::find_actor_return xuanpin_api::find_actor( const string& name )
    {
        return my->_remote_api->find_actor( name );
    }

    baiyujing_api::find_actors_return xuanpin_api::find_actors( vector< int64_t > actor_ids )
    {
        return my->_remote_api->find_actors( actor_ids );
    }

    vector< database_api::api_actor_object > xuanpin_api::list_actors(const account_name_type& owner, uint32_t limit)
    {
       return my->_remote_api->list_actors( owner, limit );
    }

    map< uint32_t, baiyujing_api::api_operation_object > xuanpin_api::get_actor_history( const string& name, uint32_t from, uint32_t limit )
    {
       return my->_remote_api->get_actor_history( name, from, limit );
    }

    vector< database_api::api_actor_object > xuanpin_api::list_actors_below_health(const int16_t& health, uint32_t limit)
    {
        return my->_remote_api->list_actors_below_health( health, limit );
    }

    vector< database_api::api_actor_object > xuanpin_api::list_actors_on_zone(const string& zone_name, uint32_t limit)
    {
        return my->_remote_api->list_actors_on_zone( zone_name, limit );
    }

    baiyujing_api::find_actor_talent_rules_return xuanpin_api::find_actor_talent_rules( vector< int64_t > ids )
    {
        return my->_remote_api->find_actor_talent_rules( ids );
    }

    vector<string> xuanpin_api::action_actor(const account_name_type& account, const string& actor_name, const string& action, const vector<fc::variant>& value_list)
    {
        auto actor_info = find_actor(actor_name);
        if(!actor_info.valid())
            return {FORMAT_MESSAGE("角色\"${a}\"不存在", ("a", actor_name))};

        return action_nfa(account, actor_info->nfa_id, action, value_list);
    }

    baiyujing_api::legacy_signed_transaction xuanpin_api::action_actor_consequence(const account_name_type& account, const string& actor_name, const string& action, const vector<fc::variant>& value_list, bool broadcast)
    { try {
        FC_ASSERT( !is_locked() );
        
        auto actor_info = find_actor(actor_name);
        FC_ASSERT(actor_info.valid(), "角色\"${a}\"不存在", ("a", actor_name));
        
        return action_nfa_consequence(account, actor_info->nfa_id, action, value_list, broadcast);

    } FC_CAPTURE_AND_RETHROW( (account)(actor_name)(action)(value_list) ) }

    baiyujing_api::legacy_signed_transaction xuanpin_api::create_zone(const account_name_type& creator, const string& name, bool broadcast )
    { try {
        FC_ASSERT( !is_locked() );
        create_zone_operation op;
        op.creator = creator;
        op.name = name;
        op.fee = my->_remote_api->get_chain_properties().account_creation_fee * TAIYI_QI_SHARE_PRICE;

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        auto transaction = baiyujing_api::legacy_signed_transaction(my->sign_transaction( tx, broadcast ));
        transaction.operation_results = get_transaction_results(transaction.transaction_id);
        return transaction;
    } FC_CAPTURE_AND_RETHROW( (creator)(name) ) }

    baiyujing_api::find_zones_return xuanpin_api::find_zones( vector< int64_t > ids )
    {
        return my->_remote_api->find_zones( ids );
    }

    baiyujing_api::find_zones_return xuanpin_api::find_zones_by_name( vector< string > name_list )
    {
        return my->_remote_api->find_zones_by_name( name_list );
    }

    vector< database_api::api_zone_object > xuanpin_api::list_zones(const account_name_type& owner, uint32_t limit)
    {
        return my->_remote_api->list_zones( owner, limit );
    }

    vector< database_api::api_zone_object > xuanpin_api::list_zones_by_type(E_ZONE_TYPE type, uint32_t limit)
    {
        return my->_remote_api->list_zones_by_type( type, limit );
    }

    vector< database_api::api_zone_object > xuanpin_api::list_to_zones_by_from(const string& from_zone, uint32_t limit)
    {
        return my->_remote_api->list_to_zones_by_from( from_zone, limit );
    }

    vector< database_api::api_zone_object > xuanpin_api::list_from_zones_by_to(const string& to_zone, uint32_t limit)
    {
        return my->_remote_api->list_from_zones_by_to( to_zone, limit );
    }

    vector<string> xuanpin_api::find_way_to_zone(const string& from_zone, const string& to_zone)
    {
        return my->_remote_api->find_way_to_zone( from_zone, to_zone ).way_points;
    }

    database_api::api_tiandao_property_object xuanpin_api::get_tiandao_properties() const
    {
        return my->_remote_api->get_tiandao_properties();
    }

} } // taiyi::xuanpin

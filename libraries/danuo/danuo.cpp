#include <chain/taiyi_fwd.hpp>

#include <utilities/git_revision.hpp>
#include <utilities/key_conversion.hpp>
#include <utilities/words.hpp>

#include <protocol/base.hpp>
#include <protocol/lua_types.hpp>

#include <danuo/danuo.hpp>
#include <danuo/api_documentation.hpp>
#include <danuo/reflect_util.hpp>
#include <danuo/remote_node_api.hpp>

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

#include <danuo/ansi.hpp>

#define BRAIN_KEY_WORD_COUNT 16

namespace taiyi { namespace danuo {
    
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
        
        class danuo_api_impl
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
            danuo_api& self;
            danuo_api_impl( danuo_api& s, const danuo_data& initial_data, const taiyi::protocol::chain_id_type& _taiyi_chain_id, fc::api< remote_node_api > rapi )
            : self( s ), _remote_api( rapi )
            {
                init_prototype_ops();
                
                _danuo.ws_server = initial_data.ws_server;
                taiyi_chain_id = _taiyi_chain_id;
            }
            
            virtual ~danuo_api_impl() {}
            
            void encrypt_keys()
            {
                if( !is_locked() )
                {
                    plain_keys data;
                    data.keys = _keys;
                    data.checksum = _checksum;
                    auto plain_txt = fc::raw::pack_to_vector(data);
                    _danuo.cipher_keys = fc::aes_encrypt( data.checksum, plain_txt );
                }
            }
            
            bool copy_danuo_file( string destination_filename )
            {
                fc::path src_path = get_danuo_filename();
                if( !fc::exists( src_path ) )
                    return false;
                fc::path dest_path = destination_filename + _danuo_filename_extension;
                int suffix = 0;
                while( fc::exists(dest_path) )
                {
                    ++suffix;
                    dest_path = destination_filename + "-" + std::to_string( suffix ) + _danuo_filename_extension;
                }
                wlog( "backing up danuo ${src} to ${dest}", ("src", src_path)("dest", dest_path) );
                
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
            
            string get_danuo_filename() const { return _danuo_filename; }
            
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
                
                m["start_game"] = [](variant result, const fc::variants& a)
                {
                    return result.get_string();
                };
                
                return m;
            }
            optional<fc::ecc::private_key> try_get_private_key(const public_key_type& id)const
            {
                auto it = _keys.find(id);
                if( it != _keys.end() )
                    return wif_to_key( it->second );
                return optional<fc::ecc::private_key>();
            }
            
            fc::ecc::private_key get_private_key(const public_key_type& id) const
            {
                auto has_key = try_get_private_key( id );
                FC_ASSERT( has_key );
                return *has_key;
            }
            
            fc::ecc::private_key get_private_key_for_account(const baiyujing_api::api_account_object& account) const
            {
                vector<public_key_type> active_keys = account.active.get_keys();
                if (active_keys.size() != 1)
                    FC_THROW("Expecting a simple authority with one active key");
                return get_private_key(active_keys.front());
            }
            
            // imports the private key into the danuo, and associate it in some way (?) with the
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
            
            bool load_danuo_file(string danuo_filename = "")
            {
                // TODO:  Merge imported danuo with existing danuo,
                //        instead of replacing it
                if( danuo_filename == "" )
                    danuo_filename = _danuo_filename;
                
                if( ! fc::exists( danuo_filename ) )
                    return false;
                
                _danuo = fc::json::from_file( danuo_filename ).as< danuo_data >();
                
                return true;
            }
            
            void save_danuo_file(string danuo_filename = "")
            {
                //
                // Serialize in memory, then save to disk
                //
                // This approach lessens the risk of a partially written danuo
                // if exceptions are thrown in serialization
                //
                
                encrypt_keys();
                
                if( danuo_filename == "" )
                    danuo_filename = _danuo_filename;
                
                wlog( "saving danuo to file ${fn}", ("fn", danuo_filename) );
                
                string data = fc::json::to_pretty_string( _danuo );
                try
                {
                    enable_umask_protection();
                    //
                    // Parentheses on the following declaration fails to compile,
                    // due to the Most Vexing Parse.  Thanks, C++
                    //
                    // http://en.wikipedia.org/wiki/Most_vexing_parse
                    //
                    fc::ofstream outfile{ fc::path( danuo_filename ) };
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
            
            void set_transaction_expiration( uint32_t tx_expiration_seconds )
            {
                FC_ASSERT( tx_expiration_seconds < TAIYI_MAX_TIME_UNTIL_EXPIRATION );
                _tx_expiration_seconds = tx_expiration_seconds;
            }
            
            annotated_signed_transaction sign_transaction(signed_transaction tx, bool broadcast = false)
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
                    for( const public_key_type& approving_key : v_approving_keys )
                        approving_key_set.insert( approving_key );
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
                        throw;
                    }
                }
                return tx;
            }
            
            operation get_prototype_operation( string operation_name )
            {
                auto it = _prototype_ops.find( operation_name );
                if( it == _prototype_ops.end() )
                    FC_THROW("Unsupported operation: \"${operation_name}\"", ("operation_name", operation_name));
                return it->second;
            }
            
            string                                  _danuo_filename;
            danuo_data                             _danuo;
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
            const string _danuo_filename_extension = ".danuo";
        };
        
    } } } // taiyi::danuo::detail

namespace taiyi { namespace danuo {
    
    danuo_api::danuo_api(const danuo_data& initial_data, const taiyi::protocol::chain_id_type& _taiyi_chain_id, fc::api< remote_node_api > rapi)
    : my(new detail::danuo_api_impl(*this, initial_data, _taiyi_chain_id, rapi))
    {}
    
    danuo_api::~danuo_api(){}
    
    bool danuo_api::copy_danuo_file(string destination_filename)
    {
        return my->copy_danuo_file(destination_filename);
    }
    
    string danuo_api::get_danuo_filename() const
    {
        return my->get_danuo_filename();
    }
    
    void danuo_api::set_danuo_filename(string danuo_filename) {
        my->_danuo_filename = danuo_filename;
    }
    
    baiyujing_api::legacy_signed_transaction danuo_api::sign_transaction(baiyujing_api::legacy_signed_transaction tx, bool broadcast)
    { try {
        signed_transaction appbase_tx( tx );
        baiyujing_api::annotated_signed_transaction result = my->sign_transaction( appbase_tx, broadcast);
        return baiyujing_api::legacy_signed_transaction( result );
    } FC_CAPTURE_AND_RETHROW( (tx) ) }
    
    bool danuo_api::load_danuo_file( string danuo_filename )
    {
        return my->load_danuo_file( danuo_filename );
    }
    
    void danuo_api::save_danuo_file( string danuo_filename )
    {
        my->save_danuo_file( danuo_filename );
    }
    
    std::map<string, std::function<string(fc::variant, const fc::variants&)> > danuo_api::get_result_formatters() const
    {
        return my->get_result_formatters();
    }
    
    variant_object danuo_api::about() const
    {
        return my->about();
    }
    
    bool danuo_api::is_locked()const
    {
        return my->is_locked();
    }
    
    bool danuo_api::is_new()const
    {
        return my->_danuo.cipher_keys.size() == 0;
    }
    
    void danuo_api::encrypt_keys()
    {
        my->encrypt_keys();
    }
    
    void danuo_api::lock()
    { try {
        FC_ASSERT( !is_locked() );
        encrypt_keys();
        for( auto& key : my->_keys )
            key.second = key_to_wif(fc::ecc::private_key());
        my->_keys.clear();
        my->_checksum = fc::sha512();
        my->self.lock_changed(true);
    } FC_CAPTURE_AND_RETHROW() }
    
    void danuo_api::unlock(string password)
    { try {
        FC_ASSERT(password.size() > 0);
        auto pw = fc::sha512::hash(password.c_str(), password.size());
        vector<char> decrypted = fc::aes_decrypt(pw, my->_danuo.cipher_keys);
        auto pk = fc::raw::unpack_from_vector<plain_keys>(decrypted);
        FC_ASSERT(pk.checksum == pw);
        my->_keys = std::move(pk.keys);
        my->_checksum = pk.checksum;
        my->self.lock_changed(false);
    } FC_CAPTURE_AND_RETHROW() }
    
    void danuo_api::set_password( string password )
    {
        if( !is_new() )
            FC_ASSERT( !is_locked(), "The danuo must be unlocked before the password can be set" );
        my->_checksum = fc::sha512::hash( password.c_str(), password.size() );
        lock();
    }
    
    string danuo_api::help() const
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
    
    string danuo_api::gethelp(const string& method)const
    {
        fc::api<danuo_api> tmp;
        std::stringstream ss;
        ss << "\n";
        
        std::string doxygenHelpString = my->method_documentation.get_detailed_description(method);
        if (!doxygenHelpString.empty())
            ss << doxygenHelpString;
        else
            ss << "No help defined for method " << method << "\n";
        
        return ss.str();
    }
    
    bool danuo_api::import_key(string wif_key)
    {
        FC_ASSERT(!is_locked());
        // backup danuo
        fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
        if (!optional_private_key)
            FC_THROW("Invalid private key");
        
        if( my->import_key(wif_key) )
        {
            save_danuo_file();
            return true;
        }
        
        return false;
    }
    
    map<public_key_type, string> danuo_api::list_keys()
    {
        FC_ASSERT(!is_locked());
        return my->_keys;
    }
    
    string danuo_api::get_private_key( public_key_type pubkey )const
    {
        return key_to_wif( my->get_private_key( pubkey ) );
    }
    
    void danuo_api::check_memo(const string& memo, const baiyujing_api::api_account_object& account )const
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
    
    string danuo_api::decrypt_memo( string encrypted_memo )
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
    
    void danuo_api::set_transaction_expiration(uint32_t seconds)
    {
        my->set_transaction_expiration(seconds);
    }
    
    vector<taiyi::chain::operation_result> danuo_api::get_transaction_results( transaction_id_type trx_id )const
    {
        return my->_remote_api->get_transaction_results( trx_id );
    }
    
    string danuo_api::start_game(const account_name_type& account, const string& actor_name)
    { try {
        
        vector<account_name_type> v_approving_account_names;
        v_approving_account_names.push_back(account);
        auto approving_account_objects = my->_remote_api->get_accounts( v_approving_account_names );
        FC_ASSERT( approving_account_objects.size() == 1 );
        
        const optional< baiyujing_api::api_account_object >& approving_acct = approving_account_objects[0];
        if( !approving_acct.valid() )
        {
            wlog( "账号\"${name}\"不存在", ("name", account) );
            return "";
        }
        vector<public_key_type> v_approving_keys = approving_acct->active.get_keys();
        bool have_key = false;
        for( const public_key_type& key : v_approving_keys )
        {
            auto it = my->_keys.find(key);
            if( it != my->_keys.end() )
            {
                fc::optional<fc::ecc::private_key> privkey = wif_to_key( it->second );
                FC_ASSERT( privkey.valid(), "Malformed private key in _keys" );
                have_key = true;
                break;
            }
        }
        
        if(!have_key)
        {
            wlog( "没有账号\"${name}\"权限，请导入账号私钥", ("name", account) );
            return "";
        }
        
        optional< database_api::api_actor_object > actor = my->_remote_api->find_actor( actor_name );
        if(!actor.valid())
        {
            wlog( "没有名为\"${name}\"的角色，请输入正确的角色名称", ("name", actor_name) );
            return "";
        }
        
        my->self.start_game_changed(true, account, actor_name);
        
        return action_actor(account, actor_name, "welcome", {});
        
    } FC_CAPTURE_AND_RETHROW() }
    
    baiyujing_api::api_contract_action_info danuo_api::get_nfa_action_info(int64_t nfa_id, const string& action)
    {
        return my->_remote_api->get_nfa_action_info( nfa_id, action );
    }

    baiyujing_api::find_actor_return danuo_api::find_actor( const string& name )
    {
        return my->_remote_api->find_actor( name );
    }
    
    string danuo_api::action_actor(const account_name_type& account, const string& actor_name, const string& action, const vector<fc::variant>& value_list)
    {
        auto actor_info = find_actor(actor_name);
        if(!actor_info.valid())
            return FORMAT_MESSAGE("角色\"${a}\"不存在", ("a", actor_name));

        try {
            vector<string> results;
            auto info = get_nfa_action_info(actor_info->nfa_id, action);
            if(!info.consequence) {
                vector<lua_types> lua_value_list = protocol::from_variants_to_lua_types(value_list);
                auto result = my->_remote_api->eval_nfa_action( actor_info->nfa_id, action, lua_value_list );
                results = result.narrate_logs;
            }
            else {
                auto transaction = action_nfa_consequence(account, actor_info->nfa_id, action, value_list, true);
                auto transaction_results = get_transaction_results(transaction.transaction_id);
                for(const auto& result : transaction_results) {
                    if(result.which() == operation_result::tag<contract_result>::value) {
                        auto cresult = result.get<contract_result>();
                        for(auto& temp : cresult.contract_affecteds) {
                            if(temp.which() == contract_affected_type::tag<contract_narrate>::value) {
                                results.push_back(temp.get<contract_narrate>().message);
                            }
                        }
                    }
                }
            }
            
            string ss;
            for(const string& s : results)
                ss += s + "\n" + NOR;
            
            ss += "\n" + NOR;            
            ansi(ss);
            return ss;
        }
        catch( fc::exception& er )
        {
            //对特定错误信息，识别解析出来直接返回
            string es = er.to_detail_string();
            size_t p1 = es.find("#t&&y#");
            size_t p2 = es.find("#a&&i#");
            if(p1 != string::npos && p2 != string::npos) {
                string ss = es.substr(p1+6, p2-(p1+6)) + "\n" + NOR;
                ansi(ss);
                return ss;
            }
            else {
                elog("Caught exception while action:  ${e}", ("e", es));
                return string("\n") + NOR;
            }
        }
        catch( const std::exception& e ) {
            string es = e.what();
            size_t p1 = es.find("#t&&y#");
            size_t p2 = es.find("#a&&i#");
            if(p1 != string::npos && p2 != string::npos) {
                string ss = es.substr(p1+6, p2-(p1+6)) + "\n" + NOR;
                ansi(ss);
                return ss;
            }
            else {
                elog(es);
                return string("\n") + NOR;
            }
        }
        catch( ... )
        {
            throw fc::unhandled_exception(fc::log_message(), std::current_exception());
        }
    }
    //=============================================================================
    baiyujing_api::legacy_signed_transaction danuo_api::action_nfa_consequence(const account_name_type& caller, int64_t nfa_id, const string& action, const vector<fc::variant>& value_list, bool broadcast)
    { try {
        FC_ASSERT( !is_locked() );
        action_nfa_operation op;
        op.caller = caller;
        op.id = nfa_id;
        op.action = action;
        op.value_list = protocol::from_variants_to_lua_types(value_list);

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        auto transaction = baiyujing_api::legacy_signed_transaction(my->sign_transaction( tx, broadcast ));
        transaction.operation_results = get_transaction_results(transaction.transaction_id);
        return transaction;
    } FC_CAPTURE_AND_RETHROW( (caller)(nfa_id)(action)(value_list) ) }

} } // taiyi::danuo

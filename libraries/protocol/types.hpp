#pragma once
#include "types_fwd.hpp"
#include "config.hpp"

#include "asset_symbol.hpp"
#include "fixed_string.hpp"

#include <fc/container/flat_fwd.hpp>
#include <fc/io/varint.hpp>
#include <fc/io/enum_type.hpp>
#include <fc/crypto/sha224.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/safe.hpp>
#include <fc/optional.hpp>
#include <fc/container/flat.hpp>
#include <fc/string.hpp>
#include <fc/io/raw.hpp>
#include <fc/uint128.hpp>
#include <fc/static_variant.hpp>
#include <fc/smart_ref_fwd.hpp>

#include <boost/multiprecision/cpp_int.hpp>

#include <memory>
#include <vector>
#include <deque>
#include <cstdint>

namespace taiyi {

    using                                    fc::uint128_t;
    typedef boost::multiprecision::uint256_t u256;
    typedef boost::multiprecision::uint512_t u512;

    using                               std::map;
    using                               std::vector;
    using                               std::unordered_map;
    using                               std::string;
    using                               std::deque;
    using                               std::shared_ptr;
    using                               std::weak_ptr;
    using                               std::unique_ptr;
    using                               std::set;
    using                               std::pair;
    using                               std::enable_shared_from_this;
    using                               std::tie;
    using                               std::make_pair;

    using                               fc::smart_ref;
    using                               fc::variant_object;
    using                               fc::variant;
    using                               fc::enum_type;
    using                               fc::optional;
    using                               fc::unsigned_int;
    using                               fc::signed_int;
    using                               fc::time_point_sec;
    using                               fc::time_point;
    using                               fc::safe;
    using                               fc::flat_map;
    using                               fc::flat_set;
    using                               fc::static_variant;
    using                               fc::ecc::range_proof_type;
    using                               fc::ecc::range_proof_info;
    using                               fc::ecc::commitment_type;
    struct void_t{};

    namespace protocol {

        typedef fc::ecc::private_key        private_key_type;
        typedef fc::sha256                  chain_id_type;
        typedef fixed_string<16>            account_name_type;
        typedef fc::ripemd160               block_id_type;
        typedef fc::ripemd160               checksum_type;
        typedef fc::ripemd160               transaction_id_type;
        typedef fc::sha256                  digest_type;
        typedef fc::ecc::compact_signature  signature_type;
        typedef safe<int64_t>               share_type;
        typedef uint16_t                    weight_type;
        typedef uint32_t                    contribution_id_type;
        typedef fixed_string<32>            custom_id_type;

        struct public_key_type
        {
            struct binary_key
            {
                binary_key() {}
                uint32_t                 check = 0;
                fc::ecc::public_key_data data;
            };
            
            fc::ecc::public_key_data key_data;
            
            public_key_type();
            public_key_type( const fc::ecc::public_key_data& data );
            public_key_type( const fc::ecc::public_key& pubkey );
            explicit public_key_type( const std::string& base58str );
            operator fc::ecc::public_key_data() const;
            operator fc::ecc::public_key() const;
            explicit operator std::string() const;
            friend bool operator == ( const public_key_type& p1, const fc::ecc::public_key& p2);
            friend bool operator == ( const public_key_type& p1, const public_key_type& p2);
            friend bool operator < ( const public_key_type& p1, const public_key_type& p2) { return p1.key_data < p2.key_data; }
            friend bool operator != ( const public_key_type& p1, const public_key_type& p2);
        };

        #define TAIYI_INIT_PUBLIC_KEY (taiyi::protocol::public_key_type(TAIYI_INIT_PUBLIC_KEY_STR))

        struct extended_public_key_type
        {
            struct binary_key
            {
                binary_key() {}
                uint32_t                   check = 0;
                fc::ecc::extended_key_data data;
            };
            
            fc::ecc::extended_key_data key_data;
            
            extended_public_key_type();
            extended_public_key_type( const fc::ecc::extended_key_data& data );
            extended_public_key_type( const fc::ecc::extended_public_key& extpubkey );
            explicit extended_public_key_type( const std::string& base58str );
            operator fc::ecc::extended_public_key() const;
            explicit operator std::string() const;
            friend bool operator == ( const extended_public_key_type& p1, const fc::ecc::extended_public_key& p2);
            friend bool operator == ( const extended_public_key_type& p1, const extended_public_key_type& p2);
            friend bool operator != ( const extended_public_key_type& p1, const extended_public_key_type& p2);
        };

        struct extended_private_key_type
        {
            struct binary_key
            {
                binary_key() {}
                uint32_t                   check = 0;
                fc::ecc::extended_key_data data;
            };
            
            fc::ecc::extended_key_data key_data;
            
            extended_private_key_type();
            extended_private_key_type( const fc::ecc::extended_key_data& data );
            extended_private_key_type( const fc::ecc::extended_private_key& extprivkey );
            explicit extended_private_key_type( const std::string& base58str );
            operator fc::ecc::extended_private_key() const;
            explicit operator std::string() const;
            friend bool operator == ( const extended_private_key_type& p1, const fc::ecc::extended_private_key& p2);
            friend bool operator == ( const extended_private_key_type& p1, const extended_private_key_type& p2);
            friend bool operator != ( const extended_private_key_type& p1, const extended_private_key_type& p2);
        };

        chain_id_type generate_chain_id( const std::string& chain_id_name );
    
        //=====================================================================
    
        struct memo_data {
            
            public_key_type from;
            public_key_type to;
            uint64_t        nonce = 0;
            uint32_t        check = 0;
            vector<char>    encrypted;

            static optional<memo_data> from_string( string str ) {
                try {
                    if( str.size() > sizeof(memo_data) && str[0] == '#') {
                        auto data = fc::from_base58( str.substr(1) );
                        auto m  = fc::raw::unpack_from_vector<memo_data>( data );
                        FC_ASSERT( string(m) == str );
                        return m;
                    }
                } catch ( ... ) {}
                return optional<memo_data>();
            }
            
            
            operator string() const {
                auto data = fc::raw::pack_to_vector(*this);
                auto base58 = fc::to_base58( data );
                return '#'+base58;
            }
            
            void set_message(const private_key_type& priv, const public_key_type& pub, const string& msg, uint64_t custom_nonce)
            {
                FC_ASSERT(priv != private_key_type());
                FC_ASSERT(pub != public_key_type());
                
                from = priv.get_public_key();
                to = pub;
                if( custom_nonce == 0 )
                    nonce = fc::time_point::now().time_since_epoch().count();
                else
                    nonce = custom_nonce;
                
                auto secret = priv.get_shared_secret(pub);
                
                fc::sha512::encoder enc;
                fc::raw::pack( enc, nonce );
                fc::raw::pack( enc, secret );
                auto encrypt_key = enc.result();

                encrypted = fc::aes_encrypt( encrypt_key, fc::raw::pack_to_vector(msg) );
                check = fc::sha256::hash( encrypt_key )._hash[0];
            }
            
            string get_message(const private_key_type& priv, const public_key_type& pub) const
            {
                FC_ASSERT(priv != private_key_type());
                FC_ASSERT(pub != public_key_type());

                auto secret = priv.get_shared_secret(pub);
                
                fc::sha512::encoder enc;
                fc::raw::pack( enc, nonce );
                fc::raw::pack( enc, secret );
                auto encryption_key = enc.result();

                uint32_t _check = fc::sha256::hash( encryption_key )._hash[0];
                if( _check != check )
                   return "";

                try
                {
                    vector<char> decrypted = fc::aes_decrypt( encryption_key, encrypted );
                    return fc::raw::unpack_from_vector<std::string>( decrypted );
                } catch ( ... ) {
                    return "";
                }
            }
        };
    } //protocol
} // taiyi

namespace fc
{
    void to_variant( const taiyi::protocol::public_key_type& var,  fc::variant& vo );
    void from_variant( const fc::variant& var,  taiyi::protocol::public_key_type& vo );
    void to_variant( const taiyi::protocol::extended_public_key_type& var, fc::variant& vo );
    void from_variant( const fc::variant& var, taiyi::protocol::extended_public_key_type& vo );
    void to_variant( const taiyi::protocol::extended_private_key_type& var, fc::variant& vo );
    void from_variant( const fc::variant& var, taiyi::protocol::extended_private_key_type& vo );
}

FC_REFLECT( taiyi::protocol::public_key_type, (key_data) )
FC_REFLECT( taiyi::protocol::public_key_type::binary_key, (data)(check) )
FC_REFLECT( taiyi::protocol::extended_public_key_type, (key_data) )
FC_REFLECT( taiyi::protocol::extended_public_key_type::binary_key, (check)(data) )
FC_REFLECT( taiyi::protocol::extended_private_key_type, (key_data) )
FC_REFLECT( taiyi::protocol::extended_private_key_type::binary_key, (check)(data) )
FC_REFLECT( taiyi::protocol::memo_data, (from)(to)(nonce)(check)(encrypted) )

FC_REFLECT_TYPENAME( taiyi::protocol::share_type )

FC_REFLECT( taiyi::void_t, )

#define FORMAT_MESSAGE( FORMAT, ... ) \
    fc::format_string( FORMAT, fc::mutable_variant_object()__VA_ARGS__ )

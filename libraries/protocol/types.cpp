#include "config.hpp"
#include "types.hpp"

#include <fc/crypto/base58.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/raw.hpp>

#include "lua_types.hpp"

namespace taiyi { namespace protocol {

    public_key_type::public_key_type():key_data(){};

    public_key_type::public_key_type( const fc::ecc::public_key_data& data )
        :key_data( data ) {};

    public_key_type::public_key_type( const fc::ecc::public_key& pubkey )
        :key_data( pubkey ) {};

    public_key_type::public_key_type( const std::string& base58str )
    {
        // TODO:  Refactor syntactic checks into static is_valid()
        //        to make public_key_type API more similar to address API
        std::string prefix( TAIYI_ADDRESS_PREFIX );
        
        const size_t prefix_len = prefix.size();
        FC_ASSERT( base58str.size() > prefix_len );
        FC_ASSERT( base58str.substr( 0, prefix_len ) ==  prefix , "", ("base58str", base58str) );
        auto bin = fc::from_base58( base58str.substr( prefix_len ) );
        auto bin_key = fc::raw::unpack_from_vector<binary_key>(bin);
        key_data = bin_key.data;
        FC_ASSERT( fc::ripemd160::hash( key_data.data, key_data.size() )._hash[0] == bin_key.check );
    };


    public_key_type::operator fc::ecc::public_key_data() const
    {
        return key_data;
    };
    
    public_key_type::operator fc::ecc::public_key() const
    {
        return fc::ecc::public_key( key_data );
    };
    
    public_key_type::operator std::string() const
    {
        binary_key k;
        k.data = key_data;
        k.check = fc::ripemd160::hash( k.data.data, k.data.size() )._hash[0];
        auto data = fc::raw::pack_to_vector( k );
        return TAIYI_ADDRESS_PREFIX + fc::to_base58( data.data(), data.size() );
    }
    
    bool operator == ( const public_key_type& p1, const fc::ecc::public_key& p2)
    {
        return p1.key_data == p2.serialize();
    }
    
    bool operator == ( const public_key_type& p1, const public_key_type& p2)
    {
        return p1.key_data == p2.key_data;
    }
    
    bool operator != ( const public_key_type& p1, const public_key_type& p2)
    {
        return p1.key_data != p2.key_data;
    }
    
    // extended_public_key_type
    
    extended_public_key_type::extended_public_key_type():key_data(){};
    
    extended_public_key_type::extended_public_key_type( const fc::ecc::extended_key_data& data )
    :key_data( data ){};
    
    extended_public_key_type::extended_public_key_type( const fc::ecc::extended_public_key& extpubkey )
    {
        key_data = extpubkey.serialize_extended();
    };
    
    extended_public_key_type::extended_public_key_type( const std::string& base58str )
    {
        std::string prefix( TAIYI_ADDRESS_PREFIX );
        
        const size_t prefix_len = prefix.size();
        FC_ASSERT( base58str.size() > prefix_len );
        FC_ASSERT( base58str.substr( 0, prefix_len ) ==  prefix , "", ("base58str", base58str) );
        auto bin = fc::from_base58( base58str.substr( prefix_len ) );
        auto bin_key = fc::raw::unpack_from_vector<binary_key>(bin);
        FC_ASSERT( fc::ripemd160::hash( bin_key.data.data, bin_key.data.size() )._hash[0] == bin_key.check );
        key_data = bin_key.data;
    }

    extended_public_key_type::operator fc::ecc::extended_public_key() const
    {
        return fc::ecc::extended_public_key::deserialize( key_data );
    }
    
    extended_public_key_type::operator std::string() const
    {
        binary_key k;
        k.data = key_data;
        k.check = fc::ripemd160::hash( k.data.data, k.data.size() )._hash[0];
        auto data = fc::raw::pack_to_vector( k );
        return TAIYI_ADDRESS_PREFIX + fc::to_base58( data.data(), data.size() );
    }
    
    bool operator == ( const extended_public_key_type& p1, const fc::ecc::extended_public_key& p2)
    {
        return p1.key_data == p2.serialize_extended();
    }
    
    bool operator == ( const extended_public_key_type& p1, const extended_public_key_type& p2)
    {
        return p1.key_data == p2.key_data;
    }
    
    bool operator != ( const extended_public_key_type& p1, const extended_public_key_type& p2)
    {
        return p1.key_data != p2.key_data;
    }
    
    // extended_private_key_type
    
    extended_private_key_type::extended_private_key_type():key_data(){};
    
    extended_private_key_type::extended_private_key_type( const fc::ecc::extended_key_data& data )
    :key_data( data ){};
    
    extended_private_key_type::extended_private_key_type( const fc::ecc::extended_private_key& extprivkey )
    {
        key_data = extprivkey.serialize_extended();
    };
    
    extended_private_key_type::extended_private_key_type( const std::string& base58str )
    {
        std::string prefix( TAIYI_ADDRESS_PREFIX );
        
        const size_t prefix_len = prefix.size();
        FC_ASSERT( base58str.size() > prefix_len );
        FC_ASSERT( base58str.substr( 0, prefix_len ) ==  prefix , "", ("base58str", base58str) );
        auto bin = fc::from_base58( base58str.substr( prefix_len ) );
        auto bin_key = fc::raw::unpack_from_vector<binary_key>(bin);
        FC_ASSERT( fc::ripemd160::hash( bin_key.data.data, bin_key.data.size() )._hash[0] == bin_key.check );
        key_data = bin_key.data;
    }
    
    extended_private_key_type::operator fc::ecc::extended_private_key() const
    {
        return fc::ecc::extended_private_key::deserialize( key_data );
    }
    
    extended_private_key_type::operator std::string() const
    {
        binary_key k;
        k.data = key_data;
        k.check = fc::ripemd160::hash( k.data.data, k.data.size() )._hash[0];
        auto data = fc::raw::pack_to_vector( k );
        return TAIYI_ADDRESS_PREFIX + fc::to_base58( data.data(), data.size() );
    }
    
    bool operator == ( const extended_private_key_type& p1, const fc::ecc::extended_public_key& p2)
    {
        return p1.key_data == p2.serialize_extended();
    }
    
    bool operator == ( const extended_private_key_type& p1, const extended_private_key_type& p2)
    {
        return p1.key_data == p2.key_data;
    }
    
    bool operator != ( const extended_private_key_type& p1, const extended_private_key_type& p2)
    {
        return p1.key_data != p2.key_data;
    }
    
    chain_id_type generate_chain_id( const std::string& chain_id_name )
    {
        if( chain_id_name.size() )
            return fc::sha256::hash( chain_id_name.c_str() );
        else
            return fc::sha256();
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

} } // taiyi::protocol

namespace fc
{
    using namespace std;
    void to_variant( const taiyi::protocol::public_key_type& var,  fc::variant& vo )
    {
        vo = std::string( var );
    }
    
    void from_variant( const fc::variant& var,  taiyi::protocol::public_key_type& vo )
    {
        vo = taiyi::protocol::public_key_type( var.as_string() );
    }
    
    void to_variant( const taiyi::protocol::extended_public_key_type& var, fc::variant& vo )
    {
        vo = std::string( var );
    }
    
    void from_variant( const fc::variant& var, taiyi::protocol::extended_public_key_type& vo )
    {
        vo = taiyi::protocol::extended_public_key_type( var.as_string() );
    }
    
    void to_variant( const taiyi::protocol::extended_private_key_type& var, fc::variant& vo )
    {
        vo = std::string( var );
    }
    
    void from_variant( const fc::variant& var, taiyi::protocol::extended_private_key_type& vo )
    {
        vo = taiyi::protocol::extended_private_key_type( var.as_string() );
    }
} // fc

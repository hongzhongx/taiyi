#pragma once

#include <chain/taiyi_fwd.hpp>

#include <chainbase/chainbase.hpp>

#include <protocol/types.hpp>
#include <protocol/authority.hpp>

#include <chain/buffer_type.hpp>

#include <chain/multi_index_types.hpp>

#define TAIYI_STD_ALLOCATOR_CONSTRUCTOR( object_type )   \
   public:                                               \
      object_type () {}

namespace taiyi {
    
    namespace protocol {
        
        struct asset;
        struct price;
        
    } //protocol
    
    namespace chain {
        
        using chainbase::object;
        using chainbase::oid;
        using chainbase::allocator;
        
        using taiyi::protocol::block_id_type;
        using taiyi::protocol::transaction_id_type;
        using taiyi::protocol::chain_id_type;
        using taiyi::protocol::account_name_type;
        using taiyi::protocol::share_type;
        
        struct strcmp_less
        {
            bool operator()( const std::string& a, const std::string& b )const
            {
                return less( a.c_str(), b.c_str() );
            }
            
        private:
            inline bool less( const char* a, const char* b )const
            {
                return std::strcmp( a, b ) < 0;
            }
        };
        
        struct by_id;
        struct by_name;
        
        enum object_type
        {
            dynamic_global_property_object_type,
            account_object_type,
            account_metadata_object_type,
            account_authority_object_type,
            siming_object_type,
            transaction_object_type,
            block_summary_object_type,
            siming_schedule_object_type,
            siming_adore_object_type,
            hardfork_property_object_type,
            withdraw_qi_route_object_type,
            owner_authority_history_object_type,
            account_recovery_request_object_type,
            change_recovery_account_request_object_type,
            decline_adoring_rights_request_object_type,
            block_stats_object_type,
            reward_fund_object_type,
            qi_delegation_object_type,
            qi_delegation_expiration_object_type
        };
        
        class dynamic_global_property_object;
        class account_object;
        class account_metadata_object;
        class account_authority_object;
        class siming_object;
        class transaction_object;
        class block_summary_object;
        class siming_schedule_object;
        class siming_adore_object;
        class hardfork_property_object;
        class withdraw_qi_route_object;
        class owner_authority_history_object;
        class account_recovery_request_object;
        class change_recovery_account_request_object;
        class decline_adoring_rights_request_object;
        class block_stats_object;
        class reward_fund_object;
        class qi_delegation_object;
        class qi_delegation_expiration_object;
                
        typedef oid< dynamic_global_property_object         > dynamic_global_property_id_type;
        typedef oid< account_object                         > account_id_type;
        typedef oid< account_metadata_object                > account_metadata_id_type;
        typedef oid< account_authority_object               > account_authority_id_type;
        typedef oid< siming_object                          > siming_id_type;
        typedef oid< transaction_object                     > transaction_object_id_type;
        typedef oid< block_summary_object                   > block_summary_id_type;
        typedef oid< siming_schedule_object                 > siming_schedule_id_type;
        typedef oid< siming_adore_object                    > siming_adore_id_type;
        typedef oid< hardfork_property_object               > hardfork_property_id_type;
        typedef oid< withdraw_qi_route_object               > withdraw_qi_route_id_type;
        typedef oid< owner_authority_history_object         > owner_authority_history_id_type;
        typedef oid< account_recovery_request_object        > account_recovery_request_id_type;
        typedef oid< change_recovery_account_request_object > change_recovery_account_request_id_type;
        typedef oid< decline_adoring_rights_request_object  > decline_adoring_rights_request_id_type;
        typedef oid< block_stats_object                     > block_stats_id_type;
        typedef oid< reward_fund_object                     > reward_fund_id_type;
        typedef oid< qi_delegation_object                   > qi_delegation_id_type;
        typedef oid< qi_delegation_expiration_object        > qi_delegation_expiration_id_type;
                
    } //chain
    
} //taiyi

namespace mira {

    template< typename T > struct is_static_length< chainbase::oid< T > > : public boost::true_type {};
    template< typename T > struct is_static_length< fc::fixed_string< T > > : public boost::true_type {};
    template<> struct is_static_length< taiyi::protocol::account_name_type > : public boost::true_type {};
    template<> struct is_static_length< taiyi::protocol::asset_symbol_type > : public boost::true_type {};
    template<> struct is_static_length< taiyi::protocol::asset > : public boost::true_type {};
    template<> struct is_static_length< taiyi::protocol::price > : public boost::true_type {};

} // mira

namespace fc
{
    class variant;
    
    template<typename T>
    void to_variant( const chainbase::oid<T>& var,  variant& vo )
    {
        vo = var._id;
    }
    
    template<typename T>
    void from_variant( const variant& vo, chainbase::oid<T>& var )
    {
        var._id = vo.as_int64();
    }
    
    template< typename T >
    struct get_typename< chainbase::oid< T > >
    {
        static const char* name()
        {
            static std::string n = std::string( "chainbase::oid<" ) + get_typename< T >::name() + ">";
            return n.c_str();
        }
    };
    
    namespace raw
    {
        
        template<typename Stream, typename T>
        void pack( Stream& s, const chainbase::oid<T>& id )
        {
            s.write( (const char*)&id._id, sizeof(id._id) );
        }
        
        template<typename Stream, typename T>
        void unpack( Stream& s, chainbase::oid<T>& id, uint32_t )
        {
            s.read( (char*)&id._id, sizeof(id._id));
        }
        
        template< typename Stream, typename E, typename A >
        void pack( Stream& s, const boost::interprocess::deque<E, A>& dq )
        {
            // This could be optimized
            std::vector<E> temp;
            std::copy( dq.begin(), dq.end(), std::back_inserter(temp) );
            pack( s, temp );
        }

        template< typename Stream, typename E, typename A >
        void unpack( Stream& s, boost::interprocess::deque<E, A>& dq, uint32_t depth )
        {
            depth++;
            FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
            // This could be optimized
            std::vector<E> temp;
            unpack( s, temp, depth );
            dq.clear();
            std::copy( temp.begin(), temp.end(), std::back_inserter(dq) );
        }
        
        template< typename Stream, typename K, typename V, typename C, typename A >
        void pack( Stream& s, const boost::interprocess::flat_map< K, V, C, A >& value )
        {
            fc::raw::pack( s, unsigned_int((uint32_t)value.size()) );
            auto itr = value.begin();
            auto end = value.end();
            while( itr != end )
            {
                fc::raw::pack( s, *itr );
                ++itr;
            }
        }
        
        template< typename Stream, typename K, typename V, typename C, typename A >
        void unpack( Stream& s, boost::interprocess::flat_map< K, V, C, A >& value, uint32_t depth )
        {
            depth++;
            FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
            unsigned_int size;
            unpack( s, size, depth );
            value.clear();
            FC_ASSERT( size.value*(sizeof(K)+sizeof(V)) < MAX_ARRAY_ALLOC_SIZE );
            for( uint32_t i = 0; i < size.value; ++i )
            {
                std::pair<K,V> tmp;
                fc::raw::unpack( s, tmp, depth );
                value.insert( std::move(tmp) );
            }
        }

    } //raw
    
} // namespace fc

FC_REFLECT_ENUM( taiyi::chain::object_type,
    (dynamic_global_property_object_type)
    (account_object_type)
    (account_metadata_object_type)
    (account_authority_object_type)
    (siming_object_type)
    (transaction_object_type)
    (block_summary_object_type)
    (siming_schedule_object_type)
    (siming_adore_object_type)
    (hardfork_property_object_type)
    (withdraw_qi_route_object_type)
    (owner_authority_history_object_type)
    (account_recovery_request_object_type)
    (change_recovery_account_request_object_type)
    (decline_adoring_rights_request_object_type)
    (block_stats_object_type)
    (reward_fund_object_type)
    (qi_delegation_object_type)
    (qi_delegation_expiration_object_type)
)

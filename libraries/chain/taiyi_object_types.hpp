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
            qi_delegation_expiration_object_type,

            //asset objects
            account_regular_balance_object_type,
            account_rewards_balance_object_type,
            nfa_regular_balance_object_type,

            //contract objects
            contract_object_type,
            account_contract_data_object_type,
            contract_bin_code_object_type,

            //nfa objects
            nfa_symbol_object_type,
            nfa_object_type,
            
            //actor objects
            actor_object_type,
            actor_core_attributes_object_type,
            actor_group_object_type,
            actor_talent_rule_object_type,
            actor_talents_object_type,
            
            //zone objects
            zone_object_type,
            zone_connect_object_type,
            cunzhuang_object_type,
            tiandao_property_object_type
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
                
        //asset objects
        class account_regular_balance_object;
        class account_rewards_balance_object;
        class nfa_regular_balance_object;

        //contract objects
        class contract_object;
        class account_contract_data_object;
        class contract_bin_code_object;

        class nfa_symbol_object;
        class nfa_object;
        
        class actor_object;
        class actor_core_attributes_object;
        class actor_group_object;
        class actor_talent_rule_object;
        class actor_talents_object;

        class zone_object;
        class zone_connect_object;
        class cunzhuang_object;
        class tiandao_property_object;

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
                
        //asset objects
        typedef oid< account_regular_balance_object         > account_regular_balance_id_type;
        typedef oid< account_rewards_balance_object         > account_rewards_balance_id_type;
        typedef oid< nfa_regular_balance_object             > nfa_regular_balance_id_type;

        //contract objects
        typedef oid< contract_object                        > contract_id_type;
        typedef oid< account_contract_data_object           > account_contract_data_id_type;
        typedef oid< contract_bin_code_object               > contract_bin_code_id_type;

        typedef oid< nfa_symbol_object                      > nfa_symbol_id_type;
        typedef oid< nfa_object                             > nfa_id_type;
        
        typedef oid< actor_object                           > actor_id_type;
        typedef oid< actor_core_attributes_object           > actor_core_attributes_id_type;
        typedef oid< actor_group_object                     > actor_group_id_type;
        typedef oid< actor_talent_rule_object               > actor_talent_rule_id_type;
        typedef oid< actor_talents_object                   > actor_talents_id_type;

        typedef oid< zone_object                            > zone_id_type;
        typedef oid< zone_connect_object                    > zone_connect_id_type;
        typedef oid< cunzhuang_object                       > cunzhuang_id_type;
        typedef oid< tiandao_property_object                > tiandao_property_id_type;

        enum E_ZONE_TYPE
        {
            YUANYE = 0,     //原野
            HUPO,           //湖泊
            NONGTIAN,       //农田

            LINDI,          //林地
            MILIN,          //密林
            YUANLIN,        //园林
            
            SHANYUE,        //山岳
            DONGXUE,        //洞穴
            SHILIN,         //石林
            
            QIULIN,         //丘陵
            TAOYUAN,        //桃源
            SANGYUAN,       //桑园
            
            XIAGU,          //峡谷
            ZAOZE,          //沼泽
            YAOYUAN,        //药园
            
            HAIYANG,        //海洋
            SHAMO,          //沙漠
            HUANGYE,        //荒野
            ANYUAN,         //暗渊
            _NATURE_ZONE_TYPE_NUM,

            DUHUI = _NATURE_ZONE_TYPE_NUM,  //都会
            MENPAI,         //门派
            SHIZHEN,        //市镇
            GUANSAI,        //关寨
            CUNZHUANG,      //村庄
            
            _ZONE_TYPE_NUM,
            _ZONE_INVALID_TYPE
        };
        
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

    //asset objects
    (account_regular_balance_object_type)
    (account_rewards_balance_object_type)
    (nfa_regular_balance_object_type)

    //contract objects
    (contract_object_type)
    (account_contract_data_object_type)
    (contract_bin_code_object_type)                

    //nfa objects
    (nfa_symbol_object_type)
    (nfa_object_type)
                
    //actor objects
    (actor_object_type)
    (actor_core_attributes_object_type)
    (actor_group_object_type)
    (actor_talent_rule_object_type)
    (actor_talents_object_type)
                
    //zone objects
    (zone_object_type)
    (zone_connect_object_type)
    (cunzhuang_object_type)
    (tiandao_property_object_type)
)

FC_REFLECT_ENUM( taiyi::chain::E_ZONE_TYPE, (YUANYE)(HUPO)(NONGTIAN)(LINDI)(MILIN)(YUANLIN)(SHANYUE)(DONGXUE)(SHILIN)(QIULIN)(TAOYUAN)(SANGYUAN)(XIAGU)(ZAOZE)(YAOYUAN)(HAIYANG)(SHAMO)(HUANGYE)(ANYUAN)(DUHUI)(MENPAI)(SHIZHEN)(GUANSAI)(CUNZHUANG))

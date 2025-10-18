#pragma once
#include <chain/taiyi_fwd.hpp>

#include <protocol/authority.hpp>
#include <protocol/taiyi_operations.hpp>

#include <chain/taiyi_object_types.hpp>

namespace taiyi { namespace chain {

    struct zone_creation_data
    {
        string            name;
        E_ZONE_TYPE       type;
    };

    class zone_object : public object < zone_object_type, zone_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR(zone_object)

    public:
        template< typename Constructor, typename Allocator >
        zone_object(Constructor&& c, allocator< Allocator > a)
            :name(a)
        {
            c(*this);
        }

        id_type             id;
        nfa_id_type         nfa_id      = nfa_id_type::max();

        std::string         name;
        E_ZONE_TYPE         type;
        uint32_t            last_grow_vmonth = 0;
        
        id_type             ref_prohibited_contract_zone = id_type::max();
    };

    struct by_name;
    struct by_type;
    struct by_nfa_id;
    typedef multi_index_container<
        zone_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< zone_object, zone_id_type, &zone_object::id > >,
            ordered_unique< tag< by_name >, member< zone_object, std::string, &zone_object::name >, strcmp_less >,
            ordered_unique< tag< by_nfa_id >, member< zone_object, nfa_id_type, &zone_object::nfa_id> >,
            ordered_unique< tag< by_type >,
                composite_key< zone_object,
                    member< zone_object, E_ZONE_TYPE, &zone_object::type >,
                    member< zone_object, zone_id_type, &zone_object::id >
                >
            >
        >,
        allocator< zone_object >
    > zone_index;
    //=========================================================================
    class zone_connect_object : public object < zone_connect_object_type, zone_connect_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR(zone_connect_object)

    public:
        template< typename Constructor, typename Allocator >
        zone_connect_object(Constructor&& c, allocator< Allocator > a)
        {
            c(*this);
        }

        id_type             id;
        zone_id_type        from;
        zone_id_type        to;
    };

    struct by_zone_from;
    struct by_zone_to;
    typedef multi_index_container<
        zone_connect_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< zone_connect_object, zone_connect_id_type, &zone_connect_object::id > >,
            ordered_unique< tag< by_zone_from >,
                composite_key< zone_connect_object,
                    member< zone_connect_object, zone_id_type, &zone_connect_object::from>,
                    member< zone_connect_object, zone_id_type, &zone_connect_object::to >
                >
            >,
            ordered_unique< tag< by_zone_to >,
                composite_key< zone_connect_object,
                    member< zone_connect_object, zone_id_type, &zone_connect_object::to>,
                    member< zone_connect_object, zone_id_type, &zone_connect_object::from >
                >
            >
        >,
        allocator< zone_connect_object >
    > zone_connect_index;
    //=========================================================================
    class zone_contract_permission_object : public object < zone_contract_permission_object_type, zone_contract_permission_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR(zone_contract_permission_object)

    public:
        template< typename Constructor, typename Allocator >
        zone_contract_permission_object(Constructor&& c, allocator< Allocator > a)
        {
            c(*this);
        }

        id_type             id;
        zone_id_type        zone = zone_id_type::max();
        contract_id_type    contract = contract_id_type::max();
        bool                allowed = false;
    };

    struct by_zone;
    struct by_contract;
    typedef multi_index_container<
        zone_contract_permission_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< zone_contract_permission_object, zone_contract_permission_id_type, &zone_contract_permission_object::id > >,
            ordered_unique< tag< by_zone >,
                composite_key< zone_contract_permission_object,
                    member< zone_contract_permission_object, zone_id_type, &zone_contract_permission_object::zone>,
                    member< zone_contract_permission_object, contract_id_type, &zone_contract_permission_object::contract >
                >
            >,
            ordered_unique< tag< by_contract >,
                composite_key< zone_contract_permission_object,
                    member< zone_contract_permission_object, contract_id_type, &zone_contract_permission_object::contract>,
                    member< zone_contract_permission_object, zone_id_type, &zone_contract_permission_object::zone >
                >
            >
        >,
        allocator< zone_contract_permission_object >
    > zone_contract_permission_index;
    //=========================================================================
    class cunzhuang_object : public object < cunzhuang_object_type, cunzhuang_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR(cunzhuang_object)

    public:
        template< typename Constructor, typename Allocator >
        cunzhuang_object(Constructor&& c, allocator< Allocator > a)
        {
            c(*this);
        }

        id_type             id;
        zone_id_type        zone;
        actor_id_type       chief;
    };

    struct by_chief;
    struct by_zone;
    typedef multi_index_container<
        cunzhuang_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< cunzhuang_object, cunzhuang_id_type, &cunzhuang_object::id > >,
            ordered_unique< tag< by_zone >, member< cunzhuang_object, zone_id_type, &cunzhuang_object::zone > >,
            ordered_unique< tag< by_chief >,
                composite_key< cunzhuang_object,
                    member< cunzhuang_object, actor_id_type, &cunzhuang_object::chief>,
                    member< cunzhuang_object, cunzhuang_id_type, &cunzhuang_object::id >
                >
            >
        >,
        allocator< cunzhuang_object >
    > cunzhuang_index;

    E_ZONE_TYPE get_zone_type_from_string(const std::string& sType);
    string get_zone_type_string(E_ZONE_TYPE type);

} } // taiyi::chain

namespace mira {

    template<> struct is_static_length< taiyi::chain::cunzhuang_object > : public boost::true_type {};
    template<> struct is_static_length< taiyi::chain::zone_connect_object > : public boost::true_type {};
    template<> struct is_static_length< taiyi::chain::zone_contract_permission_object > : public boost::true_type {};

} // mira

FC_REFLECT( taiyi::chain::zone_creation_data, (name)(type) )

FC_REFLECT(taiyi::chain::zone_object, (id)(nfa_id)(name)(type)(last_grow_vmonth)(ref_prohibited_contract_zone))
CHAINBASE_SET_INDEX_TYPE(taiyi::chain::zone_object, taiyi::chain::zone_index)

FC_REFLECT(taiyi::chain::cunzhuang_object, (id)(zone)(chief))
CHAINBASE_SET_INDEX_TYPE(taiyi::chain::cunzhuang_object, taiyi::chain::cunzhuang_index)

FC_REFLECT(taiyi::chain::zone_connect_object, (id)(from)(to))
CHAINBASE_SET_INDEX_TYPE(taiyi::chain::zone_connect_object, taiyi::chain::zone_connect_index)

FC_REFLECT(taiyi::chain::zone_contract_permission_object, (id)(zone)(contract)(allowed))
CHAINBASE_SET_INDEX_TYPE(taiyi::chain::zone_contract_permission_object, taiyi::chain::zone_contract_permission_index)

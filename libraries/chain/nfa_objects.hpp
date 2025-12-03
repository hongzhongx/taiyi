#pragma once
#include <chain/taiyi_fwd.hpp>

#include <protocol/authority.hpp>
#include <protocol/taiyi_operations.hpp>

#include <chain/taiyi_object_types.hpp>

namespace taiyi { namespace chain {

    using protocol::lua_map;
    using protocol::asset;

    class nfa_symbol_object : public object < nfa_symbol_object_type, nfa_symbol_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR(nfa_symbol_object)
        
    public:
        template< typename Constructor, typename Allocator >
        nfa_symbol_object(Constructor&& c, allocator< Allocator > a)
            : symbol(a), describe(a)
        {
            c(*this);
        }
        
        id_type                     id;

        account_id_type             creator_account;
        account_id_type             authority_account;
        string                      symbol;
        string                      describe;
        contract_id_type            default_contract = contract_id_type::max();
        uint64_t                    count = 0;
        uint64_t                    max_count = 0;
        uint64_t                    min_equivalent_qi = 0;  //最低等效真气。材质总等效真气如果低于这个值被视为损坏
        bool                        is_sbt = false; //是否是灵魂绑定的 Soulbound Token (SBT) 
    };

    struct by_symbol;
    struct by_contract;
    typedef multi_index_container<
        nfa_symbol_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< nfa_symbol_object, nfa_symbol_id_type, &nfa_symbol_object::id > >,
            ordered_unique< tag< by_symbol >, member< nfa_symbol_object, string, &nfa_symbol_object::symbol > >,
            ordered_unique< tag< by_contract >, member< nfa_symbol_object, contract_id_type, &nfa_symbol_object::default_contract > >
        >,
        allocator< nfa_symbol_object >
    > nfa_symbol_index;

    //=========================================================================
    
    class nfa_object : public object < nfa_object_type, nfa_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR(nfa_object)

    public:
        template< typename Constructor, typename Allocator >
        nfa_object(Constructor&& c, allocator< Allocator > a)
            : contract_data(a)
        {
            c(*this);
        }

        id_type             id;
        
        account_id_type     creator_account;
        account_id_type     owner_account;
        account_id_type     active_account; // the account who has the usage rights of the nfa

        nfa_symbol_id_type  symbol_id;
        
        nfa_id_type         parent = nfa_id_type::max();
        
        contract_id_type    main_contract = contract_id_type::max();
        lua_map             contract_data;
        
        asset               qi = asset( 0, QI_SYMBOL ); /// total qi shares held by this nfa, controls its heart_beat power

        // 如果因心跳或者强制执行核心循环造成了欠费，则不能触发下次心跳或核心循环
        // 有欠费的情况下，针对NFA视角的任何调用都可能先扣除欠费，然后再继续进行
        int64_t             debt_value = 0; /// 欠费的真气值
        contract_id_type    debt_contract = contract_id_type::max(); /// 欠费的债主合约
        
        uint64_t            cultivation_value = 0; ///参与修真的真气值，> 0表示正在参与修真
        
        time_point_sec      created_time;
        uint32_t            next_tick_block = std::numeric_limits<uint32_t>::max();
    };

    struct by_symbol;
    struct by_owner;
    struct by_creator;
    struct by_parent;
    struct by_tick_time;
    //struct by_cultivation_value;
    typedef multi_index_container<
        nfa_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< nfa_object, nfa_id_type, &nfa_object::id > >,
            ordered_unique< tag< by_symbol >, 
                composite_key< nfa_object,
                    member< nfa_object, nfa_symbol_id_type, &nfa_object::symbol_id >,
                    member< nfa_object, nfa_id_type, &nfa_object::id >
                >
            >,
            ordered_unique< tag< by_owner >,
                composite_key< nfa_object,
                    member< nfa_object, account_id_type, &nfa_object::owner_account >,
                    member< nfa_object, nfa_id_type, &nfa_object::id >
                >
            >,
            ordered_unique< tag< by_creator >,
                composite_key< nfa_object,
                    member< nfa_object, account_id_type, &nfa_object::creator_account >,
                    member< nfa_object, nfa_id_type, &nfa_object::id >
                >
            >,
            ordered_unique< tag< by_parent >,
                composite_key< nfa_object,
                    member< nfa_object, nfa_id_type, &nfa_object::parent >,
                    member< nfa_object, nfa_id_type, &nfa_object::id >
                >
            >,
//            ordered_unique< tag< by_cultivation_value >,
//                composite_key< nfa_object,
//                    member< nfa_object, uint64_t, &nfa_object::cultivation_value >,
//                    member< nfa_object, nfa_id_type, &nfa_object::id >
//                >,
//                composite_key_compare< std::greater< uint64_t >, std::less< nfa_id_type > >
//            >,
            ordered_unique< tag< by_tick_time >,
                composite_key< nfa_object,
                    member< nfa_object, uint32_t, &nfa_object::next_tick_block >,
                    member< nfa_object, nfa_id_type, &nfa_object::id >
                >
            >
        >,
        allocator< nfa_object >
    > nfa_index;
        
    //=========================================================================

    class nfa_material_object : public object < nfa_material_object_type, nfa_material_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR(nfa_material_object)

    public:
        template< typename Constructor, typename Allocator >
        nfa_material_object(Constructor&& c, allocator< Allocator > a)
        {
            c(*this);
        }

        id_type             id;

        nfa_id_type         nfa = nfa_id_type::max();
        
        asset               gold = asset( 0, GOLD_SYMBOL );
        asset               food = asset( 0, FOOD_SYMBOL );
        asset               wood = asset( 0, WOOD_SYMBOL );
        asset               fabric = asset( 0, FABRIC_SYMBOL );
        asset               herb = asset( 0, HERB_SYMBOL );
        
        int64_t get_material_qi() const;
    };

    struct by_nfa_id;
    typedef multi_index_container<
        nfa_material_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< nfa_material_object, nfa_material_id_type, &nfa_material_object::id > >,
            ordered_unique< tag< by_nfa_id >, member< nfa_material_object, nfa_id_type, &nfa_material_object::nfa> >
        >,
        allocator< nfa_material_object >
    > nfa_material_index;

} } // taiyi::chain

namespace mira {

    template<> struct is_static_length< taiyi::chain::nfa_material_object > : public boost::true_type {};

} // mira

FC_REFLECT(taiyi::chain::nfa_symbol_object, (id)(creator_account)(symbol)(describe)(default_contract)(count)(max_count)(min_equivalent_qi)(is_sbt))
CHAINBASE_SET_INDEX_TYPE(taiyi::chain::nfa_symbol_object, taiyi::chain::nfa_symbol_index)

FC_REFLECT(taiyi::chain::nfa_object, (id)(creator_account)(owner_account)(active_account)(symbol_id)(parent)(main_contract)(contract_data)(qi)(debt_value)(debt_contract)(cultivation_value)(created_time)(next_tick_block))
CHAINBASE_SET_INDEX_TYPE(taiyi::chain::nfa_object, taiyi::chain::nfa_index)

FC_REFLECT(taiyi::chain::nfa_material_object, (id)(nfa)(gold)(food)(wood)(fabric)(herb))
CHAINBASE_SET_INDEX_TYPE(taiyi::chain::nfa_material_object, taiyi::chain::nfa_material_index)

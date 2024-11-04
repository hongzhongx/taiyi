#pragma once
#include <chain/taiyi_fwd.hpp>

#include <protocol/authority.hpp>
#include <protocol/taiyi_operations.hpp>

#include <chain/taiyi_object_types.hpp>

#include <chain/util/manabar.hpp>

namespace taiyi { namespace chain {

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

        account_name_type           creator;
        string                      symbol;
        string                      describe;
        contract_id_type            default_contract = std::numeric_limits<contract_id_type>::max();
        uint64_t                    count = 0;
    };

    struct by_symbol;
    typedef multi_index_container<
        nfa_symbol_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< nfa_symbol_object, nfa_symbol_id_type, &nfa_symbol_object::id > >,
            ordered_unique< tag< by_symbol >, member< nfa_symbol_object, string, &nfa_symbol_object::symbol > >
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
            : parents(a), children(a), data(a)
        {
            c(*this);
        }

        id_type                     id;
        
        account_id_type             creator_account;
        account_id_type             owner_account;

        nfa_symbol_id_type          symbol_id;
        
        vector<nfa_id_type>         parents;
        vector<nfa_id_type>         children;
        
        contract_id_type            main_contract = std::numeric_limits<contract_id_type>::max();
        lua_map                     data;
        
        asset                       qi_shares = asset( 0, QI_SYMBOL ); ///< total qi shares held by this nfa, controls its heart_beat power
        util::manabar               manabar;

        time_point_sec              created_time;
    };

    struct by_nfa_symbol;
    struct by_owner;
    struct by_creator;
    typedef multi_index_container<
        nfa_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< nfa_object, nfa_id_type, &nfa_object::id > >,
            ordered_unique< tag< by_nfa_symbol >, 
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
            >
        >,
        allocator< nfa_object >
    > nfa_index;
    
    namespace util {
        //对get_effective_qi_shares模版函数的特化
        template<> int64_t get_effective_qi_shares<nfa_object>( const nfa_object& obj );
    }

} } // taiyi::chain

FC_REFLECT(taiyi::chain::nfa_symbol_object, (id)(creator)(symbol)(describe)(default_contract)(count))
CHAINBASE_SET_INDEX_TYPE(taiyi::chain::nfa_symbol_object, taiyi::chain::nfa_symbol_index)

FC_REFLECT(taiyi::chain::nfa_object, (id)(creator_account)(owner_account)(symbol_id)(parents)(children)(main_contract)(data)(qi_shares)(manabar)(created_time))
CHAINBASE_SET_INDEX_TYPE(taiyi::chain::nfa_object, taiyi::chain::nfa_index)

#pragma once

#include <chain/taiyi_object_types.hpp>

namespace taiyi { namespace chain {

    /**
     * Class responsible for holding resource balance of FA for given NFA.
     */
    class nfa_regular_balance_object : public object< nfa_regular_balance_object_type, nfa_regular_balance_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR( nfa_regular_balance_object );
        
        public:
        template <typename Constructor, typename Allocator>
        nfa_regular_balance_object(Constructor&& c, allocator< Allocator > a)
        {
            c( *this );
        }
        
        id_type             id;
        
        nfa_id_type         nfa;
        
        asset               liquid;   /// 'balance' for YANG
        
        /** Set of simple methods that allow unification of
         *  regular and rewards balance manipulation code.
         */
        ///@{
        asset_symbol_type get_liquid_symbol() const { return liquid.symbol; }
        void clear_balance( asset_symbol_type liquid_symbol )
        {
            nfa = -1;
            liquid = asset( 0, liquid_symbol);
        }
        ///@}
    };

    struct by_nfa_liquid_symbol;
    typedef multi_index_container <
        nfa_regular_balance_object,
        indexed_by <
            ordered_unique< tag<by_id>, member< nfa_regular_balance_object, nfa_regular_balance_id_type, &nfa_regular_balance_object::id> >,
            ordered_unique< tag<by_nfa_liquid_symbol>,
                composite_key<nfa_regular_balance_object,
                    member< nfa_regular_balance_object, nfa_id_type, &nfa_regular_balance_object::nfa >,
                    const_mem_fun< nfa_regular_balance_object, asset_symbol_type, &nfa_regular_balance_object::get_liquid_symbol >
                >
            >
        >,
        allocator< nfa_regular_balance_object >
    > nfa_regular_balance_index;

} } // namespace taiyi::chain

namespace mira {

    template<> struct is_static_length< taiyi::chain::nfa_regular_balance_object > : public boost::true_type {};

} // mira

FC_REFLECT( taiyi::chain::nfa_regular_balance_object, (id)(nfa)(liquid) )

CHAINBASE_SET_INDEX_TYPE( taiyi::chain::nfa_regular_balance_object, taiyi::chain::nfa_regular_balance_index )

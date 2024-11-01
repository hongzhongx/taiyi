#pragma once

#include <chain/taiyi_object_types.hpp>

namespace taiyi { namespace chain {

    /**
     * Class responsible for holding regular (i.e. non-reward) balance of FA for given account.
     * It has not been unified with reward balance object counterpart, due to different number
     * of fields needed to hold balances (2 for regular, 3 for reward).
     */
    class account_regular_balance_object : public object< account_regular_balance_object_type, account_regular_balance_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR( account_regular_balance_object );
        
        public:
        template <typename Constructor, typename Allocator>
        account_regular_balance_object(Constructor&& c, allocator< Allocator > a)
        {
            c( *this );
        }

        // id_type is actually oid<account_regular_balance_object>
        id_type             id;
        /// Name of the account, the balance is held for.
        account_name_type   owner;
        asset               liquid;     /// 'balance' for resource asset
        asset               qi;         /// 'qi_shares' for QI

        /** Set of simple methods that allow unification of
         *  regular and rewards balance manipulation code.
         */
        ///@{
        asset_symbol_type get_liquid_symbol() const
        {
            return liquid.symbol;
        }
        
        void clear_balance( asset_symbol_type liquid_symbol )
        {
            owner = "";
            liquid = asset( 0, liquid_symbol);
            qi = asset( 0, liquid_symbol.get_paired_symbol() );
        }
        
        void add_qi( const asset& qi_shares, const asset& qi_value )
        {
            // There's no need to store qi value (in liquid FA variant) in regular balance.
            qi += qi_shares;
        }
        ///@}
        
        bool validate() const { return liquid.symbol == qi.symbol.get_paired_symbol(); }
    };

    /**
     * Class responsible for holding reward balance of FA for given account.
     * It has not been unified with regular balance object counterpart, due to different number
     * of fields needed to hold balances (2 for regular, 3 for reward).
     */
    class account_rewards_balance_object : public object< account_rewards_balance_object_type, account_rewards_balance_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR( account_rewards_balance_object );
        
        public:
        template <typename Constructor, typename Allocator>
        account_rewards_balance_object(Constructor&& c, allocator< Allocator > a)
        {
            c( *this );
        }

        // id_type is actually oid<account_rewards_balance_object>
        id_type             id;
        /// Name of the account, the balance is held for.
        account_name_type   owner;
        asset               pending_liquid;     /// 'reward_yang_balance' for pending resource asset
        asset               pending_qi_shares;  /// 'reward_qi_balance' for pending QI
        
        /** Set of simple methods that allow unification of
         *  regular and rewards balance manipulation code.
         */
        ///@{
        asset_symbol_type get_liquid_symbol() const
        {
            return pending_liquid.symbol;
        }
        
        void clear_balance( asset_symbol_type liquid_symbol )
        {
            owner = "";
            pending_liquid = asset( 0, liquid_symbol);
            pending_qi_shares = asset( 0, liquid_symbol.get_paired_symbol() );
        }
        
        void add_qi( const asset& qi_shares, const asset& qi_liquid )
        {
            pending_qi_shares += qi_shares;
        }
        ///@}
        
        bool validate() const
        {
            return pending_liquid.symbol == pending_qi_shares.symbol.get_paired_symbol();
        }
    };
    
    struct by_owner_liquid_symbol;

    typedef multi_index_container <
        account_regular_balance_object,
            indexed_by <
                ordered_unique< tag< by_id >,
                    member< account_regular_balance_object, account_regular_balance_id_type, &account_regular_balance_object::id>
                >,
                ordered_unique<tag<by_owner_liquid_symbol>,
                    composite_key<account_regular_balance_object,
                        member< account_regular_balance_object, account_name_type, &account_regular_balance_object::owner >,
                        const_mem_fun< account_regular_balance_object, asset_symbol_type, &account_regular_balance_object::get_liquid_symbol >
                    >
                >
            >,
        allocator< account_regular_balance_object >
    > account_regular_balance_index;

    typedef multi_index_container <
        account_rewards_balance_object,
            indexed_by <
                ordered_unique< tag< by_id >,
                    member< account_rewards_balance_object, account_rewards_balance_id_type, &account_rewards_balance_object::id>
                >,
                ordered_unique<tag<by_owner_liquid_symbol>,
                    composite_key<account_rewards_balance_object,
                        member< account_rewards_balance_object, account_name_type, &account_rewards_balance_object::owner >,
                        const_mem_fun< account_rewards_balance_object, asset_symbol_type, &account_rewards_balance_object::get_liquid_symbol >
                    >
                >
            >,
        allocator< account_rewards_balance_object >
    > account_rewards_balance_index;

} } // namespace taiyi::chain

namespace mira {

   template<> struct is_static_length< taiyi::chain::account_regular_balance_object > : public boost::true_type {};
   template<> struct is_static_length< taiyi::chain::account_rewards_balance_object > : public boost::true_type {};

} // mira

FC_REFLECT( taiyi::chain::account_regular_balance_object, (id)(owner)(liquid)(qi) )
CHAINBASE_SET_INDEX_TYPE( taiyi::chain::account_regular_balance_object, taiyi::chain::account_regular_balance_index )

FC_REFLECT( taiyi::chain::account_rewards_balance_object, (id)(owner)(pending_liquid)(pending_qi_shares) )
CHAINBASE_SET_INDEX_TYPE( taiyi::chain::account_rewards_balance_object, taiyi::chain::account_rewards_balance_index )

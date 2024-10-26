#pragma once
#include <chain/taiyi_fwd.hpp>

#include <fc/uint128.hpp>

#include <chain/taiyi_object_types.hpp>

#include <protocol/asset.hpp>

namespace taiyi { namespace chain {

    using taiyi::protocol::asset;
    using taiyi::protocol::price;

    /**
     * @class dynamic_global_property_object
     * @brief Maintains global state information
     * @ingroup object
     * @ingroup implementation
     *
     * This is an implementation detail. The values here are calculated during normal chain operations and reflect the
     * current values of global blockchain properties.
     */
    class dynamic_global_property_object : public object< dynamic_global_property_object_type, dynamic_global_property_object>
    {
    public:
        template< typename Constructor, typename Allocator >
        dynamic_global_property_object( Constructor&& c, allocator< Allocator > a )
        {
            c( *this );
        }

        dynamic_global_property_object(){}

        id_type           id;

        uint32_t          head_block_number = 0;
        block_id_type     head_block_id;
        time_point_sec    time;
        account_name_type current_siming;


        asset       virtual_supply              = asset( 0, YANG_SYMBOL );
        asset       current_supply              = asset( 0, YANG_SYMBOL );
        asset       confidential_supply         = asset( 0, YANG_SYMBOL ); ///< total asset held in confidential balances
        asset       total_qi_fund_yang          = asset( 0, YANG_SYMBOL );
        asset       total_qi_shares             = asset( 0, QI_SYMBOL );
        asset       total_reward_fund_yang      = asset( 0, YANG_SYMBOL );
        asset       pending_rewarded_qi_shares  = asset( 0, QI_SYMBOL );
        asset       pending_rewarded_qi_yang    = asset( 0, YANG_SYMBOL );

        price get_qi_share_price() const
        {
            if ( total_qi_fund_yang.amount == 0 || total_qi_shares.amount == 0 )
                return price ( asset( 1000, YANG_SYMBOL ), asset( 1000000, QI_SYMBOL ) );
            
            return price( total_qi_shares, total_qi_fund_yang );
        }

        price get_reward_qi_share_price() const
        {
            return price( total_qi_shares + pending_rewarded_qi_shares, total_qi_fund_yang + pending_rewarded_qi_yang );
        }
        
        /**
         *  Maximum block size is decided by the set of active simings which change every round.
         *  Each siming posts what they think the maximum size should be as part of their siming
         *  properties, the median size is chosen to be the maximum block size for the round.
         *
         *  @note the minimum value for maximum_block_size is defined by the protocol to prevent the
         *  network from getting stuck by simings attempting to set this too low.
         */
        uint32_t maximum_block_size = 0;

        /**
         * The current absolute slot number.  Equal to the total
         * number of slots since genesis.  Also equal to the total
         * number of missed slots plus head_block_number.
         */
        uint64_t current_aslot = 0;
        
        /**
         * used to compute siming participation.
         */
        fc::uint128_t recent_slots_filled;
        uint8_t participation_count = 0; ///< Divide by 128 to compute participation percentage
        
        uint32_t last_irreversible_block_num = 0;
                
        uint32_t delegation_return_period = TAIYI_DELEGATION_RETURN_PERIOD;
                                
        uint16_t content_reward_percent = TAIYI_CONTENT_REWARD_PERCENT;
        uint16_t qi_reward_percent = TAIYI_QI_FUND_PERCENT;
        uint16_t sps_fund_percent = TAIYI_PROPOSAL_FUND_PERCENT;        
    };

    typedef multi_index_container<
        dynamic_global_property_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< dynamic_global_property_object, dynamic_global_property_object::id_type, &dynamic_global_property_object::id > >
        >,
        allocator< dynamic_global_property_object >
    > dynamic_global_property_index;

} } // taiyi::chain

namespace mira {
    template<> struct is_static_length< taiyi::chain::dynamic_global_property_object > : public boost::true_type {};
} // mira

FC_REFLECT( taiyi::chain::dynamic_global_property_object, (id)(head_block_number)(head_block_id)(time)(current_siming)(virtual_supply)(current_supply)(confidential_supply)(total_qi_fund_yang)(total_qi_shares)(total_reward_fund_yang)(pending_rewarded_qi_shares)(pending_rewarded_qi_yang)(maximum_block_size)(current_aslot)(recent_slots_filled)(participation_count)(last_irreversible_block_num)(delegation_return_period)(content_reward_percent)(qi_reward_percent)(sps_fund_percent) )
CHAINBASE_SET_INDEX_TYPE( taiyi::chain::dynamic_global_property_object, taiyi::chain::dynamic_global_property_index )

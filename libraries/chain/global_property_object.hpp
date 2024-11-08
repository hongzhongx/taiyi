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


        asset       current_supply              = asset( 0, YANG_SYMBOL );  ///当前总阳寿供应量（包含转换为真气的阳寿）

        asset       total_qi             = asset( 0, QI_SYMBOL );    ///当前总的真气
        asset       pending_rewarded_qi  = asset( 0, QI_SYMBOL );    ///当前待领取真气奖励总量
        
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
                                
        uint16_t content_reward_yang_percent = TAIYI_CONTENT_REWARD_YANG_PERCENT;
        uint16_t content_reward_qi_fund_percent = TAIYI_CONTENT_REWARD_QI_FUND_PERCENT;
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

FC_REFLECT( taiyi::chain::dynamic_global_property_object, (id)(head_block_number)(head_block_id)(time)(current_siming)(current_supply)(total_qi)(pending_rewarded_qi)(maximum_block_size)(current_aslot)(recent_slots_filled)(participation_count)(last_irreversible_block_num)(delegation_return_period)(content_reward_yang_percent)(content_reward_qi_fund_percent) )
CHAINBASE_SET_INDEX_TYPE( taiyi::chain::dynamic_global_property_object, taiyi::chain::dynamic_global_property_index )

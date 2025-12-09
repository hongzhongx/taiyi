#pragma once
#include <chain/taiyi_fwd.hpp>

#include <protocol/authority.hpp>
#include <protocol/taiyi_operations.hpp>

#include <chain/taiyi_object_types.hpp>

namespace taiyi { namespace chain {

    using taiyi::protocol::digest_type;
    using taiyi::protocol::public_key_type;
    using taiyi::protocol::version;
    using taiyi::protocol::hardfork_version;
    using taiyi::protocol::price;
    using taiyi::protocol::asset;
    using taiyi::protocol::asset_symbol_type;
    
    /**
     * Simings must vote on how to set certain chain properties to ensure a smooth
     * and well functioning network.  Any time @owner is in the active set of simings these
     * properties will be used to control the blockchain configuration.
     */
    struct chain_properties
    {
        /**
         *  This fee, paid in YANG, is converted into QI SHARES for the new account. Accounts
         *  without qi shares cannot earn usage rations and therefore are powerless. This minimum
         *  fee requires all accounts to have some kind of commitment to the network that includes the
         *  ability to vote and make transactions.
         */
        asset account_creation_fee = asset( TAIYI_MIN_ACCOUNT_CREATION_FEE, YANG_SYMBOL );
        
        /**
         *  This simings vote for the maximum_block_size which is used by the network
         *  to tune rate limiting and capacity
         */
        uint32_t maximum_block_size = TAIYI_MIN_BLOCK_SIZE_LIMIT * 2;
        
        /**
         *  The minimum number of votes required for the proposal to be adopted. If the total amount
         *  of xinsu is lower than this value, then the total amount of xinsu shall be regarded as the
         *  minimum requirement.
         */
        uint32_t proposal_adopted_votes_threshold = TAIYI_PROPOSAL_ADOPTED_VOTES_THRESHOLD_INIT;
    };

    /**
     *  All simings with at least 1% net positive approval and
     *  at least 2 weeks old are able to participate in block
     *  production.
     */
    class siming_object : public object< siming_object_type, siming_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR( siming_object )
        
    public:
        enum siming_schedule_type
        {
            elected,
            timeshare,
            miner,
            none
        };

        template< typename Constructor, typename Allocator >
        siming_object( Constructor&& c, allocator< Allocator > a )
            :url( a )
        {
            c( *this );
        }

        id_type           id;
        
        /** the account that has authority over this siming */
        account_name_type owner;
        time_point_sec    created;
        std::string       url;
        uint32_t          total_missed = 0;
        uint64_t          last_aslot = 0;
        uint64_t          last_confirmed_block_num = 0;
        
        /**
         *  This is the key used to sign blocks on behalf of this siming
         */
        public_key_type   signing_key;
        
        chain_properties  props;


        /**
         *  The total adores for this siming. This determines how the siming is ranked for
         *  scheduling.  The top N simings by adores are scheduled every round, every one
         *  else takes turns being scheduled proportional to their adores.
         */
        share_type        adores;
        siming_schedule_type schedule = none; /// How the siming was scheduled the last time it was scheduled

        /**
         * These fields are used for the siming scheduling algorithm which uses
         * virtual time to ensure that all simings are given proportional time
         * for producing blocks.
         *
         * @ref adores is used to determine speed. The @ref virtual_scheduled_time is
         * the expected time at which this siming should complete a virtual lap which
         * is defined as the position equal to 1000 times MAXADORES.
         *
         * virtual_scheduled_time = virtual_last_update + (1000*MAXADORES - virtual_position) / adores
         *
         * Every time the number of adores changes the virtual_position and virtual_scheduled_time must
         * update.  There is a global current virtual_scheduled_time which gets updated every time
         * a siming is scheduled.  To update the virtual_position the following math is performed.
         *
         * virtual_position       = virtual_position + adores * (virtual_current_time - virtual_last_update)
         * virtual_last_update    = virtual_current_time
         * adores                  += delta_adore
         * virtual_scheduled_time = virtual_last_update + (1000*MAXADORES - virtual_position) / adores
         *
         * @defgroup virtual_time Virtual Time Scheduling
         */
        ///@{
        fc::uint128       virtual_last_update;
        fc::uint128       virtual_position;
        fc::uint128       virtual_scheduled_time = fc::uint128::max_value();
        ///@}
        
        /**
         * This field represents the Taiyi blockchain version the siming is running.
         */
        version           running_version;
        
        hardfork_version  hardfork_version_vote;
        time_point_sec    hardfork_time_vote = TAIYI_GENESIS_TIME;
    };


    class siming_adore_object : public object< siming_adore_object_type, siming_adore_object >
    {
    public:
        template< typename Constructor, typename Allocator >
        siming_adore_object( Constructor&& c, allocator< Allocator > a )
        {
            c( *this );
        }
        
        siming_adore_object(){}
        
        id_type           id;
        
        account_name_type siming;
        account_name_type account;
    };

    class siming_schedule_object : public object< siming_schedule_object_type, siming_schedule_object >
    {
    public:
        template< typename Constructor, typename Allocator >
        siming_schedule_object( Constructor&& c, allocator< Allocator > a )
        {
            c( *this );
        }
        
        siming_schedule_object(){}
        
        id_type                                             id;
        
        fc::uint128                                         current_virtual_time;
        uint32_t                                            next_shuffle_block_num = 1;
        fc::array< account_name_type, TAIYI_MAX_SIMINGS >   current_shuffled_simings;
        uint8_t                                             num_scheduled_simings = 1;
        chain_properties                                    median_props;
        version                                             majority_version;
        
        uint8_t                                             max_adored_simings = TAIYI_MAX_SIMINGS;
        uint8_t                                             hardfork_required_simings = TAIYI_HARDFORK_REQUIRED_SIMINGS;        
    };


    struct by_adore_name;
    struct by_schedule_time;
    /**
     * @ingroup object_index
     */
    typedef multi_index_container<
        siming_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< siming_object, siming_id_type, &siming_object::id > >,
            ordered_unique< tag< by_name >, member< siming_object, account_name_type, &siming_object::owner > >,
            ordered_unique< tag< by_adore_name >,
                composite_key< siming_object,
                    member< siming_object, share_type, &siming_object::adores >,
                    member< siming_object, account_name_type, &siming_object::owner >
                >,
                composite_key_compare< std::greater< share_type >, std::less< account_name_type > >
            >,
            ordered_unique< tag< by_schedule_time >,
                composite_key< siming_object,
                    member< siming_object, fc::uint128, &siming_object::virtual_scheduled_time >,
                    member< siming_object, siming_id_type, &siming_object::id >
                >
            >
        >,
        allocator< siming_object >
    > siming_index;

    struct by_account_siming;
    struct by_siming_account;
    typedef multi_index_container<
        siming_adore_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< siming_adore_object, siming_adore_id_type, &siming_adore_object::id > >,
            ordered_unique< tag< by_account_siming >,
                composite_key< siming_adore_object,
                    member< siming_adore_object, account_name_type, &siming_adore_object::account >,
                    member< siming_adore_object, account_name_type, &siming_adore_object::siming >
                >,
                composite_key_compare< std::less< account_name_type >, std::less< account_name_type > >
            >,
            ordered_unique< tag< by_siming_account >,
                composite_key< siming_adore_object,
                    member< siming_adore_object, account_name_type, &siming_adore_object::siming >,
                    member< siming_adore_object, account_name_type, &siming_adore_object::account >
                >,
                composite_key_compare< std::less< account_name_type >, std::less< account_name_type > >
            >
        >, // indexed_by
        allocator< siming_adore_object >
    > siming_adore_index;

    typedef multi_index_container<
        siming_schedule_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< siming_schedule_object, siming_schedule_id_type, &siming_schedule_object::id > >
        >,
        allocator< siming_schedule_object >
    > siming_schedule_index;

} } //taiyi::chain

namespace mira {

   template<> struct is_static_length< taiyi::chain::siming_adore_object > : public boost::true_type {};
   template<> struct is_static_length< taiyi::chain::siming_schedule_object > : public boost::true_type {};

} // mira

FC_REFLECT_ENUM( taiyi::chain::siming_object::siming_schedule_type, (elected)(timeshare)(miner)(none) )

FC_REFLECT( taiyi::chain::chain_properties, (account_creation_fee)(maximum_block_size)(proposal_adopted_votes_threshold) )

FC_REFLECT( taiyi::chain::siming_object, (id)(owner)(created)(url)(adores)(schedule)(virtual_last_update)(virtual_position)(virtual_scheduled_time)(total_missed)(last_aslot)(last_confirmed_block_num)(signing_key)(props)(running_version)(hardfork_version_vote)(hardfork_time_vote) )
CHAINBASE_SET_INDEX_TYPE( taiyi::chain::siming_object, taiyi::chain::siming_index )

FC_REFLECT( taiyi::chain::siming_adore_object, (id)(siming)(account) )
CHAINBASE_SET_INDEX_TYPE( taiyi::chain::siming_adore_object, taiyi::chain::siming_adore_index )

FC_REFLECT( taiyi::chain::siming_schedule_object, (id)(current_virtual_time)(next_shuffle_block_num)(current_shuffled_simings)(num_scheduled_simings)(median_props)(majority_version)(max_adored_simings)(hardfork_required_simings) )
CHAINBASE_SET_INDEX_TYPE( taiyi::chain::siming_schedule_object, taiyi::chain::siming_schedule_index )

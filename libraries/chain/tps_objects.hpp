#pragma once
#include <chain/taiyi_fwd.hpp>

#include <protocol/authority.hpp>
#include <protocol/taiyi_operations.hpp>

#include <chain/taiyi_object_types.hpp>

namespace taiyi { namespace chain {
    
    using protocol::asset;

    class proposal_object : public object< proposal_object_type, proposal_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR( proposal_object )
        
    public:
        
        template<typename Constructor, typename Allocator>
        proposal_object( Constructor&& c, allocator< Allocator > a )
            : subject(a)
        {
            c(*this);
        };
        
        id_type             id;
                        
        account_id_type     creator;        // account that created the proposal
        account_id_type     target_account; // account that to be empowered
        time_point_sec      end_date;       // end_date (when the proposal expires and can no longer valid)
        std::string         subject;        // subject (a very brief description or title for the proposal)
                        
        uint64_t            total_votes = 0;// This will be calculate every maintenance period        
        bool                removed = false;
        
        time_point_sec get_end_date_with_delay() const { return end_date + TAIYI_PROPOSAL_MAINTENANCE_CLEANUP; }
    };

    struct by_id;
    struct by_end_date;
    struct by_creator;
    struct by_total_votes;
    
    typedef multi_index_container<
        proposal_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< proposal_object, proposal_id_type, &proposal_object::id > >,
            ordered_unique< tag< by_end_date >,
                composite_key< proposal_object,
                    const_mem_fun< proposal_object, time_point_sec, &proposal_object::get_end_date_with_delay >,
                    member< proposal_object, proposal_id_type, &proposal_object::id >
                >,
                composite_key_compare< std::less< time_point_sec >, std::less< proposal_id_type > >
            >,
            ordered_unique< tag< by_creator >,
                composite_key< proposal_object,
                    member< proposal_object, account_id_type, &proposal_object::creator >,
                    member< proposal_object, proposal_id_type, &proposal_object::id >
                >,
                composite_key_compare< std::less< account_id_type >, std::less< proposal_id_type > >
            >,
            ordered_unique< tag< by_total_votes >,
                composite_key< proposal_object,
                    member< proposal_object, uint64_t, &proposal_object::total_votes >,
                    member< proposal_object, proposal_id_type, &proposal_object::id >
                >,
                composite_key_compare< std::less< uint64_t >, std::less< proposal_id_type > >
            >
        >,
        allocator< proposal_object >
    > proposal_index;
    
    //=========================================================================

    class proposal_vote_object : public object< proposal_vote_object_type, proposal_vote_object>
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR( proposal_vote_object )
        
    public:
        
        template< typename Constructor, typename Allocator >
        proposal_vote_object( Constructor&& c, allocator< Allocator > a )
        {
            c( *this );
        }
        
        id_type             id;
                
        account_id_type     voter;          // account that made voting
        proposal_id_type    proposal_id;    //the voter voted for this proposal number
    };
    
    
    struct by_voter_proposal;
    struct by_proposal_voter;
    
    typedef multi_index_container<
        proposal_vote_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< proposal_vote_object, proposal_vote_id_type, &proposal_vote_object::id > >,
            ordered_unique< tag< by_voter_proposal >,
                composite_key< proposal_vote_object,
                    member< proposal_vote_object, account_id_type, &proposal_vote_object::voter >,
                    member< proposal_vote_object, proposal_id_type, &proposal_vote_object::proposal_id >
                >
            >,
            ordered_unique< tag< by_proposal_voter >,
                composite_key< proposal_vote_object,
                    member< proposal_vote_object, proposal_id_type, &proposal_vote_object::proposal_id >,
                    member< proposal_vote_object, account_id_type, &proposal_vote_object::voter >
                >
            >
        >,
        allocator< proposal_vote_object >
    > proposal_vote_index;
    
} } // taiyi::chain

namespace mira {
    template<> struct is_static_length< taiyi::chain::proposal_vote_object > : public boost::true_type {};
} // mira

FC_REFLECT( taiyi::chain::proposal_object, (id)(creator)(target_account)(end_date)(subject)(total_votes)(removed) )
CHAINBASE_SET_INDEX_TYPE( taiyi::chain::proposal_object, taiyi::chain::proposal_index )

FC_REFLECT( taiyi::chain::proposal_vote_object, (id)(voter)(proposal_id) )
CHAINBASE_SET_INDEX_TYPE( taiyi::chain::proposal_vote_object, taiyi::chain::proposal_vote_index )

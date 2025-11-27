#include <chain/taiyi_fwd.hpp>

#include <chain/tps_helper.hpp>

namespace taiyi { namespace chain {
    
    void tps_helper::remove_proposals(database& db, const flat_set<int64_t>& proposal_ids, const account_id_type& proposal_owner)
    {
        if(proposal_ids.empty())
            return;
        
        auto& proposalIndex = db.get_mutable_index<proposal_index>();
        auto& byIdIdx = proposalIndex.indices().get< by_id >();
        
        auto& votesIndex = db.get_mutable_index<proposal_vote_index>();
        auto& byVoterIdx = votesIndex.indices().get<by_proposal_voter>();
        
        tps_removing_reducer obj_perf( db.get_tps_remove_threshold() );
        
        for(auto id : proposal_ids)
        {
            auto foundPosI = byIdIdx.find(id);
            
            if(foundPosI == byIdIdx.end())
                continue;
            
            FC_ASSERT(foundPosI->creator == proposal_owner, "Only proposal owner can remove it...");
            
            remove_proposal< by_id >(foundPosI, proposalIndex, votesIndex, byVoterIdx, obj_perf);
            
            if( obj_perf.done )
                break;
        }        
    }
    
} } // taiyi::chain

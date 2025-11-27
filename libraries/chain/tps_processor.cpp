#include <chain/tps_processor.hpp>

namespace taiyi { namespace chain {
    
    using taiyi::protocol::asset;
    using taiyi::protocol::operation;
    
    using taiyi::chain::proposal_object;
    using taiyi::chain::by_end_date;
    using taiyi::chain::proposal_index;
    using taiyi::chain::proposal_id_type;
    using taiyi::chain::proposal_vote_index;
    using taiyi::chain::by_proposal_voter;
    using taiyi::chain::by_voter_proposal;
    using taiyi::protocol::proposal_execute_operation;
    using taiyi::chain::tps_helper;
    using taiyi::chain::dynamic_global_property_object;
    using taiyi::chain::block_notification;
    
    const std::string tps_processor::removing_name = "tps_processor_remove";
    const std::string tps_processor::calculating_name = "tps_processor_calculate";
    
    bool tps_processor::is_maintenance_period(const time_point_sec& head_time) const
    {
        return db.get_dynamic_global_properties().next_maintenance_time <= head_time;
    }
    //=========================================================================
    void tps_processor::remove_proposals(const time_point_sec& head_time)
    {
        auto& proposalIndex = db.get_mutable_index< proposal_index >();
        auto& byEndDateIdx = proposalIndex.indices().get< by_end_date >();
        
        auto& votesIndex = db.get_mutable_index< proposal_vote_index >();
        auto& byVoterIdx = votesIndex.indices().get< by_proposal_voter >();
        
        auto found = byEndDateIdx.upper_bound( head_time );
        auto itr = byEndDateIdx.begin();
        
        tps_removing_reducer obj_perf( db.get_tps_remove_threshold() );
        
        while( itr != found )
        {
            itr = tps_helper::remove_proposal<by_end_date>(itr, proposalIndex, votesIndex, byVoterIdx, obj_perf);
            if(obj_perf.done)
                break;
        }
    }
    //=========================================================================
    void tps_processor::find_active_proposals(const time_point_sec& head_time, t_proposals& proposals)
    {
        const auto& pidx = db.get_index<proposal_index>().indices().get< by_end_date >();
        
        std::for_each(pidx.begin(), pidx.upper_bound(head_time), [&](auto& proposal) {
            if( head_time <= proposal.end_date )
                proposals.emplace_back( proposal );
        });
    }
    //=========================================================================
    uint64_t tps_processor::calculate_votes(const proposal_id_type& id)
    {
        uint64_t ret = 0;
        
        const auto& pvidx = db.get_index<proposal_vote_index>().indices().get<by_proposal_voter>();
        auto found = pvidx.find(id);
        while(found != pvidx.end() && found->proposal_id == id)
        {
            const auto& _voter = db.get<account_object, by_id>(found->voter);
            if(_voter.is_xinsu)
                ret += 1;
            
            ++found;
        }
        
        return ret;
    }
    //=========================================================================
    void tps_processor::calculate_votes(const t_proposals& proposals)
    {
        for( auto& item : proposals )
        {
            const proposal_object& _item = item;
            auto total_votes = calculate_votes(_item.id);
            
            db.modify(_item, [&](auto& proposal) {
                proposal.total_votes = total_votes;
            });
        }
    }
    //=========================================================================
    void tps_processor::filter_by_votes(t_proposals& proposals)
    {
        const auto& props = db.get_dynamic_global_properties();
        uint32_t votes_threshold = std::min<uint32_t>(props.xinsu_count, 5);
        
        proposals.erase(std::remove_if(proposals.begin(), proposals.end(), [&](const proposal_object& p) {
            return p.total_votes < votes_threshold;
        }), proposals.end());
    }
    //=========================================================================
    void tps_processor::update_settings(const time_point_sec& head_time)
    {
        db.modify(db.get_dynamic_global_properties(), [&](dynamic_global_property_object& _dgpo) {
            _dgpo.next_maintenance_time = head_time + fc::seconds( TAIYI_PROPOSAL_MAINTENANCE_PERIOD );
        });
    }
    //=========================================================================
    void tps_processor::execute_proposals(const time_point_sec& head_time, const t_proposals& proposals)
    {
        auto processing = [this](const proposal_object& _item) {
            const auto& target_account = db.get<account_object, by_id>(_item.target_account);
            
            operation vop = proposal_execute_operation(target_account.name);
            db.pre_push_virtual_operation(vop);
            
            db.modify(target_account, [&](account_object& obj) {
                obj.is_xinsu = true;
            });
            
            db.modify(db.get_dynamic_global_properties(), [&](dynamic_global_property_object& obj) {
                obj.xinsu_count++;
            });

            db.post_push_virtual_operation(vop);
        };
        
        for( auto& item : proposals )
        {
            const proposal_object& _item = item;
            
            //Proposals without any votes shouldn't be treated as active
            if( _item.total_votes == 0 )
                break;
            
            processing(_item);
            
            /*
                Because of performance removing proposals are restricted due to the `tps_remove_threshold` threshold.
                Therefore all proposals are marked with flag `removed` and `end_date` is moved beyond 'head_time + TAIYI_PROPOSAL_MAINTENANCE_CLEANUP`
                flag `removed` - it's information for 'tps_api' plugin
                moving `end_date` - triggers the algorithm in `tps_processor::remove_proposals`
            */
            db.modify(_item, [&](proposal_object& obj) {
                obj.removed = true;
                obj.end_date = head_time - fc::seconds(TAIYI_PROPOSAL_MAINTENANCE_CLEANUP);
            });
        }
    }
    //=========================================================================
    void tps_processor::remove_old_proposals(const block_notification& note)
    {
        auto head_time = note.block.timestamp;
        
        if(db.get_benchmark_dumper().is_enabled())
            db.get_benchmark_dumper().begin();
        
        remove_proposals(head_time);
        
        if(db.get_benchmark_dumper().is_enabled())
            db.get_benchmark_dumper().end(tps_processor::removing_name);
    }
    //=========================================================================
    void tps_processor::process_proposals(const block_notification& note)
    {
        auto head_time = note.block.timestamp;
        
        //Check maintenance period
        if(!is_maintenance_period(head_time))
            return;
        
        if(db.get_benchmark_dumper().is_enabled())
            db.get_benchmark_dumper().begin();
        
        //Find all active proposals, where actual_time <= end_date
        t_proposals active_proposals;
        find_active_proposals(head_time, active_proposals);

        if(active_proposals.empty())
        {
            if(db.get_benchmark_dumper().is_enabled())
                db.get_benchmark_dumper().end(tps_processor::calculating_name);
            
            //Set `new maintenance time`
            update_settings(head_time);
            return;
        }
        
        //Calculate total_votes for every active proposal
        calculate_votes(active_proposals);
        
        //Filter all active proposals by total_votes, remove proposals which votes is less than set
        filter_by_votes(active_proposals);
                
        //Execute for every active proposal
        execute_proposals( head_time, active_proposals );
        
        //Set `new maintenance time`
        update_settings(head_time);
        
        if(db.get_benchmark_dumper().is_enabled())
            db.get_benchmark_dumper().end(tps_processor::calculating_name);
    }
    //=========================================================================
    const std::string& tps_processor::get_removing_name()
    {
        return removing_name;
    }
    //=========================================================================
    const std::string& tps_processor::get_calculating_name()
    {
        return calculating_name;
    }
    //=========================================================================
    void tps_processor::run(const block_notification& note)
    {
        remove_old_proposals(note);
        process_proposals(note);
    }
    
} } // namespace taiyi::chain

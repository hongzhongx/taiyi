#include <chain/taiyi_fwd.hpp>

#include <chain/database.hpp>

#include <chain/taiyi_objects.hpp>
#include <chain/contract_objects.hpp>

#include <chain/lua_context.hpp>
#include <chain/contract_worker.hpp>
#include <chain/proposal_processor.hpp>

#include <lua.hpp>

#include <fc/macros.hpp>

namespace taiyi { namespace chain {
        
    const std::string proposal_processor::removing_name = "proposal_processor_remove";
    const std::string proposal_processor::calculating_name = "proposal_processor_calculate";
    
    bool proposal_processor::is_maintenance_period(const time_point_sec& head_time) const
    {
        return db.get_dynamic_global_properties().next_maintenance_time <= head_time;
    }
    //=========================================================================
    void proposal_processor::remove_proposals(const time_point_sec& head_time)
    {
        auto& proposalIndex = db.get_mutable_index< proposal_index >();
        auto& byEndDateIdx = proposalIndex.indices().get< by_end_date >();
        
        auto& votesIndex = db.get_mutable_index< proposal_vote_index >();
        auto& byVoterIdx = votesIndex.indices().get< by_proposal_voter >();
        
        auto found = byEndDateIdx.upper_bound( head_time );
        auto itr = byEndDateIdx.begin();
        
        proposal_removing_reducer obj_perf( db.get_proposal_remove_threshold() );
        
        while( itr != found )
        {
            itr = proposal_helper::remove_proposal<by_end_date>(itr, proposalIndex, votesIndex, byVoterIdx, obj_perf);
            if(obj_perf.done)
                break;
        }
    }
    //=========================================================================
    void proposal_processor::find_active_proposals(const time_point_sec& head_time, t_proposals& proposals)
    {
        const auto& pidx = db.get_index<proposal_index>().indices().get< by_end_date >();
        std::for_each(pidx.lower_bound(head_time), pidx.end(), [&](auto& proposal) {
            if( head_time <= proposal.end_date )
                proposals.emplace_back( proposal );
        });
    }
    //=========================================================================
    uint64_t proposal_processor::calculate_votes(const proposal_id_type& id)
    {
        uint64_t ret = 0;
        
        const auto& pvidx = db.get_index<proposal_vote_index>().indices().get<by_proposal_voter>();
        auto found = pvidx.find(id);
        while(found != pvidx.end() && found->proposal_id == id)
        {
            const auto& _voter = db.get<account_object, by_id>(found->voter);
            if( db.is_xinsu(_voter) )
                ret += 1;
            
            ++found;
        }
        
        return ret;
    }
    //=========================================================================
    void proposal_processor::calculate_votes(const t_proposals& proposals)
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
    void proposal_processor::filter_by_votes(t_proposals& proposals)
    {
        const auto& props = db.get_dynamic_global_properties();
        const auto& wso = db.get_siming_schedule_object();
        uint32_t votes_threshold = std::min<uint32_t>(props.xinsu_count, wso.median_props.proposal_adopted_votes_threshold);
        
        proposals.erase(std::remove_if(proposals.begin(), proposals.end(), [&](const proposal_object& p) {
            return p.total_votes < votes_threshold;
        }), proposals.end());
    }
    //=========================================================================
    void proposal_processor::update_settings(const time_point_sec& head_time)
    {
        db.modify(db.get_dynamic_global_properties(), [&](dynamic_global_property_object& _dgpo) {
            _dgpo.next_maintenance_time = head_time + fc::seconds( TAIYI_PROPOSAL_MAINTENANCE_PERIOD );
        });
    }
    //=========================================================================
    void proposal_processor::execute_proposals(const time_point_sec& head_time, const t_proposals& proposals)
    {
        auto processing = [this](const proposal_object& _item) -> bool {

            const auto& caller = db.get<account_object, by_name>(TAIYI_DAO_ACCOUNT);
            if(caller.qi.amount.value < 100) {
                wlog("account \"${a}\" have not enough qi to execute proposal #${p}", ("a", TAIYI_DAO_ACCOUNT)("p", _item.id));
                return false; //真气不够，未执行提案
            }
            
            const auto* contract = db.find<contract_object, by_name>(_item.contract_name);
            if(contract == nullptr) {
                wlog("proposal #${p} can not be processed with non exist contract \"$c\"", ("p", _item.id)("c", _item.contract_name));
                return true; //尽管合约不存在，也要算提案执行了
            }

            contract_worker worker;
            LuaContext context;
            
            //qi可能在执行合约中被进一步使用，所以这里记录当前的qi来计算虚拟机的执行消耗
            long long old_drops = caller.qi.amount.value / TAIYI_USEMANA_EXECUTION_SCALE;
            long long vm_drops = old_drops;
            db.clear_contract_handler_exe_point(); //初始化api执行消耗统计

            try {
                operation vop = proposal_execute_operation(_item.contract_name, _item.function_name, _item.value_list, _item.subject);
                db.pre_push_virtual_operation(vop);

                auto session = db.start_undo_session();
                worker.do_contract_function(caller, _item.function_name, _item.value_list, *contract, vm_drops, true,  context, db);

                db.post_push_virtual_operation(vop);
                session.squash();
            }
            catch (const fc::exception& e) {
                //任何错误都不能造成核心循环崩溃
                wlog("Proposal (#${i}) process fail. err: ${e}", ("i", _item.id)("e", e.to_string()));
            }
            catch (...) {
                //任何错误都不能造成核心循环崩溃
                wlog("Proposal (${i}) process fail.", ("i", _item.id));
            }

            int64_t used_drops = old_drops - vm_drops;
            int64_t api_exe_point = db.get_contract_handler_exe_point();

            //reward contract owner
            int64_t used_qi = used_drops * TAIYI_USEMANA_EXECUTION_SCALE;
            int64_t used_qi_for_treasury = used_qi * TAIYI_CONTRACT_USEMANA_REWARD_TREASURY_PERCENT / TAIYI_100_PERCENT;
            used_qi -= used_qi_for_treasury;
            const auto& contract_owner = db.get<account_object, by_id>(contract->owner);
            if(caller.qi.amount.value >= used_qi) {
                db.reward_feigang(contract_owner, caller, asset(used_qi, QI_SYMBOL));
            }
            else {
                wlog("account \"${a}\" does not have enough qi to call contract of proposal #${p}.", ("a", TAIYI_DAO_ACCOUNT)("p", _item.id));
                if(caller.qi.amount > 0)
                    db.reward_feigang(contract_owner, caller, caller.qi); //真气不够也要消耗完
            }

            //reward to treasury
            used_qi = (50 + api_exe_point) * TAIYI_USEMANA_EXECUTION_SCALE + used_qi_for_treasury;
            if(caller.qi.amount.value >= used_qi) {
                db.reward_feigang(db.get<account_object, by_name>(TAIYI_DAO_ACCOUNT), caller, asset(used_qi, QI_SYMBOL));
            }
            else {
                wlog("account \"${a}\" does not have enough qi to call contract of proposal #${p}.", ("a", TAIYI_DAO_ACCOUNT)("p", _item.id));
                if(caller.qi.amount > 0)
                    db.reward_feigang(contract_owner, caller, caller.qi); //真气不够也要消耗完
            }
            
            return true;
        };
        
        for( auto& item : proposals )
        {
            const proposal_object& _item = item;
            
            //Proposals without any votes shouldn't be treated as active
            if( _item.total_votes == 0 )
                break;
            
            if( processing(_item) ) {
                /*
                 Because of performance removing proposals are restricted due to the `proposal_remove_threshold` threshold.
                 Therefore all proposals are marked with flag `removed` and `end_date` is moved beyond 'head_time + TAIYI_PROPOSAL_MAINTENANCE_CLEANUP`
                 flag `removed` - it's information for api plugins
                 moving `end_date` - triggers the algorithm in `proposal_processor::remove_proposals`
                 */
                db.modify(_item, [&](proposal_object& obj) {
                    obj.removed = true;
                    obj.end_date = head_time - fc::seconds(TAIYI_PROPOSAL_MAINTENANCE_CLEANUP);
                });
            }
        }
    }
    //=========================================================================
    void proposal_processor::remove_old_proposals(const block_notification& note)
    {
        auto head_time = note.block.timestamp;
        
        if(db.get_benchmark_dumper().is_enabled())
            db.get_benchmark_dumper().begin();
        
        remove_proposals(head_time);
        
        if(db.get_benchmark_dumper().is_enabled())
            db.get_benchmark_dumper().end(proposal_processor::removing_name);
    }
    //=========================================================================
    void proposal_processor::process_proposals(const block_notification& note)
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
                db.get_benchmark_dumper().end(proposal_processor::calculating_name);
            
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
            db.get_benchmark_dumper().end(proposal_processor::calculating_name);
    }
    //=========================================================================
    const std::string& proposal_processor::get_removing_name()
    {
        return removing_name;
    }
    //=========================================================================
    const std::string& proposal_processor::get_calculating_name()
    {
        return calculating_name;
    }
    //=========================================================================
    void proposal_processor::run(const block_notification& note)
    {
        remove_old_proposals(note);
        process_proposals(note);
    }
    
} } // namespace taiyi::chain

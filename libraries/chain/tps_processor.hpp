#pragma once

#include <chain/tps_objects.hpp>
#include <chain/notifications.hpp>
#include <chain/database.hpp>
#include <chain/index.hpp>
#include <chain/account_object.hpp>

#include <chain/tps_helper.hpp>

namespace taiyi { namespace chain {
    
    class tps_processor
    {
    public:
        
        using t_proposals = std::vector< std::reference_wrapper< const proposal_object > >;
        
    private:
        
        const static std::string removing_name;
        const static std::string calculating_name;
        const static uint32_t total_amount_divider = 100;
        
        //Get number of microseconds for 1 day( daily_ms )
        const int64_t daily_seconds = fc::days(1).to_seconds();
        
        chain::database& db;
        
        bool is_maintenance_period( const time_point_sec& head_time ) const;
        void remove_proposals( const time_point_sec& head_time );
        void find_active_proposals( const time_point_sec& head_time, t_proposals& proposals );
        
        uint64_t calculate_votes( const proposal_id_type& id );
        void calculate_votes( const t_proposals& proposals );
        void filter_by_votes( t_proposals& proposals );
                
        void update_settings( const time_point_sec& head_time );
        void execute_proposals( const time_point_sec& head_time, const t_proposals& proposals );
        void remove_old_proposals( const block_notification& note );
        void process_proposals( const block_notification& note );
        
    public:
        tps_processor(chain::database& _db) : db(_db){}
        
        const static std::string& get_removing_name();
        const static std::string& get_calculating_name();
        
        void run( const block_notification& note );
    };
    
} } // namespace taiyi::chain

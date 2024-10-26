#pragma once

namespace taiyi { namespace chain {

    class database;
    
    void update_siming_schedule( database& db );
    void reset_virtual_schedule_time( database& db );

} } //taiyi::chain

#pragma once

#include <fc/signals.hpp>

namespace taiyi { namespace chain { namespace util {

    inline void disconnect_signal( boost::signals2::connection& signal )
    {
        if( signal.connected() )
            signal.disconnect();
        FC_ASSERT( !signal.connected() );
    }

} } } //taiyi::chain::util

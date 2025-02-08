#pragma once

#include <vector>

namespace taiyi{ namespace plugins { namespace p2p {

#ifdef IS_TEST_NET
    const std::vector< std::string > default_seeds;
#else
    const std::vector< std::string > default_seeds = {
        "seed.taiyi.com:2025",                // @taiyi
        "seed-west.taiyi.com:2025"            // taiyi
    };
#endif

} } } // taiyi::plugins::p2p

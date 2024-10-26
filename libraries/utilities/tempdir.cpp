#include "tempdir.hpp"

#include <cstdlib>

namespace taiyi { namespace utilities {

fc::path temp_directory_path() {
    const char* taiyi_tempdir = getenv("TAIYI_TEMPDIR");
    if( taiyi_tempdir != nullptr )
        return fc::path( taiyi_tempdir );
    return fc::temp_directory_path() / "taiyi-tmp";
}

} } // taiyi::utilities

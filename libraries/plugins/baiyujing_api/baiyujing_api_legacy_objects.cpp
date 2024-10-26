#include <plugins/baiyujing_api/baiyujing_api_legacy_objects.hpp>

namespace fc {

    void to_variant( const taiyi::plugins::baiyujing_api::legacy_block_header_extensions& sv, fc::variant& v )
    {
        old_sv_to_variant( sv, v );
    }
    
    void from_variant( const fc::variant& v, taiyi::plugins::baiyujing_api::legacy_block_header_extensions& sv )
    {
        old_sv_from_variant( v, sv );
    }

}

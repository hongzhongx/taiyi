#include <plugins/baiyujing_api/baiyujing_api_plugin.hpp>
#include <plugins/baiyujing_api/baiyujing_api.hpp>
#include <plugins/chain/chain_plugin.hpp>

namespace taiyi { namespace plugins { namespace baiyujing_api {

    baiyujing_api_plugin::baiyujing_api_plugin() {}
    baiyujing_api_plugin::~baiyujing_api_plugin() {}
    
    void baiyujing_api_plugin::set_program_options( options_description& cli, options_description& cfg )
    {
        cli.add_options()
        ("disable-get-block", "Disable get_block API call" )
        ;
    }
    
    void baiyujing_api_plugin::plugin_initialize( const variables_map& options )
    {
        api = std::make_shared< baiyujing_api >();
    }
    
    void baiyujing_api_plugin::plugin_startup()
    {
        api->api_startup();
    }
    
    void baiyujing_api_plugin::plugin_shutdown() {}

} } } // taiyi::plugins::baiyujing_api

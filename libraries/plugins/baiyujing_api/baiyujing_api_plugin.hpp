#pragma once
#include <chain/taiyi_fwd.hpp>

#include <plugins/json_rpc/json_rpc_plugin.hpp>
#include <plugins/database_api/database_api_plugin.hpp>
#include <plugins/block_api/block_api_plugin.hpp>

#define TAIYI_BAIYUJING_API_PLUGIN_NAME "baiyujing_api"

namespace taiyi { namespace plugins { namespace baiyujing_api {

    using namespace appbase;
    
    class baiyujing_api_plugin : public appbase::plugin< baiyujing_api_plugin >
    {
    public:
        APPBASE_PLUGIN_REQUIRES(
                                (taiyi::plugins::json_rpc::json_rpc_plugin)
                                (taiyi::plugins::database_api::database_api_plugin)
                                )
        
        baiyujing_api_plugin();
        virtual ~baiyujing_api_plugin();
        
        static const std::string& name() { static std::string name = TAIYI_BAIYUJING_API_PLUGIN_NAME; return name; }
        
        virtual void set_program_options( options_description& cli, options_description& cfg ) override;
        
        virtual void plugin_initialize( const variables_map& options ) override;
        virtual void plugin_startup() override;
        virtual void plugin_shutdown() override;
        
        std::shared_ptr< class baiyujing_api > api;
    };

} } } // taiyi::plugins::baiyujing_api

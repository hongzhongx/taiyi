#pragma once
#include <chain/taiyi_fwd.hpp>
#include <plugins/chain/chain_plugin.hpp>
#include <plugins/p2p/p2p_plugin.hpp>
#include <plugins/siming/block_producer.hpp>

#include <appbase/application.hpp>

#define TAIYI_SIMING_PLUGIN_NAME "siming"

#define RESERVE_RATIO_PRECISION ((int64_t)10000)
#define RESERVE_RATIO_MIN_INCREMENT ((int64_t)5000)

#define SIMING_CUSTOM_OP_BLOCK_LIMIT 5

namespace taiyi { namespace plugins { namespace siming {

    namespace detail { class siming_plugin_impl; }

    namespace block_production_condition
    {
        enum block_production_condition_enum
        {
            produced = 0,
            not_synced = 1,
            not_my_turn = 2,
            not_time_yet = 3,
            no_private_key = 4,
            low_participation = 5,
            lag = 6,
            consecutive = 7,
            wait_for_genesis = 8,
            exception_producing_block = 9
        };
    }

    class siming_plugin : public appbase::plugin< siming_plugin >
    {
    public:
        APPBASE_PLUGIN_REQUIRES(
            (taiyi::plugins::chain::chain_plugin)
            (taiyi::plugins::p2p::p2p_plugin)
        )
        
        siming_plugin();
        virtual ~siming_plugin();
        
        static const std::string& name() { static std::string name = TAIYI_SIMING_PLUGIN_NAME; return name; }
        
        virtual void set_program_options(boost::program_options::options_description &command_line_options, boost::program_options::options_description &config_file_options) override;
        
        virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
        virtual void plugin_startup() override;
        virtual void plugin_shutdown() override;
        
    private:
        std::unique_ptr< detail::siming_plugin_impl > my;
    };

} } } // taiyi::plugins::siming

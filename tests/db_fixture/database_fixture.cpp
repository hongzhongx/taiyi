#include <boost/test/unit_test.hpp>
#include <boost/program_options.hpp>

#include <utilities/tempdir.hpp>
#include <utilities/database_configuration.hpp>

#include <chain/taiyi_fwd.hpp>
#include <chain/taiyi_objects.hpp>
#include <chain/account_object.hpp>
#include <chain/proposal_objects.hpp>

#include <plugins/account_history/account_history_objects.hpp>
#include <plugins/account_history/account_history_plugin.hpp>
#include <plugins/chain/chain_plugin.hpp>
#include <plugins/webserver/webserver_plugin.hpp>
#include <plugins/siming/siming_plugin.hpp>

#include <plugins/baiyujing_api/baiyujing_api_plugin.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/smart_ref_impl.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>

#include "database_fixture.hpp"

//using namespace taiyi::chain::test;

uint32_t TAIYI_TESTING_GENESIS_TIMESTAMP = 1431700000;

using namespace taiyi::plugins::webserver;
using namespace taiyi::plugins::database_api;
using namespace taiyi::plugins::block_api;
using taiyi::plugins::baiyujing_api::baiyujing_api_plugin;

namespace taiyi { namespace chain {

    using std::cout;
    using std::cerr;
    
    const string s_code_proposal_basic = "   \
        function create_proposal(contract_name, function_name, params, subject, end_time) \
            return contract_helper:create_proposal(contract_name, function_name, params, subject, end_time) \
        end                             \
        function update_proposal_votes(proposal_ids, approve) \
            contract_helper:update_proposal_votes(proposal_ids, approve) \
        end                             \
        function remove_proposals(proposal_ids) \
            contract_helper:remove_proposals(proposal_ids)  \
        end                             \
    ";
    
    const string s_code_xinsu_basic = "   \
        function grant_xinsu(receiver_account) \
            assert(contract_base_info.caller == contract_helper:zuowangdao_account_name(), 'no auth') \
            return contract_helper:grant_xinsu(receiver_account) \
        end                             \
        function revoke_xinsu(account_name) \
            assert(contract_base_info.caller == contract_helper:zuowangdao_account_name(), 'no auth') \
            contract_helper:revoke_xinsu(account_name) \
        end                             \
    ";
    
    clean_database_fixture::clean_database_fixture()
    {
        try {
            int argc = boost::unit_test::framework::master_test_suite().argc;
            char** argv = boost::unit_test::framework::master_test_suite().argv;
            for( int i=1; i<argc; i++ )
            {
                const std::string arg = argv[i];
                if( arg == "--record-assert-trip" )
                    fc::enable_record_assert_trip = true;
                if( arg == "--show-test-names" )
                    std::cout << "running test " << boost::unit_test::framework::current_test_case().p_name << std::endl;
            }

            appbase::app().register_plugin< taiyi::plugins::account_history::account_history_plugin >();
            db_plugin = &appbase::app().register_plugin< taiyi::plugins::debug_node::debug_node_plugin >();
            appbase::app().register_plugin< taiyi::plugins::siming::siming_plugin >();
            
            db_plugin->logging = false;
            appbase::app().initialize<
                taiyi::plugins::account_history::account_history_plugin,
                taiyi::plugins::debug_node::debug_node_plugin,
                taiyi::plugins::siming::siming_plugin
            >( argc, argv );

            db = &appbase::app().get_plugin< taiyi::plugins::chain::chain_plugin >().db();
            BOOST_REQUIRE( db );
            
            init_account_pub_key = init_account_priv_key.get_public_key();

            open_database();
            
            generate_block();
            db->set_hardfork( TAIYI_BLOCKCHAIN_VERSION.minor_v() );
            generate_block();

            vest( TAIYI_INIT_SIMING_NAME, 10000 );
            
            // Fill up the rest of the required miners
            for( int i = TAIYI_NUM_INIT_SIMINGS; i < TAIYI_MAX_SIMINGS; i++ )
            {
                account_create( TAIYI_INIT_SIMING_NAME + fc::to_string( i ), init_account_pub_key );
                fund( TAIYI_INIT_SIMING_NAME + fc::to_string( i ), TAIYI_MIN_REWARD_FUND * 20 );
                vest( TAIYI_INIT_SIMING_NAME + fc::to_string( i ), TAIYI_MIN_REWARD_FUND * 10 );
                siming_create( TAIYI_INIT_SIMING_NAME + fc::to_string( i ), init_account_priv_key, "foo.bar", init_account_pub_key );
            }
            
            validate_database();
            
            signed_transaction tx;
            create_contract_operation op;
            
            op.owner = TAIYI_INIT_SIMING_NAME;
            op.name = "contract.proposal.basic";
            op.data = s_code_proposal_basic;
            tx.operations.push_back( op );
            
            op.owner = TAIYI_INIT_SIMING_NAME;
            op.name = "contract.xinsu.basic";
            op.data = s_code_xinsu_basic;
            tx.operations.push_back( op );
            
            tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
            sign( tx, init_account_priv_key );
            db->push_transaction( tx, 0 );

            validate_database();
        }
        catch ( const fc::exception& e )
        {
            edump( (e.to_detail_string()) );
            throw;
        }
    }

    clean_database_fixture::~clean_database_fixture()
    {
        try {
            // If we're unwinding due to an exception, don't do any more checks.
            // This way, boost test's last checkpoint tells us approximately where the error was.
            if( !std::uncaught_exception() )
            {
                BOOST_CHECK( db->get_node_properties().skip_flags == database::skip_nothing );
            }
            
            if( data_dir )
                db->wipe( data_dir->path(), data_dir->path(), true );

        } FC_CAPTURE_AND_LOG( () )
    }
        
    void clean_database_fixture::resize_shared_mem( uint64_t size )
    {
        db->wipe( data_dir->path(), data_dir->path(), true );
        int argc = boost::unit_test::framework::master_test_suite().argc;
        char** argv = boost::unit_test::framework::master_test_suite().argv;
        for( int i=1; i<argc; i++ )
        {
            const std::string arg = argv[i];
            if( arg == "--record-assert-trip" )
                fc::enable_record_assert_trip = true;
            if( arg == "--show-test-names" )
                std::cout << "running test " << boost::unit_test::framework::current_test_case().p_name << std::endl;
        }
        init_account_pub_key = init_account_priv_key.get_public_key();
        
        {
            database::open_args args;
            args.data_dir = data_dir->path();
            args.state_storage_dir = args.data_dir;
            args.initial_supply = INITIAL_TEST_SUPPLY;
            args.initial_qi_supply = INITIAL_TEST_QI_SUPPLY;
            args.database_cfg = taiyi::utilities::default_database_configuration();
            db->open( args );
        }
        
        boost::program_options::variables_map options;
        
        
        generate_block();
        db->set_hardfork( TAIYI_BLOCKCHAIN_VERSION.minor_v() );
        generate_block();
        
        vest( "danuo", 10000 );
        
        // Fill up the rest of the required miners
        for( int i = TAIYI_NUM_INIT_SIMINGS; i < TAIYI_MAX_SIMINGS; i++ )
        {
            account_create( TAIYI_INIT_SIMING_NAME + fc::to_string( i ), init_account_pub_key );
            fund( TAIYI_INIT_SIMING_NAME + fc::to_string( i ), TAIYI_MIN_REWARD_FUND * 20 );
            vest( TAIYI_INIT_SIMING_NAME + fc::to_string( i ), TAIYI_MIN_REWARD_FUND * 10 );
            siming_create( TAIYI_INIT_SIMING_NAME + fc::to_string( i ), init_account_priv_key, "foo.bar", init_account_pub_key );
        }
        
        validate_database();
    }

    void clean_database_fixture::create_contract(const std::string& creator, const string& contract_name, const string& contract_code)
    {
        signed_transaction tx;
        create_contract_operation op;

        op.owner = creator;
        op.name = contract_name;
        op.data = contract_code;
        tx.operations.push_back( op );

        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, init_account_priv_key );
        db->push_transaction( tx, 0 );
        tx.signatures.clear();
        tx.operations.clear();

        validate_database();
    }
    
    int64_t clean_database_fixture::create_proposal(const std::string& creator, const string& contract_name, const string& function_name, const lua_map& params, const string& subject, time_point_sec end_date, const fc::ecc::private_key& key)
    {
        signed_transaction tx;

        call_contract_function_operation cop;
        cop.caller = creator;
        cop.contract_name = "contract.proposal.basic";
        cop.function_name = "create_proposal";
        cop.value_list = {
            lua_string(contract_name),
            lua_string(function_name),
            lua_table(params),
            lua_string(subject),
            lua_int(end_date.sec_since_epoch())
        };
            
        tx.operations.push_back( cop );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, key );
        db->push_transaction( tx, 0 );
        tx.signatures.clear();
        tx.operations.clear();

        const auto& proposal_idx = db-> template get_index<proposal_index>().indices(). template get< by_id >();
        auto itr = proposal_idx.end();
        BOOST_REQUIRE( proposal_idx.begin() != itr );
        --itr;
        BOOST_REQUIRE( db->get_account(creator).id == itr->creator) ;

        return itr->id;
    }
    
    void clean_database_fixture::vote_proposal(const std::string& voter, const std::vector<int64_t>& id_proposals, bool approve, const fc::ecc::private_key& key)
    {
        lua_map params;
        for (size_t i=0; i<id_proposals.size(); i++)
            params[lua_key(lua_int(i+1))] = lua_types(lua_int(id_proposals[i]));
        
        call_contract_function_operation cop;
        cop.caller = voter;
        cop.contract_name = "contract.proposal.basic";
        cop.function_name = "update_proposal_votes";
        cop.value_list = {
            lua_table(params),
            lua_bool(approve)
        };

        signed_transaction tx;
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.operations.push_back( cop );
        sign( tx, key );
        db->push_transaction( tx, 0 );
    }
    
    uint64_t clean_database_fixture::get_nr_blocks_until_maintenance_block()
    {
        auto block_time = db->head_block_time();
        
        auto next_maintenance_time = db->get_dynamic_global_properties().next_maintenance_time;
        auto ret = ( next_maintenance_time - block_time ).to_seconds() / TAIYI_BLOCK_INTERVAL;
        
        FC_ASSERT( next_maintenance_time >= block_time );
        
        return ret;
    }
        
    void clean_database_fixture::generate_xinsu(const std::vector<account_name_type>& accounts)
    {
        auto end_date = db->head_block_time() + fc::days( 2 );
                    
        static uint32_t cnt = 0;
        auto subject = string("granting_xinsu_proposal#") + std::to_string(cnt);
        
        std::vector<int64_t> proposal_ids;
        lua_map params;
        for (size_t i=0; i<accounts.size(); i++) {
            params[lua_key(lua_int(1))] = lua_string(accounts[i]);
            int64_t id_proposal = create_proposal(TAIYI_INIT_SIMING_NAME, "contract.xinsu.basic", "grant_xinsu", params, subject, end_date, init_account_priv_key);
            proposal_ids.push_back(id_proposal);
            if(i % 100 == 0)
                generate_blocks( 1 );
        }
        generate_blocks( 1 );
        
        //简单避免一下一次性提交大量投票的情况
        for (size_t i=0; i<proposal_ids.size(); i++) {
            vote_proposal(TAIYI_INIT_SIMING_NAME, {proposal_ids[i]}, true/*approve*/, init_account_priv_key );
            if(i % 100 == 0)
                generate_blocks( 1 );
        }
        generate_blocks( 1 );

        auto next_block = get_nr_blocks_until_maintenance_block();
        generate_blocks(next_block);

        for (size_t i=0; i<accounts.size(); i++) {
            FC_ASSERT(db->is_xinsu(db->get_account(accounts[i])) == true );
        }
    }

    live_database_fixture::live_database_fixture()
    {
        try
        {
            int argc = boost::unit_test::framework::master_test_suite().argc;
            char** argv = boost::unit_test::framework::master_test_suite().argv;
            
            ilog( "Loading saved chain" );
            _chain_dir = fc::current_path() / "test_blockchain";
            FC_ASSERT( fc::exists( _chain_dir ), "Requires blockchain to test on in ./test_blockchain" );
            
            appbase::app().register_plugin< taiyi::plugins::account_history::account_history_plugin >();
            appbase::app().initialize<
                taiyi::plugins::account_history::account_history_plugin
            >( argc, argv );
            
            db = &appbase::app().get_plugin< taiyi::plugins::chain::chain_plugin >().db();
            BOOST_REQUIRE( db );
            
            {
                database::open_args args;
                args.data_dir = _chain_dir;
                args.state_storage_dir = args.data_dir;
                args.database_cfg = taiyi::utilities::default_database_configuration();
                db->open( args );
            }
            
            validate_database();
            generate_block();
            
            ilog( "Done loading saved chain" );
        }
        FC_LOG_AND_RETHROW()
    }

    live_database_fixture::~live_database_fixture()
    {
        try
        {
            // If we're unwinding due to an exception, don't do any more checks.
            // This way, boost test's last checkpoint tells us approximately where the error was.
            if( !std::uncaught_exception() )
            {
                BOOST_CHECK( db->get_node_properties().skip_flags == database::skip_nothing );
            }
            
            db->pop_block();
            db->close();
            return;
        }
        FC_CAPTURE_AND_LOG( () )
    }
    
    fc::ecc::private_key database_fixture::generate_private_key(string seed)
    {
        static const fc::ecc::private_key committee = fc::ecc::private_key::regenerate( fc::sha256::hash( string( "init_key" ) ) );
        if( seed == "init_key" )
            return committee;
        return fc::ecc::private_key::regenerate( fc::sha256::hash( seed ) );
    }
    
    void database_fixture::open_database()
    {
        if( !data_dir )
        {
            data_dir = fc::temp_directory( taiyi::utilities::temp_directory_path() );
            db->set_log_hardforks(false);
            
            idump( (data_dir->path()) );
            
            database::open_args args;
            args.data_dir = data_dir->path();
            args.state_storage_dir = args.data_dir;
            args.initial_supply = INITIAL_TEST_SUPPLY;
            args.initial_qi_supply = INITIAL_TEST_QI_SUPPLY;
            args.database_cfg = taiyi::utilities::default_database_configuration();
            db->open(args);
        }
        else
        {
            idump( (data_dir->path()) );
        }
    }

    void database_fixture::generate_block(uint32_t skip, const fc::ecc::private_key& key, int miss_blocks)
    {
        skip |= default_skip;
        db_plugin->debug_generate_blocks( taiyi::utilities::key_to_wif( key ), 1, skip, miss_blocks );
    }
    
    void database_fixture::generate_blocks( uint32_t block_count )
    {
        auto produced = db_plugin->debug_generate_blocks( debug_key, block_count, default_skip, 0 );
        BOOST_REQUIRE( produced == block_count );
    }
    
    void database_fixture::generate_blocks(fc::time_point_sec timestamp, bool miss_intermediate_blocks)
    {
        db_plugin->debug_generate_blocks_until( debug_key, timestamp, miss_intermediate_blocks, default_skip );
        BOOST_REQUIRE( ( db->head_block_time() - timestamp ).to_seconds() < TAIYI_BLOCK_INTERVAL );
    }

    const account_object& database_fixture::account_create(const string& name, const string& creator, const private_key_type& creator_key, const share_type& fee, const public_key_type& key, const public_key_type& post_key, const string& json_metadata)
    {
        try
        {
            auto actual_fee = std::min( fee, db->get_siming_schedule_object().median_props.account_creation_fee.amount );
            auto fee_remainder = fee - actual_fee;
            
            account_create_operation op;
            op.new_account_name = name;
            op.creator = creator;
            op.fee = asset( actual_fee, YANG_SYMBOL );
            op.owner = authority( 1, key, 1 );
            op.active = authority( 1, key, 1 );
            op.posting = authority( 1, post_key, 1 );
            op.memo_key = key;
            op.json_metadata = json_metadata;
            
            trx.operations.push_back( op );
            
            trx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
            sign( trx, creator_key );
            trx.validate();
            db->push_transaction( trx, 0 );
            trx.clear();
            
            if( fee_remainder > 0 )
            {
                vest( TAIYI_INIT_SIMING_NAME, name, asset( fee_remainder, YANG_SYMBOL ) );
            }
            
            const account_object& acct = db->get_account( name );
            
            return acct;
        }
        FC_CAPTURE_AND_RETHROW( (name)(creator) )
    }

    const account_object& database_fixture::account_create(const string& name, const public_key_type& key, const public_key_type& post_key)
    {
        try
        {
            return account_create(name, TAIYI_INIT_SIMING_NAME, init_account_priv_key, std::max( db->get_siming_schedule_object().median_props.account_creation_fee.amount, share_type( 100 ) ), key, post_key, "" );
        }
        FC_CAPTURE_AND_RETHROW( (name) );
    }

    const account_object& database_fixture::account_create(const string& name, const public_key_type& key)
    {
        return account_create( name, key, key );
    }

    const siming_object& database_fixture::siming_create(const string& owner, const private_key_type& owner_key, const string& url, const public_key_type& signing_key )
    {
        try
        {
            siming_update_operation op;
            op.owner = owner;
            op.url = url;
            op.block_signing_key = signing_key;
            
            trx.operations.push_back( op );
            trx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
            sign( trx, owner_key );
            trx.validate();
            db->push_transaction( trx, 0 );
            trx.clear();
            
            return db->get_siming( owner );
        }
        FC_CAPTURE_AND_RETHROW( (owner)(url) )
    }

    void database_fixture::fund(const string& account_name, const share_type& amount)
    {
        try
        {
            transfer( TAIYI_INIT_SIMING_NAME, account_name, asset( amount, YANG_SYMBOL ) );
        } FC_CAPTURE_AND_RETHROW( (account_name)(amount) )
    }

    void database_fixture::fund(const string& account_name, const asset& amount)
    {
        try
        {
            db_plugin->debug_update( [=]( database& db) {
                if( amount.symbol.space() == asset_symbol_type::fai_space )
                {
                    db.adjust_balance(account_name, amount);
                    return;
                }
                
                db.modify( db.get_account( account_name ), [&]( account_object& a ) {
                    if( amount.symbol == YANG_SYMBOL )
                        a.balance += amount;
                });
                
                db.modify( db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo ) {
                    if( amount.symbol == YANG_SYMBOL )
                        gpo.current_supply += amount;
                });
                
            }, default_skip );
        }
        FC_CAPTURE_AND_RETHROW( (account_name)(amount) )
    }

    void database_fixture::transfer(const string& from, const string& to, const asset& amount )
    {
        try
        {
            transfer_operation op;
            op.from = from;
            op.to = to;
            op.amount = amount;
            
            trx.operations.push_back( op );
            trx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
            trx.validate();
            
            if( from == TAIYI_INIT_SIMING_NAME )
            {
                sign( trx, init_account_priv_key );
            }
            
            db->push_transaction( trx, ~0 );
            trx.clear();
        } FC_CAPTURE_AND_RETHROW( (from)(to)(amount) )
    }
    
    void database_fixture::vest( const string& from, const string& to, const asset& amount )
    {
        try
        {
            FC_ASSERT( amount.symbol == YANG_SYMBOL, "Can only vest YANG" );
            
            transfer_to_qi_operation op;
            op.from = from;
            op.to = to;
            op.amount = amount;
            
            trx.operations.push_back( op );
            trx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
            trx.validate();
            
            // This sign() call fixes some tests, like withdraw_qi_apply, that use this method
            //   with debug_plugin such that trx may be re-applied with less generous skip flags.
            if( from == TAIYI_INIT_SIMING_NAME )
            {
                sign( trx, init_account_priv_key );
            }
            
            db->push_transaction( trx, ~0 );
            trx.clear();
        } FC_CAPTURE_AND_RETHROW( (from)(to)(amount) )
    }

    void database_fixture::vest( const string& from, const share_type& amount )
    {
        try
        {
            transfer_to_qi_operation op;
            op.from = from;
            op.to = "";
            op.amount = asset( amount, YANG_SYMBOL );
            
            trx.operations.push_back( op );
            trx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
            trx.validate();
            
            if( from == TAIYI_INIT_SIMING_NAME )
            {
                sign( trx, init_account_priv_key );
            }
            
            db->push_transaction( trx, ~0 );
            trx.clear();
        } FC_CAPTURE_AND_RETHROW( (from)(amount) )
    }
    
    void database_fixture::proxy( const string& account, const string& proxy )
    {
        try
        {
            account_siming_proxy_operation op;
            op.account = account;
            op.proxy = proxy;
            trx.operations.push_back( op );
            db->push_transaction( trx, ~0 );
            trx.clear();
        } FC_CAPTURE_AND_RETHROW( (account)(proxy) )
    }

    void database_fixture::set_siming_props( const flat_map< string, vector< char > >& props )
    {
        trx.clear();
        for( size_t i=0; i<TAIYI_MAX_SIMINGS; i++ )
        {
            siming_set_properties_operation op;
            op.owner = TAIYI_INIT_SIMING_NAME + (i == 0 ? "" : fc::to_string( i ));
            op.props = props;
            if( props.find( "key" ) == props.end() )
                op.props["key"] = fc::raw::pack_to_vector( init_account_pub_key );
            
            trx.operations.push_back( op );
            trx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
            db->push_transaction( trx, ~0 );
            trx.clear();
        }
        
        const siming_schedule_object* wso = &(db->get_siming_schedule_object());
        uint32_t old_next_shuffle = wso->next_shuffle_block_num;
        
        for( size_t i=0; i<2*TAIYI_MAX_SIMINGS+1; i++ )
        {
            generate_block();
            wso = &(db->get_siming_schedule_object());
            if( wso->next_shuffle_block_num != old_next_shuffle )
                return;
        }
        FC_ASSERT( false, "Couldn't apply properties in ${n} blocks", ("n", 2*TAIYI_MAX_SIMINGS+1) );
    }
    
    const asset& database_fixture::get_balance( const string& account_name )const
    {
        return db->get_account( account_name ).balance;
    }
    
    void database_fixture::sign(signed_transaction& trx, const fc::ecc::private_key& key)
    {
        trx.sign( key, db->get_chain_id(), default_sig_canon );
    }
    
    void database_fixture::validate_database()
    {
        try
        {
            db->validate_invariants();
        }
        FC_LOG_AND_RETHROW();
    }
    
    void push_invalid_operation(const operation& invalid_op, const fc::ecc::private_key& key, database* db)
    {
        signed_transaction tx;
        tx.operations.push_back( invalid_op );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.sign( key, db->get_chain_id(), fc::ecc::bip_0062 );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::assert_exception );
    }
    
    proposal_database_fixture::proposal_database_fixture() : clean_database_fixture()
    {
    }
    
    bool proposal_database_fixture::exist_proposal( int64_t id )
    {
        const auto& proposal_idx = db->get_index< proposal_index >().indices(). template get< by_id >();
        return proposal_idx.find( id ) != proposal_idx.end();
    }
    
    const proposal_object* proposal_database_fixture::find_proposal( int64_t id )
    {
        const auto& proposal_idx = db->get_index< proposal_index >().indices(). template get< by_id >();
        auto found = proposal_idx.find( id );
        
        if( found != proposal_idx.end() )
            return &(*found);
        else
            return nullptr;
    }

    void proposal_database_fixture::remove_proposal(const account_name_type& deleter, const std::vector<int64_t>& proposal_id, const fc::ecc::private_key& key)
    {
        lua_map params;
        for (size_t i=0; i<proposal_id.size(); i++)
            params[lua_key(lua_int(i+1))] = lua_types(lua_int(proposal_id[i]));
        
        call_contract_function_operation cop;
        cop.caller = deleter;
        cop.contract_name = "contract.proposal.basic";
        cop.function_name = "remove_proposals";
        cop.value_list = {
            lua_table(params)
        };

        signed_transaction trx;
        trx.operations.push_back( cop );
        trx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( trx, key );
        db->push_transaction( trx, 0 );
        trx.signatures.clear();
        trx.operations.clear();
    }

    bool proposal_database_fixture::find_vote_for_proposal(const std::string& user, int64_t id)
    {
        const auto& proposal_vote_idx = db->get_index< proposal_vote_index >().indices(). template get< by_voter_proposal >();
        const auto& account = db->get_account(user);
        auto found_vote = proposal_vote_idx.find( boost::make_tuple(account.id, id) );
        return found_vote != proposal_vote_idx.end() ;
    }

    json_rpc_database_fixture::json_rpc_database_fixture()
    {
        try {
            int argc = boost::unit_test::framework::master_test_suite().argc;
            char** argv = boost::unit_test::framework::master_test_suite().argv;
            for( int i=1; i<argc; i++ )
            {
                const std::string arg = argv[i];
                if( arg == "--record-assert-trip" )
                    fc::enable_record_assert_trip = true;
                if( arg == "--show-test-names" )
                    std::cout << "running test " << boost::unit_test::framework::current_test_case().p_name << std::endl;
            }
            
            appbase::app().register_plugin< taiyi::plugins::account_history::account_history_plugin >();
            db_plugin = &appbase::app().register_plugin< taiyi::plugins::debug_node::debug_node_plugin >();
            appbase::app().register_plugin< taiyi::plugins::siming::siming_plugin >();
            rpc_plugin = &appbase::app().register_plugin< taiyi::plugins::json_rpc::json_rpc_plugin >();
            appbase::app().register_plugin< taiyi::plugins::block_api::block_api_plugin >();
            appbase::app().register_plugin< taiyi::plugins::database_api::database_api_plugin >();
            appbase::app().register_plugin< taiyi::plugins::baiyujing_api::baiyujing_api_plugin >();
            
            db_plugin->logging = false;
            appbase::app().initialize<
                taiyi::plugins::account_history::account_history_plugin,
                taiyi::plugins::debug_node::debug_node_plugin,
                taiyi::plugins::json_rpc::json_rpc_plugin,
                taiyi::plugins::block_api::block_api_plugin,
                taiyi::plugins::database_api::database_api_plugin,
                taiyi::plugins::baiyujing_api::baiyujing_api_plugin
            >( argc, argv );
            
            appbase::app().get_plugin< taiyi::plugins::baiyujing_api::baiyujing_api_plugin >().plugin_startup();
            
            db = &appbase::app().get_plugin< taiyi::plugins::chain::chain_plugin >().db();
            BOOST_REQUIRE( db );
            
            init_account_pub_key = init_account_priv_key.get_public_key();
            
            open_database();
            
            generate_block();
            db->set_hardfork( TAIYI_BLOCKCHAIN_VERSION.minor_v() );
            generate_block();
            
            vest( "danuo", 10000 );
            
            // Fill up the rest of the required miners
            for( int i = TAIYI_NUM_INIT_SIMINGS; i < TAIYI_MAX_SIMINGS; i++ )
            {
                account_create( TAIYI_INIT_SIMING_NAME + fc::to_string( i ), init_account_pub_key );
                fund( TAIYI_INIT_SIMING_NAME + fc::to_string( i ), TAIYI_MIN_REWARD_FUND * 20 );
                vest( TAIYI_INIT_SIMING_NAME + fc::to_string( i ), TAIYI_MIN_REWARD_FUND * 10 );
                siming_create( TAIYI_INIT_SIMING_NAME + fc::to_string( i ), init_account_priv_key, "foo.bar", init_account_pub_key );
            }
            
            validate_database();
        } catch ( const fc::exception& e )
        {
            edump( (e.to_detail_string()) );
            throw;
        }
        
        return;
    }

    json_rpc_database_fixture::~json_rpc_database_fixture()
    {
        // If we're unwinding due to an exception, don't do any more checks.
        // This way, boost test's last checkpoint tells us approximately where the error was.
        if( !std::uncaught_exception() )
        {
            BOOST_CHECK( db->get_node_properties().skip_flags == database::skip_nothing );
        }
        
        if( data_dir )
            db->wipe( data_dir->path(), data_dir->path(), true );
        return;
    }
    
    fc::variant json_rpc_database_fixture::get_answer( std::string& request )
    {
        return fc::json::from_string( rpc_plugin->call( request ) );
    }
    
    void check_id_equal( const fc::variant& id_a, const fc::variant& id_b )
    {
        BOOST_REQUIRE( id_a.get_type() == id_b.get_type() );
        
        switch( id_a.get_type() )
        {
            case fc::variant::int64_type:
                BOOST_REQUIRE( id_a.as_int64() == id_b.as_int64() );
                break;
            case fc::variant::uint64_type:
                BOOST_REQUIRE( id_a.as_uint64() == id_b.as_uint64() );
                break;
            case fc::variant::string_type:
                BOOST_REQUIRE( id_a.as_string() == id_b.as_string() );
                break;
            case fc::variant::null_type:
                break;
            default:
                BOOST_REQUIRE( false );
        }
    }

    void json_rpc_database_fixture::review_answer( fc::variant& answer, int64_t code, bool is_warning, bool is_fail, fc::optional< fc::variant > id )
    {
        fc::variant_object error;
        int64_t answer_code;
        
        if( is_fail )
        {
            if( id.valid() && code != JSON_RPC_INVALID_REQUEST )
            {
                BOOST_REQUIRE( answer.get_object().contains( "id" ) );
                check_id_equal( answer[ "id" ], *id );
            }
            
            BOOST_REQUIRE( answer.get_object().contains( "error" ) );
            BOOST_REQUIRE( answer["error"].is_object() );
            error = answer["error"].get_object();
            BOOST_REQUIRE( error.contains( "code" ) );
            BOOST_REQUIRE( error["code"].is_int64() );
            answer_code = error["code"].as_int64();
            BOOST_REQUIRE( answer_code == code );
            if( is_warning )
                BOOST_TEST_MESSAGE( error["message"].as_string() );
        }
        else
        {
            BOOST_REQUIRE( answer.get_object().contains( "result" ) );
            BOOST_REQUIRE( answer.get_object().contains( "id" ) );
            if( id.valid() )
                check_id_equal( answer[ "id" ], *id );
        }
    }
    
    void json_rpc_database_fixture::make_array_request( std::string& request, int64_t code, bool is_warning, bool is_fail )
    {
        fc::variant answer = get_answer( request );
        BOOST_REQUIRE( answer.is_array() );
        
        fc::variants request_array = fc::json::from_string( request ).get_array();
        fc::variants array = answer.get_array();
        
        BOOST_REQUIRE( array.size() == request_array.size() );
        for( size_t i = 0; i < array.size(); ++i )
        {
            fc::optional< fc::variant > id;
            
            try
            {
                id = request_array[i][ "id" ];
            }
            catch( ... ) {}
            
            review_answer( array[i], code, is_warning, is_fail, id );
        }
    }
    
    fc::variant json_rpc_database_fixture::make_request( std::string& request, int64_t code, bool is_warning, bool is_fail )
    {
        fc::variant answer = get_answer( request );
        BOOST_REQUIRE( answer.is_object() );
        fc::optional< fc::variant > id;
        
        try
        {
            id = fc::json::from_string( request ).get_object()[ "id" ];
        }
        catch( ... ) {}
        
        review_answer( answer, code, is_warning, is_fail, id );
        
        return answer;
    }
    
    void json_rpc_database_fixture::make_positive_request( std::string& request )
    {
        make_request( request, 0/*code*/, false/*is_warning*/, false/*is_fail*/);
    }
    
    namespace test
    {
        
        bool _push_block( database& db, const signed_block& b, uint32_t skip_flags /* = 0 */ )
        {
            return db.push_block( b, skip_flags);
        }
        
        void _push_transaction( database& db, const signed_transaction& tx, uint32_t skip_flags /* = 0 */ )
        { try {
            db.push_transaction( tx, skip_flags );
        } FC_CAPTURE_AND_RETHROW((tx)) }
        
    } // taiyi::chain::test

} } // taiyi::chain

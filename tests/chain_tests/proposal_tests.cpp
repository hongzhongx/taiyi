#include <boost/test/unit_test.hpp>

#include <chain/taiyi_fwd.hpp>

#include <protocol/exceptions.hpp>
#include <protocol/hardfork.hpp>

#include <chain/database.hpp>
#include <chain/database_exceptions.hpp>

#include <chain/taiyi_objects.hpp>
#include <chain/tps_objects.hpp>
#include <chain/tps_processor.hpp>

#include <chain/lua_context.hpp>

#include <fc/macros.hpp>
#include <fc/crypto/digest.hpp>

#include "../db_fixture/database_fixture.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace taiyi;
using namespace taiyi::chain;
using namespace taiyi::protocol;
using std::string;

template< typename PROPOSAL_IDX >
int64_t calc_proposals( const PROPOSAL_IDX& proposal_idx, const std::vector< int64_t >& proposals_id )
{
    auto cnt = 0;
    for( auto id : proposals_id )
        cnt += proposal_idx.find( id ) != proposal_idx.end() ? 1 : 0;
    return cnt;
}

template< typename PROPOSAL_VOTE_IDX >
int64_t calc_proposal_votes( const PROPOSAL_VOTE_IDX& proposal_vote_idx, int64_t proposal_id )
{
    auto cnt = 0;
    auto found = proposal_vote_idx.find( proposal_id );
    while( found != proposal_vote_idx.end() && found->proposal_id == (proposal_id_type)proposal_id )
    {
        ++cnt;
        ++found;
    }
    return cnt;
}

template< typename PROPOSAL_VOTE_IDX >
int64_t calc_votes( const PROPOSAL_VOTE_IDX& proposal_vote_idx, const std::vector< int64_t >& proposals_id )
{
    auto cnt = 0;
    for( auto id : proposals_id )
        cnt += calc_proposal_votes( proposal_vote_idx, id );
    return cnt;
}

const string s_code_xinsu_sample = "   \
    function grant_xinsu(receiver_account) \
        assert(contract_base_info.caller == contract_helper:zuowangdao_account_name(), 'no auth') \
        return contract_helper:grant_xinsu(receiver_account) \
    end                             \
    function revoke_xinsu(account_name) \
        assert(contract_base_info.caller == contract_helper:zuowangdao_account_name(), 'no auth') \
        contract_helper:revoke_xinsu(account_name) \
    end                             \
";

struct create_proposal_data {
    std::string creator;
    fc::time_point_sec end_date;
    std::string subject;
    std::string contract_name;
    std::string function_name;
    lua_map params;
    
    create_proposal_data(fc::time_point_sec _start) {
        creator    = "alice";
        end_date   = _start + fc::days( 3 );
        subject    = "hello";
        contract_name = "contract.proposal.test";
        function_name = "proposal_test";
        params[lua_key(lua_int(1))] = lua_string("proposal check");
    }
};

const string s_code_proposal_test = " \
    function proposal_test(info)    \
        contract_helper:log(info)   \
    end                             \
    function transfer_1_to(account) \
        contract_helper:transfer_from_caller(account, 1000, 'YANG', true) \
    end                             \
";

BOOST_FIXTURE_TEST_SUITE( proposal_tests, tps_database_fixture )

BOOST_AUTO_TEST_CASE( generating_proposal )
{ try {
    BOOST_TEST_MESSAGE( "Testing: generating proposal" );
    
    ACTORS( (alice)(bob)(carol) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    generate_block();
    
    //=====================preparing=====================
  
    const string code_proposal_test = " \
        function proposal_test(info)    \
            contract_helper:log(info)   \
        end                             \
    ";

    signed_transaction tx;
    create_contract_operation op;

    op.owner = TAIYI_INIT_SIMING_NAME;
    op.name = "contract.proposal.test";
    op.data = code_proposal_test;
    tx.operations.push_back( op );

    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    sign( tx, init_account_priv_key );
    db->push_transaction( tx, 0 );
    generate_block();
    validate_database();

    auto creator = TAIYI_INIT_SIMING_NAME;
    
    auto end_date = db->head_block_time() + fc::days( 2 );
        
    auto voter_01 = TAIYI_INIT_SIMING_NAME;
    //=====================preparing=====================
    
    //Needed basic operations
    lua_map params;
    params[lua_key(lua_int(1))] = lua_string("proposal check");
    int64_t id_proposal_00 = create_proposal( creator, "contract.proposal.test", "proposal_test", params, "subject1", end_date, init_account_priv_key );
    generate_blocks( 1 );
    
    vote_proposal( voter_01, { id_proposal_00 }, true/*approve*/, init_account_priv_key );
    generate_blocks( 1 );
            
    {
        BOOST_TEST_MESSAGE( "---Processing proposal---" );
                
        auto next_block = get_nr_blocks_until_maintenance_block();
        generate_blocks( next_block - 1 );
        generate_blocks( 1 );
    }
    
    validate_database();
    
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( generating_xinsu )
{ try {
    BOOST_TEST_MESSAGE( "Testing: generating xinsu" );
    
    ACTORS( (alice)(bob)(carol) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    generate_block();
    
    //=====================preparing=====================
    signed_transaction tx;
    create_contract_operation op;

    op.owner = TAIYI_INIT_SIMING_NAME;
    op.name = "contract.xinsu.sample";
    op.data = s_code_xinsu_sample;
    tx.operations.push_back( op );

    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    sign( tx, init_account_priv_key );
    db->push_transaction( tx, 0 );
    generate_block();
    validate_database();

    auto creator = TAIYI_INIT_SIMING_NAME;
    auto receiver = "bob";
    
    auto end_date = db->head_block_time() + fc::days( 2 );
        
    auto voter_01 = TAIYI_INIT_SIMING_NAME;
    //=====================preparing=====================
    
    //Needed basic operations
    lua_map params;
    params[lua_key(lua_int(1))] = lua_string(receiver);
    int64_t id_proposal_00 = create_proposal( creator, "contract.xinsu.sample", "grant_xinsu", params, "subject1", end_date, init_account_priv_key );
    generate_blocks( 1 );
    
    vote_proposal( voter_01, { id_proposal_00 }, true/*approve*/, init_account_priv_key );
    generate_blocks( 1 );
            
    const account_object& _receiver = db->get_account(receiver);
    const proposal_object* _proposal = db->find<proposal_object, by_id>(id_proposal_00);
    BOOST_REQUIRE( _proposal != nullptr );

    {
        BOOST_TEST_MESSAGE( "---Granting---" );
                
        auto next_block = get_nr_blocks_until_maintenance_block();
        generate_blocks( next_block - 1 );
        generate_blocks( 1 );

        BOOST_REQUIRE( db->is_xinsu(_receiver) == true );
    }
    
    validate_database();
    
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( proposals_maintenance)
{ try {
    BOOST_TEST_MESSAGE( "Testing: removing inactive proposals" );
    
    ACTORS( (alice)(bob) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    generate_block();
    
    //=====================preparing=====================
    const string code_proposal_test = " \
        function proposal_test(info)    \
            contract_helper:log(info)   \
        end                             \
    ";

    signed_transaction tx;
    create_contract_operation op;

    op.owner = TAIYI_INIT_SIMING_NAME;
    op.name = "contract.proposal.test";
    op.data = code_proposal_test;
    tx.operations.push_back( op );

    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    sign( tx, init_account_priv_key );
    db->push_transaction( tx, 0 );
    generate_block();
    validate_database();

    auto creator = TAIYI_INIT_SIMING_NAME;
    
    auto start_time = db->head_block_time();
    
    auto end_date_00 = start_time + fc::minutes( 10 );
    auto end_date_01 = start_time + fc::minutes( 30 );
    auto end_date_02 = start_time + fc::minutes( 20 );
    
    lua_map params;
    params[lua_key(lua_int(1))] = lua_string("proposal check");

    //=====================preparing=====================
    
    int64_t id_proposal_00 = create_proposal( creator, "contract.proposal.test", "proposal_test", params, "subject00", end_date_00, init_account_priv_key );
    generate_block();
    
    int64_t id_proposal_01 = create_proposal( creator, "contract.proposal.test", "proposal_test", params, "subject01", end_date_01, init_account_priv_key );
    generate_block();
    
    int64_t id_proposal_02 = create_proposal( creator, "contract.proposal.test", "proposal_test", params, "subject02", end_date_02, init_account_priv_key );
    generate_block();
    
    {
        BOOST_REQUIRE( exist_proposal( id_proposal_00 ) );
        BOOST_REQUIRE( exist_proposal( id_proposal_01 ) );
        BOOST_REQUIRE( exist_proposal( id_proposal_02 ) );
        
        generate_blocks( start_time + fc::seconds( TAIYI_PROPOSAL_MAINTENANCE_CLEANUP ) );
        start_time = db->head_block_time();
        
        BOOST_REQUIRE( exist_proposal( id_proposal_00 ) );
        BOOST_REQUIRE( exist_proposal( id_proposal_01 ) );
        BOOST_REQUIRE( exist_proposal( id_proposal_02 ) );
        
        generate_blocks( start_time + fc::minutes( 11 ) );
        BOOST_REQUIRE( !exist_proposal( id_proposal_00 ) );
        BOOST_REQUIRE( exist_proposal( id_proposal_01 ) );
        BOOST_REQUIRE( exist_proposal( id_proposal_02 ) );
        
        generate_blocks( start_time + fc::minutes( 21 ) );
        BOOST_REQUIRE( !exist_proposal( id_proposal_00 ) );
        BOOST_REQUIRE( exist_proposal( id_proposal_01 ) );
        BOOST_REQUIRE( !exist_proposal( id_proposal_02 ) );
        
        generate_blocks( start_time + fc::minutes( 31 ) );
        BOOST_REQUIRE( !exist_proposal( id_proposal_00 ) );
        BOOST_REQUIRE( !exist_proposal( id_proposal_01 ) );
        BOOST_REQUIRE( !exist_proposal( id_proposal_02 ) );
    }
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( proposal_object_apply )
{ try {
    BOOST_TEST_MESSAGE( "Testing: create_proposal_operation" );
    
    ACTORS( (alice)(bob) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    generate_block();
        
    //=====================preparing=====================
    const string code_proposal_test = " \
        function proposal_test(info)    \
            contract_helper:log(info)   \
        end                             \
    ";

    signed_transaction tx;
    create_contract_operation op;

    op.owner = TAIYI_INIT_SIMING_NAME;
    op.name = "contract.proposal.test";
    op.data = code_proposal_test;
    tx.operations.push_back( op );

    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    sign( tx, init_account_priv_key );
    db->push_transaction( tx, 0 );
    tx.signatures.clear();
    tx.operations.clear();
    generate_block();
    validate_database();

    //=====================creating=====================

    auto creator = TAIYI_INIT_SIMING_NAME;
    auto key = init_account_priv_key;
    auto start_date = db->head_block_time() + fc::days( 1 );
    auto end_date = start_date + fc::days( 2 );
    
    auto contract_name = "contract.proposal.test";
    auto function_name = "proposal_test";
    auto subject = "hello";

    lua_map params;
    params[lua_key(lua_int(1))] = lua_string("proposal check");

    call_contract_function_operation cop;
    cop.caller = creator;
    cop.contract_name = "contract.tps.basic";
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

    const auto& creator_account = db->get_account(creator);
    const auto& proposal_idx = db->get_index<proposal_index>().indices().get<by_creator>();
    auto found = proposal_idx.find( creator_account.id );
    BOOST_REQUIRE( found != proposal_idx.end() );

    BOOST_REQUIRE( found->creator == creator_account.id );
    BOOST_REQUIRE( found->contract_name == contract_name );
    BOOST_REQUIRE( found->function_name == function_name );
    BOOST_REQUIRE( found->end_date == end_date );
    BOOST_REQUIRE( found->subject == subject );
    BOOST_REQUIRE( found->value_list[0] == params[lua_key(lua_int(1))] );
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( proposal_vote_object_apply )
{ try {
    BOOST_TEST_MESSAGE( "Testing: proposal_vote_object_operation" );
    
    ACTORS( (alice)(bob)(carol)(dan) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "carol", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "dan", ASSET( "1000.000 YANG" ) );
    generate_block();
    
    generate_xinsu({"alice", "bob", "carol", "dan"});
    
    //=====================preparing=====================

    const string code_proposal_test = " \
        function proposal_test(info)    \
            contract_helper:log(info)   \
        end                             \
    ";

    signed_transaction tx;
    create_contract_operation op;

    op.owner = TAIYI_INIT_SIMING_NAME;
    op.name = "contract.proposal.test";
    op.data = code_proposal_test;
    tx.operations.push_back( op );

    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    sign( tx, init_account_priv_key );
    db->push_transaction( tx, 0 );
    tx.signatures.clear();
    tx.operations.clear();
    generate_block();
    validate_database();

    //=====================creating proposal=====================

    auto start_date = db->head_block_time() + fc::days( 1 );
    auto end_date = start_date + fc::days( 2 );
    
    lua_map params;
    params[lua_key(lua_int(1))] = lua_string("proposal check");

    int64_t id_proposal_00 = create_proposal( "alice", "contract.proposal.test", "proposal_test", params, "subject00", end_date, alice_private_key );
    generate_block();

    //=====================voting=====================

    auto voter_01 = "carol";
    auto voter_01_key = carol_private_key;
    
    call_contract_function_operation cop;
    cop.contract_name = "contract.tps.basic";
    cop.function_name = "update_proposal_votes";

    const auto& proposal_vote_idx = db->get_index< proposal_vote_index >().indices().get< by_voter_proposal >();
    
    {
        BOOST_TEST_MESSAGE( "---Voting for proposal( `id_proposal_00` )---" );

        cop.caller = voter_01;
        params[lua_key(lua_int(1))] = lua_types(lua_int(id_proposal_00));
        cop.value_list = {
            lua_table(params),
            lua_bool(true)
        };

        tx.operations.push_back( cop );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, voter_01_key );
        db->push_transaction( tx, 0 );
        tx.operations.clear();
        tx.signatures.clear();
        
        const auto& voter_01_account = db->get_account(voter_01);
        auto found = proposal_vote_idx.find( boost::make_tuple( voter_01_account.id, id_proposal_00 ) );
        BOOST_REQUIRE( found->voter == voter_01_account.id );
        BOOST_REQUIRE( static_cast< int64_t >( found->proposal_id ) == id_proposal_00 );
    }
    
    {
        BOOST_TEST_MESSAGE( "---Unvoting proposal( `id_proposal_00` )---" );
        cop.caller = voter_01;
        params[lua_key(lua_int(1))] = lua_types(lua_int(id_proposal_00));
        cop.value_list = {
            lua_table(params),
            lua_bool(false)
        };

        tx.operations.push_back( cop );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, voter_01_key );
        db->push_transaction( tx, 0 );
        tx.operations.clear();
        tx.signatures.clear();
        
        const auto& voter_01_account = db->get_account(voter_01);
        auto found = proposal_vote_idx.find( boost::make_tuple( voter_01_account.id, id_proposal_00 ) );
        BOOST_REQUIRE( found == proposal_vote_idx.end() );
    }
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( proposal_vote_object_01_apply )
{ try {
    BOOST_TEST_MESSAGE( "Testing: proposal_vote_object_operation" );
    
    ACTORS( (alice)(bob)(carol)(dan) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "carol", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "dan", ASSET( "1000.000 YANG" ) );
    generate_block();
    
    generate_xinsu({"alice", "bob", "carol", "dan"});
    
    //=====================preparing=====================

    const string code_proposal_test = " \
        function proposal_test(info)    \
            contract_helper:log(info)   \
        end                             \
    ";

    signed_transaction tx;
    create_contract_operation op;

    op.owner = TAIYI_INIT_SIMING_NAME;
    op.name = "contract.proposal.test";
    op.data = code_proposal_test;
    tx.operations.push_back( op );

    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    sign( tx, init_account_priv_key );
    db->push_transaction( tx, 0 );
    tx.signatures.clear();
    tx.operations.clear();
    generate_block();
    validate_database();

    //=====================creating proposal=====================

    auto creator = "alice";
    
    auto start_date = db->head_block_time() + fc::days( 1 );
    auto end_date = start_date + fc::days( 2 );

    lua_map params;
    params[lua_key(lua_int(1))] = lua_string("proposal check");

    int64_t id_proposal_00 = create_proposal( creator, "contract.proposal.test", "proposal_test", params, "subject00", end_date, alice_private_key );
    int64_t id_proposal_01 = create_proposal( creator, "contract.proposal.test", "proposal_test", params, "subject01", end_date, alice_private_key );
    generate_block();
    
    //=====================voting=====================

    auto voter_01 = "carol";
    auto voter_01_key = carol_private_key;
    
    call_contract_function_operation cop;
    cop.contract_name = "contract.tps.basic";
    cop.function_name = "update_proposal_votes";

    const auto& proposal_vote_idx = db->get_index< proposal_vote_index >().indices().get< by_voter_proposal >();
    
    {
        BOOST_TEST_MESSAGE( "---Voting by `voter_01` for proposals( `id_proposal_00`, `id_proposal_01` )---" );
        cop.caller = voter_01;
        params.clear();
        params[lua_key(lua_int(1))] = lua_types(lua_int(id_proposal_00));
        params[lua_key(lua_int(2))] = lua_types(lua_int(id_proposal_01));
        cop.value_list = {
            lua_table(params),
            lua_bool(true)
        };

        tx.operations.push_back( cop );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, voter_01_key );
        db->push_transaction( tx, 0 );
        tx.operations.clear();
        tx.signatures.clear();
        
        int32_t cnt = 0;
        const auto& voter_01_account = db->get_account(voter_01);
        auto found = proposal_vote_idx.find( voter_01_account.id );
        while( found != proposal_vote_idx.end() && found->voter == voter_01_account.id )
        {
            ++cnt;
            ++found;
        }
        BOOST_REQUIRE( cnt == 2 );
    }

    params.clear();
    params[lua_key(lua_int(1))] = lua_string("proposal check");
    int64_t id_proposal_02 = create_proposal( creator, "contract.proposal.test", "proposal_test", params, "subject02", end_date, alice_private_key );

    std::string voter_02 = "dan";
    auto voter_02_key = dan_private_key;
    
    {
        BOOST_TEST_MESSAGE( "---Voting by `voter_02` for proposals( `id_proposal_00`, `id_proposal_01`, `id_proposal_02` )---" );
        cop.caller = voter_02;
        params.clear();
        params[lua_key(lua_int(1))] = lua_types(lua_int(id_proposal_00));
        params[lua_key(lua_int(2))] = lua_types(lua_int(id_proposal_01));
        params[lua_key(lua_int(3))] = lua_types(lua_int(id_proposal_02));
        cop.value_list = {
            lua_table(params),
            lua_bool(true)
        };

        tx.operations.push_back( cop );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, voter_02_key );
        db->push_transaction( tx, 0 );
        tx.operations.clear();
        tx.signatures.clear();
        
        int32_t cnt = 0;
        const auto& voter_02_account = db->get_account(voter_02);
        auto found = proposal_vote_idx.find( voter_02_account.id );
        while( found != proposal_vote_idx.end() && found->voter == voter_02_account.id )
        {
            ++cnt;
            ++found;
        }
        BOOST_REQUIRE( cnt == 3 );
    }
    
    {
        BOOST_TEST_MESSAGE( "---Voting by `voter_01` for proposals( `id_proposal_02` )---" );
        cop.caller = voter_01;
        params.clear();
        params[lua_key(lua_int(1))] = lua_types(lua_int(id_proposal_02));
        cop.value_list = {
            lua_table(params),
            lua_bool(true)
        };

        tx.operations.push_back( cop );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, voter_01_key );
        db->push_transaction( tx, 0 );
        tx.operations.clear();
        tx.signatures.clear();
        
        int32_t cnt = 0;
        const auto& voter_01_account = db->get_account(voter_01);
        auto found = proposal_vote_idx.find( voter_01_account.id );
        while( found != proposal_vote_idx.end() && found->voter == voter_01_account.id )
        {
            ++cnt;
            ++found;
        }
        BOOST_REQUIRE( cnt == 3 );
    }
    
    {
        BOOST_TEST_MESSAGE( "---Unvoting by `voter_01` proposals( `id_proposal_02` )---" );
        cop.caller = voter_01;
        params.clear();
        params[lua_key(lua_int(1))] = lua_types(lua_int(id_proposal_02));
        cop.value_list = {
            lua_table(params),
            lua_bool(false)
        };

        tx.operations.push_back( cop );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, voter_01_key );
        db->push_transaction( tx, 0 );
        tx.operations.clear();
        tx.signatures.clear();
        
        int32_t cnt = 0;
        const auto& voter_01_account = db->get_account(voter_01);
        auto found = proposal_vote_idx.find( voter_01_account.id );
        while( found != proposal_vote_idx.end() && found->voter == voter_01_account.id )
        {
            ++cnt;
            ++found;
        }
        BOOST_REQUIRE( cnt == 2 );
    }
    
    {
        BOOST_TEST_MESSAGE( "---Unvoting by `voter_01` proposals( `id_proposal_00` )---" );
        cop.caller = voter_01;
        params.clear();
        params[lua_key(lua_int(1))] = lua_types(lua_int(id_proposal_00));
        cop.value_list = {
            lua_table(params),
            lua_bool(false)
        };

        tx.operations.push_back( cop );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, voter_01_key );
        db->push_transaction( tx, 0 );
        tx.operations.clear();
        tx.signatures.clear();
        
        int32_t cnt = 0;
        const auto& voter_01_account = db->get_account(voter_01);
        auto found = proposal_vote_idx.find( voter_01_account.id );
        while( found != proposal_vote_idx.end() && found->voter == voter_01_account.id )
        {
            ++cnt;
            ++found;
        }
        BOOST_REQUIRE( cnt == 1 );
    }
    
    {
        BOOST_TEST_MESSAGE( "---Unvoting by `voter_02` proposals( `id_proposal_00`, `id_proposal_01`, `id_proposal_02` )---" );
        cop.caller = voter_02;
        params.clear();
        params[lua_key(lua_int(1))] = lua_types(lua_int(id_proposal_02));
        params[lua_key(lua_int(2))] = lua_types(lua_int(id_proposal_01));
        params[lua_key(lua_int(3))] = lua_types(lua_int(id_proposal_00));
        cop.value_list = {
            lua_table(params),
            lua_bool(false)
        };

        tx.operations.push_back( cop );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, voter_02_key );
        db->push_transaction( tx, 0 );
        tx.operations.clear();
        tx.signatures.clear();
        
        int32_t cnt = 0;
        const auto& voter_02_account = db->get_account(voter_02);
        auto found = proposal_vote_idx.find( voter_02_account.id );
        while( found != proposal_vote_idx.end() && found->voter == voter_02_account.id )
        {
            ++cnt;
            ++found;
        }
        BOOST_REQUIRE( cnt == 0 );
    }
    
    {
        BOOST_TEST_MESSAGE( "---Unvoting by `voter_01` proposals( `id_proposal_01` )---" );
        cop.caller = voter_01;
        params.clear();
        params[lua_key(lua_int(1))] = lua_types(lua_int(id_proposal_01));
        cop.value_list = {
            lua_table(params),
            lua_bool(false)
        };

        tx.operations.push_back( cop );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, voter_01_key );
        db->push_transaction( tx, 0 );
        tx.operations.clear();
        tx.signatures.clear();
        
        int32_t cnt = 0;
        const auto& voter_01_account = db->get_account(voter_01);
        auto found = proposal_vote_idx.find( voter_01_account.id );
        while( found != proposal_vote_idx.end() && found->voter == voter_01_account.id )
        {
            ++cnt;
            ++found;
        }
        BOOST_REQUIRE( cnt == 0 );
    }
    
    {
        BOOST_TEST_MESSAGE( "---Voting by `voter_01` for nothing---" );
        cop.caller = voter_01;
        params.clear();
        cop.value_list = {
            lua_table(params),
            lua_bool(true)
        };

        tx.operations.push_back( cop );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, voter_01_key );
        TAIYI_REQUIRE_THROW(db->push_transaction( tx, 0 ), fc::exception);
        tx.operations.clear();
        tx.signatures.clear();

        int32_t cnt = 0;
        const auto& voter_01_account = db->get_account(voter_01);
        auto found = proposal_vote_idx.find( voter_01_account.id );
        while( found != proposal_vote_idx.end() && found->voter == voter_01_account.id )
        {
            ++cnt;
            ++found;
        }
        BOOST_REQUIRE( cnt == 0 );
    }
    
    {
        BOOST_TEST_MESSAGE( "---Unvoting by `voter_01` nothing---" );
        cop.caller = voter_01;
        params.clear();
        cop.value_list = {
            lua_table(params),
            lua_bool(false)
        };

        tx.operations.push_back( cop );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, voter_01_key );
        TAIYI_REQUIRE_THROW(db->push_transaction( tx, 0 ), fc::exception);
        tx.operations.clear();
        tx.signatures.clear();
        
        int32_t cnt = 0;
        const auto& voter_01_account = db->get_account(voter_01);
        auto found = proposal_vote_idx.find( voter_01_account.id );
        while( found != proposal_vote_idx.end() && found->voter == voter_01_account.id )
        {
            ++cnt;
            ++found;
        }
        BOOST_REQUIRE( cnt == 0 );
    }
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( create_proposal_000 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - all args are ok" );

    ACTORS( (alice)(bob) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    generate_block();
    generate_xinsu({"alice"});
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    int64_t proposal = create_proposal(cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, alice_private_key);

    BOOST_REQUIRE( proposal >= 0 );

    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( create_proposal_001 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - invalid creator" );

    ACTORS( (alice)(bob) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    generate_block();
    generate_xinsu({"alice"});
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    TAIYI_REQUIRE_THROW( create_proposal( "", cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, alice_private_key ), fc::exception);

    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( create_proposal_002 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - invalid contarct_name" );

    ACTORS( (alice)(bob) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    generate_block();
    generate_xinsu({"alice"});
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    TAIYI_REQUIRE_THROW( create_proposal( cpd.creator, "", cpd.function_name, cpd.params, cpd.subject, cpd.end_date, alice_private_key ), fc::exception);

    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( create_proposal_003 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - invalid contract function name" );

    ACTORS( (alice)(bob) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    generate_block();
    generate_xinsu({"alice"});
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    TAIYI_REQUIRE_THROW( create_proposal( cpd.creator, cpd.contract_name, "", cpd.params, cpd.subject, cpd.end_date, alice_private_key ), fc::exception);

    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( create_proposal_004 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - invalid contract parameters" );

    ACTORS( (alice)(bob) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    generate_block();
    generate_xinsu({"alice"});
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    cpd.params[lua_key(lua_int(2))] = lua_string("invalid more parameter");
    TAIYI_REQUIRE_THROW( create_proposal( cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, alice_private_key ), fc::exception);

    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( create_proposal_005 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - invalid subject(empty)" );
    
    ACTORS( (alice)(bob) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    generate_block();
    generate_xinsu({"alice"});
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    cpd.subject = "";
    TAIYI_REQUIRE_THROW( create_proposal( cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, alice_private_key ), fc::exception);

    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( create_proposal_006 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - invalid subject(too long)" );

    ACTORS( (alice)(bob) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    generate_block();
    generate_xinsu({"alice"});
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    cpd.subject = "very very very very very very long long long long long long subject subject subject subject subject subject";
    TAIYI_REQUIRE_THROW( create_proposal( cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, alice_private_key ), fc::exception);

    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( create_proposal_007 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: create proposal: authorization test(not xinsu)" );
    
    ACTORS( (alice)(bob) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    generate_block();
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    TAIYI_REQUIRE_THROW( create_proposal( cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, alice_private_key ), fc::exception);

    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( update_proposal_votes_000 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: update proposal votes: opration arguments validation - all ok (approve true)" );

    ACTORS( (alice)(bob)(carol) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "carol", ASSET( "1000.000 YANG" ) );
    generate_block();
    generate_xinsu({"alice", "carol"});
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    int64_t proposal_1 = create_proposal(cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, alice_private_key);
    BOOST_REQUIRE(proposal_1 >= 0);
    
    std::vector< int64_t > proposals = {proposal_1};
    vote_proposal("carol", proposals, true, carol_private_key);
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( update_proposal_votes_001 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: update proposal votes: opration arguments validation - all ok (approve false)" );
    
    ACTORS( (alice)(bob)(carol) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "carol", ASSET( "1000.000 YANG" ) );
    generate_block();
    generate_xinsu({"alice", "carol"});
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    int64_t proposal_1 = create_proposal(cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, alice_private_key);
    BOOST_REQUIRE(proposal_1 >= 0);

    std::vector< int64_t > proposals = {proposal_1};
    vote_proposal("carol", proposals, false, carol_private_key);

    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( update_proposal_votes_002 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: update proposal votes: opration arguments validation - all ok (empty array)" );

    ACTORS( (alice)(bob)(carol) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "carol", ASSET( "1000.000 YANG" ) );
    generate_block();
    generate_xinsu({"alice", "carol"});
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    int64_t proposal_1 = create_proposal(cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, alice_private_key);
    BOOST_REQUIRE(proposal_1 >= 0);

    std::vector< int64_t > proposals;
    TAIYI_REQUIRE_THROW( vote_proposal("carol", proposals, true, carol_private_key), fc::exception);
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( update_proposal_votes_003 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: update proposal votes: opration arguments validation - all ok (array with negative digits)" );

    ACTORS( (alice)(bob)(carol) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "carol", ASSET( "1000.000 YANG" ) );
    generate_block();
    generate_xinsu({"alice", "carol"});

    std::vector< int64_t > proposals = {-1, -2, -3, -4, -5};
    vote_proposal("carol", proposals, true, carol_private_key);

    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( update_proposal_votes_004 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: update proposal votes: opration arguments validation - invalid voter" );

    ACTORS( (alice)(bob)(carol) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "carol", ASSET( "1000.000 YANG" ) );
    generate_block();
    generate_xinsu({"alice", "carol"});
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    int64_t proposal_1 = create_proposal(cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, alice_private_key);
    BOOST_REQUIRE(proposal_1 >= 0);

    std::vector< int64_t > proposals = {proposal_1};
    TAIYI_REQUIRE_THROW(vote_proposal("urp", proposals, false, carol_private_key), fc::exception);

    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( update_proposal_votes_005 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: update proposal votes: opration arguments validation - invalid id array (array with greater number of digits than allowed)" );

    ACTORS( (alice)(bob)(carol) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "carol", ASSET( "1000.000 YANG" ) );
    generate_block();
    generate_xinsu({"alice", "carol"});
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    std::vector< int64_t > proposals;
    for(int i = 0; i <= TAIYI_PROPOSAL_MAX_IDS_NUMBER; i++) {
        proposals.push_back(i);
    }
    TAIYI_REQUIRE_THROW(vote_proposal("carol", proposals, true, carol_private_key), fc::exception);
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( update_proposal_votes_006 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: update proposal votes: authorization test(not xinsu)" );

    ACTORS( (alice)(bob)(carol) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "carol", ASSET( "1000.000 YANG" ) );
    generate_block();
    generate_xinsu({"alice"});
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    int64_t proposal_1 = create_proposal(cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, alice_private_key);
    BOOST_REQUIRE(proposal_1 >= 0);

    std::vector< int64_t > proposals = {proposal_1};
    TAIYI_REQUIRE_THROW(vote_proposal("carol", proposals, true, carol_private_key), fc::exception);

    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( remove_proposal_000 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: remove proposal: basic verification operation - proposal removal (only one)." );

    ACTORS( (alice)(bob) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    generate_block();
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    cpd.creator = TAIYI_INIT_SIMING_NAME;
    int64_t proposal_1 = create_proposal(cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, init_account_priv_key);
    BOOST_REQUIRE(proposal_1 >= 0);

    auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
    const auto& creator = db->get_account(cpd.creator);
    auto found = proposal_idx.find(creator.id);
    BOOST_REQUIRE( found != proposal_idx.end() );
    BOOST_REQUIRE( proposal_idx.size() == 1 );
    
    std::vector<int64_t> proposals = { proposal_1 };
    remove_proposal(cpd.creator, proposals, init_account_priv_key);
    
    found = proposal_idx.find( creator.id );
    BOOST_REQUIRE( found == proposal_idx.end() );
    BOOST_REQUIRE( proposal_idx.empty() );
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( remove_proposal_001 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: remove proposal: basic verification operation - proposal removal (one from many)." );

    ACTORS( (alice)(bob) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    generate_block();
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    cpd.creator = TAIYI_INIT_SIMING_NAME;
    int64_t proposal_1 = create_proposal(cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, init_account_priv_key);
    cpd.subject = "hello2";
    int64_t proposal_2 = create_proposal(cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, init_account_priv_key);
    cpd.subject = "hello3";
    int64_t proposal_3 = create_proposal(cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, init_account_priv_key);
    BOOST_REQUIRE(proposal_1 >= 0);
    BOOST_REQUIRE(proposal_2 >= 0);
    BOOST_REQUIRE(proposal_3 >= 0);
    
    const auto& creator = db->get_account(cpd.creator);
    auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
    auto found = proposal_idx.find( creator.id );
    BOOST_REQUIRE( found != proposal_idx.end() );
    BOOST_REQUIRE( proposal_idx.size() == 3 );
    
    std::vector<int64_t> proposals = { proposal_1 };
    remove_proposal(cpd.creator, proposals, init_account_priv_key);
    
    found = proposal_idx.find( creator.id );
    BOOST_REQUIRE( found != proposal_idx.end() );   //two left
    BOOST_REQUIRE( proposal_idx.size() == 2 );
    
    proposals.clear();
    proposals.push_back(proposal_2);
    remove_proposal(cpd.creator, proposals, init_account_priv_key);
    
    found = proposal_idx.find( creator.id );
    BOOST_REQUIRE( found != proposal_idx.end() );   //one left
    BOOST_REQUIRE( proposal_idx.size() == 1 );
    
    proposals.clear();
    proposals.push_back(proposal_3);
    remove_proposal(cpd.creator, proposals, init_account_priv_key);
    
    found = proposal_idx.find( creator.id );
    BOOST_REQUIRE( found == proposal_idx.end() );   //none
    BOOST_REQUIRE( proposal_idx.empty() );
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( remove_proposal_002 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: remove proposal: basic verification operation - proposal removal (n from many in two steps)." );
    
    ACTORS( (alice)(bob) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    generate_block();
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    cpd.creator = TAIYI_INIT_SIMING_NAME;

    int64_t proposal = -1;
    std::vector<int64_t> proposals;
    
    for(int i = 0; i < 6; i++) {
        cpd.subject = "hello" + std::to_string(i);
        proposal = create_proposal(cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, init_account_priv_key);
        BOOST_REQUIRE(proposal >= 0);
        proposals.push_back(proposal);
    }
    BOOST_REQUIRE(proposals.size() == 6);
    
    const auto& creator = db->get_account(cpd.creator);
    auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
    auto found = proposal_idx.find( creator.id );
    BOOST_REQUIRE( found != proposal_idx.end() );
    BOOST_REQUIRE(proposal_idx.size() == 6);
    
    std::vector<int64_t> proposals_to_erase = {proposals[0], proposals[1], proposals[2], proposals[3]};
    remove_proposal(cpd.creator, proposals_to_erase, init_account_priv_key);
    
    found = proposal_idx.find( creator.id );
    BOOST_REQUIRE( found != proposal_idx.end() );
    BOOST_REQUIRE( proposal_idx.size() == 2 );
    
    proposals_to_erase.clear();
    proposals_to_erase.push_back(proposals[4]);
    proposals_to_erase.push_back(proposals[5]);
    
    remove_proposal(cpd.creator, proposals_to_erase, init_account_priv_key);
    found = proposal_idx.find( creator.id );
    BOOST_REQUIRE( found == proposal_idx.end() );
    BOOST_REQUIRE( proposal_idx.empty() );
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( remove_proposal_003 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: remove proposal: basic verification operation - proper proposal deletion check (one at time)." );

    ACTORS( (alice)(bob) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    generate_block();
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    cpd.creator = TAIYI_INIT_SIMING_NAME;

    int64_t proposal = -1;
    std::vector<int64_t> proposals;
    
    for(int i = 0; i < 2; i++) {
        cpd.subject = "hello" + std::to_string(i);
        proposal = create_proposal(cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, init_account_priv_key);
        BOOST_REQUIRE(proposal >= 0);
        proposals.push_back(proposal);
    }
    BOOST_REQUIRE(proposals.size() == 2);
    
    const auto& creator = db->get_account(cpd.creator);
    auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
    auto found = proposal_idx.find( creator.id );
    BOOST_REQUIRE( found != proposal_idx.end() );
    BOOST_REQUIRE(proposal_idx.size() == 2);
    
    std::vector<int64_t> proposals_to_erase = {proposals[0]};
    remove_proposal(cpd.creator, proposals_to_erase, init_account_priv_key);
    
    found = proposal_idx.find( creator.id );
    BOOST_REQUIRE( found != proposal_idx.end() );
    BOOST_REQUIRE( int64_t(found->id)  == proposals[1]); //这个很关键，即使删除了提案，旧提案的id也不能变化
    BOOST_REQUIRE( proposal_idx.size() == 1 );
    
    proposals_to_erase.clear();
    proposals_to_erase.push_back(proposals[1]);
    
    remove_proposal(cpd.creator, proposals_to_erase, init_account_priv_key);
    found = proposal_idx.find( creator.id );
    BOOST_REQUIRE( found == proposal_idx.end() );
    BOOST_REQUIRE( proposal_idx.empty() );
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( remove_proposal_004 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: remove proposal: basic verification operation - proper proposal deletion check (two at one time)." );

    ACTORS( (alice)(bob) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    generate_block();
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    cpd.creator = TAIYI_INIT_SIMING_NAME;

    int64_t proposal = -1;
    std::vector<int64_t> proposals;
    
    for(int i = 0; i < 6; i++) {
        cpd.subject = "hello" + std::to_string(i);
        proposal = create_proposal(cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, init_account_priv_key);
        BOOST_REQUIRE(proposal >= 0);
        proposals.push_back(proposal);
    }
    BOOST_REQUIRE(proposals.size() == 6);
    
    const auto& creator = db->get_account(cpd.creator);
    auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
    auto found = proposal_idx.find( creator.id );
    BOOST_REQUIRE( found != proposal_idx.end() );
    BOOST_REQUIRE(proposal_idx.size() == 6);
    
    std::vector<int64_t> proposals_to_erase = {proposals[0], proposals[5]};
    remove_proposal(cpd.creator, proposals_to_erase, init_account_priv_key);
    
    found = proposal_idx.find( creator.id );
    BOOST_REQUIRE( found != proposal_idx.end() );
    for(auto& it : proposal_idx) {
        BOOST_REQUIRE( static_cast< int64_t >(it.id) != proposals[0] );
        BOOST_REQUIRE( static_cast< int64_t >(it.id) != proposals[5] );
    }
    BOOST_REQUIRE( proposal_idx.size() == 4 );
    
    proposals_to_erase.clear();
    proposals_to_erase.push_back(proposals[1]);
    proposals_to_erase.push_back(proposals[4]);
    
    remove_proposal(cpd.creator, proposals_to_erase, init_account_priv_key);
    found = proposal_idx.find( creator.id );
    for(auto& it : proposal_idx) {
        BOOST_REQUIRE( static_cast< int64_t >(it.id) != proposals[0] );
        BOOST_REQUIRE( static_cast< int64_t >(it.id) != proposals[1] );
        BOOST_REQUIRE( static_cast< int64_t >(it.id) != proposals[4] );
        BOOST_REQUIRE( static_cast< int64_t >(it.id) != proposals[5] );
    }
    BOOST_REQUIRE( found != proposal_idx.end() );
    BOOST_REQUIRE( proposal_idx.size() == 2 );
    
    proposals_to_erase.clear();
    proposals_to_erase.push_back(proposals[2]);
    proposals_to_erase.push_back(proposals[3]);
    remove_proposal(cpd.creator, proposals_to_erase, init_account_priv_key);
    found = proposal_idx.find( creator.id );
    BOOST_REQUIRE( found == proposal_idx.end() );
    BOOST_REQUIRE( proposal_idx.empty() );
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( remove_proposal_005 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: remove proposal: basic verification operation - proposal with votes removal (only one)." );

    ACTORS( (alice)(bob)(carol) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "carol", ASSET( "1000.000 YANG" ) );
    generate_block();
    generate_xinsu({"alice", "bob", "carol"}); //此处已经产生了3个提案
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    int64_t proposal_1 = create_proposal(cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, alice_private_key);
    BOOST_REQUIRE(proposal_1 >= 0);
    
    const auto& creator = db->get_account(cpd.creator);
    auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
    auto found = proposal_idx.find( creator.id );
    
    BOOST_REQUIRE( found != proposal_idx.end() );
    BOOST_REQUIRE( proposal_idx.size() == 4 );
    
    std::vector<int64_t> vote_proposals = {proposal_1};
    vote_proposal( "bob", vote_proposals, true, bob_private_key );
    BOOST_REQUIRE( find_vote_for_proposal("bob", proposal_1) );
    
    std::vector<int64_t> proposals = { proposal_1 };
    remove_proposal(cpd.creator, proposals, alice_private_key);
    
    found = proposal_idx.find( creator.id );
    BOOST_REQUIRE( found == proposal_idx.end() );
    BOOST_REQUIRE( proposal_idx.size() == 3 );
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( remove_proposal_006 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: remove proposal: basic verification operation - remove proposal with votes and one voteless at same time." );

    ACTORS( (alice)(bob) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    generate_block();
    generate_xinsu({"alice", "bob"}); //此处已经产生了2个提案
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    int64_t proposal_1 = create_proposal(cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, alice_private_key);
    cpd.subject = "hello2";
    int64_t proposal_2 = create_proposal(cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, alice_private_key);
    BOOST_REQUIRE(proposal_1 >= 0);
    BOOST_REQUIRE(proposal_2 >= 0);
    
    const auto& creator = db->get_account(cpd.creator);
    auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
    auto found = proposal_idx.find( creator.id );
    BOOST_REQUIRE( found != proposal_idx.end() );
    BOOST_REQUIRE( proposal_idx.size() == 4 );
    
    std::vector<int64_t> vote_proposals = {proposal_1};
    
    vote_proposal( "bob", vote_proposals, true, bob_private_key );
    BOOST_REQUIRE( find_vote_for_proposal("bob", proposal_1) );
    
    std::vector<int64_t> proposals = { proposal_1, proposal_2 };
    remove_proposal(cpd.creator, proposals, alice_private_key);
    
    found = proposal_idx.find( creator.id );
    BOOST_REQUIRE( found == proposal_idx.end() );
    BOOST_REQUIRE( proposal_idx.size() == 2 );
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( remove_proposal_007 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: remove proposal: basic verification operation - remove proposals with votes at same time." );
    ACTORS( (alice)(bob)(carol) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "carol", ASSET( "1000.000 YANG" ) );
    generate_block();
    generate_xinsu({"alice", "bob", "carol"}); //此处已经产生了3个提案
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    int64_t proposal_1 = create_proposal(cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, alice_private_key);
    cpd.subject = "hello2";
    int64_t proposal_2 = create_proposal(cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, alice_private_key);
    BOOST_REQUIRE(proposal_1 >= 0);
    BOOST_REQUIRE(proposal_2 >= 0);
    
    const auto& creator = db->get_account(cpd.creator);
    auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
    auto found = proposal_idx.find( creator.id );
    BOOST_REQUIRE( found != proposal_idx.end() );
    BOOST_REQUIRE( proposal_idx.size() == 5 );
    
    std::vector<int64_t> vote_proposals = {proposal_1};
    vote_proposal( "bob", vote_proposals, true, bob_private_key );
    BOOST_REQUIRE( find_vote_for_proposal("bob", proposal_1) );
    vote_proposals.clear();
    vote_proposals.push_back(proposal_2);
    vote_proposal( "carol", vote_proposals, true, carol_private_key );
    BOOST_REQUIRE( find_vote_for_proposal("carol", proposal_2) );
    
    std::vector<int64_t> proposals = { proposal_1, proposal_2 };
    remove_proposal(cpd.creator, proposals, alice_private_key);
    
    found = proposal_idx.find( creator.id );
    BOOST_REQUIRE( found == proposal_idx.end() );
    BOOST_REQUIRE( proposal_idx.size() == 3 );
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( remove_proposal_008 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: remove proposal: opration arguments validation - all ok" );

    ACTORS( (alice)(bob)(carol) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    generate_block();

    vector<int64_t> proposals = { 0 };
    remove_proposal(TAIYI_INIT_SIMING_NAME, proposals, init_account_priv_key);
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( remove_proposal_009 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: remove proposal: opration arguments validation - invalid deleter" );

    ACTORS( (alice)(bob)(carol) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "carol", ASSET( "1000.000 YANG" ) );
    generate_block();
    generate_xinsu({"alice", "bob", "carol"}); //此处已经产生了3个提案
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    int64_t proposal_1 = create_proposal(cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, alice_private_key);

    vector<int64_t> proposals = { proposal_1 };
    TAIYI_REQUIRE_THROW(remove_proposal("bob", proposals, bob_private_key), fc::exception);
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( remove_proposal_010 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: remove proposal: opration arguments validation - invalid array(empty array)" );

    ACTORS( (alice)(bob)(carol) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "carol", ASSET( "1000.000 YANG" ) );
    generate_block();
    generate_xinsu({"alice", "bob", "carol"}); //此处已经产生了3个提案
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());

    vector<int64_t> proposals;
    TAIYI_REQUIRE_THROW(remove_proposal(cpd.creator, proposals, alice_private_key), fc::exception);
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( remove_proposal_011 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: remove proposal: opration arguments validation - invalid array(array with greater number of digits than allowed)" );
    ACTORS( (alice)(bob)(carol) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "carol", ASSET( "1000.000 YANG" ) );
    generate_block();
    generate_xinsu({"alice", "bob", "carol"}); //此处已经产生了3个提案
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    vector<int64_t> proposals;
    for(int i = 0; i <= TAIYI_PROPOSAL_MAX_IDS_NUMBER; i++) {
        cpd.subject = "hello" + std::to_string(i);
        proposals.push_back(create_proposal(cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, alice_private_key));
    }

    TAIYI_REQUIRE_THROW(remove_proposal(cpd.creator, proposals, alice_private_key), fc::exception);
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( remove_proposal_012 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: remove proposal: authorization test" );
    ACTORS( (alice)(bob)(carol) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "carol", ASSET( "1000.000 YANG" ) );
    generate_block();
    generate_xinsu({"alice", "bob", "carol"}); //此处已经产生了3个提案
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    create_proposal_data cpd(db->head_block_time());
    vector<int64_t> proposals;
    for(int i = 0; i <= 2; i++) {
        cpd.subject = "hello" + std::to_string(i);
        proposals.push_back(create_proposal(cpd.creator, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, cpd.end_date, alice_private_key));
    }
    
    TAIYI_REQUIRE_THROW(remove_proposal("bob", proposals, bob_private_key), fc::exception);

    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( proposals_maintenance_01 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: removing of old proposals using threshold" );
    
    ACTORS( (a00)(a01)(a02)(a03)(a04) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    generate_block();
    generate_xinsu({"a00", "a01", "a02", "a03", "a04"}); //此处已经产生了5个提案
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    //=====================preparing=====================
    auto start_time = db->head_block_time();
    
    const auto start_time_shift = fc::hours( 11 );
    const auto end_time_shift = fc::hours( 10 );
    const auto block_interval = fc::seconds( TAIYI_BLOCK_INTERVAL );
    
    auto start_date_00 = start_time + start_time_shift;
    auto end_date_00 = start_date_00 + end_time_shift;
        
    const auto nr_proposals = 200;
    std::vector< int64_t > proposals_id;
    
    struct initial_data
    {
        std::string account;
        fc::ecc::private_key key;
    };
    
    std::vector< initial_data > inits = {
        {"a00", a00_private_key },
        {"a01", a01_private_key },
        {"a02", a02_private_key },
        {"a03", a03_private_key },
        {"a04", a04_private_key },
    };
    
    for( auto item : inits ) {
        vest( TAIYI_INIT_SIMING_NAME, item.account, ASSET( "1000.000 YANG" ) );
    }
    //=====================preparing=====================
    
    create_proposal_data cpd(db->head_block_time());
    for( int32_t i = 0; i < nr_proposals; ++i )
    {
        auto item = inits[ i % inits.size() ];
        cpd.subject = "hello" + std::to_string(i);
        proposals_id.push_back(create_proposal(item.account, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, end_date_00, item.key));
        generate_block();
    }
    
    const auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_id >();
    
    auto current_active_proposals = nr_proposals;
    BOOST_REQUIRE( calc_proposals( proposal_idx, proposals_id ) == current_active_proposals );
    
    generate_blocks( start_time + fc::seconds( TAIYI_PROPOSAL_MAINTENANCE_CLEANUP ) );
    start_time = db->head_block_time();
    
    generate_blocks( start_time + ( start_time_shift + end_time_shift - block_interval ) );
    
    db->set_tps_remove_threshold(20);
    auto threshold = db->get_tps_remove_threshold();
    auto nr_stages = current_active_proposals / threshold;
    
    for( auto i = 0; i < nr_stages; ++i )
    {
        generate_block();
        
        current_active_proposals -= threshold;
        auto found = calc_proposals( proposal_idx, proposals_id );
        
        BOOST_REQUIRE( current_active_proposals == found );
    }
    
    BOOST_REQUIRE( current_active_proposals == 0 );
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( proposals_maintenance_02 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: removing of old proposals + votes using threshold" );
    
    ACTORS( (a00)(a01)(a02)(a03)(a04) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    generate_block();
    generate_xinsu({"a00", "a01", "a02", "a03", "a04"}); //此处已经产生了5个提案
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    //=====================preparing=====================
    auto start_time = db->head_block_time();
    
    const auto start_time_shift = fc::hours( 11 );
    const auto end_time_shift = fc::hours( 10 );
    const auto block_interval = fc::seconds( TAIYI_BLOCK_INTERVAL );
    
    auto start_date_00 = start_time + start_time_shift;
    auto end_date_00 = start_date_00 + end_time_shift;
    
    const auto nr_proposals = 10;
    std::vector< int64_t > proposals_id;
    
    struct initial_data
    {
        std::string account;
        fc::ecc::private_key key;
    };
    
    std::vector< initial_data > inits = {
        {"a00", a00_private_key },
        {"a01", a01_private_key },
        {"a02", a02_private_key },
        {"a03", a03_private_key },
        {"a04", a04_private_key },
    };
    
    for( auto item : inits )
    {
        vest( TAIYI_INIT_SIMING_NAME, item.account, ASSET( "1000.000 YANG" ) );
    }
    //=====================preparing=====================
    
    create_proposal_data cpd(db->head_block_time());
    for( auto i = 0; i < nr_proposals; ++i )
    {
        auto item = inits[ i % inits.size() ];
        cpd.subject = "hello" + std::to_string(i);
        proposals_id.push_back(create_proposal(item.account, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, end_date_00, item.key));
        generate_block();
    }
    
    auto itr_begin_2 = proposals_id.begin() + TAIYI_PROPOSAL_MAX_IDS_NUMBER;
    std::vector< int64_t > v1( proposals_id.begin(), itr_begin_2 );
    std::vector< int64_t > v2( itr_begin_2, proposals_id.end() );
    
    for( auto item : inits )
    {
        vote_proposal( item.account, v1, true/*approve*/, item.key);
        generate_blocks( 1 );
        vote_proposal( item.account, v2, true/*approve*/, item.key);
        generate_blocks( 1 );
    }
    
    const auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_id >();
    const auto& proposal_vote_idx = db->get_index< proposal_vote_index >().indices().get< by_proposal_voter >();
    
    auto current_active_proposals = nr_proposals;
    BOOST_REQUIRE( calc_proposals( proposal_idx, proposals_id ) == current_active_proposals );
    
    auto current_active_votes = current_active_proposals * static_cast< int16_t > ( inits.size() );
    BOOST_REQUIRE( calc_votes( proposal_vote_idx, proposals_id ) == current_active_votes );
    
    generate_blocks( start_time + fc::seconds( TAIYI_PROPOSAL_MAINTENANCE_CLEANUP ) );
    start_time = db->head_block_time();
    
    generate_blocks( start_time + ( start_time_shift + end_time_shift - block_interval ) );
    
    db->set_tps_remove_threshold(2);
    auto threshold = db->get_tps_remove_threshold();
    auto current_active_anything = current_active_proposals + current_active_votes;
    auto nr_stages = current_active_anything / threshold;
    
    for( auto i = 0; i < nr_stages; ++i )
    {
        generate_block();
        
        current_active_anything -= threshold;
        auto found_proposals = calc_proposals( proposal_idx, proposals_id );
        auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
        
        BOOST_REQUIRE( current_active_anything == found_proposals + found_votes );
    }
    
    BOOST_REQUIRE( current_active_anything == 0 );
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( proposals_removing_with_threshold )
{ try {
    BOOST_TEST_MESSAGE( "Testing: removing of old proposals + votes using threshold" );
    
    ACTORS( (a00)(a01)(a02)(a03)(a04) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    generate_block();
    generate_xinsu({"a00", "a01", "a02", "a03", "a04"}); //此处已经产生了5个提案
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    //=====================preparing=====================
    auto start_time = db->head_block_time();
    
    const auto start_time_shift = fc::days( 100 );
    const auto end_time_shift = fc::days( 101 );
    
    auto start_date_00 = start_time + start_time_shift;
    auto end_date_00 = start_date_00 + end_time_shift;
        
    const auto nr_proposals = 5;
    std::vector< int64_t > proposals_id;
    
    struct initial_data
    {
        std::string account;
        fc::ecc::private_key key;
    };
    
    std::vector< initial_data > inits = {
        {"a00", a00_private_key },
        {"a01", a01_private_key },
        {"a02", a02_private_key },
        {"a03", a03_private_key },
        {"a04", a04_private_key },
    };
    
    for( auto item : inits )
    {
        vest( TAIYI_INIT_SIMING_NAME, item.account, ASSET( "1000.000 YANG" ) );
    }
    //=====================preparing=====================
    
    create_proposal_data cpd(db->head_block_time());
    auto item_creator = inits[ 0 ];
    for( auto i = 0; i < nr_proposals; ++i )
    {
        cpd.subject = "hello" + std::to_string(i);
        proposals_id.push_back(create_proposal(item_creator.account, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, end_date_00, item_creator.key));
        generate_block();
    }
    
    for( auto item : inits )
    {
        vote_proposal( item.account, proposals_id, true/*approve*/, item.key);
        generate_blocks( 1 );
    }
    
    const auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_id >();
    const auto& proposal_vote_idx = db->get_index< proposal_vote_index >().indices().get< by_proposal_voter >();
    
    auto current_active_proposals = nr_proposals;
    BOOST_REQUIRE( calc_proposals( proposal_idx, proposals_id ) == current_active_proposals );
    
    auto current_active_votes = current_active_proposals * static_cast< int16_t > ( inits.size() );
    BOOST_REQUIRE( calc_votes( proposal_vote_idx, proposals_id ) == current_active_votes );
    
    db->set_tps_remove_threshold(20);
    auto threshold = db->get_tps_remove_threshold();
    BOOST_REQUIRE( threshold == 20 );
    
    vector< int64_t > _proposals_id( proposals_id.begin(), proposals_id.end() );
    
    {
        remove_proposal( item_creator.account,  _proposals_id, item_creator.key );
        auto found_proposals = calc_proposals( proposal_idx, proposals_id );
        auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
        BOOST_REQUIRE( found_proposals == 2 );
        BOOST_REQUIRE( found_votes == 8 );
        generate_blocks( 1 );
    }
    
    {
        remove_proposal( item_creator.account,  _proposals_id, item_creator.key );
        auto found_proposals = calc_proposals( proposal_idx, proposals_id );
        auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
        BOOST_REQUIRE( found_proposals == 0 );
        BOOST_REQUIRE( found_votes == 0 );
        generate_blocks( 1 );
    }
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( proposals_removing_with_threshold_01 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: removing of old proposals/votes using threshold " );
    
    ACTORS( (a00)(a01)(a02)(a03)(a04)(a05) )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    generate_block();
    generate_xinsu({"a00", "a01", "a02", "a03", "a04", "a05"}); //此处已经产生了5个提案
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    //=====================preparing=====================
    auto start_time = db->head_block_time();
    
    const auto start_time_shift = fc::days( 100 );
    const auto end_time_shift = fc::days( 101 );
    
    auto start_date_00 = start_time + start_time_shift;
    auto end_date_00 = start_date_00 + end_time_shift;
        
    const auto nr_proposals = 10;
    std::vector< int64_t > proposals_id;
    
    struct initial_data
    {
        std::string account;
        fc::ecc::private_key key;
    };
    
    std::vector< initial_data > inits = {
        {"a00", a00_private_key },
        {"a01", a01_private_key },
        {"a02", a02_private_key },
        {"a03", a03_private_key },
        {"a04", a04_private_key },
        {"a05", a05_private_key },
    };
    
    for( auto item : inits )
    {
        vest( TAIYI_INIT_SIMING_NAME, item.account, ASSET( "1000.000 YANG" ) );
    }
    //=====================preparing=====================
    
    create_proposal_data cpd(db->head_block_time());
    auto item_creator = inits[ 0 ];
    for( auto i = 0; i < nr_proposals; ++i )
    {
        cpd.subject = "hello" + std::to_string(i);
        proposals_id.push_back(create_proposal(item_creator.account, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, end_date_00, item_creator.key));
        generate_block();
    }
    
    auto itr_begin_2 = proposals_id.begin() + TAIYI_PROPOSAL_MAX_IDS_NUMBER;
    std::vector< int64_t > v1( proposals_id.begin(), itr_begin_2 );
    std::vector< int64_t > v2( itr_begin_2, proposals_id.end() );
    
    for( auto item : inits )
    {
        vote_proposal( item.account, v1, true/*approve*/, item.key);
        generate_block();
        vote_proposal( item.account, v2, true/*approve*/, item.key);
        generate_block();
    }
    
    const auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_id >();
    const auto& proposal_vote_idx = db->get_index< proposal_vote_index >().indices().get< by_proposal_voter >();
    
    auto current_active_proposals = nr_proposals;
    BOOST_REQUIRE( calc_proposals( proposal_idx, proposals_id ) == current_active_proposals );
    
    auto current_active_votes = current_active_proposals * static_cast< int16_t > ( inits.size() );
    BOOST_REQUIRE( calc_votes( proposal_vote_idx, proposals_id ) == current_active_votes );
    
    db->set_tps_remove_threshold(20);
    auto threshold = db->get_tps_remove_threshold();
    BOOST_REQUIRE( threshold == 20 );
    
    /*
     nr_proposals = 10
     nr_votes_per_proposal = 6
     
     Info:
     Pn - number of proposal
     vn - number of vote
     xx - elements removed in given block
     oo - elements removed earlier
     rr - elements with status `removed`
     
     Below in matrix is situation before removal any element.
     
     P0 P1 P2 P3 P4 P5 P6 P7 P8 P9
     
     v0 v0 v0 v0 v0 v0 v0 v0 v0 v0
     v1 v1 v1 v1 v1 v1 v1 v1 v1 v1
     v2 v2 v2 v2 v2 v2 v2 v2 v2 v2
     v3 v3 v3 v3 v3 v3 v3 v3 v3 v3
     v4 v4 v4 v4 v4 v4 v4 v4 v4 v4
     v5 v5 v5 v5 v5 v5 v5 v5 v5 v5
     
     Total number of elements to be removed:
     Total = nr_proposals * nr_votes_per_proposal + nr_proposals = 10 * 6 + 10 = 70
     */
    
    /*
     P0 P1 P2 xx P4 P5 P6 P7 P8 P9
     
     v0 v0 v0 xx v0 v0 v0 v0 v0 v0
     v1 v1 v1 xx v1 v1 v1 v1 v1 v1
     v2 v2 v2 xx v2 v2 v2 v2 v2 v2
     v3 v3 v3 xx v3 v3 v3 v3 v3 v3
     v4 v4 v4 xx v4 v4 v4 v4 v4 v4
     v5 v5 v5 xx v5 v5 v5 v5 v5 v5
     */
    {
        remove_proposal( item_creator.account, {proposals_id[3]}, item_creator.key );
        generate_block();
        auto found_proposals = calc_proposals( proposal_idx, proposals_id );
        auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
        BOOST_REQUIRE( found_proposals == 9 );
        BOOST_REQUIRE( found_votes == 54 );
        
        int32_t cnt = 0;
        for( auto id : proposals_id )
            cnt += ( id != proposals_id[3] ) ? calc_proposal_votes( proposal_vote_idx, id ) : 0;
        BOOST_REQUIRE( cnt == found_votes );
    }
    
    /*
     P0 xx xx oo P4 P5 P6 rr P8 P9
     
     v0 xx xx oo v0 v0 v0 xx v0 v0
     v1 xx xx oo v1 v1 v1 xx v1 v1
     v2 xx xx oo v2 v2 v2 xx v2 v2
     v3 xx xx oo v3 v3 v3 xx v3 v3
     v4 xx xx oo v4 v4 v4 xx v4 v4
     v5 xx xx oo v5 v5 v5 xx v5 v5
     */
    {
        remove_proposal( item_creator.account, {proposals_id[1], proposals_id[2], proposals_id[7]}, item_creator.key );
        auto found_proposals = calc_proposals( proposal_idx, proposals_id );
        auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
        BOOST_REQUIRE( exist_proposal( proposals_id[7] ) && find_proposal( proposals_id[7] )->removed );
        BOOST_REQUIRE( found_proposals == 7 );
        BOOST_REQUIRE( found_votes == 36 );
    }
    
    
    /*
     P0 oo oo oo P4 P5 P6 xx P8 P9
     
     v0 oo oo oo v0 v0 v0 oo v0 v0
     v1 oo oo oo v1 v1 v1 oo v1 v1
     v2 oo oo oo v2 v2 v2 oo v2 v2
     v3 oo oo oo v3 v3 v3 oo v3 v3
     v4 oo oo oo v4 v4 v4 oo v4 v4
     v5 oo oo oo v5 v5 v5 oo v5 v5
     */
    {
        //Only the proposal P7 is removed.
        generate_block();
        
        auto found_proposals = calc_proposals( proposal_idx, proposals_id );
        auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
        BOOST_REQUIRE( !exist_proposal( proposals_id[7] ) );
        BOOST_REQUIRE( found_proposals == 6 );
        BOOST_REQUIRE( found_votes == 36 );
    }
    
    /*
     P0 oo oo oo P4 xx xx oo rr rr
     
     v0 oo oo oo v0 xx xx oo xx rr
     v1 oo oo oo v1 xx xx oo xx rr
     v2 oo oo oo v2 xx xx oo xx rr
     v3 oo oo oo v3 xx xx oo xx rr
     v4 oo oo oo v4 xx xx oo xx rr
     v5 oo oo oo v5 xx xx oo xx rr
     */
    {
        remove_proposal( item_creator.account, {proposals_id[5], proposals_id[6], proposals_id[7]/*doesn't exist*/, proposals_id[8], proposals_id[9]}, item_creator.key );
        auto found_proposals = calc_proposals( proposal_idx, proposals_id );
        auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
        BOOST_REQUIRE( exist_proposal( proposals_id[8] ) && find_proposal( proposals_id[8] )->removed );
        BOOST_REQUIRE( found_proposals == 4 );
        BOOST_REQUIRE( found_votes == 18 );
        
        int32_t cnt = 0;
        for( auto id : proposals_id )
            cnt += ( id == proposals_id[0] || id == proposals_id[4] || id == proposals_id[8] || id == proposals_id[9] ) ? calc_proposal_votes( proposal_vote_idx, id ) : 0;
        BOOST_REQUIRE( cnt == found_votes );
    }
    
    /*
     xx oo oo oo xx oo oo oo xx rr
     
     xx oo oo oo xx oo oo oo oo rr
     xx oo oo oo xx oo oo oo oo xx
     xx oo oo oo xx oo oo oo oo xx
     xx oo oo oo xx oo oo oo oo xx
     xx oo oo oo xx oo oo oo oo xx
     xx oo oo oo xx oo oo oo oo xx
     */
    {
        remove_proposal( item_creator.account, {proposals_id[0], proposals_id[4], proposals_id[8], proposals_id[9]}, item_creator.key );
        auto found_proposals = calc_proposals( proposal_idx, proposals_id );
        auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
        BOOST_REQUIRE( exist_proposal( proposals_id[9] ) && find_proposal( proposals_id[9] )->removed );
        BOOST_REQUIRE( found_proposals == 1 );
        BOOST_REQUIRE( found_votes == 1 );
    }
    
    /*
     oo oo oo oo oo oo oo oo oo xx
     
     oo oo oo oo oo oo oo oo oo xx
     oo oo oo oo oo oo oo oo oo oo
     oo oo oo oo oo oo oo oo oo oo
     oo oo oo oo oo oo oo oo oo oo
     oo oo oo oo oo oo oo oo oo oo
     oo oo oo oo oo oo oo oo oo oo
     */
    {
        //Only the proposal P9 is removed.
        generate_block();
        
        auto found_proposals = calc_proposals( proposal_idx, proposals_id );
        auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
        BOOST_REQUIRE( found_proposals == 0 );
        BOOST_REQUIRE( found_votes == 0 );
    }
    
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( proposals_removing_with_threshold_02 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: removing of old proposals/votes using threshold " );
    
    ACTORS(
           (a00)(a01)(a02)(a03)(a04)(a05)(a06)(a07)(a08)(a09)
           (a10)(a11)(a12)(a13)(a14)(a15)(a16)(a17)(a18)(a19)
           (a20)(a21)(a22)(a23)(a24)(a25)(a26)(a27)(a28)(a29)
           (a30)(a31)(a32)(a33)(a34)(a35)(a36)(a37)(a38)(a39)
           (a40)(a41)(a42)(a43)(a44)(a45)(a46)(a47)(a48)(a49)
           )
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    generate_block();
    generate_xinsu({
        "a00","a01","a02","a03","a04","a05","a06","a07","a08","a09",
        "a10","a11","a12","a13","a14","a15","a16","a17","a18","a19",
        "a20","a21","a22","a23","a24","a25","a26","a27","a28","a29",
        "a30","a31","a32","a33","a34","a35","a36","a37","a38","a39",
        "a40","a41","a42","a43","a44","a45","a46","a47","a48","a49"
    });
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    //=====================preparing=====================
    auto start_time = db->head_block_time();
    
    const auto start_time_shift = fc::days( 100 );
    const auto end_time_shift = fc::days( 101 );
    
    auto start_date_00 = start_time + start_time_shift;
    auto end_date_00 = start_date_00 + end_time_shift;
    
    const auto nr_proposals = 5;
    std::vector< int64_t > proposals_id;
    
    struct initial_data
    {
        std::string account;
        fc::ecc::private_key key;
    };
    
    std::vector< initial_data > inits = {
        {"a00", a00_private_key }, {"a01", a01_private_key }, {"a02", a02_private_key }, {"a03", a03_private_key }, {"a04", a04_private_key },
        {"a05", a05_private_key }, {"a06", a06_private_key }, {"a07", a07_private_key }, {"a08", a08_private_key }, {"a09", a09_private_key },
        {"a10", a10_private_key }, {"a11", a11_private_key }, {"a12", a12_private_key }, {"a13", a13_private_key }, {"a14", a14_private_key },
        {"a15", a15_private_key }, {"a16", a16_private_key }, {"a17", a17_private_key }, {"a18", a18_private_key }, {"a19", a19_private_key },
        {"a20", a20_private_key }, {"a21", a21_private_key }, {"a22", a22_private_key }, {"a23", a23_private_key }, {"a24", a24_private_key },
        {"a25", a25_private_key }, {"a26", a26_private_key }, {"a27", a27_private_key }, {"a28", a28_private_key }, {"a29", a29_private_key },
        {"a30", a30_private_key }, {"a31", a31_private_key }, {"a32", a32_private_key }, {"a33", a33_private_key }, {"a34", a34_private_key },
        {"a35", a35_private_key }, {"a36", a36_private_key }, {"a37", a37_private_key }, {"a38", a38_private_key }, {"a39", a39_private_key },
        {"a40", a40_private_key }, {"a41", a41_private_key }, {"a42", a42_private_key }, {"a43", a43_private_key }, {"a44", a44_private_key },
        {"a45", a45_private_key }, {"a46", a46_private_key }, {"a47", a47_private_key }, {"a48", a48_private_key }, {"a49", a49_private_key }
    };
    
    for( auto item : inits )
    {
        vest( TAIYI_INIT_SIMING_NAME, item.account, ASSET( "1000.000 YANG" ) );
    }
    //=====================preparing=====================
    
    create_proposal_data cpd(db->head_block_time());
    auto item_creator = inits[ 0 ];
    for( auto i = 0; i < nr_proposals; ++i )
    {
        cpd.subject = "hello" + std::to_string(i);
        proposals_id.push_back(create_proposal(item_creator.account, cpd.contract_name, cpd.function_name, cpd.params, cpd.subject, end_date_00, item_creator.key));
        generate_block();
    }
    
    for( auto item : inits )
    {
        vote_proposal( item.account, proposals_id, true/*approve*/, item.key);
        generate_block();
    }
    
    const auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_id >();
    const auto& proposal_vote_idx = db->get_index< proposal_vote_index >().indices().get< by_proposal_voter >();
    
    auto current_active_proposals = nr_proposals;
    BOOST_REQUIRE( calc_proposals( proposal_idx, proposals_id ) == current_active_proposals );
    
    auto current_active_votes = current_active_proposals * static_cast< int16_t > ( inits.size() );
    BOOST_REQUIRE( calc_votes( proposal_vote_idx, proposals_id ) == current_active_votes );
    
    db->set_tps_remove_threshold(20);
    auto threshold = db->get_tps_remove_threshold();
    BOOST_REQUIRE( threshold == 20 );
    
    /*
     nr_proposals = 5
     nr_votes_per_proposal = 50
     
     Info:
     Pn - number of proposal
     vn - number of vote
     xx - elements removed in given block
     oo - elements removed earlier
     rr - elements with status `removed`
     
     Below in matrix is situation before removal any element.
     
     P0    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     P1    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     P2    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     P3    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     P4    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     
     Total number of elements to be removed:
     Total = nr_proposals * nr_votes_per_proposal + nr_proposals = 5 * 50 + 5 = 255
     */
    
    /*
     rr    xx xx xx xx xx xx xx xx xx xx rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr
     P1    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     P2    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     rr    xx xx xx xx xx xx xx xx xx xx rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr
     P4    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     */
    {
        remove_proposal( item_creator.account, {proposals_id[0], proposals_id[3]}, item_creator.key );
        auto found_proposals = calc_proposals( proposal_idx, proposals_id );
        auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
        BOOST_REQUIRE( exist_proposal( proposals_id[0] ) && find_proposal( proposals_id[0] )->removed );
        BOOST_REQUIRE( exist_proposal( proposals_id[3] ) && find_proposal( proposals_id[3] )->removed );
        BOOST_REQUIRE( found_proposals == 5 );
        BOOST_REQUIRE( found_votes == 230 );
    }
    
    /*
     rr    xx xx xx xx xx xx xx xx xx xx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr
     P1    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     P2    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     rr    xx xx xx xx xx xx xx xx xx xx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr
     P4    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     */
    {
        generate_block();
        
        auto found_proposals = calc_proposals( proposal_idx, proposals_id );
        auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
        BOOST_REQUIRE( exist_proposal( proposals_id[0] ) && find_proposal( proposals_id[0] )->removed );
        BOOST_REQUIRE( exist_proposal( proposals_id[3] ) && find_proposal( proposals_id[3] )->removed );
        BOOST_REQUIRE( found_proposals == 5 );
        BOOST_REQUIRE( found_votes == 210 );
    }
    
    /*
     xx    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx
     P1    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     P2    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     xx    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx
     P4    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     */
    {
        generate_block();
        generate_block();
        generate_block();
        generate_block();
        
        auto found_proposals = calc_proposals( proposal_idx, proposals_id );
        auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
        BOOST_REQUIRE( !exist_proposal( proposals_id[0] ) );
        BOOST_REQUIRE( !exist_proposal( proposals_id[3] ) );
        BOOST_REQUIRE( found_proposals == 3 );
        BOOST_REQUIRE( found_votes == 150 );
    }
    
    /*
     nothing changed - the same state as in previous block
     
     oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
     P1    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     P2    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
     P4    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     */
    {
        generate_block();
        
        auto found_proposals = calc_proposals( proposal_idx, proposals_id );
        auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
        BOOST_REQUIRE( !exist_proposal( proposals_id[0] ) );
        BOOST_REQUIRE( !exist_proposal( proposals_id[3] ) );
        BOOST_REQUIRE( found_proposals == 3 );
        BOOST_REQUIRE( found_votes == 150 );
    }
    
    /*
     nothing changed - the same state as in previous block
     */
    {
        generate_block();
        
        auto found_proposals = calc_proposals( proposal_idx, proposals_id );
        auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
        BOOST_REQUIRE( !exist_proposal( proposals_id[0] ) );
        BOOST_REQUIRE( !exist_proposal( proposals_id[3] ) );
        BOOST_REQUIRE( found_proposals == 3 );
        BOOST_REQUIRE( found_votes == 150 );
    }
    
    /*
     oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
     rr    xx xx xx xx xx xx xx xx xx xx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr
     P2    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
     P4    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     */
    {
        remove_proposal( item_creator.account, {proposals_id[1]}, item_creator.key );
        auto found_proposals = calc_proposals( proposal_idx, proposals_id );
        auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
        BOOST_REQUIRE( exist_proposal( proposals_id[1] ) && find_proposal( proposals_id[1] )->removed );
        BOOST_REQUIRE( found_proposals == 3 );
        BOOST_REQUIRE( found_votes == 130 );
    }
    
    /*
     oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
     rr    xx xx xx xx xx xx xx xx xx xx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr
     rr    xx xx xx xx xx xx xx xx xx xx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr
     oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
     P4    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     */
    {
        remove_proposal( item_creator.account, {proposals_id[2]}, item_creator.key );
        auto found_proposals = calc_proposals( proposal_idx, proposals_id );
        auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
        BOOST_REQUIRE( exist_proposal( proposals_id[1] ) && find_proposal( proposals_id[1] )->removed );
        BOOST_REQUIRE( exist_proposal( proposals_id[2] ) && find_proposal( proposals_id[2] )->removed );
        BOOST_REQUIRE( found_proposals == 3 );
        BOOST_REQUIRE( found_votes == 110 );
    }
    
    /*
     oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
     oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
     rr    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo rrr
     oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
     P4    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     */
    {
        generate_block();
        generate_block();
        generate_block();
        
        auto found_proposals = calc_proposals( proposal_idx, proposals_id );
        auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
        BOOST_REQUIRE( !exist_proposal( proposals_id[1] ) );
        BOOST_REQUIRE( exist_proposal( proposals_id[2] ) && find_proposal( proposals_id[2] )->removed );
        BOOST_REQUIRE( found_proposals == 2 );
        BOOST_REQUIRE( found_votes == 51 );
    }
    
    /*
     oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
     oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
     xx    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo xxx
     oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
     P4    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     */
    {
        generate_block();
        
        auto found_proposals = calc_proposals( proposal_idx, proposals_id );
        auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
        BOOST_REQUIRE( !exist_proposal( proposals_id[1] ) );
        BOOST_REQUIRE( !exist_proposal( proposals_id[2] ) );
        BOOST_REQUIRE( found_proposals == 1 );
        BOOST_REQUIRE( found_votes == 50 );
    }
    
    /*
     oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
     oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
     xx    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo xxx
     oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
     P4    xx xx xx xx xx xx xx xx xx xx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     */
    {
        remove_proposal( item_creator.account, {proposals_id[4]}, item_creator.key );
        auto found_proposals = calc_proposals( proposal_idx, proposals_id );
        auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
        BOOST_REQUIRE( exist_proposal( proposals_id[4] ) && find_proposal( proposals_id[4] )->removed );
        BOOST_REQUIRE( found_proposals == 1 );
        BOOST_REQUIRE( found_votes == 30 );
        
        for( auto item : inits )
            vote_proposal( item.account, {proposals_id[4]}, true/*approve*/, item.key);
        
        found_votes = calc_votes( proposal_vote_idx, proposals_id );
        BOOST_REQUIRE( found_votes == 30 );
    }
    
    /*
     oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
     oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
     rr    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo rrr
     oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
     P4    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
     */
    {
        generate_block();
        generate_block();
        
        auto found_proposals = calc_proposals( proposal_idx, proposals_id );
        auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
        BOOST_REQUIRE( !exist_proposal( proposals_id[4] ) );
        BOOST_REQUIRE( found_proposals == 0 );
        BOOST_REQUIRE( found_votes == 0 );
        
        for( auto item : inits )
            vote_proposal( item.account, {proposals_id[4]}, true/*approve*/, item.key);
        
        found_votes = calc_votes( proposal_vote_idx, proposals_id );
        BOOST_REQUIRE( found_votes == 0 );
    }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE( proposal_tests_performance, tps_database_fixture_performance )

int32_t get_time( database& db, const std::string& name )
{
    int32_t benchmark_time = -1;
    
    db.get_benchmark_dumper().dump();
    
    fc::path benchmark_file( "advanced_benchmark.json" );
    
    if( !fc::exists( benchmark_file ) )
        return benchmark_time;
    
    /*
     {
     "total_time": 1421,
     "items": [{
     "op_name": "tps_processor",
     "time": 80
     },...
     */
    auto file = fc::json::from_file( benchmark_file ).as< fc::variant >();
    if( file.is_object() )
    {
        auto vo = file.get_object();
        if( vo.contains( "items" ) )
        {
            auto items = vo[ "items" ];
            if( items.is_array() )
            {
                auto v = items.as< std::vector< fc::variant > >();
                for( auto& item : v )
                {
                    if( !item.is_object() )
                        continue;
                    
                    auto vo_item = item.get_object();
                    if( vo_item.contains( "op_name" ) && vo_item[ "op_name" ].is_string() )
                    {
                        std::string str = vo_item[ "op_name" ].as_string();
                        if( str == name )
                        {
                            if( vo_item.contains( "time" ) && vo_item[ "time" ].is_uint64() )
                            {
                                benchmark_time = vo_item[ "time" ].as_int64();
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    return benchmark_time;
}

struct initial_data
{
    std::string account;
    
    fc::ecc::private_key key;
    
    initial_data( database_fixture* db, const std::string& _account ): account( _account )
    {
        key = db->generate_private_key( account );
        
        db->account_create( account, key.get_public_key(), db->generate_private_key( account + "_post" ).get_public_key() );
    }
};

std::vector< initial_data > generate_accounts( database_fixture* db, int32_t number_accounts )
{
    const std::string basic_name = "tester";
    
    std::vector< initial_data > res;
    
    for( int32_t i = 0; i< number_accounts; ++i  )
    {
        std::string name = basic_name + std::to_string( i );
        res.push_back( initial_data( db, name ) );
        
        if( ( i + 1 ) % 100 == 0 )
            db->generate_block();
        
        if( ( i + 1 ) % 1000 == 0 )
            ilog( "Created: ${accs} accounts",( "accs", i+1 ) );
    }
    
    db->validate_database();
    db->generate_block();
    
    return res;
}

BOOST_AUTO_TEST_CASE( proposals_removing_with_threshold_03 )
{ try {
    BOOST_TEST_MESSAGE( "Testing: removing of all proposals/votes in one block using threshold = -1" );
    
    std::vector< initial_data > inits = generate_accounts( this, 200 );
    FUND( TAIYI_TREASURY_ACCOUNT, ASSET( "5000000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    generate_block();
    std::vector< account_name_type > xinsu_accounts;
    std::for_each(inits.begin(), inits.end(), [&](const initial_data& init){
        xinsu_accounts.push_back(init.account);
    });
    generate_xinsu(xinsu_accounts);
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);

    //=====================preparing=====================
    auto start_time = db->head_block_time();
    
    const auto start_time_shift = fc::hours( 20 );
    const auto end_time_shift = fc::hours( 6 );
    const auto block_interval = fc::seconds( TAIYI_BLOCK_INTERVAL );
    
    auto start_date_00 = start_time + start_time_shift;
    auto end_date_00 = start_date_00 + end_time_shift;
        
    const auto nr_proposals = 200;
    std::vector< int64_t > proposals_id;
    
    struct initial_data
    {
        std::string account;
        fc::ecc::private_key key;
    };
    
    for( auto item : inits )
    {
        vest( TAIYI_INIT_SIMING_NAME, item.account, ASSET( "1000.000 YANG" ) );
    }
    //=====================preparing=====================
    create_proposal_data cpd(db->head_block_time());
    cpd.params.clear();
    for( auto i = 0; i < nr_proposals; ++i )
    {
        auto item = inits[ i ];
        cpd.subject = "hello" + std::to_string(i);
        cpd.params[lua_key(lua_int(1))] = lua_string(item.account);
        proposals_id.push_back(create_proposal(item.account, cpd.contract_name, "transfer_1_to", cpd.params, cpd.subject, end_date_00, item.key));
        if( ( i + 1 ) % 10 == 0 )
            generate_block();
    }
    generate_block();
    
    std::vector< int64_t > ids;
    uint32_t i = 0;
    for( auto id : proposals_id )
    {
        ids.push_back( id );
        if( ids.size() == TAIYI_PROPOSAL_MAX_IDS_NUMBER )
        {
            for( auto item : inits )
            {
                ++i;
                vote_proposal( item.account, ids, true/*approve*/, item.key );
                if( ( i + 1 ) % 10 == 0 )
                    generate_block();
            }
            ids.clear();
        }
    }
    generate_block();
    
    const auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_id >();
    const auto& proposal_vote_idx = db->get_index< proposal_vote_index >().indices().get< by_proposal_voter >();
    
    auto current_active_proposals = nr_proposals;
    BOOST_REQUIRE( calc_proposals( proposal_idx, proposals_id ) == current_active_proposals );
    
    auto current_active_votes = current_active_proposals * static_cast< int16_t > ( inits.size() );
    BOOST_REQUIRE( calc_votes( proposal_vote_idx, proposals_id ) == current_active_votes );
    
    auto threshold = db->get_tps_remove_threshold();
    BOOST_REQUIRE( threshold == -1 );
    
    generate_blocks( start_time + fc::seconds( TAIYI_PROPOSAL_MAINTENANCE_CLEANUP ) );
    start_time = db->head_block_time();
    
    generate_blocks( start_time + ( start_time_shift + end_time_shift - block_interval ) );
    
    generate_blocks( 1 );
    
    BOOST_REQUIRE( calc_proposals( proposal_idx, proposals_id ) == 0 );
    BOOST_REQUIRE( calc_votes( proposal_vote_idx, proposals_id ) == 0 );
    
    int32_t benchmark_time = get_time( *db, tps_processor::get_removing_name() );
    idump( (benchmark_time) );
    BOOST_REQUIRE( benchmark_time == -1 || benchmark_time < 400 );
    
    /*
     Local test: 4 cores/16 MB RAM
     nr objects to be removed: `40200`
     time of removing: 360 ms
     speed of removal: ~111/ms
     */
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( executing_proposals )
{ try {
    BOOST_TEST_MESSAGE( "Testing: ececuting a lot of proposals" );
    
    std::vector< initial_data > inits = generate_accounts( this, 1000 );
    FUND( TAIYI_TREASURY_ACCOUNT, ASSET( "5000000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, TAIYI_TREASURY_ACCOUNT, ASSET( "1000.000 YANG" ) ); //执行提案需要真气
    generate_block();
    std::vector< account_name_type > xinsu_accounts;
    std::for_each(inits.begin(), inits.end(), [&](const initial_data& init) {
        xinsu_accounts.push_back(init.account);
    });
    generate_xinsu(xinsu_accounts);
    create_contract(TAIYI_INIT_SIMING_NAME, "contract.proposal.test", s_code_proposal_test);
    
    //=====================preparing=====================
    const auto nr_proposals = 5;
    std::vector< int64_t > proposals_id;
    flat_map< std::string, asset > before_yangs;
    
    auto call = [&]( uint32_t& i, uint32_t max, const std::string& info )
    {
        ++i;
        if( i % max == 0 )
            generate_block();
        
        if( i % 1000 == 0 )
            ilog( info.c_str(),( "x", i ) );
    };
    
    uint32_t i = 0;
    for( auto item : inits )
    {
        vest( TAIYI_INIT_SIMING_NAME, item.account, ASSET( "30.000 YANG" ) );
        call( i, 50, "${x} accounts got QI" );
    }
    
    auto start_time = db->head_block_time();
    
    const auto start_time_shift = fc::hours( 10 );
    const auto end_time_shift = fc::hours( 1 );
    const auto block_interval = fc::seconds( TAIYI_BLOCK_INTERVAL );
    
    auto start_date = start_time + start_time_shift;
    auto end_date = start_date + end_time_shift;
    
    //=====================preparing=====================
    create_proposal_data cpd(db->head_block_time());
    cpd.params.clear();
    for( int32_t i = 0; i < nr_proposals; ++i )
    {
        auto item = inits[ i % inits.size() ];
        cpd.subject = "hello" + std::to_string(i);
        cpd.params[lua_key(lua_int(1))] = lua_string(item.account);
        proposals_id.push_back(create_proposal(item.account, cpd.contract_name, "transfer_1_to", cpd.params, cpd.subject, end_date, item.key));
        generate_block();
    }
    
    i = 0;
    for( auto item : inits )
    {
        vote_proposal( item.account, proposals_id, true/*approve*/, item.key );
        call( i, 100, "${x} accounts voted" );
    }
    
    for( int32_t i = 0; i < nr_proposals; ++i )
    {
        auto item = inits[ i % inits.size() ];
        const account_object& account = db->get_account( item.account );
        before_yangs[ item.account ] = account.balance;
    }
    
    generate_blocks( start_time + ( start_time_shift - block_interval ) );
    start_time = db->head_block_time();
    generate_blocks( start_time + end_time_shift + block_interval );
    
    auto paid = ASSET( "1.000 YANG" );
    for( int32_t i = 0; i < nr_proposals; ++i )
    {
        auto item = inits[ i % inits.size() ];
        const account_object& account = db->get_account( item.account );
        
        auto after_yang = account.balance;
        auto before_yang = before_yangs[ item.account ];
        BOOST_REQUIRE( before_yang == after_yang - paid );
    }
    
    validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()


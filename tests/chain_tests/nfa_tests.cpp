#include <boost/test/unit_test.hpp>

#include <chain/taiyi_fwd.hpp>

#include <protocol/exceptions.hpp>
#include <protocol/hardfork.hpp>

#include <chain/database.hpp>
#include <chain/database_exceptions.hpp>

#include <chain/taiyi_objects.hpp>
#include <chain/transaction_object.hpp>
#include <chain/account_object.hpp>
#include <chain/contract_objects.hpp>
#include <chain/nfa_objects.hpp>

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

BOOST_FIXTURE_TEST_SUITE( nfa_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( create_nfa_symbol_apply )
{ try {
    BOOST_TEST_MESSAGE( "Testing: create_nfa_symbol_apply" );

    string nfa_init_lua_f = "function get_data() return {} end";
    string nfa_init_lua_ok = "function init_data() return {} end";

    signed_transaction tx;
    ACTORS( (alice)(bob)(charlie) )
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    generate_block();
        
    create_contract_operation op;
    op.owner = "bob";
    op.name = "contract.nfa.fail";
    op.data = nfa_init_lua_f;
    tx.operations.push_back( op );
    
    op.owner = "bob";
    op.name = "contract.nfa.good";
    op.data = nfa_init_lua_ok;
    tx.operations.push_back( op );

    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    sign( tx, bob_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();

    BOOST_TEST_MESSAGE( "--- Test failure when no entrance in contract" );

    create_nfa_symbol_operation cnsop;
    cnsop.creator = "alice";
    cnsop.symbol = "nfa.test";
    cnsop.describe = "test";
    cnsop.default_contract = "contract.nfa.fail";
        
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cnsop );
    sign( tx, alice_private_key );
    BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

    BOOST_TEST_MESSAGE( "--- Test failure when invalid symbol" );

    cnsop.symbol = "nft.test";
    cnsop.default_contract = "contract.nfa.good";
        
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cnsop );
    sign( tx, alice_private_key );
    BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

    BOOST_TEST_MESSAGE( "--- Test create nfa symbol" );

    const auto& contract_base = db->get<contract_object, by_name>("contract.nfa.good");

    cnsop.symbol = "nfa.test";
        
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cnsop );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );
    
    const auto& nfa_symbol = db->get<nfa_symbol_object, by_symbol>("nfa.test");
    BOOST_REQUIRE( nfa_symbol.creator == "alice" );
    BOOST_REQUIRE( nfa_symbol.symbol == "nfa.test" );
    BOOST_REQUIRE( nfa_symbol.describe == "test" );
    BOOST_REQUIRE( nfa_symbol.default_contract == contract_base.id );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( create_nfa_apply )
{ try {
    
    BOOST_TEST_MESSAGE( "Testing: create_nfa_apply" );

    string nfa_init_lua = "function init_data() return {} end";

    signed_transaction tx;
    ACTORS( (alice)(bob)(charlie) )
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "charlie", ASSET( "1000.000 YANG" ) );
    generate_block();
        
    create_contract_operation op;
    op.owner = "bob";
    op.name = "contract.nfa.base";
    op.data = nfa_init_lua;

    tx.operations.push_back( op );
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    sign( tx, bob_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();
    
    create_nfa_symbol_operation cnsop;
    cnsop.creator = "alice";
    cnsop.symbol = "nfa.test";
    cnsop.describe = "test";
    cnsop.default_contract = "contract.nfa.base";
        
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cnsop );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();

    BOOST_TEST_MESSAGE( "--- Test failure create nfa with invalid symbol" );

    create_nfa_operation cnop;
    cnop.creator = "charlie";
    cnop.symbol = "nfa.not.exist";
        
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cnop );
    sign( tx, charlie_private_key );
    BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

    BOOST_TEST_MESSAGE( "--- Test create nfa" );

    cnop.symbol = "nfa.test";
        
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cnop );
    sign( tx, charlie_private_key );
    db->push_transaction( tx, 0 );
    
    const auto& to = db->get<transaction_object, by_trx_id>(tx.id());
    BOOST_REQUIRE( to.operation_results.size() == 1 );
    contract_result result = to.operation_results[0].get<contract_result>();
    BOOST_REQUIRE( result.contract_affecteds.size() == 2 );
    nfa_affected affected = result.contract_affecteds[0].get<nfa_affected>();
    
    BOOST_REQUIRE( affected.affected_account == "charlie" );
    BOOST_REQUIRE( affected.affected_item == 0 );
    BOOST_REQUIRE( affected.action == nfa_affected_type::create_for );

    const auto& nfa = db->get<nfa_object, by_id>(affected.affected_item);
    const auto& nfa_symbol = db->get<nfa_symbol_object, by_symbol>("nfa.test");
    BOOST_REQUIRE( nfa.creator_account == charlie_id );
    BOOST_REQUIRE( nfa.owner_account == charlie_id );
    BOOST_REQUIRE( nfa.symbol_id == nfa_symbol.id );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( transfer_nfa_apply )
{ try {
    
    BOOST_TEST_MESSAGE( "Testing: transfer_nfa_apply" );

    string nfa_init_lua = "function init_data() return {} end";

    signed_transaction tx;
    ACTORS( (alice)(bob)(charlie) )
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "charlie", ASSET( "1000.000 YANG" ) );
    generate_block();
        
    create_contract_operation op;
    op.owner = "bob";
    op.name = "contract.nfa.base";
    op.data = nfa_init_lua;

    tx.operations.push_back( op );
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    sign( tx, bob_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();
    
    create_nfa_symbol_operation cnsop;
    cnsop.creator = "alice";
    cnsop.symbol = "nfa.test";
    cnsop.describe = "test";
    cnsop.default_contract = "contract.nfa.base";
        
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cnsop );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();

    create_nfa_operation cnop;
    cnop.creator = "charlie";
    cnop.symbol = "nfa.test";

    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cnop );
    sign( tx, charlie_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();

    const auto& to1 = db->get<transaction_object, by_trx_id>(tx.id());
    BOOST_REQUIRE( to1.operation_results.size() == 1 );
    contract_result result = to1.operation_results[0].get<contract_result>();
    BOOST_REQUIRE( result.contract_affecteds.size() == 2 );
    nfa_affected affected = result.contract_affecteds[0].get<nfa_affected>();
    
    BOOST_REQUIRE( affected.affected_account == "charlie" );
    BOOST_REQUIRE( affected.affected_item == 0 );
    BOOST_REQUIRE( affected.action == nfa_affected_type::create_for );

    const auto& nfa = db->get<nfa_object, by_id>(affected.affected_item);
    const auto& nfa_symbol = db->get<nfa_symbol_object, by_symbol>("nfa.test");
    BOOST_REQUIRE( nfa.creator_account == charlie_id );
    BOOST_REQUIRE( nfa.owner_account == charlie_id );
    BOOST_REQUIRE( nfa.symbol_id == nfa_symbol.id );
    
    BOOST_TEST_MESSAGE( "--- Test transfer nfa" );

    transfer_nfa_operation tnop;

    tnop.from = "charlie";
    tnop.to = "alice";
    tnop.id = nfa.id;
        
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( tnop );
    sign( tx, charlie_private_key );
    db->push_transaction( tx, 0 );

    const auto& to2 = db->get<transaction_object, by_trx_id>(tx.id());
    BOOST_REQUIRE( to2.operation_results.size() == 1 );
    result = to2.operation_results[0].get<contract_result>();
    BOOST_REQUIRE( result.contract_affecteds.size() == 2 );
    
    affected = result.contract_affecteds[0].get<nfa_affected>();
    BOOST_REQUIRE( affected.affected_account == "charlie" );
    BOOST_REQUIRE( affected.affected_item == nfa.id._id );
    BOOST_REQUIRE( affected.action == nfa_affected_type::transfer_from );
    
    affected = result.contract_affecteds[1].get<nfa_affected>();
    BOOST_REQUIRE( affected.affected_account == "alice" );
    BOOST_REQUIRE( affected.affected_item == nfa.id._id );
    BOOST_REQUIRE( affected.action == nfa_affected_type::transfer_to );
    
    BOOST_REQUIRE( nfa.creator_account == charlie_id );
    BOOST_REQUIRE( nfa.owner_account == alice_id );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( action_nfa_apply )
{ try {
    
    BOOST_TEST_MESSAGE( "Testing: action_nfa_apply" );

    string nfa_code_lua = " hello_nfa = { consequence = true }  \n \
                            function init_data() return {} end  \n \
                            function do_hello_nfa()             \n \
                                contract_helper:log('hello nfa')\n \
                            end";

    signed_transaction tx;
    ACTORS( (alice)(bob)(charlie) )
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "charlie", ASSET( "1000.000 YANG" ) );
    generate_block();
        
    create_contract_operation op;
    op.owner = "bob";
    op.name = "contract.nfa.base";
    op.data = nfa_code_lua;

    tx.operations.push_back( op );
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    sign( tx, bob_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();
    
    create_nfa_symbol_operation cnsop;
    cnsop.creator = "alice";
    cnsop.symbol = "nfa.test";
    cnsop.describe = "test";
    cnsop.default_contract = "contract.nfa.base";
        
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cnsop );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();

    create_nfa_operation cnop;
    cnop.creator = "charlie";
    cnop.symbol = "nfa.test";

    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cnop );
    sign( tx, charlie_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();

    const auto& to1 = db->get<transaction_object, by_trx_id>(tx.id());
    BOOST_REQUIRE( to1.operation_results.size() == 1 );
    contract_result cresult = to1.operation_results[0].get<contract_result>();
    BOOST_REQUIRE( cresult.contract_affecteds.size() == 2 );
    nfa_affected affected = cresult.contract_affecteds[0].get<nfa_affected>();
    
    BOOST_REQUIRE( affected.affected_account == "charlie" );
    BOOST_REQUIRE( affected.affected_item == 0 );
    BOOST_REQUIRE( affected.action == nfa_affected_type::create_for );

    const auto& nfa = db->get<nfa_object, by_id>(affected.affected_item);
    const auto& nfa_symbol = db->get<nfa_symbol_object, by_symbol>("nfa.test");
    BOOST_REQUIRE( nfa.creator_account == charlie_id );
    BOOST_REQUIRE( nfa.owner_account == charlie_id );
    BOOST_REQUIRE( nfa.symbol_id == nfa_symbol.id );
    
    generate_blocks(10); //给奖励基金充值

    BOOST_TEST_MESSAGE( "--- Test action nfa" );
    
    action_nfa_operation anop;

    anop.owner = "charlie";
    anop.id = nfa.id;
    anop.action = "hello_nfa";

    db->modify( db->get_account( "charlie" ), [&]( account_object& a ) {
        a.manabar.current_mana = util::get_effective_qi(a);
        a.manabar.last_update_time = db->head_block_time().sec_since_epoch();
    });

    auto old_manabar = db->get_account( "charlie" ).manabar;
    auto old_reward_qi = db->get_account("bob").reward_qi_balance;

    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( anop );
    sign( tx, charlie_private_key );
    db->push_transaction( tx, 0 );

    const auto& to2 = db->get<transaction_object, by_trx_id>(tx.id());
    BOOST_REQUIRE( to2.operation_results.size() == 1 );
    cresult = to2.operation_results[0].get<contract_result>();
    vector<string> sresult;
    for(auto& temp : cresult.contract_affecteds) {
        if(temp.which() == contract_affected_type::tag<contract_logger>::value) {
            sresult.push_back(temp.get<contract_logger>().message);
        }
    }
    
    //idump( (sresult) );
    BOOST_REQUIRE( sresult.size() == 1 );
    BOOST_REQUIRE( sresult[0] == "hello nfa" );
    
    int64_t used_mana = old_manabar.current_mana - db->get_account( "charlie" ).manabar.current_mana;
    idump( (used_mana) );
    BOOST_REQUIRE( used_mana == 448 );

    asset reward_qi = db->get_account("bob").reward_qi_balance - old_reward_qi;
    //idump( (reward_qi) );
    BOOST_REQUIRE( reward_qi == asset(448, QI_SYMBOL) );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( action_drops )
{ try {
    
    BOOST_TEST_MESSAGE( "Testing: heart_beat" );

    string nfa_code_lua = " heart_beat = { consequence = true } \n \
                            active = { consequence = true }     \n \
                                                                \n \
                            function init_data() return {} end  \n \
                                                                \n \
                            function do_active()                \n \
                                nfa_helper:enable_tick()        \n \
                            end                                 \n \
                            function do_heart_beat()            \n \
                                contract_helper:log('beat')     \n \
                            end";

    signed_transaction tx;
    ACTORS( (alice)(bob)(charlie) )
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "charlie", ASSET( "1000.000 YANG" ) );
    generate_block();
        
    create_contract_operation op;
    op.owner = "bob";
    op.name = "contract.nfa.base";
    op.data = nfa_code_lua;

    tx.operations.push_back( op );
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    sign( tx, bob_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();
    
    create_nfa_symbol_operation cnsop;
    cnsop.creator = "alice";
    cnsop.symbol = "nfa.test";
    cnsop.describe = "test";
    cnsop.default_contract = "contract.nfa.base";
        
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cnsop );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();

    create_nfa_operation cnop;
    cnop.creator = "charlie";
    cnop.symbol = "nfa.test";

    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cnop );
    sign( tx, charlie_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();

    const auto& to1 = db->get<transaction_object, by_trx_id>(tx.id());
    BOOST_REQUIRE( to1.operation_results.size() == 1 );
    contract_result cresult = to1.operation_results[0].get<contract_result>();
    BOOST_REQUIRE( cresult.contract_affecteds.size() == 2 );
    nfa_affected affected = cresult.contract_affecteds[0].get<nfa_affected>();
    
    BOOST_REQUIRE( affected.affected_account == "charlie" );
    BOOST_REQUIRE( affected.affected_item == 0 );
    BOOST_REQUIRE( affected.action == nfa_affected_type::create_for );

    const auto& nfa = db->get<nfa_object, by_id>(affected.affected_item);
    const auto& nfa_symbol = db->get<nfa_symbol_object, by_symbol>("nfa.test");
    BOOST_REQUIRE( nfa.creator_account == charlie_id );
    BOOST_REQUIRE( nfa.owner_account == charlie_id );
    BOOST_REQUIRE( nfa.symbol_id == nfa_symbol.id );
    
    db_plugin->debug_update( [=]( database& db ) {
        db.modify( db.get_account( "charlie" ), [&]( account_object& a ) {
            a.manabar.current_mana = util::get_effective_qi(a);
            a.manabar.last_update_time = db.head_block_time().sec_since_epoch();
        });
    });

    generate_blocks(10); //给奖励基金充值

    BOOST_TEST_MESSAGE( "--- Test active action drops" );
    
    int64_t used_mana;
    auto old_manabar = db->get_account( "charlie" ).manabar;

    action_nfa_operation anop;

    anop.owner = "charlie";
    anop.id = nfa.id;

//    anop.action = "active";
//
//    tx.operations.clear();
//    tx.signatures.clear();
//    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
//    tx.operations.push_back( anop );
//    sign( tx, charlie_private_key );
//    db->push_transaction( tx, 0 );
//
//    BOOST_REQUIRE( nfa.next_tick_time == time_point_sec::min() );
//
//    used_mana = old_manabar.current_mana - db->get_account( "charlie" ).manabar.current_mana;
//    idump( (used_mana) );
//    BOOST_REQUIRE( used_mana == 525 );
    

    anop.action = "heart_beat";

    db->modify( db->get_account( "charlie" ), [&]( account_object& a ) {
        a.manabar.current_mana = util::get_effective_qi(a);
        a.manabar.last_update_time = db->head_block_time().sec_since_epoch();
    });
    old_manabar = db->get_account( "charlie" ).manabar;

    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( anop );
    sign( tx, charlie_private_key );
    db->push_transaction( tx, 0 );

    used_mana = old_manabar.current_mana - db->get_account( "charlie" ).manabar.current_mana;
    idump( (used_mana) );
    BOOST_REQUIRE( used_mana == 472 );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( heart_beat )
{ try {
    
    BOOST_TEST_MESSAGE( "Testing: heart_beat" );

    string nfa_code_lua = " heart_beat = { consequence = true } \n \
                            active = { consequence = true }     \n \
                                                                \n \
                            function init_data() return {} end  \n \
                                                                \n \
                            function do_active()                \n \
                                nfa_helper:enable_tick()        \n \
                            end                                 \n \
                            function do_heart_beat()            \n \
                                contract_helper:log('beat')     \n \
                            end";

    signed_transaction tx;
    ACTORS( (alice)(bob)(charlie) )
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "charlie", ASSET( "1000.000 YANG" ) );
    generate_block();
        
    create_contract_operation op;
    op.owner = "bob";
    op.name = "contract.nfa.base";
    op.data = nfa_code_lua;

    tx.operations.push_back( op );
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    sign( tx, bob_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();
    
    create_nfa_symbol_operation cnsop;
    cnsop.creator = "alice";
    cnsop.symbol = "nfa.test";
    cnsop.describe = "test";
    cnsop.default_contract = "contract.nfa.base";
        
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cnsop );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();

    create_nfa_operation cnop;
    cnop.creator = "charlie";
    cnop.symbol = "nfa.test";

    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cnop );
    sign( tx, charlie_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();

    const auto& to1 = db->get<transaction_object, by_trx_id>(tx.id());
    BOOST_REQUIRE( to1.operation_results.size() == 1 );
    contract_result cresult = to1.operation_results[0].get<contract_result>();
    BOOST_REQUIRE( cresult.contract_affecteds.size() == 2 );
    nfa_affected affected = cresult.contract_affecteds[0].get<nfa_affected>();
    
    BOOST_REQUIRE( affected.affected_account == "charlie" );
    BOOST_REQUIRE( affected.affected_item == 0 );
    BOOST_REQUIRE( affected.action == nfa_affected_type::create_for );

    const auto& nfa = db->get<nfa_object, by_id>(affected.affected_item);
    const auto& nfa_symbol = db->get<nfa_symbol_object, by_symbol>("nfa.test");
    BOOST_REQUIRE( nfa.creator_account == charlie_id );
    BOOST_REQUIRE( nfa.owner_account == charlie_id );
    BOOST_REQUIRE( nfa.symbol_id == nfa_symbol.id );
    
    //给nfa充气
    db_plugin->debug_update( [=]( database& db ) {
        db.modify( db.get_account( "charlie" ), [&]( account_object& a ) {
            a.manabar.current_mana = util::get_effective_qi(a);
            a.manabar.last_update_time = db.head_block_time().sec_since_epoch();
        });
        
        db.modify( nfa, [&](nfa_object& obj) {
            obj.qi += asset(10, QI_SYMBOL);
            util::update_manabar( db.get_dynamic_global_properties(), obj, true );
        });
    });
    generate_block();

    BOOST_REQUIRE( nfa.manabar.current_mana == 10 );

    BOOST_TEST_MESSAGE( "--- Test enable tick nfa" );
    
    action_nfa_operation anop;
    anop.owner = "charlie";
    anop.id = nfa.id;
    anop.action = "active";

    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( anop );
    sign( tx, charlie_private_key );
    db->push_transaction( tx, 0 );
    
    BOOST_REQUIRE( nfa.next_tick_time == time_point_sec::min() );

    BOOST_TEST_MESSAGE( "--- Test heat beat failure when nfa has not enough qi" );

    generate_block();
    
    BOOST_REQUIRE_EQUAL( nfa.manabar.current_mana, 0 );
    BOOST_REQUIRE( nfa.next_tick_time == time_point_sec::maximum() );

    BOOST_TEST_MESSAGE( "--- Test heat beat nfa" );

    //给nfa充足量真气
    asset old_nfa_qi = nfa.qi;
    
    db_plugin->debug_update( [=]( database& db ) {
        db.modify( nfa, [&](nfa_object& obj) {
            obj.qi += asset(5000000, QI_SYMBOL);
            util::update_manabar( db.get_dynamic_global_properties(), obj, true );
        });
    });
    generate_block();

    BOOST_REQUIRE( nfa.qi == old_nfa_qi + asset(5000000, QI_SYMBOL) );

    int64_t used_mana = 0;
    for(int k = 0; k < 10; k ++) {
        BOOST_TEST_MESSAGE( FORMAT_MESSAGE("----- same heat beat ${i}", ("i", k)));

        //立即恢复力量并激活
        db_plugin->debug_update( [=]( database& db ) {
            db.modify( nfa, [&]( nfa_object& obj ) {
                obj.manabar.current_mana = util::get_effective_qi(obj);
                obj.manabar.last_update_time = db.head_block_time().sec_since_epoch();
                obj.next_tick_time = time_point_sec::min();
            });
        });
        BOOST_REQUIRE( nfa.manabar.current_mana == (5000000 + 10) );
        
        int64_t old_mana = nfa.manabar.current_mana;
#if 0
        //再次激活
        tx.operations.clear();
        tx.signatures.clear();
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.operations.push_back( anop );
        sign( tx, charlie_private_key );
        db->push_transaction( tx, 0 );
        
        BOOST_REQUIRE( nfa.next_tick_time == time_point_sec::min() );
#endif
                
        if(k == 0) {
            generate_block();

            used_mana = old_mana - nfa.manabar.current_mana;
            idump( (old_mana)(nfa.manabar.current_mana)(used_mana) );
            BOOST_CHECK_EQUAL( used_mana, 472);
        }
        else {
            generate_block();

            idump( (old_mana)(nfa.manabar.current_mana) );
            BOOST_CHECK_EQUAL( nfa.manabar.current_mana, old_mana - used_mana );
            BOOST_REQUIRE( nfa.next_tick_time == (db->head_block_time() + TAIYI_NFA_TICK_PERIOD_MAX_BLOCK_NUM * TAIYI_BLOCK_INTERVAL) );
        }
    }
    
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()

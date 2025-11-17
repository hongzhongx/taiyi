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

const string s_code_nfa_basic = " \
    function create_nfa_symbol(symbol, describe, contract, max_count, min_equivalent_qi) \
        contract_helper:create_nfa_symbol(symbol, describe, contract, max_count, min_equivalent_qi) \
    end                           \
    function create_nfa_to_me(symbol) \
        contract_helper:create_nfa_to_account(contract_base_info.caller, symbol, {}, true) \
    end                           \
";

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
    op.name = "contract.nfa.basic";
    op.data = s_code_nfa_basic;
    tx.operations.push_back( op );
    
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

    call_contract_function_operation cop;
    cop.caller = "alice";
    cop.contract_name = "contract.nfa.basic";
    cop.function_name = "create_nfa_symbol";
    cop.value_list = {
        lua_string("nfa.test"),
        lua_string("test"),
        lua_string("contract.nfa.fail"),
        lua_int(100),
        lua_int(0)
    };

    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cop );
    sign( tx, alice_private_key );
    BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

    BOOST_TEST_MESSAGE( "--- Test failure when invalid symbol" );

    cop.value_list[0] = lua_string("nft.test");
    cop.value_list[2] = lua_string("contract.nfa.good");
        
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cop );
    sign( tx, alice_private_key );
    BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

    BOOST_TEST_MESSAGE( "--- Test create nfa symbol" );

    const auto& contract_base = db->get<contract_object, by_name>("contract.nfa.good");

    cop.value_list[0] = lua_string("nfa.test");
        
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cop );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );
    
    const auto& nfa_symbol = db->get<nfa_symbol_object, by_symbol>("nfa.test");
    const auto& aac = db->get<account_object, by_name>("alice");
    BOOST_REQUIRE( nfa_symbol.creator_account == aac.id );
    BOOST_REQUIRE( nfa_symbol.authority_account == aac.id );
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
    op.name = "contract.nfa.basic";
    op.data = s_code_nfa_basic;
    tx.operations.push_back( op );

    op.owner = "bob";
    op.name = "contract.nfa.base";
    op.data = nfa_init_lua;
    tx.operations.push_back( op );
    
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    sign( tx, bob_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();
    
    call_contract_function_operation cop;
    cop.caller = "alice";
    cop.contract_name = "contract.nfa.basic";
    cop.function_name = "create_nfa_symbol";
    cop.value_list = {
        lua_string("nfa.test"),
        lua_string("test"),
        lua_string("contract.nfa.base"),
        lua_int(10),
        lua_int(0)
    };
        
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cop );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();

    BOOST_TEST_MESSAGE( "--- Test failure create nfa with invalid symbol" );

    cop.caller = "charlie";
    cop.contract_name = "contract.nfa.basic";
    cop.function_name = "create_nfa_to_me";
    cop.value_list = {
        lua_string("nfa.not.exist")
    };

    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cop );
    sign( tx, charlie_private_key );
    BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

    BOOST_TEST_MESSAGE( "--- Test failure create nfa from account without authority" );

    cop.value_list[0] = lua_string("nfa.test");
        
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cop );
    sign( tx, charlie_private_key );
    BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

    BOOST_TEST_MESSAGE( "--- Test create nfa" );

    cop.caller = "alice";
        
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cop );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );
    
    const auto& to = db->get<transaction_object, by_trx_id>(tx.id());
    BOOST_REQUIRE( to.operation_results.size() == 1 );
    contract_result result = to.operation_results[0].get<contract_result>();
    BOOST_REQUIRE( result.contract_affecteds.size() == 2 );
    nfa_affected affected = result.contract_affecteds[0].get<nfa_affected>();
    
    BOOST_REQUIRE( affected.affected_account == "alice" );
    BOOST_REQUIRE( affected.affected_item == 0 );
    BOOST_REQUIRE( affected.action == nfa_affected_type::create_for );

    const auto& nfa = db->get<nfa_object, by_id>(affected.affected_item);
    const auto& nfa_symbol = db->get<nfa_symbol_object, by_symbol>("nfa.test");
    const auto& acc = db->get<account_object, by_name>("alice");
    BOOST_REQUIRE( nfa.creator_account == acc.id );
    BOOST_REQUIRE( nfa.owner_account == acc.id );
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
    op.name = "contract.nfa.basic";
    op.data = s_code_nfa_basic;
    tx.operations.push_back( op );

    op.owner = "bob";
    op.name = "contract.nfa.base";
    op.data = nfa_init_lua;
    tx.operations.push_back( op );
    
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    sign( tx, bob_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();
    
    call_contract_function_operation cop;
    cop.caller = "alice";
    cop.contract_name = "contract.nfa.basic";
    cop.function_name = "create_nfa_symbol";
    cop.value_list = {
        lua_string("nfa.test"),
        lua_string("test"),
        lua_string("contract.nfa.base"),
        lua_int(100),
        lua_int(0)
    };
        
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cop );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();

    cop.caller = "alice";
    cop.contract_name = "contract.nfa.basic";
    cop.function_name = "create_nfa_to_me";
    cop.value_list = {
        lua_string("nfa.test")
    };

    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cop );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();

    const auto& to1 = db->get<transaction_object, by_trx_id>(tx.id());
    BOOST_REQUIRE( to1.operation_results.size() == 1 );
    contract_result result = to1.operation_results[0].get<contract_result>();
    BOOST_REQUIRE( result.contract_affecteds.size() == 2 );
    nfa_affected affected = result.contract_affecteds[0].get<nfa_affected>();
    
    BOOST_REQUIRE( affected.affected_account == "alice" );
    BOOST_REQUIRE( affected.affected_item == 0 );
    BOOST_REQUIRE( affected.action == nfa_affected_type::create_for );

    const auto& nfa = db->get<nfa_object, by_id>(affected.affected_item);
    const auto& nfa_symbol = db->get<nfa_symbol_object, by_symbol>("nfa.test");
    const auto& ida = db->get<account_object, by_name>("alice").id;
    BOOST_REQUIRE( nfa.creator_account == ida );
    BOOST_REQUIRE( nfa.owner_account == ida );
    BOOST_REQUIRE( nfa.symbol_id == nfa_symbol.id );
    
    BOOST_TEST_MESSAGE( "--- Test transfer nfa" );

    transfer_nfa_operation tnop;

    tnop.from = "alice";
    tnop.to = "charlie";
    tnop.id = nfa.id;
        
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( tnop );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );

    const auto& to2 = db->get<transaction_object, by_trx_id>(tx.id());
    BOOST_REQUIRE( to2.operation_results.size() == 1 );
    result = to2.operation_results[0].get<contract_result>();
    BOOST_REQUIRE( result.contract_affecteds.size() == 2 );
    
    affected = result.contract_affecteds[0].get<nfa_affected>();
    BOOST_REQUIRE( affected.affected_account == "alice" );
    BOOST_REQUIRE( affected.affected_item == nfa.id._id );
    BOOST_REQUIRE( affected.action == nfa_affected_type::transfer_from );
    
    affected = result.contract_affecteds[1].get<nfa_affected>();
    BOOST_REQUIRE( affected.affected_account == "charlie" );
    BOOST_REQUIRE( affected.affected_item == nfa.id._id );
    BOOST_REQUIRE( affected.action == nfa_affected_type::transfer_to );
    
    const auto& id1 = db->get<account_object, by_name>("alice").id;
    BOOST_REQUIRE( nfa.creator_account == id1 );
    const auto& id2 = db->get<account_object, by_name>("charlie").id;
    BOOST_REQUIRE( nfa.owner_account == id2 );

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
    
    auto init_mana = db->get_account( "charlie" ).qi;
        
    create_contract_operation op;

    op.owner = "bob";
    op.name = "contract.nfa.basic";
    op.data = s_code_nfa_basic;
    tx.operations.push_back( op );

    op.owner = "bob";
    op.name = "contract.nfa.base";
    op.data = nfa_code_lua;
    tx.operations.push_back( op );
    
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    sign( tx, bob_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();
    
    call_contract_function_operation cop;
    cop.caller = "alice";
    cop.contract_name = "contract.nfa.basic";
    cop.function_name = "create_nfa_symbol";
    cop.value_list = {
        lua_string("nfa.test"),
        lua_string("test"),
        lua_string("contract.nfa.base"),
        lua_int(100),
        lua_int(0)
    };
        
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cop );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();

    cop.caller = "alice";
    cop.contract_name = "contract.nfa.basic";
    cop.function_name = "create_nfa_to_me";
    cop.value_list = {
        lua_string("nfa.test")
    };

    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cop );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();

    const auto& to1 = db->get<transaction_object, by_trx_id>(tx.id());
    BOOST_REQUIRE( to1.operation_results.size() == 1 );
    contract_result cresult = to1.operation_results[0].get<contract_result>();
    BOOST_REQUIRE( cresult.contract_affecteds.size() == 2 );
    nfa_affected affected = cresult.contract_affecteds[0].get<nfa_affected>();
    
    BOOST_REQUIRE( affected.affected_account == "alice" );
    BOOST_REQUIRE( affected.affected_item == 0 );
    BOOST_REQUIRE( affected.action == nfa_affected_type::create_for );

    const auto& nfa = db->get<nfa_object, by_id>(affected.affected_item);
    const auto& nfa_symbol = db->get<nfa_symbol_object, by_symbol>("nfa.test");
    const auto& id1 = db->get<account_object, by_name>("alice").id;
    BOOST_REQUIRE( nfa.creator_account == id1 );
    BOOST_REQUIRE( nfa.owner_account == id1 );
    BOOST_REQUIRE( nfa.symbol_id == nfa_symbol.id );
    
    generate_blocks(10); //给奖励基金充值

    BOOST_TEST_MESSAGE( "--- Test action nfa" );
    
    action_nfa_operation anop;

    anop.caller = "alice";
    anop.id = nfa.id;
    anop.action = "hello_nfa";

    db->modify( db->get_account( "alice" ), [&]( account_object& a ) {
        a.qi = init_mana;
    });

    auto old_mana = db->get_account( "alice" ).qi;
    auto old_reward_feigang = db->get_account("bob").reward_feigang_balance;

    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( anop );
    sign( tx, alice_private_key );
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
    
    int64_t used_mana = old_mana.amount.value - db->get_account( "alice" ).qi.amount.value;
    idump( (used_mana) );
    BOOST_REQUIRE( used_mana == 442 );

    asset reward_feigang = db->get_account("bob").reward_feigang_balance - old_reward_feigang;
    idump( (reward_feigang) );
    BOOST_REQUIRE( reward_feigang.amount == 313 );

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

    auto init_mana = db->get_account( "charlie" ).qi;

    create_contract_operation op;

    op.owner = "bob";
    op.name = "contract.nfa.basic";
    op.data = s_code_nfa_basic;
    tx.operations.push_back( op );

    op.owner = "bob";
    op.name = "contract.nfa.base";
    op.data = nfa_code_lua;
    tx.operations.push_back( op );
    
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    sign( tx, bob_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();
    
    call_contract_function_operation cop;
    cop.caller = "alice";
    cop.contract_name = "contract.nfa.basic";
    cop.function_name = "create_nfa_symbol";
    cop.value_list = {
        lua_string("nfa.test"),
        lua_string("test"),
        lua_string("contract.nfa.base"),
        lua_int(3),
        lua_int(0)
    };
                
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cop );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();

    cop.caller = "alice";
    cop.contract_name = "contract.nfa.basic";
    cop.function_name = "create_nfa_to_me";
    cop.value_list = {
        lua_string("nfa.test")
    };

    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cop );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();

    const auto& to1 = db->get<transaction_object, by_trx_id>(tx.id());
    BOOST_REQUIRE( to1.operation_results.size() == 1 );
    contract_result cresult = to1.operation_results[0].get<contract_result>();
    BOOST_REQUIRE( cresult.contract_affecteds.size() == 2 );
    nfa_affected affected = cresult.contract_affecteds[0].get<nfa_affected>();
    
    BOOST_REQUIRE( affected.affected_account == "alice" );
    BOOST_REQUIRE( affected.affected_item == 0 );
    BOOST_REQUIRE( affected.action == nfa_affected_type::create_for );

    const auto& nfa = db->get<nfa_object, by_id>(affected.affected_item);
    const auto& nfa_symbol = db->get<nfa_symbol_object, by_symbol>("nfa.test");
    const auto& id1 = db->get<account_object, by_name>("alice").id;
    BOOST_REQUIRE( nfa.creator_account == id1 );
    BOOST_REQUIRE( nfa.owner_account == id1 );
    BOOST_REQUIRE( nfa.symbol_id == nfa_symbol.id );
    
    generate_blocks(10); //给奖励基金充值

    BOOST_TEST_MESSAGE( "--- Test active action drops" );
    
    int64_t used_mana;
    auto old_mana = db->get_account( "charlie" ).qi;

    action_nfa_operation anop;

    anop.caller = "alice";
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
//    BOOST_REQUIRE( nfa.next_tick_block == 0 );
//
//    used_mana = old_mana.amount - db->get_account( "charlie" ).qi.amount;
//    idump( (used_mana) );
//    BOOST_REQUIRE( used_mana == 525 );
    

    anop.action = "heart_beat";

    old_mana = db->get_account( "alice" ).qi;

    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( anop );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );

    used_mana = old_mana.amount.value - db->get_account( "alice" ).qi.amount.value;
    idump( (used_mana) );
    BOOST_REQUIRE( used_mana == 466 );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( heart_beat )
{ try {
    
    BOOST_TEST_MESSAGE( "Testing: heart_beat" );

    string nfa_code_lua = " active = { consequence = true }     \n \
                                                                \n \
                            function init_data() return {} end  \n \
                                                                \n \
                            function do_active()                \n \
                                nfa_helper:enable_tick()        \n \
                            end                                 \n \
                            function on_heart_beat()            \n \
                                contract_helper:log('beat')     \n \
                            end";

    signed_transaction tx;
    ACTORS( (alice)(bob)(charlie) )
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "charlie", ASSET( "1000.000 YANG" ) );
    generate_block();

    auto init_mana = db->get_account( "charlie" ).qi;

    create_contract_operation op;

    op.owner = "bob";
    op.name = "contract.nfa.basic";
    op.data = s_code_nfa_basic;
    tx.operations.push_back( op );

    op.owner = "bob";
    op.name = "contract.nfa.base";
    op.data = nfa_code_lua;
    tx.operations.push_back( op );

    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    sign( tx, bob_private_key );
    db->push_transaction( tx, 0 );
    validate_database();

    generate_block();

    call_contract_function_operation cop;
    cop.caller = "alice";
    cop.contract_name = "contract.nfa.basic";
    cop.function_name = "create_nfa_symbol";
    cop.value_list = {
        lua_string("nfa.test"),
        lua_string("test"),
        lua_string("contract.nfa.base"),
        lua_int(3),
        lua_int(0)
    };
                
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cop );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );
    validate_database();

    generate_block();

    cop.caller = "alice";
    cop.contract_name = "contract.nfa.basic";
    cop.function_name = "create_nfa_to_me";
    cop.value_list = {
        lua_string("nfa.test")
    };

    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cop );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );
    validate_database();
    
    generate_block();

    const auto& to1 = db->get<transaction_object, by_trx_id>(tx.id());
    BOOST_REQUIRE( to1.operation_results.size() == 1 );
    contract_result cresult = to1.operation_results[0].get<contract_result>();
    BOOST_REQUIRE( cresult.contract_affecteds.size() == 2 );
    nfa_affected affected = cresult.contract_affecteds[0].get<nfa_affected>();
    
    BOOST_REQUIRE( affected.affected_account == "alice" );
    BOOST_REQUIRE( affected.affected_item == 0 );
    BOOST_REQUIRE( affected.action == nfa_affected_type::create_for );

    const auto* nfa = db->find<nfa_object, by_id>(affected.affected_item);
    const auto& nfa_symbol = db->get<nfa_symbol_object, by_symbol>("nfa.test");
    const auto& id1 = db->get<account_object, by_name>("alice").id;
    BOOST_REQUIRE( nfa->creator_account == id1 );
    BOOST_REQUIRE( nfa->owner_account == id1 );
    BOOST_REQUIRE( nfa->symbol_id == nfa_symbol.id );
    
    auto old_nfa_qi = nfa->qi;

    //给nfa充气
    db_plugin->debug_update( [&]( database& db ) {
        //注意之前的nfa指向的对象有可能被db释放重建，因此不可保持
        nfa = db.find<nfa_object, by_id>(affected.affected_item);
        db.modify( *nfa, [&](nfa_object& obj) {
            obj.qi += asset(10, QI_SYMBOL);
        });
        db.modify( db.get_dynamic_global_properties(), [&](dynamic_global_property_object& obj) {
            obj.total_qi += asset(10, QI_SYMBOL);
        });
    });

    generate_block();

    //注意之前的nfa指向的对象有可能被db释放重建，因此不可保持
    nfa = db->find<nfa_object, by_id>(affected.affected_item);

    BOOST_REQUIRE_EQUAL(nfa->qi.amount.value, old_nfa_qi.amount.value + 10 );

    BOOST_TEST_MESSAGE( "--- Test enable tick nfa" );
    
    action_nfa_operation anop;
    anop.caller = "alice";
    anop.id = nfa->id;
    anop.action = "active";

    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( anop );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );
    
    nfa = db->find<nfa_object, by_id>(affected.affected_item);
    BOOST_REQUIRE( nfa->next_tick_block == 0 );

    BOOST_TEST_MESSAGE( "--- Test heat beat failure when nfa has not enough qi" );

    generate_block();
    
    BOOST_REQUIRE_EQUAL( nfa->qi.amount.value, 0 );
    BOOST_REQUIRE_EQUAL( nfa->debt_value, 58 );
    BOOST_REQUIRE( nfa->next_tick_block == std::numeric_limits<uint32_t>::max() );

    BOOST_TEST_MESSAGE( "--- Test heat beat nfa" );

    //给nfa充足量真气
    old_nfa_qi = nfa->qi;
    
    db_plugin->debug_update( [&]( database& db ) {
        db.modify( *nfa, [&](nfa_object& obj) {
            obj.qi += asset(5000000, QI_SYMBOL);
            obj.next_tick_block = 0; //激活
        });
        db.modify( db.get_dynamic_global_properties(), [&](dynamic_global_property_object& obj) {
            obj.current_supply += asset(5000, YANG_SYMBOL);
            obj.total_qi += asset(5000000, QI_SYMBOL);
        });
    });
    generate_block(); //beat且欠费在这里还

    nfa = db->find<nfa_object, by_id>(affected.affected_item);
    BOOST_REQUIRE_EQUAL(nfa->qi.amount.value, old_nfa_qi.amount.value + 5000000 - 58 - 458 ); //扣除欠费和运行费用

    int64_t used_mana = 0;
    for(int k = 0; k < 10; k ++) {
        BOOST_TEST_MESSAGE( FORMAT_MESSAGE("----- same heat beat ${i}", ("i", k)));

        nfa = db->find<nfa_object, by_id>(affected.affected_item);
        int64_t old_mana = nfa->qi.amount.value;
                
        //激活
        generate_block(); //REVIEW: 如果没有这句，后面debug_updata不会进入
        db_plugin->debug_update( [&]( database& db ) {
            db.modify( *nfa, [&]( nfa_object& obj ) {
                obj.next_tick_block = 0;
            });
        });
        generate_block(); //beat

        if(k == 0) {
            used_mana = old_mana - nfa->qi.amount.value;
            idump( (old_mana)(nfa->qi.amount)(used_mana) );
            BOOST_CHECK_EQUAL( used_mana, 458);
        }
        else {
            idump( (old_mana)(nfa->qi.amount) );
            BOOST_CHECK_EQUAL( nfa->qi.amount.value, old_mana - used_mana );
            BOOST_REQUIRE( nfa->next_tick_block == (db->head_block_num() + TAIYI_NFA_TICK_PERIOD_MAX_BLOCK_NUM) );
        }
    }

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()

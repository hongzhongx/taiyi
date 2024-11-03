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

BOOST_AUTO_TEST_SUITE_END()

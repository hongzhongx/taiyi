#include <boost/test/unit_test.hpp>

#include <chain/taiyi_fwd.hpp>

#include <protocol/exceptions.hpp>
#include <protocol/hardfork.hpp>

#include <chain/database.hpp>
#include <chain/database_exceptions.hpp>

#include <chain/taiyi_objects.hpp>
#include <chain/siming_objects.hpp>
#include <chain/account_object.hpp>

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

BOOST_FIXTURE_TEST_SUITE( operation_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( account_create_validate )
{
    try
    {
        
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_create_authorities )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: account_create_authorities" );
        
        account_create_operation op;
        op.creator = "alice";
        op.new_account_name = "bob";
        
        flat_set< account_name_type > auths;
        flat_set< account_name_type > expected;
        
        BOOST_TEST_MESSAGE( "--- Testing owner authority" );
        op.get_required_owner_authorities( auths );
        BOOST_REQUIRE( auths == expected );
        
        BOOST_TEST_MESSAGE( "--- Testing active authority" );
        expected.insert( "alice" );
        op.get_required_active_authorities( auths );
        BOOST_REQUIRE( auths == expected );
        
        BOOST_TEST_MESSAGE( "--- Testing posting authority" );
        expected.clear();
        auths.clear();
        op.get_required_posting_authorities( auths );
        BOOST_REQUIRE( auths == expected );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_create_apply )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: account_create_apply" );
        
        db_plugin->debug_update( [=]( database& db ) {
            db.modify( db.get_siming_schedule_object(), [&]( siming_schedule_object& wso ) {
                wso.median_props.account_creation_fee = ASSET( "0.100 YANG" );
            });
        });
        generate_block();
        
        signed_transaction tx;
        private_key_type priv_key = generate_private_key( "alice" );
        
        const account_object& init = db->get_account( TAIYI_INIT_SIMING_NAME );
        asset init_starting_balance = init.balance;
        
        account_create_operation op;
        
        op.new_account_name = "alice";
        op.creator = TAIYI_INIT_SIMING_NAME;
        op.owner = authority( 1, priv_key.get_public_key(), 1 );
        op.active = authority( 2, priv_key.get_public_key(), 2 );
        op.memo_key = priv_key.get_public_key();
        op.json_metadata = "{\"foo\":\"bar\"}";
        
        BOOST_TEST_MESSAGE( "--- Test failure paying more than the fee" );
        op.fee = asset( 101, YANG_SYMBOL );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.operations.push_back( op );
        sign( tx, init_account_priv_key );
        tx.validate();
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
        
        BOOST_TEST_MESSAGE( "--- Test normal account creation" );
        op.fee = asset( 100, YANG_SYMBOL );
        tx.clear();
        tx.operations.push_back( op );
        sign( tx, init_account_priv_key );
        tx.validate();
        db->push_transaction( tx, 0 );
        
        const account_object& acct = db->get_account( "alice" );
        const account_authority_object& acct_auth = db->get< account_authority_object, by_account >( "alice" );
        
        BOOST_REQUIRE( acct.name == "alice" );
        BOOST_REQUIRE( acct_auth.owner == authority( 1, priv_key.get_public_key(), 1 ) );
        BOOST_REQUIRE( acct_auth.active == authority( 2, priv_key.get_public_key(), 2 ) );
        BOOST_REQUIRE( acct.memo_key == priv_key.get_public_key() );
        BOOST_REQUIRE( acct.proxy == "" );
        BOOST_REQUIRE( acct.created == db->head_block_time() );
        BOOST_REQUIRE( acct.balance.amount.value == ASSET( "0.000 YANG" ).amount.value );
        BOOST_REQUIRE( acct.id._id == acct_auth.id._id );
        
        BOOST_REQUIRE( acct.qi.amount.value == 100000 );
        BOOST_REQUIRE( acct.qi_withdraw_rate.amount.value == ASSET( "0.000000 QI" ).amount.value );
        BOOST_REQUIRE( acct.proxied_vsf_adores_total().value == 0 );
        BOOST_REQUIRE( ( init_starting_balance - ASSET( "0.100 YANG" ) ).amount.value == init.balance.amount.value );
        validate_database();
        
        BOOST_TEST_MESSAGE( "--- Test failure of duplicate account creation" );
        BOOST_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );
        
        BOOST_REQUIRE( acct.name == "alice" );
        BOOST_REQUIRE( acct_auth.owner == authority( 1, priv_key.get_public_key(), 1 ) );
        BOOST_REQUIRE( acct_auth.active == authority( 2, priv_key.get_public_key(), 2 ) );
        BOOST_REQUIRE( acct.memo_key == priv_key.get_public_key() );
        BOOST_REQUIRE( acct.proxy == "" );
        BOOST_REQUIRE( acct.created == db->head_block_time() );
        BOOST_REQUIRE( acct.balance.amount.value == ASSET( "0.000 YANG " ).amount.value );
        BOOST_REQUIRE( acct.qi.amount.value == 100000 );
        BOOST_REQUIRE( acct.qi_withdraw_rate.amount.value == ASSET( "0.000000 QI" ).amount.value );
        BOOST_REQUIRE( acct.proxied_vsf_adores_total().value == 0 );
        BOOST_REQUIRE( ( init_starting_balance - ASSET( "0.100 YANG" ) ).amount.value == init.balance.amount.value );
        validate_database();
        
        BOOST_TEST_MESSAGE( "--- Test failure when creator cannot cover fee" );
        tx.signatures.clear();
        tx.operations.clear();
        op.fee = asset( db->get_account( TAIYI_INIT_SIMING_NAME ).balance.amount + 1, YANG_SYMBOL );
        op.new_account_name = "bob";
        tx.operations.push_back( op );
        sign( tx, init_account_priv_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
        validate_database();
        
        BOOST_TEST_MESSAGE( "--- Test failure covering siming fee" );
        generate_block();
        db_plugin->debug_update( [=]( database& db ) {
            db.modify( db.get_siming_schedule_object(), [&]( siming_schedule_object& wso ) {
                wso.median_props.account_creation_fee = ASSET( "10.000 YANG" );
            });
        });
        generate_block();
        
        tx.clear();
        op.fee = ASSET( "0.100 YANG" );
        tx.operations.push_back( op );
        sign( tx, init_account_priv_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
        validate_database();
        
        fund( TAIYI_TEMP_ACCOUNT, ASSET( "10.000 YANG" ) );
        vest( TAIYI_INIT_SIMING_NAME, TAIYI_TEMP_ACCOUNT, ASSET( "10.000 YANG" ) );
        
        BOOST_TEST_MESSAGE( "--- Test account creation with temp account does not set recovery account" );
        op.creator = TAIYI_TEMP_ACCOUNT;
        op.fee = ASSET( "10.000 YANG" );
        op.new_account_name = "bob";
        tx.clear();
        tx.operations.push_back( op );
        db->push_transaction( tx, 0 );
        
        BOOST_REQUIRE( db->get_account( "bob" ).recovery_account == account_name_type() );
        validate_database();
        
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_update_validate )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: account_update_validate" );
        
        ACTORS( (alice) )
        
        account_update_operation op;
        op.account = "alice";
        op.posting = authority();
        op.posting->weight_threshold = 1;
        op.posting->add_authorities( "abcdefghijklmnopq", 1 );
        
        try
        {
            op.validate();
            
            signed_transaction tx;
            tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
            tx.operations.push_back( op );
            sign( tx, alice_private_key );
            db->push_transaction( tx, 0 );
            
            BOOST_FAIL( "An exception was not thrown for an invalid account name" );
        }
        catch( fc::exception& ) {}
        
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_update_authorities )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: account_update_authorities" );
        
        ACTORS( (alice)(bob) )
        private_key_type active_key = generate_private_key( "new_key" );
        
        db->modify( db->get< account_authority_object, by_account >( "alice" ), [&]( account_authority_object& a ) {
            a.active = authority( 1, active_key.get_public_key(), 1 );
        });
        
        account_update_operation op;
        op.account = "alice";
        op.json_metadata = "{\"success\":true}";
        
        signed_transaction tx;
        tx.operations.push_back( op );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        
        BOOST_TEST_MESSAGE( "  Tests when owner authority is not updated ---" );
        BOOST_TEST_MESSAGE( "--- Test failure when no signature" );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
        
        BOOST_TEST_MESSAGE( "--- Test failure when wrong signature" );
        sign( tx, bob_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
        
        BOOST_TEST_MESSAGE( "--- Test failure when containing additional incorrect signature" );
        sign( tx, alice_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );
        
        BOOST_TEST_MESSAGE( "--- Test failure when containing duplicate signatures" );
        tx.signatures.clear();
        sign( tx, active_key );
        sign( tx, active_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );
        
        BOOST_TEST_MESSAGE( "--- Test success on active key" );
        tx.signatures.clear();
        sign( tx, active_key );
        db->push_transaction( tx, 0 );
        
        BOOST_TEST_MESSAGE( "--- Test success on owner key alone" );
        tx.signatures.clear();
        sign( tx, alice_private_key );
        db->push_transaction( tx, database::skip_transaction_dupe_check );
        
        BOOST_TEST_MESSAGE( "  Tests when owner authority is updated ---" );
        BOOST_TEST_MESSAGE( "--- Test failure when updating the owner authority with an active key" );
        tx.signatures.clear();
        tx.operations.clear();
        op.owner = authority( 1, active_key.get_public_key(), 1 );
        tx.operations.push_back( op );
        sign( tx, active_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_owner_auth );
        
        BOOST_TEST_MESSAGE( "--- Test failure when owner key and active key are present" );
        sign( tx, alice_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );
        
        BOOST_TEST_MESSAGE( "--- Test failure when incorrect signature" );
        tx.signatures.clear();
        sign( tx, alice_post_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0), tx_missing_owner_auth );
        
        BOOST_TEST_MESSAGE( "--- Test failure when duplicate owner keys are present" );
        tx.signatures.clear();
        sign( tx, alice_private_key );
        sign( tx, alice_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0), tx_duplicate_sig );
        
        BOOST_TEST_MESSAGE( "--- Test success when updating the owner authority with an owner key" );
        tx.signatures.clear();
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_update_apply )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: account_update_apply" );
        
        ACTORS( (alice) )
        private_key_type new_private_key = generate_private_key( "new_key" );
        
        BOOST_TEST_MESSAGE( "--- Test normal update" );
        
        account_update_operation op;
        op.account = "alice";
        op.owner = authority( 1, new_private_key.get_public_key(), 1 );
        op.active = authority( 2, new_private_key.get_public_key(), 2 );
        op.memo_key = new_private_key.get_public_key();
        op.json_metadata = "{\"bar\":\"foo\"}";
        
        signed_transaction tx;
        tx.operations.push_back( op );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        const account_object& acct = db->get_account( "alice" );
        const account_authority_object& acct_auth = db->get< account_authority_object, by_account >( "alice" );
        
        BOOST_REQUIRE( acct.name == "alice" );
        BOOST_REQUIRE( acct_auth.owner == authority( 1, new_private_key.get_public_key(), 1 ) );
        BOOST_REQUIRE( acct_auth.active == authority( 2, new_private_key.get_public_key(), 2 ) );
        BOOST_REQUIRE( acct.memo_key == new_private_key.get_public_key() );
        
        /* This is being moved out of consensus
         BOOST_REQUIRE( acct.json_metadata == "{\"bar\":\"foo\"}" );
         */
        
        validate_database();
        
        BOOST_TEST_MESSAGE( "--- Test failure when updating a non-existent account" );
        tx.operations.clear();
        tx.signatures.clear();
        op.account = "bob";
        tx.operations.push_back( op );
        sign( tx, new_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception )
        validate_database();
        
        
        BOOST_TEST_MESSAGE( "--- Test failure when account authority does not exist" );
        tx.clear();
        op = account_update_operation();
        op.account = "alice";
        op.posting = authority();
        op.posting->weight_threshold = 1;
        op.posting->add_authorities( "dave", 1 );
        tx.operations.push_back( op );
        sign( tx, new_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( transfer_validate )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: transfer_validate" );
        
        transfer_operation op;
        op.from = "alice";
        op.to = "bob";
        op.memo = "Memo";
        op.amount = asset( 100, YANG_SYMBOL );
        op.validate();
        
        BOOST_TEST_MESSAGE( " --- Invalid from account" );
        op.from = "alice-";
        TAIYI_REQUIRE_THROW( op.validate(), fc::assert_exception );
        op.from = "alice";
        
        BOOST_TEST_MESSAGE( " --- Invalid to account" );
        op.to = "bob-";
        TAIYI_REQUIRE_THROW( op.validate(), fc::assert_exception );
        op.to = "bob";
        
        BOOST_TEST_MESSAGE( " --- Memo too long" );
        std::string memo;
        for ( int i = 0; i < TAIYI_MAX_MEMO_SIZE + 1; i++ )
            memo += "x";
        op.memo = memo;
        TAIYI_REQUIRE_THROW( op.validate(), fc::assert_exception );
        op.memo = "Memo";
        
        BOOST_TEST_MESSAGE( " --- Negative amount" );
        op.amount = -op.amount;
        TAIYI_REQUIRE_THROW( op.validate(), fc::assert_exception );
        op.amount = -op.amount;
        
        BOOST_TEST_MESSAGE( " --- Transferring qi" );
        op.amount = asset( 100, QI_SYMBOL );
        TAIYI_REQUIRE_THROW( op.validate(), fc::assert_exception );
        op.amount = asset( 100, YANG_SYMBOL );
        
        op.validate();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( transfer_authorities )
{
    try
    {
        ACTORS( (alice)(bob) )
        fund( "alice", 10000 );
        
        BOOST_TEST_MESSAGE( "Testing: transfer_authorities" );
        
        transfer_operation op;
        op.from = "alice";
        op.to = "bob";
        op.amount = ASSET( "2.500 YANG" );
        
        signed_transaction tx;
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.operations.push_back( op );
        
        BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
        
        BOOST_TEST_MESSAGE( "--- Test failure when signed by a signature not in the account's authority" );
        sign( tx, alice_post_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
        
        BOOST_TEST_MESSAGE( "--- Test failure when duplicate signatures" );
        tx.signatures.clear();
        sign( tx, alice_private_key );
        sign( tx, alice_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );
        
        BOOST_TEST_MESSAGE( "--- Test failure when signed by an additional signature not in the creator's authority" );
        tx.signatures.clear();
        sign( tx, alice_private_key );
        sign( tx, bob_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );
        
        BOOST_TEST_MESSAGE( "--- Test success with siming signature" );
        tx.signatures.clear();
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( signature_stripping )
{
    try
    {
        // Alice, Bob and Sam all have 2-of-3 multisig on corp.
        // Legitimate tx signed by (Alice, Bob) goes through.
        // Sam shouldn't be able to add or remove signatures to get the transaction to process multiple times.
        
        ACTORS( (alice)(bob)(sam)(corp) )
        fund( "corp", 10000 );
        
        account_update_operation update_op;
        update_op.account = "corp";
        update_op.active = authority( 2, "alice", 1, "bob", 1, "sam", 1 );
        
        signed_transaction tx;
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.operations.push_back( update_op );
        
        sign( tx, corp_private_key );
        db->push_transaction( tx, 0 );
        
        tx.operations.clear();
        tx.signatures.clear();
        
        transfer_operation transfer_op;
        transfer_op.from = "corp";
        transfer_op.to = "sam";
        transfer_op.amount = ASSET( "1.000 YANG" );
        
        tx.operations.push_back( transfer_op );
        
        sign( tx, alice_private_key );
        signature_type alice_sig = tx.signatures.back();
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
        sign( tx, bob_private_key );
        signature_type bob_sig = tx.signatures.back();
        sign( tx, sam_private_key );
        signature_type sam_sig = tx.signatures.back();
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );
        
        tx.signatures.clear();
        tx.signatures.push_back( alice_sig );
        tx.signatures.push_back( bob_sig );
        db->push_transaction( tx, 0 );
        
        tx.signatures.clear();
        tx.signatures.push_back( alice_sig );
        tx.signatures.push_back( sam_sig );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( transfer_apply )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: transfer_apply" );
        
        ACTORS( (alice)(bob) )
        generate_block();
        fund( "alice", 10000 );
        fund( "bob", ASSET( "1.000 YANG" ) );
        
        BOOST_REQUIRE( db->get_account( "alice" ).balance.amount.value == ASSET( "10.000 YANG" ).amount.value );
        BOOST_REQUIRE( db->get_account( "bob" ).balance.amount.value == ASSET(" 1.000 YANG" ).amount.value );
        
        signed_transaction tx;
        transfer_operation op;
        
        op.from = "alice";
        op.to = "bob";
        op.amount = ASSET( "5.000 YANG" );
        
        BOOST_TEST_MESSAGE( "--- Test normal transaction" );
        tx.operations.push_back( op );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        BOOST_REQUIRE( db->get_account( "alice" ).balance.amount.value == ASSET( "5.000 YANG" ).amount.value );
        BOOST_REQUIRE( db->get_account( "bob" ).balance.amount.value == ASSET( "6.000 YANG" ).amount.value );
        validate_database();
        
        BOOST_TEST_MESSAGE( "--- Generating a block" );
        generate_block();
        
        const auto& new_alice = db->get_account( "alice" );
        const auto& new_bob = db->get_account( "bob" );
        
        BOOST_REQUIRE( new_alice.balance.amount.value == ASSET( "5.000 YANG" ).amount.value );
        BOOST_REQUIRE( new_bob.balance.amount.value == ASSET( "6.000 YANG" ).amount.value );
        validate_database();
        
        BOOST_TEST_MESSAGE( "--- Test emptying an account" );
        tx.signatures.clear();
        tx.operations.clear();
        tx.operations.push_back( op );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, alice_private_key );
        db->push_transaction( tx, database::skip_transaction_dupe_check );
        
        BOOST_REQUIRE( new_alice.balance.amount.value == ASSET( "0.000 YANG" ).amount.value );
        BOOST_REQUIRE( new_bob.balance.amount.value == ASSET( "11.000 YANG" ).amount.value );
        validate_database();
        
        BOOST_TEST_MESSAGE( "--- Test transferring non-existent funds" );
        tx.signatures.clear();
        tx.operations.clear();
        tx.operations.push_back( op );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, alice_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );
        
        BOOST_REQUIRE( new_alice.balance.amount.value == ASSET( "0.000 YANG" ).amount.value );
        BOOST_REQUIRE( new_bob.balance.amount.value == ASSET( "11.000 YANG" ).amount.value );
        validate_database();
        
        BOOST_TEST_MESSAGE( "--- Test failure transfering QI to zuowang.dao" );
        op.from = "bob";
        op.to = TAIYI_TREASURY_ACCOUNT;
        op.amount = ASSET( "1.000000 QI" );
        tx.signatures.clear();
        tx.operations.clear();
        tx.operations.push_back( op );
        sign( tx, bob_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx ), fc::exception );
        
        BOOST_REQUIRE( db->get_account( "bob" ).balance == ASSET( "11.000 YANG" ) );
        BOOST_REQUIRE( db->get_account( TAIYI_TREASURY_ACCOUNT ).balance == ASSET( "0.000 YANG" ) );
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( transfer_to_qi_validate )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: transfer_to_qi_validate" );
        
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( transfer_to_qi_authorities )
{
    try
    {
        ACTORS( (alice)(bob) )
        fund( "alice", 10000 );
        
        BOOST_TEST_MESSAGE( "Testing: transfer_to_qi_authorities" );
        
        transfer_to_qi_operation op;
        op.from = "alice";
        op.to = "bob";
        op.amount = ASSET( "2.500 YANG" );
        
        signed_transaction tx;
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.operations.push_back( op );
        
        BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
        
        BOOST_TEST_MESSAGE( "--- Test failure when signed by a signature not in the account's authority" );
        sign( tx, alice_post_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
        
        BOOST_TEST_MESSAGE( "--- Test failure when duplicate signatures" );
        tx.signatures.clear();
        sign( tx, alice_private_key );
        sign( tx, alice_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );
        
        BOOST_TEST_MESSAGE( "--- Test failure when signed by an additional signature not in the creator's authority" );
        tx.signatures.clear();
        sign( tx, alice_private_key );
        sign( tx, bob_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );
        
        BOOST_TEST_MESSAGE( "--- Test success with from signature" );
        tx.signatures.clear();
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( transfer_to_qi_apply )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: transfer_to_qi_apply" );
        
        ACTORS( (alice)(bob) )
        fund( "alice", 10000 );
        
        const auto& gpo = db->get_dynamic_global_properties();
        
        BOOST_REQUIRE( alice.balance == ASSET( "10.000 YANG" ) );
        
        auto shares = asset( gpo.total_qi.amount, QI_SYMBOL );
        auto alice_shares = alice.qi;
        auto bob_shares = bob.qi;
        
        transfer_to_qi_operation op;
        op.from = "alice";
        op.to = TAIYI_TREASURY_ACCOUNT;
        op.amount = ASSET( "7.500 YANG" );
        
        signed_transaction tx;
        tx.operations.push_back( op );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, alice_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
        
        op.to = "";
        tx.clear();
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        auto new_vest = op.amount * TAIYI_QI_SHARE_PRICE;
        shares += new_vest;
        alice_shares += new_vest;
        
        BOOST_REQUIRE( alice.balance.amount.value == ASSET( "2.500 YANG" ).amount.value );
        BOOST_REQUIRE( alice.qi.amount.value == alice_shares.amount.value );
        BOOST_REQUIRE( gpo.total_qi.amount.value == shares.amount.value );
        validate_database();
        
        op.to = "bob";
        op.amount = asset( 2000, YANG_SYMBOL );
        tx.operations.clear();
        tx.signatures.clear();
        tx.operations.push_back( op );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        new_vest = asset( ( op.amount * TAIYI_QI_SHARE_PRICE ).amount, QI_SYMBOL );
        shares += new_vest;
        bob_shares += new_vest;
        
        BOOST_REQUIRE( alice.balance.amount.value == ASSET( "0.500 YANG" ).amount.value );
        BOOST_REQUIRE( alice.qi.amount.value == alice_shares.amount.value );
        BOOST_REQUIRE( bob.balance.amount.value == ASSET( "0.000 YANG" ).amount.value );
        BOOST_REQUIRE( bob.qi.amount.value == bob_shares.amount.value );
        BOOST_REQUIRE( gpo.total_qi.amount.value == shares.amount.value );
        validate_database();
        
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );
        
        BOOST_REQUIRE( alice.balance.amount.value == ASSET( "0.500 YANG").amount.value );
        BOOST_REQUIRE( alice.qi.amount.value == alice_shares.amount.value );
        BOOST_REQUIRE( bob.balance.amount.value == ASSET( "0.000 YANG" ).amount.value );
        BOOST_REQUIRE( bob.qi.amount.value == bob_shares.amount.value );
        BOOST_REQUIRE( gpo.total_qi.amount.value == shares.amount.value );
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( withdraw_qi_validate )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: withdraw_qi_validate" );
        
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( withdraw_qi_authorities )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: withdraw_qi_authorities" );
        
        ACTORS( (alice)(bob) )
        fund( "alice", 10000 );
        vest( "alice", 10000 );
        
        withdraw_qi_operation op;
        op.account = "alice";
        op.qi = ASSET( "0.001000 QI" );
        
        signed_transaction tx;
        tx.operations.push_back( op );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        
        BOOST_TEST_MESSAGE( "--- Test failure when no signature." );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );
        
        BOOST_TEST_MESSAGE( "--- Test success with account signature" );
        sign( tx, alice_private_key );
        db->push_transaction( tx, database::skip_transaction_dupe_check );
        
        BOOST_TEST_MESSAGE( "--- Test failure with duplicate signature" );
        sign( tx, alice_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_duplicate_sig );
        
        BOOST_TEST_MESSAGE( "--- Test failure with additional incorrect signature" );
        tx.signatures.clear();
        sign( tx, alice_private_key );
        sign( tx, bob_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_irrelevant_sig );
        
        BOOST_TEST_MESSAGE( "--- Test failure with incorrect signature" );
        tx.signatures.clear();
        sign( tx, alice_post_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );
        
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( withdraw_qi_apply )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: withdraw_qi_apply" );
        
        ACTORS( (alice)(bob) )
        generate_block();
        vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "10.000 YANG" ) );
        
        BOOST_TEST_MESSAGE( "--- Test failure withdrawing negative QI" );
        
        {
            const auto& alice = db->get_account( "alice" );
            
            withdraw_qi_operation op;
            op.account = "alice";
            op.qi = asset( -1, QI_SYMBOL );
            
            signed_transaction tx;
            tx.operations.push_back( op );
            tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
            sign( tx, alice_private_key );
            TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
            
            
            BOOST_TEST_MESSAGE( "--- Test withdraw of existing QI" );
            op.qi = asset( alice.qi.amount / 2, QI_SYMBOL );
            
            auto old_qi = alice.qi;
            
            tx.clear();
            tx.operations.push_back( op );
            tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
            sign( tx, alice_private_key );
            db->push_transaction( tx, 0 );
            
            BOOST_REQUIRE( alice.qi.amount.value == old_qi.amount.value );
            BOOST_REQUIRE( alice.qi_withdraw_rate.amount.value == ( old_qi.amount / ( TAIYI_QI_WITHDRAW_INTERVALS * 2 ) ).value );
            BOOST_REQUIRE( alice.to_withdraw.value == op.qi.amount.value );
            BOOST_REQUIRE( alice.next_qi_withdrawal_time == db->head_block_time() + TAIYI_QI_WITHDRAW_INTERVAL_SECONDS );
            validate_database();
            
            BOOST_TEST_MESSAGE( "--- Test changing qi withdrawal" );
            tx.operations.clear();
            tx.signatures.clear();
            
            op.qi = asset( alice.qi.amount / 3, QI_SYMBOL );
            tx.operations.push_back( op );
            tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
            sign( tx, alice_private_key );
            db->push_transaction( tx, 0 );
            
            BOOST_REQUIRE( alice.qi.amount.value == old_qi.amount.value );
            BOOST_REQUIRE( alice.qi_withdraw_rate.amount.value == ( old_qi.amount / ( TAIYI_QI_WITHDRAW_INTERVALS * 3 ) ).value + 1 );
            BOOST_REQUIRE( alice.to_withdraw.value == op.qi.amount.value );
            BOOST_REQUIRE( alice.next_qi_withdrawal_time == db->head_block_time() + TAIYI_QI_WITHDRAW_INTERVAL_SECONDS );
            validate_database();
            
            BOOST_TEST_MESSAGE( "--- Test withdrawing more qi than available" );
            //auto old_withdraw_amount = alice.to_withdraw;
            tx.operations.clear();
            tx.signatures.clear();
            
            op.qi = asset( alice.qi.amount * 2, QI_SYMBOL );
            tx.operations.push_back( op );
            tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
            sign( tx, alice_private_key );
            TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
            
            BOOST_REQUIRE( alice.qi.amount.value == old_qi.amount.value );
            BOOST_REQUIRE( alice.qi_withdraw_rate.amount.value == ( old_qi.amount / ( TAIYI_QI_WITHDRAW_INTERVALS * 3 ) ).value + 1 );
            BOOST_REQUIRE( alice.next_qi_withdrawal_time == db->head_block_time() + TAIYI_QI_WITHDRAW_INTERVAL_SECONDS );
            validate_database();
            
            BOOST_TEST_MESSAGE( "--- Test withdrawing 0 to reset qi withdraw" );
            tx.operations.clear();
            tx.signatures.clear();
            
            op.qi = asset( 0, QI_SYMBOL );
            tx.operations.push_back( op );
            tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
            sign( tx, alice_private_key );
            db->push_transaction( tx, 0 );
            
            BOOST_REQUIRE( alice.qi.amount.value == old_qi.amount.value );
            BOOST_REQUIRE( alice.qi_withdraw_rate.amount.value == 0 );
            BOOST_REQUIRE( alice.to_withdraw.value == 0 );
            BOOST_REQUIRE( alice.next_qi_withdrawal_time == fc::time_point_sec::maximum() );
            
            
            BOOST_TEST_MESSAGE( "--- Test cancelling a withdraw when below the account creation fee" );
            op.qi = alice.qi;
            tx.clear();
            tx.operations.push_back( op );
            sign( tx, alice_private_key );
            db->push_transaction( tx, 0 );
            generate_block();
        }
        
        db_plugin->debug_update( [=]( database& db ) {
            auto& wso = db.get_siming_schedule_object();
            db.modify( wso, [&]( siming_schedule_object& w ) {
                w.median_props.account_creation_fee = ASSET( "10.000 YANG" );
            });
        }, database::skip_siming_signature );
        
        withdraw_qi_operation op;
        signed_transaction tx;
        op.account = "alice";
        op.qi = ASSET( "0.000000 QI" );
        tx.operations.push_back( op );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        BOOST_REQUIRE( db->get_account( "alice" ).qi_withdraw_rate == ASSET( "0.000000 QI" ) );
        validate_database();
        
        BOOST_TEST_MESSAGE( "--- Test withdrawing minimal QI" );
        op.account = "bob";
        op.qi = db->get_account( "bob" ).qi;
        tx.clear();
        tx.operations.push_back( op );
        sign( tx, bob_private_key );
        db->push_transaction( tx, 0 ); // We do not need to test the result of this, simply that it works.
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( siming_update_validate )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: siming_update_validate" );
        
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( siming_update_authorities )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: siming_update_authorities" );
        
        ACTORS( (alice)(bob) );
        fund( "alice", 10000 );
        
        private_key_type signing_key = generate_private_key( "new_key" );
        
        siming_update_operation op;
        op.owner = "alice";
        op.url = "foo.bar";
        op.fee = ASSET( "1.000 YANG" );
        op.block_signing_key = signing_key.get_public_key();
        
        signed_transaction tx;
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.operations.push_back( op );
        
        BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
        
        BOOST_TEST_MESSAGE( "--- Test failure when signed by a signature not in the account's authority" );
        sign( tx, alice_post_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
        
        BOOST_TEST_MESSAGE( "--- Test failure when duplicate signatures" );
        tx.signatures.clear();
        sign( tx, alice_private_key );
        sign( tx, alice_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );
        
        BOOST_TEST_MESSAGE( "--- Test failure when signed by an additional signature not in the creator's authority" );
        tx.signatures.clear();
        sign( tx, alice_private_key );
        sign( tx, bob_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );
        
        BOOST_TEST_MESSAGE( "--- Test success with siming signature" );
        tx.signatures.clear();
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        tx.signatures.clear();
        sign( tx, signing_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( siming_update_apply )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: siming_update_apply" );
        
        ACTORS( (alice) )
        fund( "alice", 10000 );
        
        private_key_type signing_key = generate_private_key( "new_key" );
        
        BOOST_TEST_MESSAGE( "--- Test upgrading an account to a siming" );
        
        siming_update_operation op;
        op.owner = "alice";
        op.url = "foo.bar";
        op.fee = ASSET( "1.000 YANG" );
        op.block_signing_key = signing_key.get_public_key();
        op.props.account_creation_fee = legacy_taiyi_asset::from_asset( asset(TAIYI_MIN_ACCOUNT_CREATION_FEE + 10, YANG_SYMBOL) );
        op.props.maximum_block_size = TAIYI_MIN_BLOCK_SIZE_LIMIT + 100;
        
        signed_transaction tx;
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        
        db->push_transaction( tx, 0 );
        
        const siming_object& alice_siming = db->get_siming( "alice" );
        
        BOOST_REQUIRE( alice_siming.owner == "alice" );
        BOOST_REQUIRE( alice_siming.created == db->head_block_time() );
        BOOST_REQUIRE( alice_siming.url == op.url );
        BOOST_REQUIRE( alice_siming.signing_key == op.block_signing_key );
        BOOST_REQUIRE( alice_siming.props.account_creation_fee == op.props.account_creation_fee.to_asset<true>() );
        BOOST_REQUIRE( alice_siming.props.maximum_block_size == op.props.maximum_block_size );
        BOOST_REQUIRE( alice_siming.total_missed == 0 );
        BOOST_REQUIRE( alice_siming.last_aslot == 0 );
        BOOST_REQUIRE( alice_siming.last_confirmed_block_num == 0 );
        BOOST_REQUIRE( alice_siming.adores.value == 0 );
        BOOST_REQUIRE( alice_siming.virtual_last_update == 0 );
        BOOST_REQUIRE( alice_siming.virtual_position == 0 );
        BOOST_REQUIRE( alice_siming.virtual_scheduled_time == fc::uint128_t::max_value() );
        BOOST_REQUIRE( alice.balance.amount.value == ASSET( "10.000 YANG" ).amount.value ); // No fee
        validate_database();
        
        BOOST_TEST_MESSAGE( "--- Test updating a siming" );
        
        tx.signatures.clear();
        tx.operations.clear();
        op.url = "bar.foo";
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        
        db->push_transaction( tx, 0 );
        
        BOOST_REQUIRE( alice_siming.owner == "alice" );
        BOOST_REQUIRE( alice_siming.created == db->head_block_time() );
        BOOST_REQUIRE( alice_siming.url == "bar.foo" );
        BOOST_REQUIRE( alice_siming.signing_key == op.block_signing_key );
        BOOST_REQUIRE( alice_siming.props.account_creation_fee == op.props.account_creation_fee.to_asset<true>() );
        BOOST_REQUIRE( alice_siming.props.maximum_block_size == op.props.maximum_block_size );
        BOOST_REQUIRE( alice_siming.total_missed == 0 );
        BOOST_REQUIRE( alice_siming.last_aslot == 0 );
        BOOST_REQUIRE( alice_siming.last_confirmed_block_num == 0 );
        BOOST_REQUIRE( alice_siming.adores.value == 0 );
        BOOST_REQUIRE( alice_siming.virtual_last_update == 0 );
        BOOST_REQUIRE( alice_siming.virtual_position == 0 );
        BOOST_REQUIRE( alice_siming.virtual_scheduled_time == fc::uint128_t::max_value() );
        BOOST_REQUIRE( alice.balance.amount.value == ASSET( "10.000 YANG" ).amount.value );
        validate_database();
        
        BOOST_TEST_MESSAGE( "--- Test failure when upgrading a non-existent account" );
        
        tx.signatures.clear();
        tx.operations.clear();
        op.owner = "bob";
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_siming_adore_validate )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: account_siming_adore_validate" );
        
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_siming_adore_authorities )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: account_siming_adore_authorities" );
        
        ACTORS( (alice)(bob)(sam) )
        
        fund( "alice", 1000 );
        private_key_type alice_siming_key = generate_private_key( "alice_siming" );
        siming_create( "alice", alice_private_key, "foo.bar", alice_siming_key.get_public_key(), 1000 );
        
        account_siming_adore_operation op;
        op.account = "bob";
        op.siming = "alice";
        
        signed_transaction tx;
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.operations.push_back( op );
        
        BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
        
        BOOST_TEST_MESSAGE( "--- Test failure when signed by a signature not in the account's authority" );
        sign( tx, bob_post_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
        
        BOOST_TEST_MESSAGE( "--- Test failure when duplicate signatures" );
        tx.signatures.clear();
        sign( tx, bob_private_key );
        sign( tx, bob_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );
        
        BOOST_TEST_MESSAGE( "--- Test failure when signed by an additional signature not in the creator's authority" );
        tx.signatures.clear();
        sign( tx, bob_private_key );
        sign( tx, alice_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );
        
        BOOST_TEST_MESSAGE( "--- Test success with siming signature" );
        tx.signatures.clear();
        sign( tx, bob_private_key );
        db->push_transaction( tx, 0 );
        
        BOOST_TEST_MESSAGE( "--- Test failure with proxy signature" );
        proxy( "bob", "sam" );
        tx.signatures.clear();
        sign( tx, sam_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );
        
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_siming_adore_apply )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: account_siming_adore_apply" );
        
        ACTORS( (alice)(bob)(sam) )
        fund( "alice" , 5000 );
        vest( "alice", 5000 );
        fund( "sam", 1000 );
        
        private_key_type sam_siming_key = generate_private_key( "sam_key" );
        siming_create( "sam", sam_private_key, "foo.bar", sam_siming_key.get_public_key(), 1000 );
        const siming_object& sam_siming = db->get_siming( "sam" );
        
        const auto& siming_adore_idx = db->get_index< siming_adore_index >().indices().get< by_siming_account >();
        
        BOOST_TEST_MESSAGE( "--- Test normal vote" );
        account_siming_adore_operation op;
        op.account = "alice";
        op.siming = "sam";
        op.approve = true;
        
        signed_transaction tx;
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        
        db->push_transaction( tx, 0 );
        
        BOOST_REQUIRE( sam_siming.adores == alice.qi.amount );
        BOOST_REQUIRE( siming_adore_idx.find( boost::make_tuple( sam_siming.owner, alice.name ) ) != siming_adore_idx.end() );
        validate_database();
        
        BOOST_TEST_MESSAGE( "--- Test revoke vote" );
        op.approve = false;
        tx.operations.clear();
        tx.signatures.clear();
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        
        db->push_transaction( tx, 0 );
        BOOST_REQUIRE( sam_siming.adores.value == 0 );
        BOOST_REQUIRE( siming_adore_idx.find( boost::make_tuple( sam_siming.owner, alice.name ) ) == siming_adore_idx.end() );
        
        BOOST_TEST_MESSAGE( "--- Test failure when attempting to revoke a non-existent vote" );
        
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );
        BOOST_REQUIRE( sam_siming.adores.value == 0 );
        BOOST_REQUIRE( siming_adore_idx.find( boost::make_tuple( sam_siming.owner, alice.name ) ) == siming_adore_idx.end() );
        
        BOOST_TEST_MESSAGE( "--- Test proxied vote" );
        proxy( "alice", "bob" );
        tx.operations.clear();
        tx.signatures.clear();
        op.approve = true;
        op.account = "bob";
        tx.operations.push_back( op );
        sign( tx, bob_private_key );
        
        db->push_transaction( tx, 0 );
        
        BOOST_REQUIRE( sam_siming.adores == ( bob.proxied_vsf_adores_total() + bob.qi.amount ) );
        BOOST_REQUIRE( siming_adore_idx.find( boost::make_tuple( sam_siming.owner, bob.name ) ) != siming_adore_idx.end() );
        BOOST_REQUIRE( siming_adore_idx.find( boost::make_tuple( sam_siming.owner, alice.name ) ) == siming_adore_idx.end() );
        
        BOOST_TEST_MESSAGE( "--- Test vote from a proxied account" );
        tx.operations.clear();
        tx.signatures.clear();
        op.account = "alice";
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );
        
        BOOST_REQUIRE( sam_siming.adores == ( bob.proxied_vsf_adores_total() + bob.qi.amount ) );
        BOOST_REQUIRE( siming_adore_idx.find( boost::make_tuple( sam_siming.owner, bob.name ) ) != siming_adore_idx.end() );
        BOOST_REQUIRE( siming_adore_idx.find( boost::make_tuple( sam_siming.owner, alice.name ) ) == siming_adore_idx.end() );
        
        BOOST_TEST_MESSAGE( "--- Test revoke proxied vote" );
        tx.operations.clear();
        tx.signatures.clear();
        op.account = "bob";
        op.approve = false;
        tx.operations.push_back( op );
        sign( tx, bob_private_key );
        
        db->push_transaction( tx, 0 );
        
        BOOST_REQUIRE( sam_siming.adores.value == 0 );
        BOOST_REQUIRE( siming_adore_idx.find( boost::make_tuple( sam_siming.owner, bob.name ) ) == siming_adore_idx.end() );
        BOOST_REQUIRE( siming_adore_idx.find( boost::make_tuple( sam_siming.owner, alice.name ) ) == siming_adore_idx.end() );
        
        BOOST_TEST_MESSAGE( "--- Test failure when voting for a non-existent account" );
        tx.operations.clear();
        tx.signatures.clear();
        op.siming = "dave";
        op.approve = true;
        tx.operations.push_back( op );
        sign( tx, bob_private_key );
        
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
        validate_database();
        
        BOOST_TEST_MESSAGE( "--- Test failure when voting for an account that is not a siming" );
        tx.operations.clear();
        tx.signatures.clear();
        op.siming = "alice";
        tx.operations.push_back( op );
        sign( tx, bob_private_key );
        
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_siming_proxy_validate )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: account_siming_proxy_validate" );
        
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_siming_proxy_authorities )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: account_siming_proxy_authorities" );
        
        ACTORS( (alice)(bob) )
        
        account_siming_proxy_operation op;
        op.account = "bob";
        op.proxy = "alice";
        
        signed_transaction tx;
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.operations.push_back( op );
        
        BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
        
        BOOST_TEST_MESSAGE( "--- Test failure when signed by a signature not in the account's authority" );
        sign( tx, bob_post_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
        
        BOOST_TEST_MESSAGE( "--- Test failure when duplicate signatures" );
        tx.signatures.clear();
        sign( tx, bob_private_key );
        sign( tx, bob_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );
        
        BOOST_TEST_MESSAGE( "--- Test failure when signed by an additional signature not in the creator's authority" );
        tx.signatures.clear();
        sign( tx, bob_private_key );
        sign( tx, alice_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );
        
        BOOST_TEST_MESSAGE( "--- Test success with siming signature" );
        tx.signatures.clear();
        sign( tx, bob_private_key );
        db->push_transaction( tx, 0 );
        
        BOOST_TEST_MESSAGE( "--- Test failure with proxy signature" );
        tx.signatures.clear();
        sign( tx, alice_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );
        
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_siming_proxy_apply )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: account_siming_proxy_apply" );
        
        ACTORS( (alice)(bob)(sam)(dave) )
        fund( "alice", 1000 );
        vest( "alice", 1000 );
        fund( "bob", 3000 );
        vest( "bob", 3000 );
        fund( "sam", 5000 );
        vest( "sam", 5000 );
        fund( "dave", 7000 );
        vest( "dave", 7000 );
        
        BOOST_TEST_MESSAGE( "--- Test setting proxy to another account from self." );
        // bob -> alice
        
        account_siming_proxy_operation op;
        op.account = "bob";
        op.proxy = "alice";
        
        signed_transaction tx;
        tx.operations.push_back( op );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, bob_private_key );
        
        db->push_transaction( tx, 0 );
        
        BOOST_REQUIRE( bob.proxy == "alice" );
        BOOST_REQUIRE( bob.proxied_vsf_adores_total().value == 0 );
        BOOST_REQUIRE( alice.proxy == TAIYI_PROXY_TO_SELF_ACCOUNT );
        BOOST_REQUIRE( alice.proxied_vsf_adores_total() == bob.qi.amount );
        validate_database();
        
        BOOST_TEST_MESSAGE( "--- Test changing proxy" );
        // bob->sam
        
        tx.operations.clear();
        tx.signatures.clear();
        op.proxy = "sam";
        tx.operations.push_back( op );
        sign( tx, bob_private_key );
        
        db->push_transaction( tx, 0 );
        
        BOOST_REQUIRE( bob.proxy == "sam" );
        BOOST_REQUIRE( bob.proxied_vsf_adores_total().value == 0 );
        BOOST_REQUIRE( alice.proxied_vsf_adores_total().value == 0 );
        BOOST_REQUIRE( sam.proxy == TAIYI_PROXY_TO_SELF_ACCOUNT );
        BOOST_REQUIRE( sam.proxied_vsf_adores_total().value == bob.qi.amount );
        validate_database();
        
        BOOST_TEST_MESSAGE( "--- Test failure when changing proxy to existing proxy" );
        
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );
        
        BOOST_REQUIRE( bob.proxy == "sam" );
        BOOST_REQUIRE( bob.proxied_vsf_adores_total().value == 0 );
        BOOST_REQUIRE( sam.proxy == TAIYI_PROXY_TO_SELF_ACCOUNT );
        BOOST_REQUIRE( sam.proxied_vsf_adores_total() == bob.qi.amount );
        validate_database();
        
        BOOST_TEST_MESSAGE( "--- Test adding a grandparent proxy" );
        // bob->sam->dave
        
        tx.operations.clear();
        tx.signatures.clear();
        op.proxy = "dave";
        op.account = "sam";
        tx.operations.push_back( op );
        sign( tx, sam_private_key );
        
        db->push_transaction( tx, 0 );
        
        BOOST_REQUIRE( bob.proxy == "sam" );
        BOOST_REQUIRE( bob.proxied_vsf_adores_total().value == 0 );
        BOOST_REQUIRE( sam.proxy == "dave" );
        BOOST_REQUIRE( sam.proxied_vsf_adores_total() == bob.qi.amount );
        BOOST_REQUIRE( dave.proxy == TAIYI_PROXY_TO_SELF_ACCOUNT );
        BOOST_REQUIRE( dave.proxied_vsf_adores_total() == ( sam.qi + bob.qi ).amount );
        validate_database();
        
        BOOST_TEST_MESSAGE( "--- Test adding a grandchild proxy" );
        //       alice
        //         |
        // bob->  sam->dave
        
        tx.operations.clear();
        tx.signatures.clear();
        op.proxy = "sam";
        op.account = "alice";
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        
        db->push_transaction( tx, 0 );
        
        BOOST_REQUIRE( alice.proxy == "sam" );
        BOOST_REQUIRE( alice.proxied_vsf_adores_total().value == 0 );
        BOOST_REQUIRE( bob.proxy == "sam" );
        BOOST_REQUIRE( bob.proxied_vsf_adores_total().value == 0 );
        BOOST_REQUIRE( sam.proxy == "dave" );
        BOOST_REQUIRE( sam.proxied_vsf_adores_total() == ( bob.qi + alice.qi ).amount );
        BOOST_REQUIRE( dave.proxy == TAIYI_PROXY_TO_SELF_ACCOUNT );
        BOOST_REQUIRE( dave.proxied_vsf_adores_total() == ( sam.qi + bob.qi + alice.qi ).amount );
        validate_database();
        
        BOOST_TEST_MESSAGE( "--- Test removing a grandchild proxy" );
        // alice->sam->dave
        
        tx.operations.clear();
        tx.signatures.clear();
        op.proxy = TAIYI_PROXY_TO_SELF_ACCOUNT;
        op.account = "bob";
        tx.operations.push_back( op );
        sign( tx, bob_private_key );
        
        db->push_transaction( tx, 0 );
        
        BOOST_REQUIRE( alice.proxy == "sam" );
        BOOST_REQUIRE( alice.proxied_vsf_adores_total().value == 0 );
        BOOST_REQUIRE( bob.proxy == TAIYI_PROXY_TO_SELF_ACCOUNT );
        BOOST_REQUIRE( bob.proxied_vsf_adores_total().value == 0 );
        BOOST_REQUIRE( sam.proxy == "dave" );
        BOOST_REQUIRE( sam.proxied_vsf_adores_total() == alice.qi.amount );
        BOOST_REQUIRE( dave.proxy == TAIYI_PROXY_TO_SELF_ACCOUNT );
        BOOST_REQUIRE( dave.proxied_vsf_adores_total() == ( sam.qi + alice.qi ).amount );
        validate_database();
        
        BOOST_TEST_MESSAGE( "--- Test votes are transferred when a proxy is added" );
        account_siming_adore_operation vote;
        vote.account= "bob";
        vote.siming = TAIYI_INIT_SIMING_NAME;
        tx.operations.clear();
        tx.signatures.clear();
        tx.operations.push_back( vote );
        sign( tx, bob_private_key );
        
        db->push_transaction( tx, 0 );
        
        tx.operations.clear();
        tx.signatures.clear();
        op.account = "alice";
        op.proxy = "bob";
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        
        db->push_transaction( tx, 0 );
        
        BOOST_REQUIRE( db->get_siming( TAIYI_INIT_SIMING_NAME ).adores == ( alice.qi + bob.qi ).amount );
        validate_database();
        
        BOOST_TEST_MESSAGE( "--- Test votes are removed when a proxy is removed" );
        op.proxy = TAIYI_PROXY_TO_SELF_ACCOUNT;
        tx.signatures.clear();
        tx.operations.clear();
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        
        db->push_transaction( tx, 0 );
        
        BOOST_REQUIRE( db->get_siming( TAIYI_INIT_SIMING_NAME ).adores == bob.qi.amount );
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( custom_authorities )
{
    custom_operation op;
    op.required_auths.insert( "alice" );
    op.required_auths.insert( "bob" );
    
    flat_set< account_name_type > auths;
    flat_set< account_name_type > expected;
    
    op.get_required_owner_authorities( auths );
    BOOST_REQUIRE( auths == expected );
    
    op.get_required_posting_authorities( auths );
    BOOST_REQUIRE( auths == expected );
    
    expected.insert( "alice" );
    expected.insert( "bob" );
    op.get_required_active_authorities( auths );
    BOOST_REQUIRE( auths == expected );
}

BOOST_AUTO_TEST_CASE( custom_json_authorities )
{
    custom_json_operation op;
    op.required_auths.insert( "alice" );
    op.required_posting_auths.insert( "bob" );
    
    flat_set< account_name_type > auths;
    flat_set< account_name_type > expected;
    
    op.get_required_owner_authorities( auths );
    BOOST_REQUIRE( auths == expected );
    
    expected.insert( "alice" );
    op.get_required_active_authorities( auths );
    BOOST_REQUIRE( auths == expected );
    
    auths.clear();
    expected.clear();
    expected.insert( "bob" );
    op.get_required_posting_authorities( auths );
    BOOST_REQUIRE( auths == expected );
}

BOOST_AUTO_TEST_CASE( custom_json_rate_limit )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: custom_json_rate_limit" );
        
        ACTORS( (alice)(bob)(sam) )
        
        BOOST_TEST_MESSAGE( "--- Testing 5 custom json ops as separate transactions" );
        
        custom_json_operation op;
        signed_transaction tx;
        op.required_posting_auths.insert( "alice" );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        
        for( int i = 0; i < 5; i++ )
        {
            op.json = fc::to_string( i );
            tx.clear();
            tx.operations.push_back( op );
            sign( tx, alice_private_key );
            db->push_transaction( tx, 0 );
        }
        
        
        BOOST_TEST_MESSAGE( "--- Testing failure pushing 6th custom json op tx" );
        
        op.json = "toomany";
        tx.clear();
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), plugin_exception );
        
        
        BOOST_TEST_MESSAGE( "--- Testing 5 custom json ops in one transaction" );
        tx.clear();
        op.json = "foobar";
        op.required_posting_auths.clear();
        op.required_posting_auths.insert( "bob" );
        
        for( int i = 0; i < 5; i++ )
        {
            tx.operations.push_back( op );
        }
        
        sign( tx, bob_private_key );
        db->push_transaction( tx, 0 );
        
        
        BOOST_TEST_MESSAGE( "--- Testing failure pushing 6th custom json op tx" );
        
        op.json = "toomany";
        tx.clear();
        tx.operations.push_back( op );
        sign( tx, bob_private_key );
        
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), plugin_exception );
        
        
        BOOST_TEST_MESSAGE( "--- Testing failure of 6 custom json ops in one transaction" );
        tx.clear();
        op.required_posting_auths.clear();
        op.required_posting_auths.insert( "sam" );
        
        for( int i = 0; i < 6; i++ )
        {
            tx.operations.push_back( op );
        }
        
        sign( tx, sam_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), plugin_exception );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_recovery )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: account recovery" );
        
        ACTORS( (alice) );
        fund( "alice", 1000000 );
        generate_block();
        
        db_plugin->debug_update( [=]( database& db ) {
            db.modify( db.get_siming_schedule_object(), [&]( siming_schedule_object& wso ) {
                wso.median_props.account_creation_fee = ASSET( "0.100 YANG" );
            });
        });
        
        generate_block();
        
        BOOST_TEST_MESSAGE( "Creating account bob with alice" );
        
        account_create_operation acc_create;
        acc_create.fee = ASSET( "0.100 YANG" );
        acc_create.creator = "alice";
        acc_create.new_account_name = "bob";
        acc_create.owner = authority( 1, generate_private_key( "bob_owner" ).get_public_key(), 1 );
        acc_create.active = authority( 1, generate_private_key( "bob_active" ).get_public_key(), 1 );
        acc_create.posting = authority( 1, generate_private_key( "bob_posting" ).get_public_key(), 1 );
        acc_create.memo_key = generate_private_key( "bob_memo" ).get_public_key();
        acc_create.json_metadata = "";
        
        
        signed_transaction tx;
        tx.operations.push_back( acc_create );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        vest( TAIYI_INIT_SIMING_NAME, "bob", asset( 1000, YANG_SYMBOL ) );
        
        const auto& bob_auth = db->get< account_authority_object, by_account >( "bob" );
        BOOST_REQUIRE( bob_auth.owner == acc_create.owner );
        
        
        BOOST_TEST_MESSAGE( "Changing bob's owner authority" );
        
        account_update_operation acc_update;
        acc_update.account = "bob";
        acc_update.owner = authority( 1, generate_private_key( "bad_key" ).get_public_key(), 1 );
        acc_update.memo_key = acc_create.memo_key;
        acc_update.json_metadata = "";
        
        tx.operations.clear();
        tx.signatures.clear();
        
        tx.operations.push_back( acc_update );
        sign( tx, generate_private_key( "bob_owner" ) );
        db->push_transaction( tx, 0 );
        
        BOOST_REQUIRE( bob_auth.owner == *acc_update.owner );
        
        
        BOOST_TEST_MESSAGE( "Creating recover request for bob with alice" );
        
        request_account_recovery_operation request;
        request.recovery_account = "alice";
        request.account_to_recover = "bob";
        request.new_owner_authority = authority( 1, generate_private_key( "new_key" ).get_public_key(), 1 );
        
        tx.operations.clear();
        tx.signatures.clear();
        
        tx.operations.push_back( request );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        BOOST_REQUIRE( bob_auth.owner == *acc_update.owner );
        
        
        BOOST_TEST_MESSAGE( "Recovering bob's account with original owner auth and new secret" );
        
        generate_blocks( db->head_block_time() + TAIYI_OWNER_UPDATE_LIMIT );
        
        recover_account_operation recover;
        recover.account_to_recover = "bob";
        recover.new_owner_authority = request.new_owner_authority;
        recover.recent_owner_authority = acc_create.owner;
        
        tx.operations.clear();
        tx.signatures.clear();
        
        tx.operations.push_back( recover );
        sign( tx, generate_private_key( "bob_owner" ) );
        sign( tx, generate_private_key( "new_key" ) );
        db->push_transaction( tx, 0 );
        const auto& owner1 = db->get< account_authority_object, by_account >("bob").owner;
        
        BOOST_REQUIRE( owner1 == recover.new_owner_authority );
        
        
        BOOST_TEST_MESSAGE( "Creating new recover request for a bogus key" );
        
        request.new_owner_authority = authority( 1, generate_private_key( "foo bar" ).get_public_key(), 1 );
        
        tx.operations.clear();
        tx.signatures.clear();
        
        tx.operations.push_back( request );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        
        BOOST_TEST_MESSAGE( "Testing failure when bob does not have new authority" );
        
        generate_blocks( db->head_block_time() + TAIYI_OWNER_UPDATE_LIMIT + fc::seconds( TAIYI_BLOCK_INTERVAL ) );
        
        recover.new_owner_authority = authority( 1, generate_private_key( "idontknow" ).get_public_key(), 1 );
        
        tx.operations.clear();
        tx.signatures.clear();
        
        tx.operations.push_back( recover );
        sign( tx, generate_private_key( "bob_owner" ) );
        sign( tx, generate_private_key( "idontknow" ) );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
        const auto& owner2 = db->get< account_authority_object, by_account >("bob").owner;
        BOOST_REQUIRE( owner2 == authority( 1, generate_private_key( "new_key" ).get_public_key(), 1 ) );
        
        
        BOOST_TEST_MESSAGE( "Testing failure when bob does not have old authority" );
        
        recover.recent_owner_authority = authority( 1, generate_private_key( "idontknow" ).get_public_key(), 1 );
        recover.new_owner_authority = authority( 1, generate_private_key( "foo bar" ).get_public_key(), 1 );
        
        tx.operations.clear();
        tx.signatures.clear();
        
        tx.operations.push_back( recover );
        sign( tx, generate_private_key( "foo bar" ) );
        sign( tx, generate_private_key( "idontknow" ) );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
        const auto& owner3 = db->get< account_authority_object, by_account >("bob").owner;
        BOOST_REQUIRE( owner3 == authority( 1, generate_private_key( "new_key" ).get_public_key(), 1 ) );
        
        
        BOOST_TEST_MESSAGE( "Testing using the same old owner auth again for recovery" );
        
        recover.recent_owner_authority = authority( 1, generate_private_key( "bob_owner" ).get_public_key(), 1 );
        recover.new_owner_authority = authority( 1, generate_private_key( "foo bar" ).get_public_key(), 1 );
        
        tx.operations.clear();
        tx.signatures.clear();
        
        tx.operations.push_back( recover );
        sign( tx, generate_private_key( "bob_owner" ) );
        sign( tx, generate_private_key( "foo bar" ) );
        db->push_transaction( tx, 0 );
        
        const auto& owner4 = db->get< account_authority_object, by_account >("bob").owner;
        BOOST_REQUIRE( owner4 == recover.new_owner_authority );
        
        BOOST_TEST_MESSAGE( "Creating a recovery request that will expire" );
        
        request.new_owner_authority = authority( 1, generate_private_key( "expire" ).get_public_key(), 1 );
        
        tx.operations.clear();
        tx.signatures.clear();
        
        tx.operations.push_back( request );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        const auto& request_idx = db->get_index< account_recovery_request_index >().indices();
        auto req_itr = request_idx.begin();
        
        BOOST_REQUIRE( req_itr->account_to_recover == "bob" );
        BOOST_REQUIRE( req_itr->new_owner_authority == authority( 1, generate_private_key( "expire" ).get_public_key(), 1 ) );
        BOOST_REQUIRE( req_itr->expires == db->head_block_time() + TAIYI_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD );
        auto expires = req_itr->expires;
        ++req_itr;
        BOOST_REQUIRE( req_itr == request_idx.end() );
        
        generate_blocks( time_point_sec( expires - TAIYI_BLOCK_INTERVAL ), true );
        
        const auto& new_request_idx = db->get_index< account_recovery_request_index >().indices();
        BOOST_REQUIRE( new_request_idx.begin() != new_request_idx.end() );
        
        generate_block();
        
        BOOST_REQUIRE( new_request_idx.begin() == new_request_idx.end() );
        
        recover.new_owner_authority = authority( 1, generate_private_key( "expire" ).get_public_key(), 1 );
        recover.recent_owner_authority = authority( 1, generate_private_key( "bob_owner" ).get_public_key(), 1 );
        
        tx.operations.clear();
        tx.signatures.clear();
        
        tx.operations.push_back( recover );
        tx.set_expiration( db->head_block_time() );
        sign( tx, generate_private_key( "expire" ) );
        sign( tx, generate_private_key( "bob_owner" ) );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
        const auto& owner5 = db->get< account_authority_object, by_account >("bob").owner;
        BOOST_REQUIRE( owner5 == authority( 1, generate_private_key( "foo bar" ).get_public_key(), 1 ) );
        
        BOOST_TEST_MESSAGE( "Expiring owner authority history" );
        
        acc_update.owner = authority( 1, generate_private_key( "new_key" ).get_public_key(), 1 );
        
        tx.operations.clear();
        tx.signatures.clear();
        
        tx.operations.push_back( acc_update );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, generate_private_key( "foo bar" ) );
        db->push_transaction( tx, 0 );
        
        generate_blocks( db->head_block_time() + ( TAIYI_OWNER_AUTH_RECOVERY_PERIOD - TAIYI_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD ) );
        generate_block();
        
        request.new_owner_authority = authority( 1, generate_private_key( "last key" ).get_public_key(), 1 );
        
        tx.operations.clear();
        tx.signatures.clear();
        
        tx.operations.push_back( request );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        recover.new_owner_authority = request.new_owner_authority;
        recover.recent_owner_authority = authority( 1, generate_private_key( "bob_owner" ).get_public_key(), 1 );
        
        tx.operations.clear();
        tx.signatures.clear();
        
        tx.operations.push_back( recover );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, generate_private_key( "bob_owner" ) );
        sign( tx, generate_private_key( "last key" ) );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
        const auto& owner6 = db->get< account_authority_object, by_account >("bob").owner;
        BOOST_REQUIRE( owner6 == authority( 1, generate_private_key( "new_key" ).get_public_key(), 1 ) );
        
        recover.recent_owner_authority = authority( 1, generate_private_key( "foo bar" ).get_public_key(), 1 );
        
        tx.operations.clear();
        tx.signatures.clear();
        
        tx.operations.push_back( recover );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, generate_private_key( "foo bar" ) );
        sign( tx, generate_private_key( "last key" ) );
        db->push_transaction( tx, 0 );
        const auto& owner7 = db->get< account_authority_object, by_account >("bob").owner;
        BOOST_REQUIRE( owner7 == authority( 1, generate_private_key( "last key" ).get_public_key(), 1 ) );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( change_recovery_account )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing change_recovery_account_operation" );
        
        ACTORS( (alice)(bob)(sam)(tyler) )
        
        auto change_recovery_account = [&]( const std::string& account_to_recover, const std::string& new_recovery_account )
        {
            change_recovery_account_operation op;
            op.account_to_recover = account_to_recover;
            op.new_recovery_account = new_recovery_account;
            
            signed_transaction tx;
            tx.operations.push_back( op );
            tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
            sign( tx, alice_private_key );
            db->push_transaction( tx, 0 );
        };
        
        auto recover_account = [&]( const std::string& account_to_recover, const fc::ecc::private_key& new_owner_key, const fc::ecc::private_key& recent_owner_key )
        {
            recover_account_operation op;
            op.account_to_recover = account_to_recover;
            op.new_owner_authority = authority( 1, public_key_type( new_owner_key.get_public_key() ), 1 );
            op.recent_owner_authority = authority( 1, public_key_type( recent_owner_key.get_public_key() ), 1 );
            
            signed_transaction tx;
            tx.operations.push_back( op );
            tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
            sign( tx, recent_owner_key );
            // only Alice -> throw
            TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
            tx.signatures.clear();
            sign( tx, new_owner_key );
            // only Sam -> throw
            TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
            sign( tx, recent_owner_key );
            // Alice+Sam -> OK
            db->push_transaction( tx, 0 );
        };
        
        auto request_account_recovery = [&]( const std::string& recovery_account, const fc::ecc::private_key& recovery_account_key, const std::string& account_to_recover, const public_key_type& new_owner_key )
        {
            request_account_recovery_operation op;
            op.recovery_account    = recovery_account;
            op.account_to_recover  = account_to_recover;
            op.new_owner_authority = authority( 1, new_owner_key, 1 );
            
            signed_transaction tx;
            tx.operations.push_back( op );
            tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
            sign( tx, recovery_account_key );
            db->push_transaction( tx, 0 );
        };
        
        auto change_owner = [&]( const std::string& account, const fc::ecc::private_key& old_private_key, const public_key_type& new_public_key )
        {
            account_update_operation op;
            op.account = account;
            op.owner = authority( 1, new_public_key, 1 );
            
            signed_transaction tx;
            tx.operations.push_back( op );
            tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
            sign( tx, old_private_key );
            db->push_transaction( tx, 0 );
        };
        
        // if either/both users do not exist, we shouldn't allow it
        TAIYI_REQUIRE_THROW( change_recovery_account("alice", "nobody"), fc::exception );
        TAIYI_REQUIRE_THROW( change_recovery_account("haxer", "sam"   ), fc::exception );
        TAIYI_REQUIRE_THROW( change_recovery_account("haxer", "nobody"), fc::exception );
        change_recovery_account("alice", "sam");
        
        fc::ecc::private_key alice_priv1 = fc::ecc::private_key::regenerate( fc::sha256::hash( "alice_k1" ) );
        fc::ecc::private_key alice_priv2 = fc::ecc::private_key::regenerate( fc::sha256::hash( "alice_k2" ) );
        public_key_type alice_pub1 = public_key_type( alice_priv1.get_public_key() );
        
        generate_blocks( db->head_block_time() + TAIYI_OWNER_AUTH_RECOVERY_PERIOD - fc::seconds( TAIYI_BLOCK_INTERVAL ), true );
        // cannot request account recovery until recovery account is approved
        TAIYI_REQUIRE_THROW( request_account_recovery( "sam", sam_private_key, "alice", alice_pub1 ), fc::exception );
        generate_blocks(1);
        // cannot finish account recovery until requested
        TAIYI_REQUIRE_THROW( recover_account( "alice", alice_priv1, alice_private_key ), fc::exception );
        // do the request
        request_account_recovery( "sam", sam_private_key, "alice", alice_pub1 );
        // can't recover with the current owner key
        TAIYI_REQUIRE_THROW( recover_account( "alice", alice_priv1, alice_private_key ), fc::exception );
        // unless we change it!
        change_owner( "alice", alice_private_key, public_key_type( alice_priv2.get_public_key() ) );
        recover_account( "alice", alice_priv1, alice_private_key );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( decline_adoring_rights_authorities )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: decline_adoring_rights_authorities" );
        
        decline_adoring_rights_operation op;
        op.account = "alice";
        
        flat_set< account_name_type > auths;
        flat_set< account_name_type > expected;
        
        op.get_required_active_authorities( auths );
        BOOST_REQUIRE( auths == expected );
        
        op.get_required_posting_authorities( auths );
        BOOST_REQUIRE( auths == expected );
        
        expected.insert( "alice" );
        op.get_required_owner_authorities( auths );
        BOOST_REQUIRE( auths == expected );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( decline_adoring_rights_apply )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: decline_adoring_rights_apply" );
        
        ACTORS( (alice)(bob) );
        generate_block();
        vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "10.000 YANG" ) );
        vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "10.000 YANG" ) );
        generate_block();
        
        account_siming_proxy_operation proxy;
        proxy.account = "bob";
        proxy.proxy = "alice";
        
        signed_transaction tx;
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.operations.push_back( proxy );
        sign( tx, bob_private_key );
        db->push_transaction( tx, 0 );
        
        
        decline_adoring_rights_operation op;
        op.account = "alice";
        
        
        BOOST_TEST_MESSAGE( "--- success" );
        tx.clear();
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        const auto& request_idx = db->get_index< decline_adoring_rights_request_index >().indices().get< by_account >();
        auto itr = request_idx.find( db->get_account( "alice" ).name );
        BOOST_REQUIRE( itr != request_idx.end() );
        BOOST_REQUIRE( itr->effective_date == db->head_block_time() + TAIYI_OWNER_AUTH_RECOVERY_PERIOD );
        
        
        BOOST_TEST_MESSAGE( "--- failure revoking voting rights with existing request" );
        generate_block();
        tx.clear();
        tx.operations.push_back( op );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, alice_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
        
        
        BOOST_TEST_MESSAGE( "--- successs cancelling a request" );
        op.decline = false;
        tx.clear();
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        itr = request_idx.find( db->get_account( "alice" ).name );
        BOOST_REQUIRE( itr == request_idx.end() );
        
        
        BOOST_TEST_MESSAGE( "--- failure cancelling a request that doesn't exist" );
        generate_block();
        tx.clear();
        tx.operations.push_back( op );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, alice_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
        
        
        BOOST_TEST_MESSAGE( "--- check account can vote during waiting period" );
        op.decline = true;
        tx.clear();
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        generate_blocks( db->head_block_time() + TAIYI_OWNER_AUTH_RECOVERY_PERIOD - fc::seconds( TAIYI_BLOCK_INTERVAL ), true );
        BOOST_REQUIRE( db->get_account( "alice" ).can_adore );
        siming_create( "alice", alice_private_key, "foo.bar", alice_private_key.get_public_key(), 0 );
        
        account_siming_adore_operation siming_adore;
        siming_adore.account = "alice";
        siming_adore.siming = "alice";
        tx.clear();
        tx.operations.push_back( siming_adore );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        
        BOOST_TEST_MESSAGE( "--- check account cannot vote after request is processed" );
        generate_block();
        BOOST_REQUIRE( !db->get_account( "alice" ).can_adore );
        validate_database();
        
        itr = request_idx.find( db->get_account( "alice" ).name );
        BOOST_REQUIRE( itr == request_idx.end() );
        
        const auto& siming_idx = db->get_index< siming_adore_index >().indices().get< by_account_siming >();
        auto siming_itr = siming_idx.find( boost::make_tuple( db->get_account( "alice" ).name, db->get_siming( "alice" ).owner ) );
        BOOST_REQUIRE( siming_itr == siming_idx.end() );
        
        tx.clear();
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.operations.push_back( siming_adore );
        sign( tx, alice_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
        
        proxy.account = "alice";
        proxy.proxy = "bob";
        tx.clear();
        tx.operations.push_back( proxy );
        sign( tx, alice_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( claim_reward_balance_validate )
{
    try
    {
        claim_reward_balance_operation op;
        op.account = "alice";
        op.reward_yang = ASSET( "0.000 YANG" );
        op.reward_qi = ASSET( "0.000000 QI" );
        
        
        BOOST_TEST_MESSAGE( "Testing all 0 amounts" );
        TAIYI_REQUIRE_THROW( op.validate(), fc::assert_exception );
        
        
        BOOST_TEST_MESSAGE( "Testing single reward claims" );
        op.reward_yang.amount = 1000;
        op.validate();
        
        op.reward_yang.amount = 0;
        TAIYI_REQUIRE_THROW( op.validate(), fc::assert_exception );

        op.reward_qi.amount = 1000;
        op.validate();
        
        op.reward_qi.amount = 0;
        
        
        BOOST_TEST_MESSAGE( "Testing wrong YANG symbol" );
        op.reward_yang = ASSET( "1.000000 QI" );
        TAIYI_REQUIRE_THROW( op.validate(), fc::assert_exception );
        
        BOOST_TEST_MESSAGE( "Testing wrong QI symbol" );
        op.reward_qi = ASSET( "1.000 YANG" );
        TAIYI_REQUIRE_THROW( op.validate(), fc::assert_exception );
        
        BOOST_TEST_MESSAGE( "Testing a single negative amount" );
        op.reward_yang.amount = -1000;
        TAIYI_REQUIRE_THROW( op.validate(), fc::assert_exception );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( claim_reward_balance_authorities )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: decline_voting_rights_authorities" );
        
        claim_reward_balance_operation op;
        op.account = "alice";
        
        flat_set< account_name_type > auths;
        flat_set< account_name_type > expected;
        
        op.get_required_owner_authorities( auths );
        BOOST_REQUIRE( auths == expected );
        
        op.get_required_active_authorities( auths );
        BOOST_REQUIRE( auths == expected );
        
        expected.insert( "alice" );
        op.get_required_posting_authorities( auths );
        BOOST_REQUIRE( auths == expected );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( claim_reward_balance_apply )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: claim_reward_balance_apply" );
        BOOST_TEST_MESSAGE( "--- Setting up test state" );
        
        ACTORS( (alice) )
        generate_block();
        
        db_plugin->debug_update( []( database& db ) {
            db.modify( db.get_account( "alice" ), []( account_object& a ) {
                a.reward_yang_balance = ASSET( "10.000 YANG" );
                a.reward_qi_balance = ASSET( "10.000000 QI" );
                a.reward_feigang_balance = ASSET( "10.000000 QI");
            });
            
            db.modify( db.get_dynamic_global_properties(), []( dynamic_global_property_object& gpo ) {
                gpo.current_supply += ASSET( "20.000 YANG" );
                gpo.pending_rewarded_qi += ASSET( "10.000000 QI" );
            });
        });
        
        generate_block();
        validate_database();
        
        auto alice_taiyi = db->get_account( "alice" ).balance;
        auto alice_qi = db->get_account( "alice" ).qi;
        
        
        BOOST_TEST_MESSAGE( "--- Attempting to claim more YANG than exists in the reward balance." );
        
        claim_reward_balance_operation op;
        signed_transaction tx;
        
        op.account = "alice";
        op.reward_yang = ASSET( "20.000 YANG" );
        op.reward_qi = ASSET( "0.000000 QI" );
        op.reward_feigang = ASSET( "0.000000 QI" );
        
        tx.operations.push_back( op );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, alice_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
        
        
        BOOST_TEST_MESSAGE( "--- Claiming a partial reward balance" );
        
        op.reward_yang = ASSET( "0.000 YANG" );
        op.reward_qi = ASSET( "5.000000 QI" );
        op.reward_feigang = ASSET( "5.000000 QI" );
        tx.clear();
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        BOOST_REQUIRE( db->get_account( "alice" ).balance == alice_taiyi + op.reward_yang );
        BOOST_REQUIRE( db->get_account( "alice" ).reward_yang_balance == ASSET( "10.000 YANG" ) );
        BOOST_REQUIRE( db->get_account( "alice" ).qi == alice_qi + op.reward_qi + op.reward_feigang );
        BOOST_REQUIRE( db->get_account( "alice" ).reward_qi_balance == ASSET( "5.000000 QI" ) );
        BOOST_REQUIRE( db->get_account( "alice" ).reward_feigang_balance == ASSET( "5.000000 QI" ) );
        validate_database();
        
        alice_qi += op.reward_qi + op.reward_feigang;
        
        
        BOOST_TEST_MESSAGE( "--- Claiming the full reward balance" );
        
        op.reward_yang = ASSET( "10.000 YANG" );
        tx.clear();
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        BOOST_REQUIRE( db->get_account( "alice" ).balance == alice_taiyi + op.reward_yang );
        BOOST_REQUIRE( db->get_account( "alice" ).reward_yang_balance == ASSET( "0.000 YANG" ) );
        BOOST_REQUIRE( db->get_account( "alice" ).qi == alice_qi + op.reward_qi + op.reward_feigang);
        BOOST_REQUIRE( db->get_account( "alice" ).reward_qi_balance == ASSET( "0.000000 QI" ) );
        BOOST_REQUIRE( db->get_account( "alice" ).reward_feigang_balance == ASSET( "0.000000 QI" ) );
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( delegate_qi_validate )
{
    try
    {
        delegate_qi_operation op;
        
        op.delegator = "alice";
        op.delegatee = "bob";
        op.qi = asset( -1, QI_SYMBOL );
        TAIYI_REQUIRE_THROW( op.validate(), fc::assert_exception );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( delegate_qi_authorities )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: delegate_qi_authorities" );
        signed_transaction tx;
        ACTORS( (alice)(bob) )
        vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "10000.000 YANG" ) );
        
        delegate_qi_operation op;
        op.qi = ASSET( "300.000000 QI");
        op.delegator = "alice";
        op.delegatee = "bob";
        
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.operations.push_back( op );
        
        BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
        
        BOOST_TEST_MESSAGE( "--- Test success with siming signature" );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        BOOST_TEST_MESSAGE( "--- Test failure when duplicate signatures" );
        tx.operations.clear();
        tx.signatures.clear();
        op.delegatee = "sam";
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        sign( tx, alice_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );
        
        BOOST_TEST_MESSAGE( "--- Test failure when signed by an additional signature not in the creator's authority" );
        tx.signatures.clear();
        sign( tx, init_account_priv_key );
        sign( tx, alice_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );
        
        BOOST_TEST_MESSAGE( "--- Test failure when signed by a signature not in the creator's authority" );
        tx.signatures.clear();
        sign( tx, init_account_priv_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( delegate_qi_apply )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: delegate_qi_apply" );
        signed_transaction tx;
        ACTORS( (alice)(bob)(charlie) )
        generate_block();
        
        vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "50000.000 YANG" ) );
        
        generate_block();
        
        db_plugin->debug_update( [=]( database& db ) {
            db.modify( db.get_siming_schedule_object(), [&]( siming_schedule_object& w ) {
                w.median_props.account_creation_fee = ASSET( "1.000 YANG" );
            });
        });
        
        generate_block();
        
        delegate_qi_operation op;
        op.qi = ASSET( "10000.000000 QI");
        op.delegator = "alice";
        op.delegatee = "bob";
        
        util::manabar old_manabar = db->get_account( "alice" ).manabar;
        util::manabar_params params( util::get_effective_qi( db->get_account( "alice" ) ), TAIYI_MANA_REGENERATION_SECONDS );
        old_manabar.regenerate_mana( params, db->head_block_time() );

        util::manabar old_bob_manabar = db->get_account( "bob" ).manabar;
        params.max_mana = util::get_effective_qi( db->get_account( "bob" ) );
        old_bob_manabar.regenerate_mana( params, db->head_block_time() );

        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        generate_blocks( 1 );
        const account_object& alice_acc = db->get_account( "alice" );
        const account_object& bob_acc = db->get_account( "bob" );
        
        BOOST_REQUIRE( alice_acc.delegated_qi == ASSET( "10000.000000 QI" ) );
        BOOST_REQUIRE( alice_acc.manabar.current_mana == old_manabar.current_mana - op.qi.amount.value );
        BOOST_REQUIRE( bob_acc.received_qi == ASSET( "10000.000000 QI" ) );
        BOOST_REQUIRE( bob_acc.manabar.current_mana == old_bob_manabar.current_mana + op.qi.amount.value );

        BOOST_TEST_MESSAGE( "--- Test that the delegation object is correct. " );
        auto delegation = db->find< qi_delegation_object, by_delegation >( boost::make_tuple( op.delegator, op.delegatee ) );
        
        BOOST_REQUIRE( delegation != nullptr );
        BOOST_REQUIRE( delegation->delegator == op.delegator);
        BOOST_REQUIRE( delegation->qi  == ASSET( "10000.000000 QI"));
        
        old_manabar = db->get_account( "alice" ).manabar;
        params.max_mana = util::get_effective_qi( db->get_account( "alice" ) );
        old_manabar.regenerate_mana( params, db->head_block_time() );

        old_bob_manabar = db->get_account( "bob" ).manabar;
        params.max_mana = util::get_effective_qi( db->get_account( "bob" ) );
        old_bob_manabar.regenerate_mana( params, db->head_block_time() );

        int64_t delta = 10000000000;
        
        validate_database();
        tx.clear();
        op.qi.amount += delta;
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        generate_blocks(1);
        
        idump( (alice_acc.manabar)(old_manabar)(delta) );

        BOOST_REQUIRE( delegation != nullptr );
        BOOST_REQUIRE( delegation->delegator == op.delegator);
        BOOST_REQUIRE( delegation->qi == ASSET( "20000.000000 QI"));
        BOOST_REQUIRE( alice_acc.delegated_qi == ASSET( "20000.000000 QI"));
        BOOST_REQUIRE( alice_acc.manabar.current_mana == old_manabar.current_mana - delta );
        BOOST_REQUIRE( bob_acc.received_qi == ASSET( "20000.000000 QI"));
        BOOST_REQUIRE( bob_acc.manabar.current_mana == old_bob_manabar.current_mana + delta );

        BOOST_TEST_MESSAGE( "--- Test failure delegating delgated QI." );
        
        op.delegator = "bob";
        op.delegatee = "charlie";
        tx.clear();
        tx.operations.push_back( op );
        sign( tx, bob_private_key );
        BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
        
        
        //TODO: BOOST_TEST_MESSAGE( "--- Test that effective qi shares is accurate and being applied." );
        tx.operations.clear();
        tx.signatures.clear();
        
        ACTORS( (sam)(dave) )
        generate_block();

        vest( TAIYI_INIT_SIMING_NAME, "sam", ASSET( "1000.000 YANG" ) );

        generate_block();

        auto sam_vest = db->get_account( "sam" ).qi;

        BOOST_TEST_MESSAGE( "--- Test failure when delegating 0 QI" );
        tx.clear();
        op.delegator = "sam";
        op.delegatee = "dave";
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, sam_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx ), fc::assert_exception );

        BOOST_TEST_MESSAGE( "--- Testing failure delegating more qi shares than account has." );
        tx.clear();
        op.qi = asset( sam_vest.amount + 1, QI_SYMBOL );
        tx.operations.push_back( op );
        sign( tx, sam_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx ), fc::assert_exception );

        BOOST_TEST_MESSAGE( "--- Testing failure delegating when there is not enough mana" );

        generate_block();
        db_plugin->debug_update( [=]( database& db ) {
            db.modify( db.get_account( "sam" ), [&]( account_object& a ) {
                a.manabar.current_mana = a.manabar.current_mana * 3 / 4;
                a.manabar.last_update_time = db.head_block_time().sec_since_epoch();
            });
        });
        
        op.qi = sam_vest;
        tx.clear();
        tx.operations.push_back( op );
        sign( tx, sam_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx ), fc::assert_exception );

        BOOST_TEST_MESSAGE( "--- Test failure delegating qi shares that are part of a power down" );
        generate_block();
        db_plugin->debug_update( [=]( database& db ) {
            db.modify( db.get_account( "sam" ), [&]( account_object& a ) {
                a.manabar.current_mana = util::get_effective_qi( a ) / 4;
                a.manabar.last_update_time = db.head_block_time().sec_since_epoch();
            });
        });
        
        tx.clear();
        sam_vest = asset( sam_vest.amount / 2, QI_SYMBOL );
        withdraw_qi_operation withdraw;
        withdraw.account = "sam";
        withdraw.qi = sam_vest;
        tx.operations.push_back( withdraw );
        sign( tx, sam_private_key );
        db->push_transaction( tx, 0 );
        
        tx.clear();
        op.qi = asset( sam_vest.amount + 2, QI_SYMBOL );
        tx.operations.push_back( op );
        sign( tx, sam_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx ), fc::assert_exception );
        
        tx.clear();
        withdraw.qi = ASSET( "0.000000 QI" );
        tx.operations.push_back( withdraw );
        sign( tx, sam_private_key );
        db->push_transaction( tx, 0 );

        BOOST_TEST_MESSAGE( "--- Test failure powering down qi shares that are delegated" );
        db_plugin->debug_update( [=]( database& db ) {
            db.modify( db.get_account( "sam" ), [&]( account_object& a ) {
                a.manabar.current_mana = sam_vest.amount.value * 2;
                a.manabar.last_update_time = db.head_block_time().sec_since_epoch();
            });
        });

        sam_vest.amount += 1000;
        op.qi = sam_vest;
        tx.clear();
        tx.operations.push_back( op );
        sign( tx, sam_private_key );
        db->push_transaction( tx, 0 );

        tx.clear();
        withdraw.qi = asset( sam_vest.amount, QI_SYMBOL );
        tx.operations.push_back( withdraw );
        sign( tx, sam_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx ), fc::assert_exception );

        BOOST_TEST_MESSAGE( "--- Remove a delegation and ensure it is returned after 1 week" );

        util::manabar old_sam_manabar = db->get_account( "sam" ).manabar;
        util::manabar old_dave_manabar = db->get_account( "dave" ).manabar;

        util::manabar_params sam_params( util::get_effective_qi( db->get_account( "sam" ) ), TAIYI_MANA_REGENERATION_SECONDS );
        util::manabar_params dave_params( util::get_effective_qi( db->get_account( "dave" ) ), TAIYI_MANA_REGENERATION_SECONDS );

        tx.clear();
        op.qi = ASSET( "0.000000 QI" );
        tx.operations.push_back( op );
        sign( tx, sam_private_key );
        db->push_transaction( tx, 0 );

        auto exp_obj = db->get_index< qi_delegation_expiration_index, by_id >().begin();
        auto end = db->get_index< qi_delegation_expiration_index, by_id >().end();
        auto gpo = db->get_dynamic_global_properties();

        BOOST_REQUIRE( gpo.delegation_return_period == TAIYI_DELEGATION_RETURN_PERIOD );

        BOOST_REQUIRE( exp_obj != end );
        BOOST_REQUIRE( exp_obj->delegator == "sam" );
        BOOST_REQUIRE( exp_obj->qi == sam_vest );
        BOOST_REQUIRE( exp_obj->expiration == db->head_block_time() + gpo.delegation_return_period );
        BOOST_REQUIRE( db->get_account( "sam" ).delegated_qi == sam_vest );
        BOOST_REQUIRE( db->get_account( "dave" ).received_qi == ASSET( "0.000000 QI" ) );
        delegation = db->find< qi_delegation_object, by_delegation >( boost::make_tuple( op.delegator, op.delegatee ) );
        BOOST_REQUIRE( delegation == nullptr );

        old_sam_manabar.regenerate_mana( sam_params, db->head_block_time() );
        sam_params.max_mana /= 4;

        old_dave_manabar.regenerate_mana( dave_params, db->head_block_time() );
        dave_params.max_mana /= 4;

        BOOST_REQUIRE( db->get_account( "sam" ).manabar.current_mana == old_sam_manabar.current_mana );
        BOOST_REQUIRE( db->get_account( "dave" ).manabar.current_mana == old_dave_manabar.current_mana - sam_vest.amount.value );

        old_sam_manabar = db->get_account( "sam" ).manabar;
        old_dave_manabar = db->get_account( "dave" ).manabar;

        sam_params.max_mana = util::get_effective_qi( db->get_account( "sam" ) );
        dave_params.max_mana = util::get_effective_qi( db->get_account( "dave" ) );

        generate_blocks( exp_obj->expiration + TAIYI_BLOCK_INTERVAL );

        old_sam_manabar.regenerate_mana( sam_params, db->head_block_time() );
        sam_params.max_mana /= 4;

        old_dave_manabar.regenerate_mana( dave_params, db->head_block_time() );
        dave_params.max_mana /= 4;

        exp_obj = db->get_index< qi_delegation_expiration_index, by_id >().begin();
        end = db->get_index< qi_delegation_expiration_index, by_id >().end();

        BOOST_REQUIRE( exp_obj == end );
        BOOST_REQUIRE( db->get_account( "sam" ).delegated_qi == ASSET( "0.000000 QI" ) );
        BOOST_REQUIRE( db->get_account( "sam" ).manabar.current_mana == old_sam_manabar.current_mana + sam_vest.amount.value );
        BOOST_REQUIRE( db->get_account( "dave" ).manabar.current_mana == old_dave_manabar.current_mana );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( issue_971_qi_removal )
{
    // This is a regression test specifically for issue #971
    try
    {
        BOOST_TEST_MESSAGE( "Test Issue 971 Vesting Removal" );
        ACTORS( (alice)(bob) )
        generate_block();
        
        vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "50000.000 YANG" ) );
        
        generate_block();
        
        db_plugin->debug_update( [=]( database& db ) {
            db.modify( db.get_siming_schedule_object(), [&]( siming_schedule_object& w ) {
                w.median_props.account_creation_fee = ASSET( "1.000 YANG" );
            });
        });
        
        generate_block();
        
        signed_transaction tx;
        delegate_qi_operation op;
        op.qi = ASSET( "10000.000000 QI");
        op.delegator = "alice";
        op.delegatee = "bob";
        
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        generate_block();
        const account_object& alice_acc = db->get_account( "alice" );
        const account_object& bob_acc = db->get_account( "bob" );
        
        BOOST_REQUIRE( alice_acc.delegated_qi == ASSET( "10000.000000 QI"));
        BOOST_REQUIRE( bob_acc.received_qi == ASSET( "10000.000000 QI"));
        
        generate_block();
        
        db_plugin->debug_update( [=]( database& db ) {
            db.modify( db.get_siming_schedule_object(), [&]( siming_schedule_object& w ) {
                w.median_props.account_creation_fee = ASSET( "100.000 YANG" );
            });
        });
        
        generate_block();
        
        op.qi = ASSET( "0.000000 QI" );
        
        tx.clear();
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        generate_block();
        
        BOOST_REQUIRE( alice_acc.delegated_qi == ASSET( "10000.000000 QI"));
        BOOST_REQUIRE( bob_acc.received_qi == ASSET( "0.000000 QI"));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( siming_set_properties_validate )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: siming_set_properties_validate" );
        
        ACTORS( (alice) )
        fund( "alice", 10000 );
        private_key_type signing_key = generate_private_key( "old_key" );
        
        siming_update_operation op;
        op.owner = "alice";
        op.url = "foo.bar";
        op.fee = ASSET( "1.000 YANG" );
        op.block_signing_key = signing_key.get_public_key();
        op.props.account_creation_fee = legacy_taiyi_asset::from_asset( asset(TAIYI_MIN_ACCOUNT_CREATION_FEE + 10, YANG_SYMBOL) );
        op.props.maximum_block_size = TAIYI_MIN_BLOCK_SIZE_LIMIT + 100;
        
        signed_transaction tx;
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        generate_block();
        
        BOOST_TEST_MESSAGE( "--- failure when signing key is not present" );
        siming_set_properties_operation prop_op;
        prop_op.owner = "alice";
        TAIYI_REQUIRE_THROW( prop_op.validate(), fc::assert_exception );
        
        BOOST_TEST_MESSAGE( "--- success when signing key is present" );
        prop_op.props[ "key" ] = fc::raw::pack_to_vector( signing_key.get_public_key() );
        prop_op.validate();
        
        BOOST_TEST_MESSAGE( "--- failure when setting account_creation_fee with incorrect symbol" );
        prop_op.props[ "account_creation_fee" ] = fc::raw::pack_to_vector( ASSET( "2.000000 QI" ) );
        TAIYI_REQUIRE_THROW( prop_op.validate(), fc::assert_exception );
        
        BOOST_TEST_MESSAGE( "--- failure when setting maximum_block_size below TAIYI_MIN_BLOCK_SIZE_LIMIT" );
        prop_op.props.erase( "account_creation_fee" );
        prop_op.props[ "maximum_block_size" ] = fc::raw::pack_to_vector( TAIYI_MIN_BLOCK_SIZE_LIMIT - 1 );
        TAIYI_REQUIRE_THROW( prop_op.validate(), fc::assert_exception );
        
        BOOST_TEST_MESSAGE( "--- failure when setting new url with length of zero" );
        prop_op.props.erase( "maximum_block_size" );
        prop_op.props[ "url" ] = fc::raw::pack_to_vector( "" );
        TAIYI_REQUIRE_THROW( prop_op.validate(), fc::assert_exception );
        
        BOOST_TEST_MESSAGE( "--- failure when setting new url with non UTF-8 character" );
        prop_op.props[ "url" ].clear();
        prop_op.props[ "url" ] = fc::raw::pack_to_vector( "\xE0\x80\x80" );
        TAIYI_REQUIRE_THROW( prop_op.validate(), fc::assert_exception );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( siming_set_properties_authorities )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: siming_set_properties_authorities" );
        
        siming_set_properties_operation op;
        op.owner = "alice";
        op.props[ "key" ] = fc::raw::pack_to_vector( generate_private_key( "key" ).get_public_key() );
        
        flat_set< account_name_type > auths;
        flat_set< account_name_type > expected;
        
        op.get_required_owner_authorities( auths );
        BOOST_REQUIRE( auths == expected );
        
        op.get_required_active_authorities( auths );
        BOOST_REQUIRE( auths == expected );
        
        op.get_required_posting_authorities( auths );
        BOOST_REQUIRE( auths == expected );
        
        vector< authority > key_auths;
        vector< authority > expected_keys;
        expected_keys.push_back( authority( 1, generate_private_key( "key" ).get_public_key(), 1 ) );
        op.get_required_authorities( key_auths );
        BOOST_REQUIRE( key_auths == expected_keys );
        
        op.props.erase( "key" );
        key_auths.clear();
        expected_keys.clear();
        
        op.get_required_owner_authorities( auths );
        BOOST_REQUIRE( auths == expected );
        
        op.get_required_active_authorities( auths );
        BOOST_REQUIRE( auths == expected );
        
        op.get_required_posting_authorities( auths );
        BOOST_REQUIRE( auths == expected );
        
        expected_keys.push_back( authority( 1, TAIYI_NULL_ACCOUNT, 1 ) );
        op.get_required_authorities( key_auths );
        BOOST_REQUIRE( key_auths == expected_keys );
        
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( siming_set_properties_apply )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: siming_set_properties_apply" );
        
        ACTORS( (alice) )
        fund( "alice", 10000 );
        private_key_type signing_key = generate_private_key( "old_key" );
        
        siming_update_operation op;
        op.owner = "alice";
        op.url = "foo.bar";
        op.fee = ASSET( "1.000 YANG" );
        op.block_signing_key = signing_key.get_public_key();
        op.props.account_creation_fee = legacy_taiyi_asset::from_asset( asset(TAIYI_MIN_ACCOUNT_CREATION_FEE + 10, YANG_SYMBOL) );
        op.props.maximum_block_size = TAIYI_MIN_BLOCK_SIZE_LIMIT + 100;
        
        signed_transaction tx;
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.operations.push_back( op );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        
        BOOST_TEST_MESSAGE( "--- Test setting runtime parameters" );
        
        // Setting account_creation_fee
        const siming_object& alice_siming = db->get_siming( "alice" );
        siming_set_properties_operation prop_op;
        prop_op.owner = "alice";
        prop_op.props[ "key" ] = fc::raw::pack_to_vector( signing_key.get_public_key() );
        prop_op.props[ "account_creation_fee" ] = fc::raw::pack_to_vector( ASSET( "2.000 YANG" ) );
        tx.clear();
        tx.operations.push_back( prop_op );
        sign( tx, signing_key );
        db->push_transaction( tx, 0 );
        BOOST_REQUIRE( alice_siming.props.account_creation_fee == ASSET( "2.000 YANG" ) );
        
        // Setting maximum_block_size
        prop_op.props.erase( "account_creation_fee" );
        prop_op.props[ "maximum_block_size" ] = fc::raw::pack_to_vector( TAIYI_MIN_BLOCK_SIZE_LIMIT + 1 );
        tx.clear();
        tx.operations.push_back( prop_op );
        sign( tx, signing_key );
        db->push_transaction( tx, 0 );
        BOOST_REQUIRE( alice_siming.props.maximum_block_size == TAIYI_MIN_BLOCK_SIZE_LIMIT + 1 );
        
        // Setting new signing_key
        private_key_type old_signing_key = signing_key;
        signing_key = generate_private_key( "new_key" );
        public_key_type alice_pub = signing_key.get_public_key();
        prop_op.props.erase( "maximum_block_size" );
        prop_op.props[ "new_signing_key" ] = fc::raw::pack_to_vector( alice_pub );
        tx.clear();
        tx.operations.push_back( prop_op );
        sign( tx, old_signing_key );
        db->push_transaction( tx, 0 );
        BOOST_REQUIRE( alice_siming.signing_key == alice_pub );
        
        // Setting new url
        prop_op.props.erase( "new_signing_key" );
        prop_op.props[ "key" ] = fc::raw::pack_to_vector( signing_key.get_public_key() );
        prop_op.props[ "url" ] = fc::raw::pack_to_vector( "foo.bar" );
        tx.clear();
        tx.operations.push_back( prop_op );
        sign( tx, signing_key );
        db->push_transaction( tx, 0 );
        BOOST_REQUIRE( alice_siming.url == "foo.bar" );
        
        // Setting new extranious_property
        prop_op.props.erase( "url" );
        prop_op.props[ "extraneous_property" ] = fc::raw::pack_to_vector( "foo" );
        tx.clear();
        tx.operations.push_back( prop_op );
        sign( tx, signing_key );
        db->push_transaction( tx, 0 );
        
        BOOST_TEST_MESSAGE( "--- Testing failure when 'key' does not match siming signing key" );
        prop_op.props.erase( "extranious_property" );
        prop_op.props[ "key" ].clear();
        prop_op.props[ "key" ] = fc::raw::pack_to_vector( old_signing_key.get_public_key() );
        tx.clear();
        tx.operations.push_back( prop_op );
        sign( tx, old_signing_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
        
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_auth_tests )
{
    try
    {
        ACTORS( (alice)(bob)(charlie) )
        generate_block();
        
        fund( "alice", ASSET( "20.000 YANG" ) );
        fund( "bob", ASSET( "20.000 YANG" ) );
        fund( "charlie", ASSET( "20.000 YANG" ) );
        vest( TAIYI_INIT_SIMING_NAME, "alice" , ASSET( "10.000 YANG" ) );
        vest( TAIYI_INIT_SIMING_NAME, "bob" , ASSET( "10.000 YANG" ) );
        vest( TAIYI_INIT_SIMING_NAME, "charlie" , ASSET( "10.000 YANG" ) );
        generate_block();
        
        private_key_type bob_active_private_key = bob_private_key;
        private_key_type bob_posting_private_key = generate_private_key( "bob_posting" );
        private_key_type charlie_active_private_key = charlie_private_key;
        private_key_type charlie_posting_private_key = generate_private_key( "charlie_posting" );
        
        db_plugin->debug_update( [=]( database& db ) {
            db.modify( db.get< account_authority_object, by_account >( "alice"), [&]( account_authority_object& auth ) {
                auth.active.add_authority( "bob", 1 );
                auth.posting.add_authority( "charlie", 1 );
            });
            
            db.modify( db.get< account_authority_object, by_account >( "bob" ), [&]( account_authority_object& auth ) {
                auth.posting = authority( 1, bob_posting_private_key.get_public_key(), 1 );
            });
            
            db.modify( db.get< account_authority_object, by_account >( "charlie" ), [&]( account_authority_object& auth ) {
                auth.posting = authority( 1, charlie_posting_private_key.get_public_key(), 1 );
            });
        });
        
        generate_block();
        
        signed_transaction tx;
        transfer_operation transfer;
        
        transfer.from = "alice";
        transfer.to = "bob";
        transfer.amount = ASSET( "1.000 YANG" );
        tx.operations.push_back( transfer );
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.signatures.clear();
        sign( tx, bob_active_private_key );
        db->push_transaction( tx, 0 );
        
        generate_block();
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.signatures.clear();
        sign( tx, bob_posting_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
        
        generate_block();
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.signatures.clear();
        sign( tx, charlie_active_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
        
        generate_block();
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.signatures.clear();
        sign( tx, charlie_posting_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
        
        custom_json_operation json;
        json.required_posting_auths.insert( "alice" );
        json.json = "{\"foo\":\"bar\"}";
        tx.operations.clear();
        tx.signatures.clear();
        tx.operations.push_back( json );
        sign( tx, bob_active_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_posting_auth );
        
        generate_block();
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.signatures.clear();
        sign( tx, bob_posting_private_key );
        db->push_transaction( tx, 0 );
        
        generate_block();
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.signatures.clear();
        sign( tx, charlie_active_private_key );
        TAIYI_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_posting_auth );
        
        generate_block();
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        tx.signatures.clear();
        sign( tx, charlie_posting_private_key );
        db->push_transaction( tx, 0 );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

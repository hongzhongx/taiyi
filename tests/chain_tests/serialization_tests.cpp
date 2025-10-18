#include <boost/test/unit_test.hpp>

#include <chain/taiyi_fwd.hpp>

#include <chain/taiyi_objects.hpp>
#include <chain/database.hpp>

#include <plugins/baiyujing_api/baiyujing_api_legacy_asset.hpp>
#include <plugins/baiyujing_api/baiyujing_api_legacy_objects.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/reflect/variant.hpp>

#include "../db_fixture/database_fixture.hpp"

#include <cmath>

using namespace taiyi;
using namespace taiyi::chain;
using namespace taiyi::protocol;

BOOST_FIXTURE_TEST_SUITE( serialization_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( serialization_raw_test )
{
    try {
        ACTORS( (alice)(bob) )
        transfer_operation op;
        op.from = "alice";
        op.to = "bob";
        op.amount = asset(100,YANG_SYMBOL);
        
        trx.operations.push_back( op );
        auto packed = fc::raw::pack_to_vector( trx );
        signed_transaction unpacked = fc::raw::unpack_from_vector<signed_transaction>(packed);
        unpacked.validate();
        BOOST_CHECK( trx.digest() == unpacked.digest() );
    }
    catch (const fc::exception& e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE( serialization_json_test )
{
    try {
        ACTORS( (alice)(bob) )
        transfer_operation op;
        op.from = "alice";
        op.to = "bob";
        op.amount = asset(100,YANG_SYMBOL);
        
        fc::variant test(op.amount);
        auto tmp = test.as<asset>();
        BOOST_REQUIRE( tmp == op.amount );
        
        trx.operations.push_back( op );
        fc::variant packed(trx);
        signed_transaction unpacked = packed.as<signed_transaction>();
        unpacked.validate();
        BOOST_CHECK( trx.digest() == unpacked.digest() );
    }
    catch (const fc::exception& e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE( legacy_asset_test )
{
    try
    {
        using taiyi::plugins::baiyujing_api::legacy_asset;
        
        BOOST_CHECK_EQUAL( legacy_asset().symbol.decimals(), 3 );
        BOOST_CHECK_EQUAL( legacy_asset().to_string(), "0.000 YANG" );
        
        BOOST_TEST_MESSAGE( "Asset Test" );
        legacy_asset taiyi = legacy_asset::from_string( "123.456 YANG" );
        legacy_asset tmp = legacy_asset::from_string( "0.456 YANG" );
        BOOST_CHECK_EQUAL( tmp.amount.value, 456 );
        tmp = legacy_asset::from_string( "0.056 YANG" );
        BOOST_CHECK_EQUAL( tmp.amount.value, 56 );
        
        BOOST_CHECK_EQUAL( taiyi.amount.value, 123456 );
        BOOST_CHECK_EQUAL( taiyi.symbol.decimals(), 3 );
        BOOST_CHECK_EQUAL( taiyi.to_string(), "123.456 YANG" );
        BOOST_CHECK( taiyi.symbol == YANG_SYMBOL );
        BOOST_CHECK_EQUAL( legacy_asset::from_asset( asset( 50, YANG_SYMBOL ) ).to_string(), "0.050 YANG" );
        BOOST_CHECK_EQUAL( legacy_asset::from_asset( asset(50000, YANG_SYMBOL ) ) .to_string(), "50.000 YANG" );
        
        BOOST_CHECK_THROW( legacy_asset::from_string( "1.00000000000000000000 YANG" ), fc::exception );
        BOOST_CHECK_THROW( legacy_asset::from_string( "1.000TESTS" ), fc::exception );
        BOOST_CHECK_THROW( legacy_asset::from_string( "1. 333 YANG" ), fc::exception ); // Fails because symbol is '333 TAIYI', which is too long
        BOOST_CHECK_THROW( legacy_asset::from_string( "1 .333 YANG" ), fc::exception );
        BOOST_CHECK_THROW( legacy_asset::from_string( "1. 333 X" ), fc::exception ); // Not a system asset
        BOOST_CHECK_THROW( legacy_asset::from_string( "1 .333 X" ), fc::exception );
        BOOST_CHECK_THROW( legacy_asset::from_string( "1 .333" ), fc::exception );
        BOOST_CHECK_THROW( legacy_asset::from_string( "1 1.1" ), fc::exception );
        BOOST_CHECK_THROW( legacy_asset::from_string( "11111111111111111111111111111111111111111111111 YANG" ), fc::exception );
        BOOST_CHECK_THROW( legacy_asset::from_string( "1.1.1 YANG" ), fc::exception );
        BOOST_CHECK_THROW( legacy_asset::from_string( "1.abc YANG" ), fc::exception );
        BOOST_CHECK_THROW( legacy_asset::from_string( " YANG" ), fc::exception );
        BOOST_CHECK_THROW( legacy_asset::from_string( "YANG" ), fc::exception );
        BOOST_CHECK_THROW( legacy_asset::from_string( "1.333" ), fc::exception );
        BOOST_CHECK_THROW( legacy_asset::from_string( "1.333 " ), fc::exception );
        BOOST_CHECK_THROW( legacy_asset::from_string( "" ), fc::exception );
        BOOST_CHECK_THROW( legacy_asset::from_string( " " ), fc::exception );
        BOOST_CHECK_THROW( legacy_asset::from_string( "  " ), fc::exception );
        BOOST_CHECK_THROW( legacy_asset::from_string( "100 YANG" ), fc::exception ); // Does not match system asset precision
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( asset_test )
{
    try
    {
        std::string s;
        
        BOOST_CHECK_EQUAL( asset().symbol.decimals(), 3 );
        BOOST_CHECK_EQUAL( fc::json::to_string( asset() ), "{\"amount\":\"0\",\"precision\":3,\"fai\":\"@@000000021\"}" );
        
        asset taiyi = fc::json::from_string( "{\"amount\":\"123456\",    \"precision\":3, \"fai\":\"@@000000021\"}" ).as< asset >();
        asset qi = fc::json::from_string( "{\"amount\":\"123456789\", \"precision\":6, \"fai\":\"@@000000037\"}" ).as< asset >();
        asset tmp =   fc::json::from_string( "{\"amount\":\"456\",       \"precision\":3, \"fai\":\"@@000000021\"}" ).as< asset >();
        BOOST_CHECK_EQUAL( tmp.amount.value, 456 );
        tmp = fc::json::from_string( "{\"amount\":\"56\", \"precision\":3, \"fai\":\"@@000000021\"}" ).as< asset >();
        BOOST_CHECK_EQUAL( tmp.amount.value, 56 );
        
        BOOST_CHECK_EQUAL( taiyi.amount.value, 123456 );
        BOOST_CHECK_EQUAL( taiyi.symbol.decimals(), 3 );
        BOOST_CHECK_EQUAL( fc::json::to_string( taiyi ), "{\"amount\":\"123456\",\"precision\":3,\"fai\":\"@@000000021\"}" );
        BOOST_CHECK( taiyi.symbol.asset_num == TAIYI_ASSET_NUM_YANG );
        BOOST_CHECK_EQUAL( fc::json::to_string( asset( 50, YANG_SYMBOL ) ), "{\"amount\":\"50\",\"precision\":3,\"fai\":\"@@000000021\"}" );
        BOOST_CHECK_EQUAL( fc::json::to_string( asset( 50000, YANG_SYMBOL ) ), "{\"amount\":\"50000\",\"precision\":3,\"fai\":\"@@000000021\"}" );
        
        BOOST_CHECK_EQUAL( qi.amount.value, 123456789 );
        BOOST_CHECK_EQUAL( qi.symbol.decimals(), 6 );
        BOOST_CHECK_EQUAL( fc::json::to_string( qi ), "{\"amount\":\"123456789\",\"precision\":6,\"fai\":\"@@000000037\"}" );
        BOOST_CHECK( qi.symbol.asset_num == TAIYI_ASSET_NUM_QI );
        BOOST_CHECK_EQUAL( fc::json::to_string( asset( 50, QI_SYMBOL ) ), "{\"amount\":\"50\",\"precision\":6,\"fai\":\"@@000000037\"}" );
        BOOST_CHECK_EQUAL( fc::json::to_string( asset( 50000, QI_SYMBOL ) ), "{\"amount\":\"50000\",\"precision\":6,\"fai\":\"@@000000037\"}" );
        
        // amount overflow
        BOOST_CHECK_THROW( fc::json::from_string( "{\"amount\":\"9223372036854775808\",\"precision\":3,\"fai\":\"@@000000021\"}" ).as< asset >(), fc::exception );
        // amount underflow
        BOOST_CHECK_THROW( fc::json::from_string( "{\"amount\":\"-1\",\"precision\":3,\"fai\":\"@@000000021\"}" ).as< asset >(), fc::exception );
        
        // precision overflow
        BOOST_CHECK_THROW( fc::json::from_string( "{\"amount\":\"10\",\"precision\":256,\"fai\":\"@@000000021\"}" ).as< asset >(), fc::exception );
        // precision underflow
        BOOST_CHECK_THROW( fc::json::from_string( "{\"amount\":\"10\",\"precision\":-1,\"fai\":\"@@000000021\"}" ).as< asset >(), fc::exception );
        
        // Check wrong size tuple
        BOOST_CHECK_THROW( fc::json::from_string( "{\"amount\":\"0\",3]" ).as< asset >(), fc::exception );
        BOOST_CHECK_THROW( fc::json::from_string( "{\"amount\":\"0\",\"precision\":3,\"fai\":\"@@000000021\",1}" ).as< asset >(), fc::exception );
        
        // Check non-numeric characters in amount
        BOOST_CHECK_THROW( fc::json::from_string( "{\"amount\":\"foobar\",\"precision\":3,\"fai\":\"@@000000021\"}" ).as< asset >(), fc::exception );
        BOOST_CHECK_THROW( fc::json::from_string( "{\"amount\":\"10a\",\"precision\":3,\"fai\":\"@@000000021\"}" ).as< asset >(), fc::exception );
        BOOST_CHECK_THROW( fc::json::from_string( "{\"amount\":\"10a00\",\"precision\":3,\"fai\":\"@@000000021\"}" ).as< asset >(), fc::exception );
        
        // Check hex value
        BOOST_CHECK_THROW( fc::json::from_string( "{\"amount\":\"0x8000\",\"precision\":3,\"fai\":\"@@000000021\"}" ).as< asset >(), fc::exception );
        
        // Check octal value
        BOOST_CHECK_EQUAL( fc::json::from_string( "{\"amount\":\"08000\",\"precision\":3,\"fai\":\"@@000000021\"}" ).as< asset >().amount.value, 8000 );
    }
    FC_LOG_AND_RETHROW()
}

template< typename T >
std::string hex_bytes( const T& obj )
{
    std::vector<char> data = fc::raw::pack_to_vector( obj );
    std::ostringstream ss;
    static const char hexdigits[] = "0123456789abcdef";
    
    for( char c : data )
    {
        ss << hexdigits[((c >> 4) & 0x0F)] << hexdigits[c & 0x0F] << ' ';
    }
    return ss.str();
}

void old_pack_symbol(vector<char>& v, asset_symbol_type sym)
{
    //64 bit
    if( sym == YANG_SYMBOL )
    {
        v.push_back('\x03'); v.push_back('Y' ); v.push_back('A' ); v.push_back('N' );
        v.push_back('G'   ); v.push_back('\0'); v.push_back('\0'); v.push_back('\0');
        // 03 59 41 4e 47 00 00 00
    }
    else if( sym == QI_SYMBOL )
    {
        v.push_back('\x06'); v.push_back('Q' ); v.push_back('I' ); v.push_back('\0');
        v.push_back('\0'); v.push_back('\0'); v.push_back('\0'); v.push_back('\0');
        // 06 51 49 00 00 00 00 00
    }
    else if( sym == YIN_SYMBOL )
    {
        v.push_back('\x03'); v.push_back('Y' ); v.push_back('I' ); v.push_back('N' );
        v.push_back('\0'); v.push_back('\0'); v.push_back('\0'); v.push_back('\0');
        // 03 59 49 4e 00 00 00 00
    }
    else
    {
        FC_ASSERT( false, "This method cannot serialize this symbol" );
    }
    return;
}

void old_pack_asset( vector<char>& v, const asset& a )
{
    int64_t x = int64_t( a.amount.value );
    v.push_back( char( x & 0xFF ) );   x >>= 8;
    v.push_back( char( x & 0xFF ) );   x >>= 8;
    v.push_back( char( x & 0xFF ) );   x >>= 8;
    v.push_back( char( x & 0xFF ) );   x >>= 8;
    v.push_back( char( x & 0xFF ) );   x >>= 8;
    v.push_back( char( x & 0xFF ) );   x >>= 8;
    v.push_back( char( x & 0xFF ) );   x >>= 8;
    v.push_back( char( x & 0xFF ) );
    old_pack_symbol( v, a.symbol );
    return;
}

std::string old_json_asset( const asset& a )
{
    size_t decimal_places = 0;
    if( (a.symbol == YANG_SYMBOL) )
        decimal_places = 3;
    else if( a.symbol == QI_SYMBOL )
        decimal_places = 6;
    else if( a.symbol == YIN_SYMBOL )
        decimal_places = 3;
    std::ostringstream ss;
    ss << std::setfill('0') << std::setw(decimal_places+1) << a.amount.value;
    std::string result = ss.str();
    result.insert( result.length() - decimal_places, 1, '.' );
    if( a.symbol == YANG_SYMBOL )
        result += " YANG";
    else if( a.symbol == QI_SYMBOL )
        result += " QI";
    else if( a.symbol == YIN_SYMBOL )
        result += " YIN";
    result.insert(0, 1, '"');
    result += '"';
    return result;
}

#define TAIYI_MAX_SHARE_SUPPLY                int64_t(1000000000000000ll)

BOOST_AUTO_TEST_CASE( asset_raw_test )
{
    try
    {
        BOOST_CHECK( YIN_SYMBOL < YANG_SYMBOL );
        BOOST_CHECK( YANG_SYMBOL < QI_SYMBOL );
        
        // get a bunch of random bits
        fc::sha256 h = fc::sha256::hash("");
        
        std::vector< share_type > amounts;
        
        for( int i=0; i<64; i++ )
        {
            uint64_t s = (uint64_t(1) << i);
            uint64_t x = (h._hash[0] & (s-1)) | s;
            if( x >= TAIYI_MAX_SHARE_SUPPLY )
                break;
            amounts.push_back( share_type( x ) );
        }
        // ilog( "h0:${h0}", ("h0", h._hash[0]) );
        
        std::vector< asset_symbol_type > symbols;
        
        symbols.push_back( YANG_SYMBOL );
        symbols.push_back( YIN_SYMBOL   );
        symbols.push_back( QI_SYMBOL );
        
        for( const share_type& amount : amounts )
        {
            for( const asset_symbol_type& symbol : symbols )
            {
                // check raw::pack() works
                asset a = asset( amount, symbol );
                vector<char> v_old;
                old_pack_asset( v_old, a );
                vector<char> v_cur = fc::raw::pack_to_vector(a);
                //ilog( "${a} : ${d}", ("a", a)("d", hex_bytes( v_old )) );
                //ilog( "${a} : ${d}", ("a", a)("d", hex_bytes( v_cur )) );
                BOOST_CHECK( v_cur == v_old );
                
                // check raw::unpack() works
                std::istringstream ss( string(v_cur.begin(), v_cur.end()) );
                asset a2;
                fc::raw::unpack( ss, a2 );
                BOOST_CHECK( a == a2 );
                
                // check conversion to JSON works
                //std::string json_old = old_json_asset(a);
                //std::string json_cur = fc::json::to_string(a);
                // ilog( "json_old: ${j}", ("j", json_old) );
                // ilog( "json_cur: ${j}", ("j", json_cur) );
                //BOOST_CHECK( json_cur == json_old );
                
                // check JSON serialization is symmetric
                std::string json_cur = fc::json::to_string(a);
                a2 = fc::json::from_string(json_cur).as< asset >();
                BOOST_CHECK( a == a2 );
            }
        }
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( json_tests )
{
    try {
        auto var = fc::json::variants_from_string( "10.6 " );
        var = fc::json::variants_from_string( "10.5" );
    }
    catch ( const fc::exception& e )
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE( extended_private_key_type_test )
{
    try
    {
        fc::ecc::extended_private_key key = fc::ecc::extended_private_key( fc::ecc::private_key::generate(), fc::sha256(), 0, 0, 0 );
        extended_private_key_type type = extended_private_key_type( key );
        std::string packed = std::string( type );
        extended_private_key_type unpacked = extended_private_key_type( packed );
        BOOST_CHECK( type == unpacked );
    }
    catch ( const fc::exception& e )
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE( extended_public_key_type_test )
{
    try
    {
        fc::ecc::extended_public_key key = fc::ecc::extended_public_key( fc::ecc::private_key::generate().get_public_key(), fc::sha256(), 0, 0, 0 );
        extended_public_key_type type = extended_public_key_type( key );
        std::string packed = std::string( type );
        extended_public_key_type unpacked = extended_public_key_type( packed );
        BOOST_CHECK( type == unpacked );
    }
    catch ( const fc::exception& e )
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE( version_test )
{
    try
    {
        BOOST_REQUIRE_EQUAL( string( version( 1, 2, 3) ), "1.2.3" );
        
        fc::variant ver_str( "3.0.0" );
        version ver;
        fc::from_variant( ver_str, ver );
        BOOST_REQUIRE( ver == version( 3, 0 , 0 ) );
        
        ver_str = fc::variant( "0.0.0" );
        fc::from_variant( ver_str, ver );
        BOOST_REQUIRE( ver == version() );
        
        ver_str = fc::variant( "1.0.1" );
        fc::from_variant( ver_str, ver );
        BOOST_REQUIRE( ver == version( 1, 0, 1 ) );
        
        ver_str = fc::variant( "1_0_1" );
        fc::from_variant( ver_str, ver );
        BOOST_REQUIRE( ver == version( 1, 0, 1 ) );
        
        ver_str = fc::variant( "12.34.56" );
        fc::from_variant( ver_str, ver );
        BOOST_REQUIRE( ver == version( 12, 34, 56 ) );
        
        ver_str = fc::variant( "256.0.0" );
        TAIYI_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );
        
        ver_str = fc::variant( "0.256.0" );
        TAIYI_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );
        
        ver_str = fc::variant( "0.0.65536" );
        TAIYI_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );
        
        ver_str = fc::variant( "1.0" );
        TAIYI_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );
        
        ver_str = fc::variant( "1.0.0.1" );
        TAIYI_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );
    }
    FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( hardfork_version_test )
{
    try
    {
        BOOST_REQUIRE_EQUAL( string( hardfork_version( 1, 2 ) ), "1.2.0" );
        
        fc::variant ver_str( "3.0.0" );
        hardfork_version ver;
        fc::from_variant( ver_str, ver );
        BOOST_REQUIRE( ver == hardfork_version( 3, 0 ) );
        
        ver_str = fc::variant( "0.0.0" );
        fc::from_variant( ver_str, ver );
        BOOST_REQUIRE( ver == hardfork_version() );
        
        ver_str = fc::variant( "1.0.0" );
        fc::from_variant( ver_str, ver );
        BOOST_REQUIRE( ver == hardfork_version( 1, 0 ) );
        
        ver_str = fc::variant( "1_0_0" );
        fc::from_variant( ver_str, ver );
        BOOST_REQUIRE( ver == hardfork_version( 1, 0 ) );
        
        ver_str = fc::variant( "12.34.00" );
        fc::from_variant( ver_str, ver );
        BOOST_REQUIRE( ver == hardfork_version( 12, 34 ) );
        
        ver_str = fc::variant( "256.0.0" );
        TAIYI_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );
        
        ver_str = fc::variant( "0.256.0" );
        TAIYI_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );
        
        ver_str = fc::variant( "0.0.1" );
        fc::from_variant( ver_str, ver );
        BOOST_REQUIRE( ver == hardfork_version( 0, 0 ) );
        
        ver_str = fc::variant( "1.0" );
        TAIYI_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );
        
        ver_str = fc::variant( "1.0.0.1" );
        TAIYI_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );
    }
    FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( min_block_size )
{
    signed_block b;
    while( b.siming.length() < TAIYI_MIN_ACCOUNT_NAME_LENGTH )
        b.siming += 'a';
    size_t min_size = fc::raw::pack_size( b );
    BOOST_CHECK( min_size == TAIYI_MIN_BLOCK_SIZE );
}

BOOST_AUTO_TEST_CASE( legacy_signed_transaction )
{
    using taiyi::plugins::baiyujing_api::legacy_signed_transaction;
    
    signed_transaction tx;
    transfer_operation op;
    op.from = "alice";
    op.to = "bob";
    op.amount = asset(1000, YANG_SYMBOL);
    op.memo = "foobar";
    tx.ref_block_num = 4000;
    tx.ref_block_prefix = 4000000000;
    tx.expiration = fc::time_point_sec( 1514764800 );
    tx.operations.push_back( op );
    
    signed_transaction tx2 = signed_transaction( fc::json::from_string( "{\"ref_block_num\":4000,\"ref_block_prefix\":4000000000,\"expiration\":\"2018-01-01T00:00:00\",\"operations\":[[\"transfer\",{\"from\":\"alice\",\"to\":\"bob\",\"amount\":\"1.000 YANG\",\"memo\":\"foobar\"}]],\"extensions\":[],\"signatures\":[\"\"]}" ).as< legacy_signed_transaction >() );
    
    BOOST_REQUIRE( tx.id() == tx2.id() );
}

BOOST_AUTO_TEST_CASE( static_variant_json_test )
{
    try
    {
        typedef static_variant<
            transfer_operation,
            transfer_to_qi_operation
        > test_operation;
        
        test_operation op;
        op = transfer_operation();
        
        auto json_str = fc::json::to_string( op );
        BOOST_CHECK_EQUAL( json_str, "{\"type\":\"transfer_operation\",\"value\":{\"from\":\"\",\"to\":\"\",\"amount\":{\"amount\":\"0\",\"precision\":3,\"fai\":\"@@000000021\"},\"memo\":\"\"}}" );
        
        json_str = "{\"type\":\"transfer_operation\",\"value\":{\"from\":\"foo\",\"to\":\"bar\",\"amount\":{\"amount\":\"1000\",\"precision\":3,\"fai\":\"@@000000021\"},\"memo\":\"\"}}";
        from_variant( fc::json::from_string( json_str ), op );
        BOOST_CHECK_EQUAL( op.which(), 0 );
        
        transfer_operation t = op.get< transfer_operation >();
        BOOST_CHECK( t.from == "foo" );
        BOOST_CHECK( t.to == "bar" );
        BOOST_CHECK( t.amount == asset( 1000, YANG_SYMBOL ) );
        BOOST_CHECK( t.memo == "" );
        
        json_str = "{\"type\":\"transfer_to_qi_operation\",\"value\":{\"from\":\"foo\",\"to\":\"bar\",\"amount\":{\"amount\":\"1000\",\"precision\":3,\"fai\":\"@@000000021\"}}}";
        
        from_variant( fc::json::from_string( json_str ), op );
        BOOST_CHECK_EQUAL( op.which(), 1 );
        
        transfer_to_qi_operation c = op.get< transfer_to_qi_operation >();
        BOOST_CHECK( c.from == "foo" );
        BOOST_CHECK( c.to == "bar" );
        BOOST_CHECK( t.amount == asset( 1000, YANG_SYMBOL ) );

        json_str = "{\"type\":\"not_a_type\",\"value\":{\"from\":\"foo\",\"to\":\"bar\",\"amount\":{\"amount\":\"1000\",\"precision\":3,\"fai\":\"@@000000021\"},\"memo\":\"\"}}";
        TAIYI_REQUIRE_THROW( from_variant( fc::json::from_string( json_str ), op ), fc::assert_exception );
    }
    FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( legacy_operation_test )
{
    try
    {
        auto v = fc::json::from_string( "{\"ref_block_num\": 41047, \"ref_block_prefix\": 4089157749, \"expiration\": \"2018-03-28T19:05:47\", \"operations\": [[\"siming_update\", {\"owner\": \"test\", \"url\": \"foo\", \"block_signing_key\": \"TAI1111111111111111111111111111111114T1Anm\", \"props\": {\"account_creation_fee\": \"0.500 YANG\", \"maximum_block_size\": 65536}, \"fee\": \"0.000 YANG\"}]], \"extensions\": [], \"signatures\": [\"1f1b2d47427a46513777ae9ed032b761b504423b18350e673beb991a1b52d2381c26c36368f9cc4a72c9de3cc16bca83b269c2ea1960e28647caf151e17c35bf3f\"]}" );
        auto ls = v.as< taiyi::plugins::baiyujing_api::legacy_signed_transaction >();
        // not throwing an error here is success
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( asset_symbol_type_test )
{
    try
    {
        uint32_t asset_num = 10000000 << TAIYI_FAI_SHIFT; // Shift FAI value in to position
        asset_num |= SGT_ASSET_NUM_CONTROL_MASK;          // Flip the control bit
        asset_num |= 3;                                   // Add the precision
        
        auto symbol = asset_symbol_type::from_asset_num( asset_num );
        
        BOOST_REQUIRE( symbol == asset_symbol_type::from_fai( 100000006, 3 ) );
        BOOST_REQUIRE( symbol == asset_symbol_type::from_fai_string( "@@100000006", 3 ) );
        BOOST_REQUIRE( asset_num == asset_symbol_type::asset_num_from_fai( 100000006, 3 ) );
        BOOST_REQUIRE( symbol.to_fai_string() == "@@100000006" );
        BOOST_REQUIRE( symbol.to_fai() == 100000006 );
        BOOST_REQUIRE( symbol.asset_num == asset_num );
        BOOST_REQUIRE( symbol.space() == asset_symbol_type::asset_symbol_space::fai_space );
        BOOST_REQUIRE( symbol.get_paired_symbol() == asset_symbol_type::from_asset_num( asset_num ^ SGT_ASSET_NUM_QI_MASK ) );
        BOOST_REQUIRE( asset_symbol_type::from_fai( symbol.to_fai(), 3 ) == symbol );
        
        asset_symbol_type taiyi = asset_symbol_type::from_asset_num( TAIYI_ASSET_NUM_YANG );
        asset_symbol_type qi = asset_symbol_type::from_asset_num( TAIYI_ASSET_NUM_QI );
        
        BOOST_REQUIRE( taiyi.space() == asset_symbol_type::asset_symbol_space::legacy_space );
        BOOST_REQUIRE( qi.space() == asset_symbol_type::asset_symbol_space::legacy_space );
        
        BOOST_REQUIRE( asset_symbol_type::from_fai( taiyi.to_fai(), TAIYI_PRECISION_YANG ) == taiyi );
        BOOST_REQUIRE( asset_symbol_type::from_fai( qi.to_fai(), TAIYI_PRECISION_QI ) == qi );
        
        TAIYI_REQUIRE_THROW( asset_symbol_type::from_fai_string( "@@100000006", TAIYI_ASSET_MAX_DECIMALS + 1 ), fc::assert_exception ); // More than max decimals
        TAIYI_REQUIRE_THROW( asset_symbol_type::from_fai_string( "@0100000006", 3 ), fc::assert_exception );                            // Invalid FAI prefix
        TAIYI_REQUIRE_THROW( asset_symbol_type::from_fai_string( "@@00000006", 3 ), fc::assert_exception );                             // Length too short
        TAIYI_REQUIRE_THROW( asset_symbol_type::from_fai_string( "@@0100000006", 3 ), fc::assert_exception );                           // Length too long
        TAIYI_REQUIRE_THROW( asset_symbol_type::from_fai_string( "@@invalid00", 3 ), fc::exception );                                   // Boost lexical bad cast
        TAIYI_REQUIRE_THROW( asset_symbol_type::from_fai_string( nullptr, 3 ), fc::exception );                                         // Null pointer
    }
    FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( unpack_clear_test )
{
    try
    {
        std::stringstream ss1;
        std::stringstream ss2;
        
        signed_block b1;
        
        for ( int i = 0; i < 10; i++ )
        {
            signed_transaction tx;
            
            transfer_operation op;
            op.from = "alice";
            op.to = "bob";
            op.memo = "memo1";
            op.amount = asset(1000, YANG_SYMBOL);
            tx.operations.push_back( op );
            
            transfer_operation op2;
            op2.from = "charlie";
            op2.to = "sam";
            op2.memo = "memo2";
            op2.amount = asset(2000, YANG_SYMBOL);
            tx.operations.push_back( op2 );
            
            tx.ref_block_num = 1000;
            tx.ref_block_prefix = 1000000000;
            tx.expiration = fc::time_point_sec( 1514764800 + i );
            
            b1.transactions.push_back( tx );
        }
        
        signed_block b2;
        
        for ( int i = 0; i < 20; i++ )
        {
            signed_transaction tx;
            transfer_operation op;
            op.from = "dave";
            op.to = "greg";
            op.memo = "foobar";
            op.amount = asset(10, QI_SYMBOL);
            tx.ref_block_num = 4000;
            tx.ref_block_prefix = 4000000000;
            tx.expiration = fc::time_point_sec( 1714764800 + i );
            tx.operations.push_back( op );
            
            b2.transactions.push_back( tx );
        }
        
        fc::raw::pack( ss2, b2 );
        fc::raw::pack( ss1, b1 );
        
        signed_block unpacked_block;
        fc::raw::unpack( ss2, unpacked_block );
        
        // This operation should completely overwrite signed block 'b2'
        fc::raw::unpack( ss1, unpacked_block );
        
        BOOST_REQUIRE( b1.transactions.size() == unpacked_block.transactions.size() );
        for ( size_t i = 0; i < unpacked_block.transactions.size(); i++ )
        {
            signed_transaction tx = unpacked_block.transactions[ i ];
            BOOST_REQUIRE( unpacked_block.transactions[ i ].operations.size() == b1.transactions[ i ].operations.size() );
            
            transfer_operation op = tx.operations[ 0 ].get< transfer_operation >();
            BOOST_REQUIRE( op.from == "alice" );
            BOOST_REQUIRE( op.to == "bob" );
            BOOST_REQUIRE( op.memo == "memo1" );
            BOOST_REQUIRE( op.amount == asset(1000, YANG_SYMBOL) );
            
            transfer_operation op2 = tx.operations[ 1 ].get< transfer_operation >();
            BOOST_REQUIRE( op2.from == "charlie" );
            BOOST_REQUIRE( op2.to == "sam" );
            BOOST_REQUIRE( op2.memo == "memo2" );
            BOOST_REQUIRE( op2.amount == asset(2000, YANG_SYMBOL) );
            
            BOOST_REQUIRE( tx.ref_block_num == 1000 );
            BOOST_REQUIRE( tx.ref_block_prefix == 1000000000 );
            BOOST_REQUIRE( tx.expiration == fc::time_point_sec( 1514764800 + i ) );
        }
    }
    FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( unpack_recursion_test )
{
    try
    {
        std::stringstream ss;
        int recursion_level = 100000;
        uint64_t allocation_per_level = 500000;
        
        for ( int i = 0; i < recursion_level; i++ )
        {
            fc::raw::pack( ss, unsigned_int( allocation_per_level ) );
            fc::raw::pack( ss, static_cast< uint8_t >( variant::array_type ) );
        }
        
        std::vector< fc::variant > v;
        TAIYI_REQUIRE_THROW( fc::raw::unpack( ss, v ), fc::assert_exception );
    }
    FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_SUITE_END()

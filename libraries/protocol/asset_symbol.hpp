#pragma once

#include <fc/io/raw.hpp>
#include <protocol/types_fwd.hpp>

#define TAIYI_ASSET_SYMBOL_PRECISION_BITS    4
#define TAIYI_ASSET_CONTROL_BITS             1
#define TAIYI_NAI_SHIFT                      ( TAIYI_ASSET_SYMBOL_PRECISION_BITS + TAIYI_ASSET_CONTROL_BITS )
#define SGT_MAX_NAI                          99999999
#define SGT_MIN_NAI                          1
#define SGT_MIN_NON_RESERVED_NAI             10000000
#define TAIYI_ASSET_SYMBOL_NAI_LENGTH        10
#define TAIYI_ASSET_SYMBOL_NAI_STRING_LENGTH ( TAIYI_ASSET_SYMBOL_NAI_LENGTH + 2 )
#define SGT_MAX_NAI_POOL_COUNT               10
#define SGT_MAX_NAI_GENERATION_TRIES         100

#define TAIYI_PRECISION_YIN     (3)
#define TAIYI_PRECISION_YANG    (3)
#define TAIYI_PRECISION_QI      (6)

// One's place is used for check digit, which means NAI 0-9 all have NAI data of 0 which is invalid
// This space is safe to use because it would alwasys result in failure to convert from NAI
#define TAIYI_NAI_YIN       (1)
#define TAIYI_NAI_YANG      (2)
#define TAIYI_NAI_QI        (3)

#define TAIYI_ASSET_NUM_YIN     (uint32_t(((SGT_MAX_NAI + TAIYI_NAI_YIN)   << TAIYI_NAI_SHIFT) | TAIYI_PRECISION_YIN))
#define TAIYI_ASSET_NUM_YANG    (uint32_t(((SGT_MAX_NAI + TAIYI_NAI_YANG) << TAIYI_NAI_SHIFT) | TAIYI_PRECISION_YANG))
#define TAIYI_ASSET_NUM_QI      (uint32_t(((SGT_MAX_NAI + TAIYI_NAI_QI) << TAIYI_NAI_SHIFT) | TAIYI_PRECISION_QI))

#define QI_SYMBOL_U64       (uint64_t('Q') | (uint64_t('I') << 8))
#define YANG_SYMBOL_U64     (uint64_t('Y') | (uint64_t('A') << 8) | (uint64_t('N') << 16) | (uint64_t('G') << 24))
#define YIN_SYMBOL_U64      (uint64_t('Y') | (uint64_t('I') << 8) | (uint64_t('N') << 16))

#define QI_SYMBOL_SER       (uint64_t(6) | (QI_SYMBOL_U64 << 8)) ///< QI with 6 digits of precision
#define YANG_SYMBOL_SER     (uint64_t(3) | (YANG_SYMBOL_U64 << 8)) ///< YANG with 3 digits of precision
#define YIN_SYMBOL_SER      (uint64_t(3) | (YIN_SYMBOL_U64 << 8))   ///< YIN with 3 digits of precision

#define TAIYI_ASSET_MAX_DECIMALS 12

#define SGT_ASSET_NUM_PRECISION_MASK   0xF
#define SGT_ASSET_NUM_CONTROL_MASK     0x10
#define SGT_ASSET_NUM_QI_MASK     0x20

#define ASSET_SYMBOL_NAI_KEY      "nai"
#define ASSET_SYMBOL_DECIMALS_KEY "decimals"

namespace taiyi { namespace protocol {

    class asset_symbol_type
    {
    public:
        enum asset_symbol_space
        {
            legacy_space = 1,
            sgt_nai_space = 2
        };
        
        explicit operator uint32_t() { return to_nai(); }
        
        // buf must have space for TAIYI_ASSET_SYMBOL_MAX_LENGTH+1
        static asset_symbol_type from_string( const std::string& str );
        static asset_symbol_type from_nai_string( const char* buf, uint8_t decimal_places );
        static asset_symbol_type from_asset_num( uint32_t asset_num ) { asset_symbol_type result; result.asset_num = asset_num; return result; }
        static uint32_t asset_num_from_nai( uint32_t nai, uint8_t decimal_places );
        static asset_symbol_type from_nai( uint32_t nai, uint8_t decimal_places ) { return from_asset_num( asset_num_from_nai( nai, decimal_places ) ); }
        static uint8_t damm_checksum_8digit(uint32_t value);
        
        std::string to_string()const;
        void to_nai_string( char* buf )const;
        std::string to_nai_string()const
        {
            char buf[ TAIYI_ASSET_SYMBOL_NAI_STRING_LENGTH ];
            to_nai_string( buf );
            return std::string( buf );
        }
        uint32_t to_nai()const;
        
        /**Returns true when symbol represents qi variant of the token,
         * false for liquid one.
         */
        bool is_qi() const;
        
        /**Returns qi symbol when called from liquid one
         * and liquid symbol when called from qi one.
         */
        asset_symbol_type get_paired_symbol() const;
        
        /**Returns asset_num stripped of precision holding bits.
         * \warning checking that it's SGT symbol is caller responsibility.
         */
        uint32_t get_stripped_precision_sgt_num() const { return asset_num & ~( SGT_ASSET_NUM_PRECISION_MASK ); }
        
        asset_symbol_space space()const;
        uint8_t decimals()const { return uint8_t( asset_num & SGT_ASSET_NUM_PRECISION_MASK ); }
        void validate()const;
        
        friend bool operator == ( const asset_symbol_type& a, const asset_symbol_type& b ) { return (a.asset_num == b.asset_num); }
        friend bool operator != ( const asset_symbol_type& a, const asset_symbol_type& b ) { return (a.asset_num != b.asset_num); }
        friend bool operator <  ( const asset_symbol_type& a, const asset_symbol_type& b ) { return (a.asset_num <  b.asset_num); }
        friend bool operator >  ( const asset_symbol_type& a, const asset_symbol_type& b ) { return (a.asset_num >  b.asset_num); }
        friend bool operator <= ( const asset_symbol_type& a, const asset_symbol_type& b ) { return (a.asset_num <= b.asset_num); }
        friend bool operator >= ( const asset_symbol_type& a, const asset_symbol_type& b ) { return (a.asset_num >= b.asset_num); }
        
        uint32_t asset_num = 0;
    };

} } // taiyi::protocol

FC_REFLECT(taiyi::protocol::asset_symbol_type, (asset_num))

namespace fc {
    
    namespace raw {
        // Legacy serialization of assets
        // 0000pppp aaaaaaaa bbbbbbbb cccccccc dddddddd eeeeeeee ffffffff 00000000
        // Symbol = abcdef
        //
        // NAI serialization of assets
        // aaa1pppp bbbbbbbb cccccccc dddddddd
        // NAI = (MSB to LSB) dddddddd cccccccc bbbbbbbb aaa
        //
        // NAI internal storage of legacy assets
        
        template< typename Stream >
        inline void pack( Stream& s, const taiyi::protocol::asset_symbol_type& sym )
        {
            switch( sym.space() )
            {
                case taiyi::protocol::asset_symbol_type::legacy_space:
                {
                    uint64_t ser = 0;
                    switch( sym.asset_num )
                    {
                        case TAIYI_ASSET_NUM_YANG:
                            ser = YANG_SYMBOL_SER;
                            break;
                        case TAIYI_ASSET_NUM_YIN:
                            ser = YIN_SYMBOL_SER;
                            break;
                        case TAIYI_ASSET_NUM_QI:
                            ser = QI_SYMBOL_SER;
                            break;
                        default:
                            FC_ASSERT( false, "Cannot serialize unknown asset symbol" );
                    }
                    pack( s, ser );
                    break;
                }
                case taiyi::protocol::asset_symbol_type::sgt_nai_space:
                    pack( s, sym.asset_num );
                    break;
                default:
                    FC_ASSERT( false, "Cannot serialize unknown asset symbol" );
            }
        }
        
        template< typename Stream >
        inline void unpack( Stream& s, taiyi::protocol::asset_symbol_type& sym, uint32_t )
        {
            uint64_t ser = 0;
            s.read( (char*) &ser, 4 );
            
            switch( ser )
            {
                case YANG_SYMBOL_SER & 0xFFFFFFFF:
                    s.read( ((char*) &ser)+4, 4 );
                    FC_ASSERT( ser == YANG_SYMBOL_SER, "invalid asset bits" );
                    sym.asset_num = TAIYI_ASSET_NUM_YANG;
                    break;
                case YIN_SYMBOL_SER & 0xFFFFFFFF:
                    s.read( ((char*) &ser)+4, 4 );
                    FC_ASSERT( ser == YIN_SYMBOL_SER, "invalid asset bits" );
                    sym.asset_num = TAIYI_ASSET_NUM_YIN;
                    break;
                case QI_SYMBOL_SER & 0xFFFFFFFF:
                    s.read( ((char*) &ser)+4, 4 );
                    FC_ASSERT( ser == QI_SYMBOL_SER, "invalid asset bits" );
                    sym.asset_num = TAIYI_ASSET_NUM_QI;
                    break;
                default:
                    sym.asset_num = uint32_t( ser );
            }
            sym.validate();
        }
    } // fc::raw

    inline void to_variant( const taiyi::protocol::asset_symbol_type& sym, fc::variant& var )
    {
        try
        {
            mutable_variant_object o;
            o( ASSET_SYMBOL_NAI_KEY, sym.to_nai_string() )
            ( ASSET_SYMBOL_DECIMALS_KEY, sym.decimals() );
            var = std::move( o );
        } FC_CAPTURE_AND_RETHROW()
    }
    
    inline void from_variant( const fc::variant& var, taiyi::protocol::asset_symbol_type& sym )
    {
        using taiyi::protocol::asset_symbol_type;
        
        try
        {
            FC_ASSERT( var.is_object(), "Asset symbol is expected to be an object." );
            
            auto& o = var.get_object();
            
            auto nai = o.find( ASSET_SYMBOL_NAI_KEY );
            FC_ASSERT( nai != o.end(), "Expected key '${key}'.", ("key", ASSET_SYMBOL_NAI_KEY) );
            FC_ASSERT( nai->value().is_string(), "Expected a string type for value '${key}'.", ("key", ASSET_SYMBOL_NAI_KEY) );
            
            auto decimals = o.find( ASSET_SYMBOL_DECIMALS_KEY );
            FC_ASSERT( decimals != o.end(), "Expected key '${key}'.", ("key", ASSET_SYMBOL_DECIMALS_KEY) );
            FC_ASSERT( decimals->value().is_uint64(), "Expected an unsigned integer type for value '${key}'.", ("key", ASSET_SYMBOL_DECIMALS_KEY) );
            FC_ASSERT( decimals->value().as_uint64() <= TAIYI_ASSET_MAX_DECIMALS, "Expected decimals to be less than or equal to ${num}", ("num", TAIYI_ASSET_MAX_DECIMALS) );
            
            sym = asset_symbol_type::from_nai_string( nai->value().as_string().c_str(), decimals->value().as< uint8_t >() );
        } FC_CAPTURE_AND_RETHROW()
    }

} // fc

#pragma once

#include <fc/io/raw.hpp>
#include <protocol/types_fwd.hpp>

// Fungible Asset Identifier (FAI)

#define TAIYI_ASSET_SYMBOL_PRECISION_BITS    4
#define TAIYI_ASSET_CONTROL_BITS             1
#define TAIYI_FAI_SHIFT                      ( TAIYI_ASSET_SYMBOL_PRECISION_BITS + TAIYI_ASSET_CONTROL_BITS )
#define SGT_MAX_FAI                          99999999
#define SGT_MIN_FAI                          1
#define SGT_MIN_NON_RESERVED_FAI             10000000
#define TAIYI_ASSET_SYMBOL_FAI_LENGTH        10
#define TAIYI_ASSET_SYMBOL_FAI_STRING_LENGTH ( TAIYI_ASSET_SYMBOL_FAI_LENGTH + 2 )
#define SGT_MAX_FAI_POOL_COUNT               10
#define SGT_MAX_FAI_GENERATION_TRIES         100

#define TAIYI_PRECISION_YIN     (3)
#define TAIYI_PRECISION_YANG    (3)
#define TAIYI_PRECISION_QI      (6)

#define TAIYI_PRECISION_GOLD    (6)
#define TAIYI_PRECISION_FOOD    (6)
#define TAIYI_PRECISION_WOOD    (6)
#define TAIYI_PRECISION_FABRIC  (6)
#define TAIYI_PRECISION_HERB    (6)

// One's place is used for check digit, which means FAI 0-9 all have FAI data of 0 which is invalid
// This space is safe to use because it would alwasys result in failure to convert from FAI
#define TAIYI_FAI_YIN       (1)
#define TAIYI_FAI_YANG      (2)
#define TAIYI_FAI_QI        (3)

#define TAIYI_FAI_GOLD      (4)
#define TAIYI_FAI_FOOD      (5)
#define TAIYI_FAI_WOOD      (6)
#define TAIYI_FAI_FABRIC    (7)
#define TAIYI_FAI_HERB      (8)

#define TAIYI_ASSET_NUM_YIN     (uint32_t(((SGT_MAX_FAI + TAIYI_FAI_YIN)    << TAIYI_FAI_SHIFT) | TAIYI_PRECISION_YIN))
#define TAIYI_ASSET_NUM_YANG    (uint32_t(((SGT_MAX_FAI + TAIYI_FAI_YANG)   << TAIYI_FAI_SHIFT) | TAIYI_PRECISION_YANG))
#define TAIYI_ASSET_NUM_QI      (uint32_t(((SGT_MAX_FAI + TAIYI_FAI_QI)     << TAIYI_FAI_SHIFT) | TAIYI_PRECISION_QI))

#define TAIYI_ASSET_NUM_GOLD    (uint32_t(((SGT_MAX_FAI + TAIYI_FAI_GOLD)   << TAIYI_FAI_SHIFT) | TAIYI_PRECISION_GOLD))
#define TAIYI_ASSET_NUM_FOOD    (uint32_t(((SGT_MAX_FAI + TAIYI_FAI_FOOD)   << TAIYI_FAI_SHIFT) | TAIYI_PRECISION_FOOD))
#define TAIYI_ASSET_NUM_WOOD    (uint32_t(((SGT_MAX_FAI + TAIYI_FAI_WOOD)   << TAIYI_FAI_SHIFT) | TAIYI_PRECISION_WOOD))
#define TAIYI_ASSET_NUM_FABRIC  (uint32_t(((SGT_MAX_FAI + TAIYI_FAI_FABRIC) << TAIYI_FAI_SHIFT) | TAIYI_PRECISION_FABRIC))
#define TAIYI_ASSET_NUM_HERB    (uint32_t(((SGT_MAX_FAI + TAIYI_FAI_HERB)   << TAIYI_FAI_SHIFT) | TAIYI_PRECISION_HERB))

#define QI_SYMBOL_U64       (uint64_t('Q') | (uint64_t('I') << 8))
#define YANG_SYMBOL_U64     (uint64_t('Y') | (uint64_t('A') << 8) | (uint64_t('N') << 16) | (uint64_t('G') << 24))
#define YIN_SYMBOL_U64      (uint64_t('Y') | (uint64_t('I') << 8) | (uint64_t('N') << 16))

#define GOLD_SYMBOL_U64     (uint64_t('G') | (uint64_t('O') << 8) | (uint64_t('L') << 16) | (uint64_t('D') << 24))
#define FOOD_SYMBOL_U64     (uint64_t('F') | (uint64_t('O') << 8) | (uint64_t('O') << 16) | (uint64_t('D') << 24))
#define WOOD_SYMBOL_U64     (uint64_t('W') | (uint64_t('O') << 8) | (uint64_t('O') << 16) | (uint64_t('D') << 24))
#define FABRIC_SYMBOL_U64   (uint64_t('F') | (uint64_t('A') << 8) | (uint64_t('B') << 16) | (uint64_t('R') << 24))
#define HERB_SYMBOL_U64     (uint64_t('H') | (uint64_t('E') << 8) | (uint64_t('R') << 16) | (uint64_t('B') << 24))

#define QI_SYMBOL_SER       (uint64_t(6) | (QI_SYMBOL_U64   << 8)) ///< QI with 6 digits of precision
#define YANG_SYMBOL_SER     (uint64_t(3) | (YANG_SYMBOL_U64 << 8)) ///< YANG with 3 digits of precision
#define YIN_SYMBOL_SER      (uint64_t(3) | (YIN_SYMBOL_U64  << 8)) ///< YIN with 3 digits of precision

#define GOLD_SYMBOL_SER     (uint64_t(6) | (GOLD_SYMBOL_U64 << 8)) ///< GOLD with 6 digits of precision
#define FOOD_SYMBOL_SER     (uint64_t(6) | (FOOD_SYMBOL_U64 << 8)) ///< FOOD with 6 digits of precision
#define WOOD_SYMBOL_SER     (uint64_t(6) | (WOOD_SYMBOL_U64 << 8)) ///< WOOD with 6 digits of precision
#define FABRIC_SYMBOL_SER   (uint64_t(6) | (FABRIC_SYMBOL_U64 << 8)) ///< FABRIC with 6 digits of precision
#define HERB_SYMBOL_SER     (uint64_t(6) | (HERB_SYMBOL_U64 << 8)) ///< HERB with 6 digits of precision

#define TAIYI_ASSET_MAX_DECIMALS 12

#define SGT_ASSET_NUM_PRECISION_MASK   0xF
#define SGT_ASSET_NUM_CONTROL_MASK     0x10
#define SGT_ASSET_NUM_QI_MASK     0x20

#define ASSET_SYMBOL_FAI_KEY      "fai"
#define ASSET_SYMBOL_DECIMALS_KEY "decimals"

namespace taiyi { namespace protocol {

    class asset_symbol_type
    {
    public:
        enum asset_symbol_space
        {
            legacy_space = 1,
            fai_space = 2
        };
        
        explicit operator uint32_t() { return to_fai(); }
        
        // buf must have space for TAIYI_ASSET_SYMBOL_MAX_LENGTH+1
        static asset_symbol_type from_string( const std::string& str );
        static asset_symbol_type from_fai_string( const char* buf, uint8_t decimal_places );
        static asset_symbol_type from_asset_num( uint32_t asset_num ) { asset_symbol_type result; result.asset_num = asset_num; return result; }
        static uint32_t asset_num_from_fai( uint32_t fai, uint8_t decimal_places );
        static asset_symbol_type from_fai( uint32_t fai, uint8_t decimal_places ) { return from_asset_num( asset_num_from_fai( fai, decimal_places ) ); }
        static uint8_t damm_checksum_8digit(uint32_t value);
        
        std::string to_string()const;
        void to_fai_string( char* buf )const;
        std::string to_fai_string()const
        {
            char buf[ TAIYI_ASSET_SYMBOL_FAI_STRING_LENGTH ];
            to_fai_string( buf );
            return std::string( buf );
        }
        uint32_t to_fai()const;
        
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
        // FAI serialization of assets
        // aaa1pppp bbbbbbbb cccccccc dddddddd
        // FAI = (MSB to LSB) dddddddd cccccccc bbbbbbbb aaa
        //
        // FAI internal storage of legacy assets
        
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
                        case TAIYI_ASSET_NUM_GOLD:
                            ser = GOLD_SYMBOL_SER;
                            break;
                        case TAIYI_ASSET_NUM_FOOD:
                            ser = FOOD_SYMBOL_SER;
                            break;
                        case TAIYI_ASSET_NUM_WOOD:
                            ser = WOOD_SYMBOL_SER;
                            break;
                        case TAIYI_ASSET_NUM_FABRIC:
                            ser = FABRIC_SYMBOL_SER;
                            break;
                        case TAIYI_ASSET_NUM_HERB:
                            ser = HERB_SYMBOL_SER;
                            break;
                        default:
                            FC_ASSERT( false, "Cannot serialize unknown asset symbol" );
                    }
                    pack( s, ser );
                    break;
                }
                case taiyi::protocol::asset_symbol_type::fai_space:
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
                case GOLD_SYMBOL_SER & 0xFFFFFFFF:
                    s.read( ((char*) &ser)+4, 4 );
                    FC_ASSERT( ser == GOLD_SYMBOL_SER, "invalid asset bits" );
                    sym.asset_num = TAIYI_ASSET_NUM_GOLD;
                    break;
                case FOOD_SYMBOL_SER & 0xFFFFFFFF:
                    s.read( ((char*) &ser)+4, 4 );
                    FC_ASSERT( ser == FOOD_SYMBOL_SER, "invalid asset bits" );
                    sym.asset_num = TAIYI_ASSET_NUM_FOOD;
                    break;
                case WOOD_SYMBOL_SER & 0xFFFFFFFF:
                    s.read( ((char*) &ser)+4, 4 );
                    FC_ASSERT( ser == WOOD_SYMBOL_SER, "invalid asset bits" );
                    sym.asset_num = TAIYI_ASSET_NUM_WOOD;
                    break;
                case FABRIC_SYMBOL_SER & 0xFFFFFFFF:
                    s.read( ((char*) &ser)+4, 4 );
                    FC_ASSERT( ser == FABRIC_SYMBOL_SER, "invalid asset bits" );
                    sym.asset_num = TAIYI_ASSET_NUM_FABRIC;
                    break;
                case HERB_SYMBOL_SER & 0xFFFFFFFF:
                    s.read( ((char*) &ser)+4, 4 );
                    FC_ASSERT( ser == HERB_SYMBOL_SER, "invalid asset bits" );
                    sym.asset_num = TAIYI_ASSET_NUM_HERB;
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
            o( ASSET_SYMBOL_FAI_KEY, sym.to_fai_string() )
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
            
            auto fai = o.find( ASSET_SYMBOL_FAI_KEY );
            FC_ASSERT( fai != o.end(), "Expected key '${key}'.", ("key", ASSET_SYMBOL_FAI_KEY) );
            FC_ASSERT( fai->value().is_string(), "Expected a string type for value '${key}'.", ("key", ASSET_SYMBOL_FAI_KEY) );
            
            auto decimals = o.find( ASSET_SYMBOL_DECIMALS_KEY );
            FC_ASSERT( decimals != o.end(), "Expected key '${key}'.", ("key", ASSET_SYMBOL_DECIMALS_KEY) );
            FC_ASSERT( decimals->value().is_uint64(), "Expected an unsigned integer type for value '${key}'.", ("key", ASSET_SYMBOL_DECIMALS_KEY) );
            FC_ASSERT( decimals->value().as_uint64() <= TAIYI_ASSET_MAX_DECIMALS, "Expected decimals to be less than or equal to ${num}", ("num", TAIYI_ASSET_MAX_DECIMALS) );
            
            sym = asset_symbol_type::from_fai_string( fai->value().as_string().c_str(), decimals->value().as< uint8_t >() );
        } FC_CAPTURE_AND_RETHROW()
    }

} // fc

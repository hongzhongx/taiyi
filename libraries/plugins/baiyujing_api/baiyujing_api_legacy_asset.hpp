#pragma once
#include <chain/taiyi_fwd.hpp>
#include <protocol/asset.hpp>

namespace taiyi { namespace plugins { namespace baiyujing_api {

    using taiyi::protocol::asset;
    using taiyi::protocol::asset_symbol_type;
    using taiyi::protocol::share_type;

    struct legacy_asset
    {
    public:
        legacy_asset() {}
        
        asset to_asset()const
        {
            return asset( amount, symbol );
        }
        
        operator asset()const { return to_asset(); }
        
        static legacy_asset from_asset( const asset& a )
        {
            legacy_asset leg;
            leg.amount = a.amount;
            leg.symbol = a.symbol;
            return leg;
        }
        
        string to_string()const;
        static legacy_asset from_string( const string& from );
        
        share_type                       amount;
        asset_symbol_type                symbol = YANG_SYMBOL;
    };
    
} } } // taiyi::plugins::baiyujing_api

namespace fc {

    inline void to_variant( const taiyi::plugins::baiyujing_api::legacy_asset& a, fc::variant& var )
    {
        var = a.to_string();
    }

    inline void from_variant( const fc::variant& var, taiyi::plugins::baiyujing_api::legacy_asset& a )
    {
        a = taiyi::plugins::baiyujing_api::legacy_asset::from_string( var.as_string() );
    }

} // fc

FC_REFLECT( taiyi::plugins::baiyujing_api::legacy_asset, (amount)(symbol) )

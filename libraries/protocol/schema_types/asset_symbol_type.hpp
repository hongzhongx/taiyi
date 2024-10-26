
#pragma once

#include <schema/abstract_schema.hpp>
#include <schema/schema_impl.hpp>

#include <protocol/asset_symbol.hpp>

namespace taiyi { namespace schema {
    
    namespace detail {

        //********************************************
        // asset_symbol_type
        //********************************************
        
        struct schema_asset_symbol_type_impl : public abstract_schema
        {
            TAIYI_SCHEMA_CLASS_BODY( schema_asset_symbol_type_impl )
        };
        
    } //detail

    template<>
    struct schema_reflect< taiyi::protocol::asset_symbol_type >
    {
        typedef detail::schema_asset_symbol_type_impl schema_impl_type;
    };

} } //taiyi::schema

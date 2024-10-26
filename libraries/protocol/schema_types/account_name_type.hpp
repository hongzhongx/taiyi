
#pragma once

#include <schema/abstract_schema.hpp>
#include <schema/schema_impl.hpp>

#include <protocol/types.hpp>

namespace taiyi { namespace schema {
    
    namespace detail {
        
        //********************************************
        // account_name_type
        //********************************************
        
        struct schema_account_name_type_impl : public abstract_schema
        {
            TAIYI_SCHEMA_CLASS_BODY( schema_account_name_type_impl )
        };
        
    }
    
    template<>
    struct schema_reflect< taiyi::protocol::account_name_type >
    {
        typedef detail::schema_account_name_type_impl           schema_impl_type;
    };

} } //taiyi::schema

namespace fc {

    template<>
    struct get_typename< taiyi::protocol::account_name_type >
    {
        static const char* name()
        {
            return "taiyi::protocol::account_name_type";
        }
    };

}

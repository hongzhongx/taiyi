
#pragma once

#include <memory>

#include <protocol/types.hpp>

namespace taiyi { namespace schema {
    struct abstract_schema;
} }

namespace taiyi { namespace protocol {
    struct custom_json_operation;
} }

namespace taiyi { namespace chain {

    class custom_operation_interpreter
    {
    public:
        virtual void apply( const protocol::custom_json_operation& op ) = 0;
        virtual taiyi::protocol::custom_id_type get_custom_id() = 0;
        virtual std::shared_ptr< taiyi::schema::abstract_schema > get_operation_schema() = 0;
    };

} } // taiyi::chain

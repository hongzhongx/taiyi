#pragma once

#include <chain/taiyi_object_types.hpp>

#ifndef TAIYI_SIMING_SPACE_ID
#define TAIYI_SIMING_SPACE_ID 19
#endif

namespace taiyi { namespace chain {
    struct by_account;
} }

namespace taiyi { namespace plugins { namespace siming {

    using namespace taiyi::chain;

    enum siming_object_types
    {
        siming_custom_op_object_type    = ( TAIYI_SIMING_SPACE_ID << 8 )
    };

    class siming_custom_op_object : public object< siming_custom_op_object_type, siming_custom_op_object >
    {
    public:
        template< typename Constructor, typename Allocator >
        siming_custom_op_object( Constructor&& c, allocator< Allocator > a )
        {
            c( *this );
        }
        
        siming_custom_op_object() {}
        
        id_type              id;
        account_name_type    account;
        uint32_t             count = 0;
    };

    typedef multi_index_container<
        siming_custom_op_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< siming_custom_op_object, siming_custom_op_object::id_type, &siming_custom_op_object::id > >,
            ordered_unique< tag< by_account >, member< siming_custom_op_object, account_name_type, &siming_custom_op_object::account > >
        >,
        allocator< siming_custom_op_object >
    > siming_custom_op_index;

} } }

FC_REFLECT( taiyi::plugins::siming::siming_custom_op_object, (id)(account)(count) )
CHAINBASE_SET_INDEX_TYPE( taiyi::plugins::siming::siming_custom_op_object, taiyi::plugins::siming::siming_custom_op_index )

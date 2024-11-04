#include <protocol/operations.hpp>

#include <protocol/operation_util_impl.hpp>

namespace taiyi { namespace protocol {

    struct is_market_op_visitor {
        typedef bool result_type;
        
        template<typename T>
        bool operator()( T&& v )const { return false; }
        bool operator()( const transfer_operation& )const { return true; }
        bool operator()( const transfer_to_qi_operation& )const { return true; }
        bool operator()( const deposit_qi_to_nfa_operation& )const { return true; }
        bool operator()( const withdraw_qi_from_nfa_operation& )const { return true; }
    };
    
    bool is_market_operation( const operation& op ) {
        return op.visit( is_market_op_visitor() );
    }
    
    struct is_vop_visitor
    {
        typedef bool result_type;
        
        template< typename T >
        bool operator()( const T& v )const { return v.is_virtual(); }
    };
    
    bool is_virtual_operation( const operation& op )
    {
        return op.visit( is_vop_visitor() );
    }

} } // taiyi::protocol

TAIYI_DEFINE_OPERATION_TYPE( taiyi::protocol::operation )

#pragma once
#include <protocol/exceptions.hpp>
#include <protocol/operations.hpp>

namespace taiyi { namespace chain {

    class database;

    template< typename OperationType=taiyi::protocol::operation >
    class evaluator
    {
    public:
        virtual ~evaluator() {}
        
        virtual protocol::operation_result apply(const OperationType& op) = 0;
        virtual int get_type()const = 0;
        virtual std::string get_name( const OperationType& op ) = 0;
    };

    template< typename EvaluatorType, typename OperationType=taiyi::protocol::operation >
    class evaluator_impl : public evaluator<OperationType>
    {
    public:
        typedef OperationType operation_sv_type;
        // typedef typename EvaluatorType::operation_type op_type;
        
        evaluator_impl( database& d )
        : _db(d) {}
        
        virtual ~evaluator_impl() {}
        
        virtual protocol::operation_result apply(const OperationType& o) final override
        {
            auto* eval = static_cast< EvaluatorType* >(this);
            const auto& op = o.template get< typename EvaluatorType::operation_type >();
            return eval->do_apply(op);
        }
        
        virtual int get_type()const override { return OperationType::template tag< typename EvaluatorType::operation_type >::value; }
        
        virtual std::string get_name( const OperationType& o ) override
        {
            const auto& op = o.template get< typename EvaluatorType::operation_type >();
            
            return boost::core::demangle( typeid( op ).name() );
        }
        
        database& db() { return _db; }
        
    protected:
        database& _db;
    };

} } //taiyi::chain

#define TAIYI_DEFINE_EVALUATOR( X ) \
class X ## _evaluator : public taiyi::chain::evaluator_impl< X ## _evaluator > \
{                                                                           \
   public:                                                                  \
      typedef X ## _operation operation_type;                               \
                                                                            \
      X ## _evaluator( database& db )                                       \
         : taiyi::chain::evaluator_impl< X ## _evaluator >( db )            \
      {}                                                                    \
                                                                            \
      taiyi::protocol::operation_result do_apply( const X ## _operation& o );  \
};

#define TAIYI_DEFINE_ACTION_EVALUATOR( X, ACTION )                               \
class X ## _evaluator : public taiyi::chain::evaluator_impl< X ## _evaluator, ACTION > \
{                                                                                \
   public:                                                                       \
      typedef X ## _action operation_type;                                       \
                                                                                 \
      X ## _evaluator( database& db )                                            \
         : taiyi::chain::evaluator_impl< X ## _evaluator, ACTION >( db )         \
      {}                                                                         \
                                                                                 \
      taiyi::protocol::operation_result do_apply( const X ## _action& o );          \
};

#define TAIYI_DEFINE_PLUGIN_EVALUATOR( PLUGIN, OPERATION, X )               \
class X ## _evaluator : public taiyi::chain::evaluator_impl< X ## _evaluator, OPERATION > \
{                                                                           \
   public:                                                                  \
      typedef X ## _operation operation_type;                               \
                                                                            \
      X ## _evaluator( taiyi::chain::database& db, PLUGIN* plugin )         \
         : taiyi::chain::evaluator_impl< X ## _evaluator, OPERATION >( db ), \
           _plugin( plugin )                                                \
      {}                                                                    \
                                                                            \
      taiyi::protocol::operation_result do_apply( const X ## _operation& o );  \
                                                                            \
      PLUGIN* _plugin;                                                      \
};

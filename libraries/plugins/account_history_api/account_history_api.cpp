#include <plugins/account_history_api/account_history_api_plugin.hpp>
#include <plugins/account_history_api/account_history_api.hpp>

#include <plugins/account_history/account_history_plugin.hpp>

#include <chain/account_object.hpp>
#include <chain/nfa_objects.hpp>
#include <chain/util/impacted.hpp>

namespace taiyi { namespace plugins { namespace account_history {


    class account_history_api_impl
    {
    public:
        account_history_api_impl() : _db( appbase::app().get_plugin< taiyi::plugins::chain::chain_plugin >().db() ), _dataSource( appbase::app().get_plugin< taiyi::plugins::account_history::account_history_plugin >() ) {}
        ~account_history_api_impl() {}
        
        get_ops_in_block_return get_ops_in_block( const get_ops_in_block_args& );
        get_transaction_return get_transaction( const get_transaction_args& );
        get_account_history_return get_account_history( const get_account_history_args& );
        get_nfa_history_return get_nfa_history( const get_nfa_history_args& );
        enum_virtual_ops_return enum_virtual_ops( const enum_virtual_ops_args& );
        
        chain::database& _db;
        const account_history::account_history_plugin& _dataSource;
    };

    DEFINE_API_IMPL( account_history_api_impl, get_ops_in_block )
    {
        get_ops_in_block_return result;
        _dataSource.find_operations_by_block(args.block_num, [&result, &args](const account_history::rocksdb_operation_object& op) {
            api_operation_object temp(op);
            if( !args.only_virtual || is_virtual_operation( temp.op ) )
                result.ops.emplace(std::move(temp));
        });
        return result;
    }

    DEFINE_API_IMPL( account_history_api_impl, get_account_history )
    {
        FC_ASSERT( args.limit <= 10000, "limit of ${l} is greater than maxmimum allowed", ("l",args.limit) );
        FC_ASSERT( args.start >= args.limit, "start must be greater than limit" );
        
        get_account_history_return result;
        
        _dataSource.find_account_history_data(args.account, args.start, args.limit, [&result](unsigned int sequence, const account_history::rocksdb_operation_object& op) {
            result.history[sequence] = api_operation_object( op );
        });
        
        return result;
    }

    DEFINE_API_IMPL( account_history_api_impl, get_nfa_history )
    {
        FC_ASSERT( args.limit <= 10000, "limit of ${l} is greater than maxmimum allowed", ("l",args.limit) );
        FC_ASSERT( args.start >= args.limit, "start must be greater than limit" );

        get_nfa_history_return result;

        const nfa_object& nfa = _db.get<nfa_object, by_id>(args.id);
        const account_object& owner = _db.get<account_object, by_id>(nfa.owner_account);
        _dataSource.find_account_history_data(owner.name, args.start, args.limit, [&result, &nfa](unsigned int sequence, const account_history::rocksdb_operation_object& op) -> bool
        {
            auto aop = api_operation_object( op );
            
            fc::flat_set<int64_t> impacted_nfas;
            taiyi::chain::operation_get_impacted_nfas( aop.op, impacted_nfas );
            if(impacted_nfas.find(nfa.id) != impacted_nfas.end()) {
                result.history[sequence] = aop;
                return true;
            }
            else
                return false;
        });

        return result;
    }

    DEFINE_API_IMPL( account_history_api_impl, get_transaction )
    {
#ifdef SKIP_BY_TX_ID
        FC_ASSERT(false, "This node's operator has disabled operation indexing by transaction_id");
#else
        uint32_t blockNo = 0;
        uint32_t txInBlock = 0;
        
        if(_dataSource.find_transaction_info(args.id, &blockNo, &txInBlock))
        {
            get_transaction_return result;
            _db.with_read_lock([this, blockNo, txInBlock, &result]() {
                auto blk = _db.fetch_block_by_number(blockNo);
                FC_ASSERT(blk.valid());
                FC_ASSERT(blk->transactions.size() > txInBlock);
                result = blk->transactions[txInBlock];
                result.block_num = blockNo;
                result.transaction_num = txInBlock;
            });
            
            return result;
        }
        else
        {
            FC_ASSERT(false, "Unknown Transaction ${t}", ("t", args.id));
        }
#endif
    }
    
    DEFINE_API_IMPL( account_history_api_impl, enum_virtual_ops)
    {
        enum_virtual_ops_return result;
        
        result.next_block_range_begin = _dataSource.enum_operations_from_block_range(args.block_range_begin, args.block_range_end, [&result](const account_history::rocksdb_operation_object& op) {
            result.ops.emplace_back(api_operation_object(op));
        });
        
        return result;
    }

    account_history_api::account_history_api()
    {
        auto ah = appbase::app().find_plugin< taiyi::plugins::account_history::account_history_plugin >();
        FC_ASSERT( ah != nullptr, "Account History API only works if account_history plugins are enabled" );

        my = std::make_unique< account_history_api_impl >();
        
        JSON_RPC_REGISTER_API( TAIYI_ACCOUNT_HISTORY_API_PLUGIN_NAME );
    }
    
    account_history_api::~account_history_api() {}
    
    DEFINE_LOCKLESS_APIS( account_history_api ,
        (get_ops_in_block)
        (get_transaction)
        (get_account_history)
        (get_nfa_history)
        (enum_virtual_ops)
    )

} } } // taiyi::plugins::account_history

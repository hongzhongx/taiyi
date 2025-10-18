#pragma once
#include <chain/block_log.hpp>
#include <chain/fork_database.hpp>
#include <chain/global_property_object.hpp>
#include <chain/hardfork_property_object.hpp>
#include <chain/node_property_object.hpp>
#include <chain/notifications.hpp>

#include <chain/util/advanced_benchmark_dumper.hpp>
#include <chain/util/signal.hpp>

#include <protocol/protocol.hpp>
#include <protocol/hardfork.hpp>

#include <appbase/plugin.hpp>

#include <fc/signals.hpp>

#include <fc/log/logger.hpp>

#include <functional>
#include <map>

namespace taiyi { namespace chain {

    using taiyi::protocol::signed_transaction;
    using taiyi::protocol::operation;
    using taiyi::protocol::authority;
    using taiyi::protocol::asset;
    using taiyi::protocol::asset_symbol_type;
    using taiyi::protocol::price;
    using abstract_plugin = appbase::abstract_plugin;

    struct hardfork_versions
    {
        fc::time_point_sec         times[ TAIYI_NUM_HARDFORKS + 1 ];
        protocol::hardfork_version versions[ TAIYI_NUM_HARDFORKS + 1 ];
    };

    class database;
    class LuaContext;

    using set_index_type_func = std::function< void(database&, mira::index_type, const boost::filesystem::path&, const boost::any&) >;
    struct index_delegate
    {
        set_index_type_func set_index_type;
    };

    using index_delegate_map = std::map< std::string, index_delegate >;

    class database_impl;
    class custom_operation_interpreter;

    namespace util
    {
        class advanced_benchmark_dumper;
    }

    struct reindex_notification;

    /**
     *   @class database
     *   @brief tracks the blockchain state in an extensible manner
     */
    class database : public chainbase::database
    {
    public:
        database();
        ~database();

        bool is_producing()const { return _is_producing; }
        void set_producing( bool p ) { _is_producing = p;  }
        
        //for test
        void set_log_hardforks( bool b ) { _log_hardforks = b; }

        bool is_pending_tx()const { return _pending_tx_session.valid(); }
        
        bool is_processing_block()const { return _currently_processing_block_id.valid(); }

        enum validation_steps
        {
            skip_nothing                = 0,
            skip_siming_signature       = 1 << 0,  ///< used while reindexing
            skip_transaction_signatures = 1 << 1,  ///< used by non-siming nodes
            skip_transaction_dupe_check = 1 << 2,  ///< used while reindexing
            skip_fork_db                = 1 << 3,  ///< used while reindexing
            skip_block_size_check       = 1 << 4,  ///< used when applying locally generated transactions
            skip_tapos_check            = 1 << 5,  ///< used while reindexing -- note this skips expiration check as well
            skip_authority_check        = 1 << 6,  ///< used while reindexing -- disables any checking of authority on transactions
            skip_merkle_check           = 1 << 7,  ///< used while reindexing
            skip_undo_history_check     = 1 << 8,  ///< used while reindexing
            skip_siming_schedule_check  = 1 << 9,  ///< used while reindexing
            skip_validate               = 1 << 10, ///< used prior to checkpoint, skips validate() call on transaction
            skip_validate_invariants    = 1 << 11, ///< used to skip database invariant check on block application
            skip_undo_block             = 1 << 12, ///< used to skip undo db on reindex
            skip_block_log              = 1 << 13  ///< used to skip block logging on reindex
        };

        typedef std::function<void(uint32_t, const abstract_index_cntr_t&)> TBenchmarkMidReport;
        typedef std::pair<uint32_t, TBenchmarkMidReport> TBenchmark;

        struct open_args
        {
            fc::path data_dir;
            fc::path state_storage_dir;
            uint64_t initial_supply = TAIYI_YANG_INIT_SUPPLY;
            uint64_t initial_qi_supply = 0;
            uint32_t chainbase_flags = 0;
            bool do_validate_invariants = false;
            bool benchmark_is_enabled = false;
            fc::variant database_cfg;
            bool replay_in_memory = false;
            std::vector< std::string > replay_memory_indices{};

            // The following fields are only used on reindexing
            uint32_t stop_replay_at = 0;
            TBenchmark benchmark = TBenchmark(0, []( uint32_t, const abstract_index_cntr_t& ){});
        };

        /**
         * @brief Open a database, creating a new one if necessary
         *
         * Opens a database in the specified directory. If no initialized database is found the database
         * will be initialized with the default state.
         *
         * @param data_dir Path to open or create database in
         */
        void open( const open_args& args );

        /**
         * @brief Rebuild object graph from block history and open detabase
         *
         * This method may be called after or instead of @ref database::open, and will rebuild the object graph by
         * replaying blockchain history. When this method exits successfully, the database will be open.
         *
         * @return the last replayed block number.
         */
        uint32_t reindex( const open_args& args );

        /**
         * @brief wipe Delete database from disk, and potentially the raw chain as well.
         * @param include_blocks If true, delete the raw chain as well as the database.
         *
         * Will close the database before wiping. Database will be closed when this function returns.
         */
        void wipe(const fc::path& data_dir, const fc::path& state_storage_dir, bool include_blocks);
        void close(bool rewind = true);

        // **************** database_block.cpp **************** //

        /**
         *  @return true if the block is in our fork DB or saved to disk as
         *  part of the official chain, otherwise return false
         */
        bool                       is_known_block( const block_id_type& id )const;
        bool                       is_known_transaction( const transaction_id_type& id )const;
        block_id_type              find_block_id_for_num( uint32_t block_num )const;
        block_id_type              get_block_id_for_num( uint32_t block_num )const;
        optional<signed_block>     fetch_block_by_id( const block_id_type& id )const;
        optional<signed_block>     fetch_block_by_number( uint32_t num )const;
        const signed_transaction   get_recent_transaction( const transaction_id_type& trx_id )const;
        std::vector<block_id_type> get_block_ids_on_fork(block_id_type head_of_fork) const;

        chain_id_type _taiyi_chain_id = TAIYI_CHAIN_ID;
        chain_id_type get_chain_id() const;
        void set_chain_id( const chain_id_type& chain_id );

        /** Allows to visit all stored blocks until processor returns true. Caller is responsible for block disasembling
         * const signed_block_header& - header of previous block
         * const signed_block& - block to be processed currently
         */
        void foreach_block(std::function<bool(const signed_block_header&, const signed_block&)> processor) const;
        /// Allows to process all blocks visit all transactions held there until processor returns true.
        void foreach_tx(std::function<bool(const signed_block_header&, const signed_block&, const signed_transaction&, uint32_t)> processor) const;
        /// Allows to process all operations held in blocks and transactions until processor returns true.
        void foreach_operation(std::function<bool(const signed_block_header&, const signed_block&, const signed_transaction&, uint32_t, const operation&, uint16_t)> processor) const;

        const siming_object&  get_siming(  const account_name_type& name )const;
        const siming_object*  find_siming( const account_name_type& name )const;

        const account_object&  get_account(  const account_name_type& name )const;
        const account_object*  find_account( const account_name_type& name )const;

        const nfa_object& create_nfa(const account_object& creator, const nfa_symbol_object& nfa_symbol, const flat_set<public_key_type>& sigkeys, bool reset_vm_memused, LuaContext& context, const nfa_object* caller_nfa = 0);
        
        const dynamic_global_property_object&  get_dynamic_global_properties()const;
        const node_property_object&            get_node_properties()const;
        const siming_schedule_object&          get_siming_schedule_object()const;
        const hardfork_property_object&        get_hardfork_property_object()const;

        /**
         *  Calculate the percent of block production slots that were missed in the
         *  past 128 blocks, not including the current block.
         */
        uint32_t siming_participation_rate()const;

        void add_checkpoints( const flat_map<uint32_t,block_id_type>& checkpts );
        const flat_map<uint32_t,block_id_type> get_checkpoints()const { return _checkpoints; }
        bool before_last_checkpoint()const;

        bool push_block( const signed_block& b, uint32_t skip = skip_nothing );
        void push_transaction( const signed_transaction& trx, uint32_t skip = skip_nothing );
        void _maybe_warn_multiple_production( uint32_t height )const;
        bool _push_block( const signed_block& b );
        void _push_transaction( const signed_transaction& trx );

        void pop_block();
        void clear_pending();

        void push_virtual_operation( const operation& op );
        void pre_push_virtual_operation( const operation& op );
        void post_push_virtual_operation( const operation& op );

        /**
          *  This method is used to track applied operations during the evaluation of a block, these
          *  operations should include any operation actually included in a transaction as well
          *  as any implied/virtual operations that resulted, such as filling an order.
          *  The applied operations are cleared after post_apply_operation.
          */
        void notify_pre_apply_operation( const operation_notification& note );
        void notify_post_apply_operation( const operation_notification& note );
        void notify_pre_apply_block( const block_notification& note );
        void notify_post_apply_block( const block_notification& note );
        void notify_irreversible_block( uint32_t block_num );
        void notify_pre_apply_transaction( const transaction_notification& note );
        void notify_post_apply_transaction( const transaction_notification& note );

        using apply_operation_handler_t = std::function< void(const operation_notification&) >;
        using apply_transaction_handler_t = std::function< void(const transaction_notification&) >;
        using apply_block_handler_t = std::function< void(const block_notification&) >;
        using irreversible_block_handler_t = std::function< void(uint32_t) >;
        using reindex_handler_t = std::function< void(const reindex_notification&) >;

    private:
        template <typename TSignal, typename TNotification = std::function<typename TSignal::signature_type>>
        boost::signals2::connection connect_impl( TSignal& signal, const TNotification& func, const abstract_plugin& plugin, int32_t group, const std::string& item_name = "" );

        template< bool IS_PRE_OPERATION >
        boost::signals2::connection any_apply_operation_handler_impl( const apply_operation_handler_t& func, const abstract_plugin& plugin, int32_t group );

    public:
        boost::signals2::connection add_pre_apply_operation_handler( const apply_operation_handler_t& func, const abstract_plugin& plugin, int32_t group = -1 );
        boost::signals2::connection add_post_apply_operation_handler( const apply_operation_handler_t& func, const abstract_plugin& plugin, int32_t group = -1 );
        boost::signals2::connection add_pre_apply_transaction_handler( const apply_transaction_handler_t& func, const abstract_plugin& plugin, int32_t group = -1 );
        boost::signals2::connection add_post_apply_transaction_handler( const apply_transaction_handler_t& func, const abstract_plugin& plugin, int32_t group = -1 );
        boost::signals2::connection add_pre_apply_block_handler( const apply_block_handler_t& func, const abstract_plugin& plugin, int32_t group = -1 );
        boost::signals2::connection add_post_apply_block_handler( const apply_block_handler_t& func, const abstract_plugin& plugin, int32_t group = -1 );
        boost::signals2::connection add_irreversible_block_handler( const irreversible_block_handler_t& func, const abstract_plugin& plugin, int32_t group = -1 );
        boost::signals2::connection add_pre_reindex_handler( const reindex_handler_t& func, const abstract_plugin& plugin, int32_t group = -1 );
        boost::signals2::connection add_post_reindex_handler( const reindex_handler_t& func, const abstract_plugin& plugin, int32_t group = -1 );

        //**************** database_siming_schedule.cpp **************//

        /**
          * @brief Get the siming scheduled for block production in a slot.
          *
          * slot_num always corresponds to a time in the future.
          *
          * If slot_num == 1, returns the next scheduled siming.
          * If slot_num == 2, returns the next scheduled siming after
          * 1 block gap.
          *
          * Use the get_slot_time() and get_slot_at_time() functions
          * to convert between slot_num and timestamp.
          *
          * Passing slot_num == 0 returns TAIYI_NULL_SIMING
          */
        account_name_type get_scheduled_siming(uint32_t slot_num)const;

        /**
          * Get the time at which the given slot occurs.
          *
          * If slot_num == 0, return time_point_sec().
          *
          * If slot_num == N for N > 0, return the Nth next
          * block-interval-aligned time greater than head_block_time().
          */
        fc::time_point_sec get_slot_time(uint32_t slot_num)const;

        /**
          * Get the last slot which occurs AT or BEFORE the given time.
          *
          * The return value is the greatest value N such that
          * get_slot_time( N ) <= when.
          *
          * If no such N exists, return 0.
          */
        uint32_t get_slot_at_time(fc::time_point_sec when)const;

        asset create_qi( const account_object& to_account, asset taiyi, bool to_reward_balance=false );

        void adjust_balance( const account_object& a, const asset& delta );
        void adjust_balance( const account_name_type& name, const asset& delta );
        void adjust_reward_balance( const account_object& a, const asset& value_delta, const asset& share_delta = asset(0, QI_SYMBOL), const asset& feigang_delta = asset(0, QI_SYMBOL) );
        void adjust_reward_balance( const account_name_type& name, const asset& value_delta, const asset& share_delta = asset(0, QI_SYMBOL), const asset& feigang_delta = asset(0, QI_SYMBOL) );
        void update_owner_authority( const account_object& account, const authority& owner_authority );

        asset get_balance( const account_object& a, asset_symbol_type symbol )const;
        asset get_balance( const account_name_type& aname, asset_symbol_type symbol )const;

        asset get_nfa_balance( const nfa_object& nfa, asset_symbol_type symbol )const;

        void adjust_nfa_balance( const nfa_object& nfa, const asset& delta );

        /** this updates the adores for simings as a result of account adoring proxy changing */
        void adjust_proxied_siming_adores( const account_object& a, const std::array< share_type, TAIYI_MAX_PROXY_RECURSION_DEPTH+1 >& delta, int depth = 0 );

        /** this updates the adores for all simings as a result of account QI changing */
        void adjust_proxied_siming_adores( const account_object& a, share_type delta, int depth = 0 );

        /** this is called by `adjust_proxied_siming_adores` when account proxy to self */
        void adjust_siming_adores( const account_object& a, share_type delta );

        /** this updates the adore of a single siming as a result of a adore being added or removed*/
        void adjust_siming_adore( const siming_object& obj, share_type delta );

        /** clears all adore records for a particular account but does not update the
          * siming adore totals.  Vote totals should be updated first via a call to
          * adjust_proxied_siming_adores( a, -a.siming_adore_weight() )
          */
        void clear_siming_adores( const account_object& a );
        void process_qi_withdrawals();
        void process_funds();
        void account_recovery_processing();
        void process_decline_adoring_rights();

        std::tuple<share_type, share_type> pay_reward_funds( share_type reward_yang, share_type reward_qi_fund );

        time_point_sec   head_block_time()const;
        uint32_t         head_block_num()const;
        block_id_type    head_block_id()const;

        node_property_object& node_properties();

        uint32_t last_non_undoable_block_num() const;
        
        //************ database_actor.cpp ************//

        void initialize_actor_object( actor_object& act, const std::string& name, const nfa_object& nfa );
        void initialize_actor_talents( const actor_object& act );
        void initialize_actor_attributes( const actor_object& act, const vector<uint16_t>& init_attrs );
        const actor_object& get_actor( const std::string& name )const;
        const actor_object* find_actor( const std::string& name )const;
        void initialize_actor_talent_rule_object(const account_object& creator, actor_talent_rule_object& rule, long long& vm_drops);
        void born_actor( const actor_object& act, int gender, int sexuality, const zone_object& zone );
        void born_actor( const actor_object& act, int gender, int sexuality, const string& zone_name );
        void try_trigger_actor_talents( const actor_object& act, uint16_t age );
        void try_trigger_actor_contract_grow( const actor_object& act );
        void prepare_actor_relations( const actor_object& actor1, const actor_object& actor2 );
        const actor_object* find_actor_with_parents( const nfa_object& nfa, const uint16_t depth = 3 );

        //************ database_zone.cpp ************//

        const tiandao_property_object& get_tiandao_properties() const;
        void initialize_zone_object( zone_object& zone, const std::string& name, const nfa_object& nfa, E_ZONE_TYPE type );
        const zone_object&  get_zone(  const std::string& name ) const;
        const zone_object*  find_zone( const std::string& name ) const;
        bool is_contract_allowed_by_zone(const contract_object& contract, const zone_id_type& zone_id) const;
        int calculate_moving_days_to_zone( const zone_object& zone );
        void process_tiandao();
        
        zone_id_type get_contract_run_zone() const { return _contract_run_zone; }
        void set_contract_run_zone(zone_id_type z) { _contract_run_zone = z; }
        
        //************ database_cultivation.cpp ************//
        const cultivation_object& create_cultivation(const nfa_object& manager_nfa, const chainbase::t_flat_map<nfa_id_type, uint>& beneficiaries, uint64_t prepare_time_seconds);
        void participate_cultivation(const cultivation_object& cult, const nfa_object& nfa, uint64_t value);
        void start_cultivation(const cultivation_object& cult);
        void stop_cultivation(const cultivation_object& cult);
        void dissolve_cultivation(const cultivation_object& cult);
        void process_cultivations();

        //************ database_init.cpp ************//

        void initialize_evaluators();
        void register_custom_operation_interpreter( std::shared_ptr< custom_operation_interpreter > interpreter );
        std::shared_ptr< custom_operation_interpreter > get_custom_json_evaluator( const custom_id_type& id );

        /// Reset the object graph in-memory
        void initialize_indexes();
        void init_genesis(uint64_t initial_supply = TAIYI_YANG_INIT_SUPPLY, uint64_t initial_qi_supply = 0 );

        /**
          *  This method validates transactions without adding it to the pending state.
          *  @throw if an error occurs
          */
        void validate_transaction( const signed_transaction& trx );

        /** when popping a block, the transactions that were removed get cached here so they
         * can be reapplied at the proper time
         */
        std::deque< signed_transaction >       _popped_tx;
        vector< signed_transaction >           _pending_tx;

        bool has_hardfork( uint32_t hardfork )const;

        uint32_t get_hardfork()const;

        /* For testing and debugging only. Given a hardfork
            with id N, applies all hardforks with id <= N */
        void set_hardfork( uint32_t hardfork, bool process_now = true );

        void validate_invariants()const;

        void set_flush_interval( uint32_t flush_blocks );

        void apply_transaction( const signed_transaction& trx, uint32_t skip = skip_nothing );

        optional< chainbase::database::session >& pending_transaction_session();

        void set_index_delegate( const std::string& n, index_delegate&& d );
        const index_delegate& get_index_delegate( const std::string& n );
        bool has_index_delegate( const std::string& n );
        const index_delegate_map& index_delegates();

        // contracts
        void initialize_VM_baseENV(LuaContext& context);

        void create_basic_contract_objects();
        size_t create_contract_objects(const account_object& owner, const string& contract_name, const string& contract_data, const public_key_type& contract_authority, long long& vm_drops);
        lua_map prepare_account_contract_data(const account_object& account, const contract_object& contract);
        
        void add_contract_handler_exe_point(int64_t p) { _contract_handler_exe_point += p; }
        int64_t get_contract_handler_exe_point() const { return _contract_handler_exe_point; }
        void clear_contract_handler_exe_point(const int64_t& init = 0) { _contract_handler_exe_point = init; }
                
        //奖励账号非罡
        void reward_feigang(const account_object& to_account, const account_object& from_account, const asset& feigang );
        void reward_feigang(const account_object& to_account, const nfa_object& from_nfa, const asset& feigang );

        // nfa
        void create_basic_nfa_symbol_objects();
        size_t create_nfa_symbol_object(const account_object& creator, const string& symbol, const string& describe, const string& default_contract, const uint64_t& max_count, const uint64_t& min_equivalent_qi);
        void modify_nfa_children_owner(const nfa_object& nfa, const account_object& new_owner, std::set<nfa_id_type>& recursion_loop_check);
        int get_nfa_five_phase(const nfa_object& nfa) const;
        bool is_nfa_material_equivalent_qi_insufficient(const nfa_object& nfa) const;
        void consume_nfa_material_random(const nfa_object& nfa, const uint32_t& seed);

    protected:
        //Mark pop_undo() as protected -- we do not want outside calling pop_undo(); it should call pop_block() instead
        //void pop_undo() { object_database::pop_undo(); }
        void notify_changed_objects();

        void process_nfa_tick();
        void process_actor_tick();

    private:
        bool _is_producing = false;
        bool _log_hardforks = true;
        optional< chainbase::database::session > _pending_tx_session;

        void apply_block( const signed_block& next_block, uint32_t skip = skip_nothing );
        void _apply_block( const signed_block& next_block );
        void _apply_transaction( const signed_transaction& trx );
        operation_result apply_operation( const operation& op );

        ///Steps involved in applying a new block
        ///@{

        const siming_object& validate_block_header( uint32_t skip, const signed_block& next_block )const;
        void create_block_summary(const signed_block& next_block);

        void update_global_dynamic_data( const signed_block& b );
        void update_signing_siming(const siming_object& signing_siming, const signed_block& new_block);
        void update_last_irreversible_block();
        void migrate_irreversible_state();
        void clear_expired_transactions();
        void clear_expired_delegations();
        void process_header_extensions( const signed_block& next_block );

        void init_hardforks();
        void process_hardforks();
        void apply_hardfork( uint32_t hardfork );

        ///@}

        template< typename asset_balance_object_type, class balance_operator_type >
        void adjust_asset_balance(const account_name_type& name, const asset& delta, bool check_account, balance_operator_type balance_operator);
        void modify_balance(const account_object& a, const asset& delta, bool check_balance);
        void modify_reward_balance(const account_object& a, const asset& value_delta, const asset& share_delta, const asset& feigang_delta, bool check_balance);

        template< typename nfa_balance_object_type, class balance_operator_type >
        void adjust_nfa_balance(const nfa_id_type& nfa_id, const asset& delta, balance_operator_type balance_operator);

        operation_notification create_operation_notification( const operation& op )const
        {
            operation_notification note(op);
            note.trx_id       = _current_trx_id;
            note.block        = _current_block_num;
            note.trx_in_block = _current_trx_in_block;
            note.op_in_trx    = _current_op_in_trx;
            return note;
        }

    public:

        const transaction_id_type& get_current_trx() const { return _current_trx_id; }
        const signed_transaction* get_current_trx_ptr() const { return _current_trx; }
        uint16_t get_current_op_in_trx() const { return _current_op_in_trx; }

        util::advanced_benchmark_dumper& get_benchmark_dumper() { return _benchmark_dumper; }

        const hardfork_versions& get_hardfork_versions() { return _hardfork_versions; }

    private:
        std::unique_ptr< database_impl > _my;

        fork_database                 _fork_db;
        hardfork_versions             _hardfork_versions;

        block_log                     _block_log;

        // this function needs access to _plugin_index_signal
        template< typename MultiIndexType >
        friend void add_plugin_index( database& db );

        transaction_id_type           _current_trx_id;
        const signed_transaction*     _current_trx = 0;
        uint32_t                      _current_block_num    = 0;
        int32_t                       _current_trx_in_block = 0;
        uint16_t                      _current_op_in_trx    = 0;
        uint16_t                      _current_virtual_op   = 0;

        optional< block_id_type >     _currently_processing_block_id;

        flat_map<uint32_t,block_id_type>  _checkpoints;

        node_property_object          _node_property_object;

        uint32_t                      _flush_blocks = 0;
        uint32_t                      _next_flush_block = 0;

        flat_map< custom_id_type, std::shared_ptr< custom_operation_interpreter > >   _custom_operation_interpreters;

        util::advanced_benchmark_dumper  _benchmark_dumper;
        index_delegate_map            _index_delegate_map;

        fc::signal<void(const operation_notification&)>       _pre_apply_operation_signal;
        /**
          *  This signal is emitted for plugins to process every operation after it has been fully applied.
          */
        fc::signal<void(const operation_notification&)>       _post_apply_operation_signal;

        /**
          *  This signal is emitted when we start processing a block.
          *
          *  You may not yield from this callback because the blockchain is holding
          *  the write lock and may be in an "inconstant state" until after it is
          *  released.
          */
        fc::signal<void(const block_notification&)>           _pre_apply_block_signal;

        fc::signal<void(uint32_t)>                            _on_irreversible_block;

        /**
          *  This signal is emitted after all operations and virtual operation for a
          *  block have been applied but before the get_applied_operations() are cleared.
          *
          *  You may not yield from this callback because the blockchain is holding
          *  the write lock and may be in an "inconstant state" until after it is
          *  released.
          */
        fc::signal<void(const block_notification&)>           _post_apply_block_signal;

        /**
          * This signal is emitted any time a new transaction is about to be applied
          * to the chain state.
          */
        fc::signal<void(const transaction_notification&)>     _pre_apply_transaction_signal;

        /**
          * This signal is emitted any time a new transaction has been applied to the
          * chain state.
          */
        fc::signal<void(const transaction_notification&)>     _post_apply_transaction_signal;

        /**
          * Emitted when reindexing starts
          */
        fc::signal<void(const reindex_notification&)>         _pre_reindex_signal;

        /**
          * Emitted when reindexing finishes
          */
        fc::signal<void(const reindex_notification&)>         _post_reindex_signal;

        /**
          *  Emitted After a block has been applied and committed.  The callback
          *  should not yield and should execute quickly.
          */
        //fc::signal<void(const vector< object_id_type >&)> changed_objects;

        /** this signal is emitted any time an object is removed and contains a
          * pointer to the last value of every object that was removed.
          */
        //fc::signal<void(const vector<const object*>&)>  removed_objects;

        /**
          * Internal signal to execute deferred registration of plugin indexes.
          */
        fc::signal<void()>                                    _plugin_index_signal;
        
        /**
          * 用于统计合约中执行handler api的运算消耗
         */
        int64_t _contract_handler_exe_point = 0;
        
        /**
          * 用于在执行合约时临时判断合约执行发起actor所在的zone
         */
        zone_id_type _contract_run_zone = zone_id_type::max();
    };

    struct reindex_notification
    {
        reindex_notification( const database::open_args& a ) : args( a ) {}
        
        bool reindex_success = false;
        uint32_t last_block_number = 0;
        const database::open_args& args;
    };

} } //taiyi::chain

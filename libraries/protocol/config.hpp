#pragma once
#include "hardfork.hpp"

// WARNING!
// Every symbol defined here needs to be handled appropriately in get_config.cpp
// This is checked by get_config_check.sh called from Dockerfile

#ifdef IS_TEST_NET

#define TAIYI_BLOCKCHAIN_VERSION                ( version(0, 0, 1) )

#define TAIYI_INIT_PRIVATE_KEY                  (fc::ecc::private_key::regenerate(fc::sha256::hash(std::string("init_key"))))
#define TAIYI_INIT_PUBLIC_KEY_STR               (std::string( taiyi::protocol::public_key_type(TAIYI_INIT_PRIVATE_KEY.get_public_key()) ))
#define TAIYI_CHAIN_ID                          (fc::sha256::hash("testnet"))

#define TAIYI_GENESIS_TIME                      (fc::time_point_sec(1761545700))  //2025-10-27 14:15:00

#define TAIYI_OWNER_AUTH_RECOVERY_PERIOD                  fc::seconds(60)
#define TAIYI_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD  fc::seconds(12)
#define TAIYI_OWNER_UPDATE_LIMIT                          fc::seconds(0)

#define TAIYI_YANG_INIT_SUPPLY                  int64_t(0)

#define TAIYI_DELEGATION_RETURN_PERIOD          (60*60*1) // 1 hour

#else // IS LIVE TAIYI NETWORK

#define TAIYI_BLOCKCHAIN_VERSION                ( version(0, 0, 0) )

#define TAIYI_INIT_PUBLIC_KEY_STR               "TAI8HpgpX6nXnqJcZCcUwDQpFeorz4ZHMegQA5Be4K88wzRSnxjeo"
#define TAIYI_CHAIN_ID                          fc::sha256()

#define TAIYI_GENESIS_TIME                      (fc::time_point_sec(1762840800))  //2025-11-11 14:00:00

#define TAIYI_OWNER_AUTH_RECOVERY_PERIOD                  fc::days(30)
#define TAIYI_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD  fc::days(1)
#define TAIYI_OWNER_UPDATE_LIMIT                          fc::minutes(60)

#define TAIYI_YANG_INIT_SUPPLY                  int64_t(0)

#define TAIYI_DELEGATION_RETURN_PERIOD          (5*60*60*24) // 5 day

#endif // END LIVE TAIYI NETWORK

#define TAIYI_ADDRESS_PREFIX                    "TAI"

#define QI_SYMBOL     (taiyi::protocol::asset_symbol_type::from_asset_num( TAIYI_ASSET_NUM_QI ) )
#define YANG_SYMBOL   (taiyi::protocol::asset_symbol_type::from_asset_num( TAIYI_ASSET_NUM_YANG ) )
#define YIN_SYMBOL    (taiyi::protocol::asset_symbol_type::from_asset_num( TAIYI_ASSET_NUM_YIN ) )

#define GOLD_SYMBOL   (taiyi::protocol::asset_symbol_type::from_asset_num( TAIYI_ASSET_NUM_GOLD ) )
#define FOOD_SYMBOL   (taiyi::protocol::asset_symbol_type::from_asset_num( TAIYI_ASSET_NUM_FOOD ) )
#define WOOD_SYMBOL   (taiyi::protocol::asset_symbol_type::from_asset_num( TAIYI_ASSET_NUM_WOOD ) )
#define FABRIC_SYMBOL (taiyi::protocol::asset_symbol_type::from_asset_num( TAIYI_ASSET_NUM_FABRIC ) )
#define HERB_SYMBOL   (taiyi::protocol::asset_symbol_type::from_asset_num( TAIYI_ASSET_NUM_HERB ) )

#define TAIYI_BLOCKCHAIN_HARDFORK_VERSION       ( hardfork_version( TAIYI_BLOCKCHAIN_VERSION ) )

#define TAIYI_100_PERCENT                       10000
#define TAIYI_1_PERCENT                         (TAIYI_100_PERCENT/100)

#define TAIYI_BLOCK_INTERVAL                    3
#define TAIYI_BLOCKS_PER_YEAR                   (365*24*60*60/TAIYI_BLOCK_INTERVAL)
#define TAIYI_BLOCKS_PER_DAY                    (24*60*60/TAIYI_BLOCK_INTERVAL)
#define TAIYI_BLOCKS_PER_HOUR                   (60*60/TAIYI_BLOCK_INTERVAL)
#define TAIYI_START_QI_BLOCK                    (TAIYI_BLOCKS_PER_DAY * 7)
#define TAIYI_START_ADORING_BLOCK               (TAIYI_BLOCKS_PER_DAY * 30)

#define TAIYI_VDAY_BLOCK_NUM                    TAIYI_BLOCKS_PER_HOUR       // 1 virtual day per hour
#define TAIYI_VMONTH_BLOCK_NUM                  (TAIYI_VDAY_BLOCK_NUM*30)   // 1 virtual month

#define TAIYI_NFA_TICK_PERIOD_MAX_BLOCK_NUM     (100) // about 5 minutes
#define TAIYI_ACTOR_TICK_PERIOD_MAX_BLOCK_NUM   (100) // about 5 minutes

#define TAIYI_INIT_SIMING_NAME                  "danuo"
#define TAIYI_NUM_INIT_SIMINGS                  1

#define TAIYI_MAX_SIMINGS                       21

#define TAIYI_HARDFORK_REQUIRED_SIMINGS         17 // 17 of the 21 dpos simings (20 elected and 1 virtual time) required for hardfork. This guarantees 75% participation on all subsequent rounds.
#define TAIYI_MAX_TIME_UNTIL_EXPIRATION         (60*60) // seconds,  aka: 1 hour
#define TAIYI_MAX_MEMO_SIZE                     2048
#define TAIYI_MAX_PROXY_RECURSION_DEPTH         4
#define TAIYI_QI_WITHDRAW_INTERVALS             4
#define TAIYI_QI_WITHDRAW_INTERVAL_SECONDS      (60*60*24*7) // 1 week per interval
#define TAIYI_MAX_WITHDRAW_ROUTES               10

#define TAIYI_QI_SHARE_PRICE                    price(asset(1000, YANG_SYMBOL), asset(1000000, QI_SYMBOL))
#define TAIYI_GOLD_QI_PRICE                     price(asset(5000000, QI_SYMBOL), asset(1000000, GOLD_SYMBOL))
#define TAIYI_FOOD_QI_PRICE                     price(asset(3000000, QI_SYMBOL), asset(1000000, FOOD_SYMBOL))
#define TAIYI_WOOD_QI_PRICE                     price(asset(2000000, QI_SYMBOL), asset(1000000, WOOD_SYMBOL))
#define TAIYI_FABRIC_QI_PRICE                   price(asset(3000000, QI_SYMBOL), asset(1000000, FABRIC_SYMBOL))
#define TAIYI_HERB_QI_PRICE                     price(asset(4000000, QI_SYMBOL), asset(1000000, HERB_SYMBOL))

#define TAIYI_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM 1

#define TAIYI_MAX_ACCOUNT_SIMING_ADORES         30

#define TAIYI_MIN_REWARD_FUND                   (1000)  // 通胀后激励各基金的最小阳寿
#define TAIYI_INFLATION_RATE_START_PERCENT      (1000)  // 10%
#define TAIYI_INFLATION_RATE_STOP_PERCENT       (100)   // 1%
#define TAIYI_INFLATION_NARROWING_PERIOD        (233600) // Narrow 0.01% every 233.6k blocks
#define TAIYI_CONTENT_REWARD_YANG_PERCENT       (15*TAIYI_1_PERCENT) //15% of inflation
#define TAIYI_CONTENT_REWARD_QI_FUND_PERCENT    (75*TAIYI_1_PERCENT) //75% of inflation

#define TAIYI_CULTIVATION_REWARD_FUND_NAME      ("cultivation")

#define TAIYI_MIN_ACCOUNT_NAME_LENGTH           3
#define TAIYI_MAX_ACCOUNT_NAME_LENGTH           16

#define TAIYI_MIN_PERMLINK_LENGTH               0
#define TAIYI_MAX_PERMLINK_LENGTH               256
#define TAIYI_MAX_SIMING_URL_LENGTH             2048

#define TAIYI_MAX_SATOSHIS                      int64_t(4611686018427387903ll)
#define TAIYI_MAX_SIG_CHECK_DEPTH               2
#define TAIYI_MAX_SIG_CHECK_ACCOUNTS            125

#define TAIYI_MAX_TRANSACTION_SIZE              (1024*64)
#define TAIYI_MIN_BLOCK_SIZE_LIMIT              (TAIYI_MAX_TRANSACTION_SIZE)
#define TAIYI_MAX_BLOCK_SIZE                    (TAIYI_MAX_TRANSACTION_SIZE*TAIYI_BLOCK_INTERVAL*2000)
#define TAIYI_SOFT_MAX_BLOCK_SIZE               (2*1024*1024)
#define TAIYI_MIN_BLOCK_SIZE                    115

#define TAIYI_MAX_UNDO_HISTORY                  10000

#define TAIYI_MAX_AUTHORITY_MEMBERSHIP          40

#define TAIYI_IRREVERSIBLE_THRESHOLD            (75 * TAIYI_1_PERCENT)

#define TAIYI_VIRTUAL_SCHEDULE_LAP_LENGTH       ( fc::uint128::max_value() )

#define TAIYI_BLOCK_GENERATION_POSTPONED_TX_LIMIT 5
#define TAIYI_PENDING_TRANSACTION_EXECUTION_LIMIT fc::milliseconds(200)

#define TAIYI_CUSTOM_OP_ID_MAX_LENGTH           (32)
#define TAIYI_CUSTOM_OP_DATA_MAX_LENGTH         (8192)

/**
 *  Reserved Account IDs with special meaning
 */
///@{
/// Represents the canonical account with NO authority (nobody can access funds in null account)
#define TAIYI_NULL_ACCOUNT                      "null"
/// Represents the canonical account with WILDCARD authority (anybody can access funds in temp account)
#define TAIYI_TEMP_ACCOUNT                      "temp"
/// Represents the canonical account for specifying you will adore for directly (as opposed to a proxy)
#define TAIYI_PROXY_TO_SELF_ACCOUNT             ""
/// Represents the account with NO authority who holds resources for payouts according to given proposals
#define TAIYI_TREASURY_ACCOUNT                  "zuowang.dao"
/// Represents the account with NO authority who can operate tiandao
#define TAIYI_DANUO_ACCOUNT                     "taiyi.danuo"

#define TAIYI_COMMITTEE_ACCOUNT                 "sifu"
///@}

#define TAIYI_MIN_ACCOUNT_CREATION_FEE          int64_t(1)
#define TAIYI_MAX_ACCOUNT_CREATION_FEE          int64_t(1000000000)

#define TAIYI_MIN_CONTRACT_NAME_LENGTH          9
#define TAIYI_MAX_CONTRACT_NAME_LENGTH          128

#define TAIYI_MIN_NFA_SYMBOL_LENGTH             4
#define TAIYI_MAX_NFA_SYMBOL_LENGTH             128

#define TAIYI_NFA_INIT_FUNC_NAME                "init_data"
#define TAIYI_ACTOR_TALENT_RULE_INIT_FUNC_NAME  "talent_data"

#define TAIYI_USEMANA_STATE_BYTES_SCALE         1
#define TAIYI_USEMANA_EXECUTION_SCALE           1

#define TAIYI_CONTRACT_USEMANA_REWARD_AUTHOR_PERCENT    (80*TAIYI_1_PERCENT)
#define TAIYI_CONTRACT_USEMANA_REWARD_TREASURY_PERCENT  (20*TAIYI_1_PERCENT)

#define TAIYI_CONTRACT_OBJ_STATE_BYTES          2000
#define TAIYI_CONTRACT_BIN_OBJ_STATE_BYTES      2000

#define TAIYI_NFA_SYMBOL_OBJ_STATE_BYTES        1000
#define TAIYI_NFA_OBJ_STATE_BYTES               2000
#define TAIYI_NFA_MATERIAL_STATE_BYTES          1000

#define TAIYI_ZONE_OBJ_STATE_BYTES              1000

#define TAIYI_ACTOR_OBJ_STATE_BYTES             1000

#define TAIYI_MAX_ACTOR_CREATION_FEE            int64_t(1000000000000)
#define TAIYI_ACTOR_NAME_LIMIT                  (64)
#define TAIYI_ACTOR_INIT_ATTRIBUTE_AMOUNT       (800)

#define TAIYI_ZONE_NAME_LIMIT                   (256)

#define TAIYI_USEMANA_ACTOR_ACTION_SCALE        1000

#define TAIYI_CULTIVATION_PREPARE_MIN_TIME_BLOCK_NUM   20                       // about 60 seconds
#define TAIYI_CULTIVATION_MAX_TIME_BLOCK_NUM           (TAIYI_BLOCKS_PER_DAY*7) // about 7 day

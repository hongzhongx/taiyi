#!/bin/bash

OBJECTS=( "account_authority"                \
          "account_history"                  \
          "account_metadata"                 \
          "account"                          \
          "account_recovery_request"         \
          "block_summary"                    \
          "change_recovery_account_request"  \
          "decline_adoring_rights_request"   \
          "dynamic_global_property"          \
          "hardfork_property"                \
          "operation"                        \
          "owner_authority_history"          \
          "reward_fund"                      \
          "transaction"                      \
          "qi_delegation_expiration"         \
          "qi_delegation"                    \
          "withdraw_qi_route"                \
          "siming"                           \
          "siming_schedule"                  \
          "siming_adore" )


DATA_DIR="$HOME/.taiyin"
TAIYIN_DIR="../.."
STATS_DUMP_PERIOD=600

ADVISOR_PATH="../../libraries/vendor/rocksdb/tools/advisor"

while (( "$#" )); do
   case "$1" in
      -d|--data-dir)
         DATA_DIR=$2
         shift 2
         ;;
      -s|--taiyin-dir)
         TAIYIN_DIR=$2
         shift 2
         ;;
      -p|--stats-dump-period)
         STATS_DUMP_PERIOD=$2
         shift 2
         ;;
      -h|--help)
         echo "Specify data directory with '--data-dir' (Default is ~/.taiyin)"
         echo "Specify taiyin directory with '--taiyin-dir' (Default is ../..)"
         echo "Specify stats dump period with '--stats-dump-period' (Default is 600)"
         exit 1
         ;;
      *)
         echo "Specify data directory with '--data-dir' (Default is ~/.taiyin)"
         echo "Specify taiyin directory with '--taiyin-dir' (Default is ../..)"
         echo "Specify stats dump period with '--stats-dump-period' (Default is 600)"
         exit 1
         ;;
   esac
done

cd "$TAIYIN_DIR/libraries/vendor/rocksdb/tools/advisor"

for OBJ in "${OBJECTS[@]}"; do
   DB_PATH="$DATA_DIR/blockchain/rocksdb_"
   DB_PATH+="$OBJ"
   DB_PATH+='_object'
   DB_OPTIONS="$(find $DB_PATH -name "OPTIONS-*" -type f -print0 | xargs -0 ls -1 -t | head -1)"

   echo "Advisor for $OBJ..."
   python3 -m advisor.rule_parser_example --rules_spec=advisor/rules.ini --rocksdb_options="$DB_OPTIONS" --log_files_path_prefix="$DB_PATH/LOG" --stats_dump_period_sec=$STATS_DUMP_PERIOD
   echo ''
done

exit 0

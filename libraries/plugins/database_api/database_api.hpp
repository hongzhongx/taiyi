#pragma once

#include <plugins/json_rpc/utility.hpp>

#include <plugins/database_api/database_api_args.hpp>
#include <plugins/database_api/database_api_objects.hpp>

#define DATABASE_API_SINGLE_QUERY_LIMIT 1000

namespace taiyi { namespace plugins { namespace database_api {

    class database_api_impl;
    
    class database_api
    {
    public:
        database_api();
        ~database_api();

        DECLARE_API(

            //***********
            // Globals //
            //***********

            /**
             * @brief Retrieve compile-time constants
             */
            (get_config)

            /**
             * @brief Return version information and chain_id of running node
             */
            (get_version)

            /**
             * @brief Retrieve the current @ref dynamic_global_property_object
             */
            (get_dynamic_global_properties)
            (get_siming_schedule)
            (get_hardfork_properties)
            (get_reward_funds)

            //***********
            // Simings //
            //***********
            (list_simings)
            (find_simings)
            (list_siming_adores)
            (get_active_simings)

            //***********
            // Accounts //
            //***********

            /**
             * @brief List accounts ordered by specified key
             *
             */
            (list_accounts)

            /**
             * @brief Find accounts by primary key (account name)
             */
            (find_accounts)
            (list_owner_histories)
            (find_owner_histories)
            (list_account_recovery_requests)
            (find_account_recovery_requests)
            (list_change_recovery_account_requests)
            (find_change_recovery_account_requests)
            (list_withdraw_qi_routes)
            (find_withdraw_qi_routes)
            (list_qi_delegations)
            (find_qi_delegations)
            (list_qi_delegation_expirations)
            (find_qi_delegation_expirations)
            (list_decline_adoring_rights_requests)
            (find_decline_adoring_rights_requests)

            //**************************
            // Authority / validation //
            //**************************

            /// @brief Get a hexdump of the serialized binary form of a transaction
            (get_transaction_hex)

            /**
             *  This API will take a partially signed transaction and a set of public keys that the owner has the ability to sign for
             *  and return the minimal subset of public keys that should add signatures to the transaction.
             */
            (get_required_signatures)

            /**
             *  This method will return the set of all public keys that could possibly sign for a given transaction.  This call can
             *  be used by wallets to filter their set of public keys to just the relevant subset prior to calling @ref get_required_signatures
             *  to get the minimum subset.
             */
            (get_potential_signatures)

            /**
             * @return true of the @ref trx has all of the required signatures, otherwise throws an exception
             */
            (verify_authority)

            /**
             * @return true if the signers have enough authority to authorize an account
             */
            (verify_account_authority)

            /*
             * This is a general purpose API that checks signatures against accounts for an arbitrary sha256 hash
             * using the existing authority structures in Taiyi
             */
            (verify_signatures)
                    
            (find_account_resources)
                    
            //********
            // NFAs //
            //********
            (find_nfa)
            (find_nfas)
            (list_nfas)
                    
            //********
            //Actors//
            //********
            (find_actor)
            (find_actors)
            (list_actors)
            (find_actor_talent_rules)

            //*********
            // Zones //
            //*********
            (find_zones)
            (list_zones)
            (find_zones_by_name)
            (find_way_to_zone)
                    
            (get_tiandao_properties)
        )
        
    private:
        std::unique_ptr< database_api_impl > my;
    };

} } } //taiyi::plugins::database_api


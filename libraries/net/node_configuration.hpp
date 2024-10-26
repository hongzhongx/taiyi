#pragma once

#include "config.hpp"

#include <fc/crypto/elliptic.hpp>
#include <fc/network/ip.hpp>

namespace taiyi { namespace net {

    struct node_configuration
    {
        node_configuration() {}
        
        fc::ip::endpoint listen_endpoint;
        bool accept_incoming_connections = true;
        bool wait_if_endpoint_is_busy = true;
        
        /**
         * Originally, our p2p code just had a 'node-id' that was a random number identifying this node
         * on the network.  This is now a private key/public key pair, where the public key is used
         * in place of the old random node-id.  The private part is unused, but might be used in
         * the future to support some notion of trusted peers.
         */
        fc::ecc::private_key private_key;
        
        /** if we have less than `desired_number_of_connections`, we will try to connect with more nodes */
        uint32_t desired_number_of_connections = TAIYI_NET_DEFAULT_DESIRED_CONNECTIONS;
        /** if we have _maximum_number_of_connections or more, we will refuse any inbound connections */
        uint32_t maximum_number_of_connections = TAIYI_NET_DEFAULT_MAX_CONNECTIONS;
        /** retry connections to peers that have failed or rejected us this often, in seconds */
        uint32_t peer_connection_retry_timeout = TAIYI_NET_DEFAULT_PEER_CONNECTION_RETRY_TIME;
        /** how many seconds of inactivity are permitted before disconnecting a peer */
        uint32_t peer_inactivity_timeout = TAIYI_NET_PEER_HANDSHAKE_INACTIVITY_TIMEOUT;
        
        bool peer_advertising_disabled = false;
        
        uint32_t maximum_number_of_blocks_to_handle_at_one_time = TAIYI_NET_MAX_NUMBER_OF_BLOCKS_TO_HANDLE_AT_ONE_TIME;
        uint32_t maximum_number_of_sync_blocks_to_prefetch = TAIYI_NET_MAX_NUMBER_OF_BLOCKS_TO_PREFETCH;
        uint32_t maximum_blocks_per_peer_during_syncing = TAIYI_NET_MAX_BLOCKS_PER_PEER_DURING_SYNCING;
        int64_t active_ignored_request_timeout_microseconds = 6000000;
    };
    
} } //taiyi::net

FC_REFLECT(taiyi::net::node_configuration,
    (listen_endpoint)
    (accept_incoming_connections)
    (wait_if_endpoint_is_busy)
    (private_key)
    (desired_number_of_connections)
    (maximum_number_of_connections)
    (peer_connection_retry_timeout)
    (peer_inactivity_timeout)
    (peer_advertising_disabled)
    (maximum_number_of_blocks_to_handle_at_one_time)
    (maximum_number_of_sync_blocks_to_prefetch)
    (maximum_blocks_per_peer_during_syncing)
    (active_ignored_request_timeout_microseconds)
)

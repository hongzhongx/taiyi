#pragma once

#define TAIYI_NET_PROTOCOL_VERSION                          101

/**
 * Define this to enable debugging code in the p2p network interface.
 * This is code that would never be executed in normal operation, but is
 * used for automated testing (creating artificial net splits,
 * tracking where messages came from and when)
 */
#define ENABLE_P2P_DEBUGGING_API                            1

/**
 * 2MiB
 */
#define MAX_MESSAGE_SIZE                                    1024*1024*2
#define TAIYI_NET_DEFAULT_PEER_CONNECTION_RETRY_TIME        30 // seconds

/**
 * AFter trying all peers, how long to wait before we check to
 * see if there are peers we can try again.
 */
#define TAIYI_PEER_DATABASE_RETRY_DELAY                     15 // seconds

#define TAIYI_NET_PEER_HANDSHAKE_INACTIVITY_TIMEOUT         5

#define TAIYI_NET_PEER_DISCONNECT_TIMEOUT                   20

#define TAIYI_NET_TEST_SEED_IP                              "127.0.0.1"
#define TAIYI_NET_TEST_P2P_PORT                             1900
#define TAIYI_NET_DEFAULT_P2P_PORT                          1976
#define TAIYI_NET_DEFAULT_DESIRED_CONNECTIONS               20
#define TAIYI_NET_DEFAULT_MAX_CONNECTIONS                   200

#define TAIYI_NET_MAXIMUM_QUEUED_MESSAGES_IN_BYTES          (1024 * 1024)

/**
 * When we receive a message from the network, we advertise it to
 * our peers and save a copy in a cache were we will find it if
 * a peer requests it.  We expire out old items out of the cache
 * after this number of blocks go by.
 *
 * Recently lowered from 30 to match the default expiration time
 * the web wallet imposes on transactions.
 */
#define TAIYI_NET_MESSAGE_CACHE_DURATION_IN_BLOCKS          20

/**
 * We prevent a peer from offering us a list of blocks which, if we fetched them
 * all, would result in a blockchain that extended into the future.
 * This parameter gives us some wiggle room, allowing a peer to give us blocks
 * that would put our blockchain up to an hour in the future, just in case
 * our clock is a bit off.
 */
#define TAIYI_NET_FUTURE_SYNC_BLOCKS_GRACE_PERIOD_SEC       (60 * 60)

#define TAIYI_NET_MAX_INVENTORY_SIZE_IN_MINUTES             2

#define TAIYI_NET_MAX_BLOCKS_PER_PEER_DURING_SYNCING        200

/**
 * During normal operation, how many items will be fetched from each
 * peer at a time.  This will only come into play when the network
 * is being flooded -- typically transactions will be fetched as soon
 * as we find out about them, so only one item will be requested
 * at a time.
 *
 * No tests have been done to find the optimal value for this
 * parameter, so consider increasing or decreasing it if performance
 * during flooding is lacking.
 */
#define TAIYI_NET_MAX_ITEMS_PER_PEER_DURING_NORMAL_OPERATION    1

/**
 * Instead of fetching all item IDs from a peer, then fetching all blocks
 * from a peer, we will interleave them.  Fetch at least this many block IDs,
 * then switch into block-fetching mode until the number of blocks we know about
 * but haven't yet fetched drops below this
 */
#define TAIYI_NET_MIN_BLOCK_IDS_TO_PREFETCH                 10000

#define TAIYI_NET_MAX_TRX_PER_SECOND                        1000

#define TAIYI_NET_MAX_NUMBER_OF_BLOCKS_TO_HANDLE_AT_ONE_TIME    200
#define TAIYI_NET_MAX_NUMBER_OF_BLOCKS_TO_PREFETCH              (10 * TAIYI_NET_MAX_NUMBER_OF_BLOCKS_TO_HANDLE_AT_ONE_TIME)

#define TAIYI_NET_MAX_TRX_PER_SECOND                        1000

/**
 * Set the ignored request time out to 1 second.  When we request a block
 * or transaction from a peer, this timeout determines how long we wait for them
 * to reply before we give up and ask another peer for the item.
 * Ideally this should be significantly shorter than the block interval, because
 * we'd like to realize the block isn't coming and fetch it from a different
 * peer before the next block comes in.  At the current target of 3 second blocks,
 * 1 second seems reasonable.  When we get closer to our eventual target of 1 second
 * blocks, this will need to be re-evaluated (i.e., can we set the timeout to 500ms
 * and still handle normal network & processing delays without excessive disconnects)
 */
#define TAIYI_NET_ACTIVE_IGNORED_REQUEST_TIMEOUT_SECONDS    6

#define TAIYI_NET_FAILED_TERMINATE_TIMEOUT_SECONDS          120

#define TAIYI_NET_PRUNE_FAILED_IDS_MINUTES                  15

#define TAIYI_NET_FETCH_UPDATED_PEER_LISTS_INTERVAL_MINUTES 15

#define TAIYI_NET_BANDWIDTH_MONITOR_INTERVAL_SECONDS        1

#define TAIYI_NET_DUMP_NODE_STATUS_INTERVAL_MINUTES         1

#define TAIYI_NET_FIREWALL_CHECK_MESSAGE_INTERVAL_MINUTES   5

#define TAIYI_NET_PORT_WAIT_DELAY_SECONDS                   5

#define TAIYI_NET_MAX_PEERDB_SIZE                           1000

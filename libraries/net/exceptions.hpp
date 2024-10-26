#pragma once
#include <fc/exception/exception.hpp>

namespace taiyi { namespace net {
    // registered in node.cpp
    
    FC_DECLARE_EXCEPTION( net_exception, 90000, "P2P Networking Exception" );
    FC_DECLARE_DERIVED_EXCEPTION( send_queue_overflow,                   taiyi::net::net_exception, 90001, "send queue for this peer exceeded maximum size" );
    FC_DECLARE_DERIVED_EXCEPTION( insufficient_relay_fee,                taiyi::net::net_exception, 90002, "insufficient relay fee" );
    FC_DECLARE_DERIVED_EXCEPTION( already_connected_to_requested_peer,   taiyi::net::net_exception, 90003, "already connected to requested peer" );
    FC_DECLARE_DERIVED_EXCEPTION( block_older_than_undo_history,         taiyi::net::net_exception, 90004, "block is older than our undo history allows us to process" );
    FC_DECLARE_DERIVED_EXCEPTION( peer_is_on_an_unreachable_fork,        taiyi::net::net_exception, 90005, "peer is on another fork" );
    FC_DECLARE_DERIVED_EXCEPTION( unlinkable_block_exception,            taiyi::net::net_exception, 90006, "unlinkable block" )
    
} }

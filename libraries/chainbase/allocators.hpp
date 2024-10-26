#pragma once

#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/flat_map.hpp>
#include <boost/interprocess/containers/deque.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/file_lock.hpp>

#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>

#include <deque>
#include <type_traits>

namespace chainbase {

    template< typename T >
    using allocator = std::allocator< T >;
    
    typedef boost::shared_mutex read_write_mutex;
    typedef boost::shared_lock< read_write_mutex > read_lock;
    typedef boost::unique_lock< read_write_mutex > write_lock;
    
    template< typename KEY_TYPE, typename VALUE_TYPE, typename LESS_FUNC = std::less<KEY_TYPE>>
    using t_flat_map = boost::container::flat_map< KEY_TYPE, VALUE_TYPE, LESS_FUNC, allocator< std::pair< KEY_TYPE, VALUE_TYPE > > >;
}

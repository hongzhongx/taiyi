#include "chainbase.hpp"
#include <boost/array.hpp>
#include <boost/any.hpp>
#include <iostream>

#ifdef WIN32
#define __VERSION__ ""
#endif

namespace chainbase {

    struct environment_check
    {
        environment_check()
        {
            memset( &compiler_version, 0, sizeof( compiler_version ) );
            memcpy( &compiler_version, __VERSION__, std::min<size_t>( strlen(__VERSION__), 256 ) );
#ifndef NDEBUG
            debug = true;
#endif
#ifdef __APPLE__
            apple = true;
#endif
#ifdef WIN32
            windows = true;
#endif
        }
        
        friend bool operator == ( const environment_check& a, const environment_check& b )
        {
            return std::make_tuple( a.compiler_version, a.debug, a.apple, a.windows ) ==  std::make_tuple( b.compiler_version, b.debug, b.apple, b.windows );
        }
        
        environment_check& operator = ( const environment_check& other )
        {
            compiler_version = other.compiler_version;
            debug = other.debug;
            apple = other.apple;
            windows = other.windows;
            
            return *this;
        }
        
        boost::array<char,256>  compiler_version;
        bool                    debug = false;
        bool                    apple = false;
        bool                    windows = false;
    };
    
    void database::open( const bfs::path& dir, uint32_t flags, const boost::any& database_cfg )
    {
        assert( dir.is_absolute() );
        bfs::create_directories( dir );
        if( _data_dir != dir ) close();
        
        _data_dir = dir;
        _database_cfg = database_cfg;
        
        for( auto& item : _index_list )
            item->open( _data_dir, _database_cfg );
        
        _is_open = true;
    }
    
    void database::flush() {
        for( auto& item : _index_list )
            item->flush();
    }
    
    size_t database::get_cache_usage() const
    {
        size_t cache_size = 0;
        for( const auto& i : _index_list )
            cache_size += i->get_cache_usage();
        return cache_size;
    }

    size_t database::get_cache_size() const
    {
        size_t cache_size = 0;
        for( const auto& i : _index_list )
            cache_size += i->get_cache_size();
        return cache_size;
    }
    
    void database::dump_lb_call_counts()
    {
        for( const auto& i : _index_list )
            i->dump_lb_call_counts();
    }
    
    void database::trim_cache()
    {
        if( _index_list.size() )
            (*_index_list.begin())->trim_cache();
    }
    
    void database::close()
    {
        if( _is_open )
        {
            undo_all();
            
            for( auto& item : _index_list )
                item->close();
            
            _is_open = false;
        }
    }
    
    void database::wipe( const bfs::path& dir )
    {
        assert( !_is_open );
        for( auto& item : _index_list )
            item->wipe( dir );
        
        _index_list.clear();
        _index_map.clear();
        _index_types.clear();
    }
    
    void database::set_require_locking( bool enable_require_locking )
    {
#ifdef CHAINBASE_CHECK_LOCKING
        _enable_require_locking = enable_require_locking;
#endif
    }
    
#ifdef CHAINBASE_CHECK_LOCKING
    void database::require_lock_fail( const char* method, const char* lock_type, const char* tname )const
    {
        std::string err_msg = "database::" + std::string( method ) + " require_" + std::string( lock_type ) + "_lock() failed on type " + std::string( tname );
        std::cerr << err_msg << std::endl;
        BOOST_THROW_EXCEPTION( std::runtime_error( err_msg ) );
    }
#endif
    
    void database::undo()
    {
        for( auto& item : _index_list )
        {
            item->undo();
        }
    }
    
    void database::squash()
    {
        for( auto& item : _index_list )
        {
            item->squash();
        }
    }
    
    void database::commit( int64_t revision )
    {
        for( auto& item : _index_list )
        {
            item->commit( revision );
        }
    }
    
    void database::undo_all()
    {
        for( auto& item : _index_list )
        {
            item->undo_all();
        }
    }
    
    database::session database::start_undo_session()
    {
        vector< std::unique_ptr<abstract_session> > _sub_sessions;
        _sub_sessions.reserve( _index_list.size() );
        for( auto& item : _index_list ) {
            _sub_sessions.push_back( item->start_undo_session() );
        }
        return session( std::move( _sub_sessions ), _undo_session_count );
    }
    
}  // namespace chainbase



#pragma once

#include <mira/index_converter.hpp>

#include <schema/schema.hpp>
#include <protocol/schema_types.hpp>
#include <chain/schema_types.hpp>

#include <chain/database.hpp>

namespace taiyi { namespace chain {

    using taiyi::schema::abstract_schema;

    struct index_info : public chainbase::index_extension
    {
        index_info();
        virtual ~index_info();
        virtual std::shared_ptr< abstract_schema > get_schema() = 0;
    };

    template< typename MultiIndexType >
    struct index_info_impl : public index_info
    {
        typedef typename MultiIndexType::value_type value_type;
        
        index_info_impl() : _schema( taiyi::schema::get_schema_for_type< value_type >() ) {}
        virtual ~index_info_impl() {}
        
        virtual std::shared_ptr< abstract_schema > get_schema() override { return _schema; }
        
        std::shared_ptr< abstract_schema > _schema;
    };

    template< typename MultiIndexType >
    void _add_index_impl( database& db )
    {
        db.add_index< MultiIndexType >();
        std::shared_ptr< chainbase::index_extension > ext = std::make_shared< index_info_impl< MultiIndexType > >();
        db.add_index_extension< MultiIndexType >( ext );
    }

    template< typename MultiIndexType >
    void add_core_index( database& db )
    {
        _add_index_impl< MultiIndexType >( db );
    }

    template< typename MultiIndexType >
    void add_plugin_index( database& db )
    {
        db._plugin_index_signal.connect( [&db](){ _add_index_impl< MultiIndexType >(db); } );
    }

} } //taiyi::chain

#define TAIYI_ADD_CORE_INDEX(db, index_name)                                                                 \
   do {                                                                                                      \
      taiyi::chain::add_core_index< index_name >( db );                                                      \
      taiyi::chain::index_delegate delegate;                                                                 \
      delegate.set_index_type =                                                                              \
         []( database& _db, mira::index_type type, const boost::filesystem::path& p, const boost::any& cfg ) \
            { _db.get_mutable_index< index_name >().mutable_indices().set_index_type( type, p, cfg ); };     \
      db.set_index_delegate( #index_name, std::move( delegate ) );                                           \
   } while( false )

#define TAIYI_ADD_PLUGIN_INDEX(db, index_name)                                                               \
   do {                                                                                                      \
      taiyi::chain::add_plugin_index< index_name >( db );                                                    \
      taiyi::chain::index_delegate delegate;                                                                 \
      delegate.set_index_type =                                                                              \
         []( database& _db, mira::index_type type, const boost::filesystem::path& p, const boost::any& cfg ) \
            { _db.get_mutable_index< index_name >().mutable_indices().set_index_type( type, p, cfg ); };     \
      db.set_index_delegate( #index_name, std::move( delegate ) );                                           \
   } while( false )

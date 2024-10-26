file(GLOB HEADERS "*.hpp")

add_library( siming_plugin
             siming_plugin.cpp
             block_producer.cpp
             ${HEADERS}
           )

target_link_libraries( siming_plugin p2p_plugin chain_plugin appbase taiyi_chain taiyi_utilities taiyi_utilities )
target_include_directories( siming_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      siming_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   siming_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

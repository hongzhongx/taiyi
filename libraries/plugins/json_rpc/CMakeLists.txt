file(GLOB HEADERS "*.hpp")

add_library( json_rpc_plugin
             json_rpc_plugin.cpp
             ${HEADERS} )

target_link_libraries( json_rpc_plugin chainbase appbase taiyi_chain fc )
target_include_directories( json_rpc_plugin 
   PUBLIC
   "${CMAKE_CURRENT_SOURCE_DIR}" 
   "${CMAKE_SOURCE_DIR}/libraries"
)

if( CLANG_TIDY_EXE )
   set_target_properties(
      json_rpc_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   json_rpc_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

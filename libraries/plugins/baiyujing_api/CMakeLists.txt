file(GLOB HEADERS "*.hpp")
add_library( baiyujing_api_plugin
             baiyujing_api.cpp
             baiyujing_api_plugin.cpp
             baiyujing_api_legacy_asset.cpp
             baiyujing_api_legacy_operations.cpp
             baiyujing_api_legacy_objects.cpp
             ${HEADERS} )

target_link_libraries( baiyujing_api_plugin
   account_by_key_api_plugin
   account_history_api_plugin
   database_api_plugin
   block_api_plugin
   network_broadcast_api_plugin
   taiyi_utilities )

target_include_directories( baiyujing_api_plugin
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/libraries"
  PRIVATE
)

if( CLANG_TIDY_EXE )
   set_target_properties(
      baiyujing_api_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   baiyujing_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/taiyi/baiyujing_api_plugin" )

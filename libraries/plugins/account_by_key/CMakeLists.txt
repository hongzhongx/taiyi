file(GLOB HEADERS "*.hpp")

add_library( account_by_key_plugin
             account_by_key_plugin.cpp
             ${HEADERS}
           )

target_link_libraries( account_by_key_plugin chain_plugin taiyi_chain taiyi_protocol )
target_include_directories( account_by_key_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      account_by_key_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   account_by_key_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

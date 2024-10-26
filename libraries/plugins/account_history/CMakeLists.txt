file(GLOB HEADERS "*.hpp")

add_library( account_history_plugin
             account_history_plugin.cpp
             ${HEADERS}
           )

target_link_libraries( account_history_plugin
   rocksdb chain_plugin taiyi_chain taiyi_protocol json_rpc_plugin rocksdb
   )

target_include_directories( account_history_plugin
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
          "${CMAKE_SOURCE_DIR}/libraries/vendor/rocksdb/include"
   )

install( TARGETS
   account_history_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

file(GLOB HEADERS "*.hpp")
add_library( taiyi_schema schema.cpp ${HEADERS} )
target_link_libraries( taiyi_schema fc )
target_include_directories( taiyi_schema
                            PUBLIC 
                            "${CMAKE_CURRENT_SOURCE_DIR}"
                            "${CMAKE_SOURCE_DIR}/libraries" )

install( TARGETS
   taiyi_schema

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/taiyi/schema" )

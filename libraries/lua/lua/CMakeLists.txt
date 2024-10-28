include_directories(${PROJECT_SOURCE_DIR}/lib )

add_executable( mylua lua.c )

target_link_libraries( mylua PRIVATE  lua  )


install( TARGETS
   mylua

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

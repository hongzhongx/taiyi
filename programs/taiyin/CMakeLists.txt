add_executable( taiyin main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling taiyin with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

if( TAIYI_STATIC_BUILD )
   target_link_libraries( taiyin PRIVATE
      "-static-libstdc++ -static-libgcc"
      appbase
      taiyi_utilities
      taiyi_plugins
      ${CMAKE_DL_LIBS}
      ${PLATFORM_SPECIFIC_LIBS}
   )
else( TAIYI_STATIC_BUILD )
   target_link_libraries( taiyin PRIVATE
      appbase
      taiyi_utilities
      taiyi_plugins
      ${CMAKE_DL_LIBS}
      ${PLATFORM_SPECIFIC_LIBS}
   )
endif( TAIYI_STATIC_BUILD )

if( CLANG_TIDY_EXE )
   set_target_properties(
      taiyin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   taiyin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

file(GLOB HEADERS "*.hpp")

add_executable( danuo main.cpp cli.cpp ${HEADERS})

if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling danuo with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

# begin editline stuff
if(WIN32)
   target_compile_definitions( danuo PRIVATE _CRT_NONSTDC_NO_DEPRECATE )
else(WIN32)
   target_compile_definitions( danuo PRIVATE HAVE_EDITLINE )
endif(WIN32)
# end editline stuff 

 if( TAIYI_STATIC_BUILD )
   target_link_libraries( danuo PRIVATE
                          "-static-libstdc++ -static-libgcc"
                          taiyi_net taiyi_chain taiyi_protocol taiyi_utilities taiyi_danuo baiyujing_api_plugin fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
else( TAIYI_STATIC_BUILD )
   target_link_libraries( danuo PRIVATE
                          taiyi_net taiyi_chain taiyi_protocol taiyi_utilities taiyi_danuo baiyujing_api_plugin fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
endif( TAIYI_STATIC_BUILD )

if( CLANG_TIDY_EXE )
   set_target_properties(
      danuo PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

if(MSVC)
  set_source_files_properties( main.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   danuo

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

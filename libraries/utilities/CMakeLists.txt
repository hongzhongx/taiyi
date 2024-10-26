list( APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/libraries/fc/GitVersionGen" )
include( GetGitRevisionDescription )
get_git_head_revision(GIT_REFSPEC TAIYI_GIT_REVISION_SHA)
get_git_unix_timestamp(TAIYI_GIT_REVISION_UNIX_TIMESTAMP)
git_describe(TAIYI_GIT_REVISION_DESCRIPTION --tags)
if(NOT TAIYI_GIT_REVISION_DESCRIPTION)
    set(TAIYI_GIT_REVISION_DESCRIPTION "unknown")
endif(NOT TAIYI_GIT_REVISION_DESCRIPTION)

if(TAIYI_GIT_REVISION_UNIX_TIMESTAMP MATCHES "HEAD-HASH-NOTFOUND")
  SET(TAIYI_GIT_REVISION_UNIX_TIMESTAMP 0)
endif()

file(GLOB HEADERS "./*.hpp")

set(sources
   benchmark_dumper.cpp
   key_conversion.cpp
   string_escape.cpp
   tempdir.cpp
   words.cpp
   logging_config.cpp
   database_configuration.cpp
   ${HEADERS})

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/git_revision.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp" @ONLY)
list(APPEND sources "${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp")

add_library( taiyi_utilities
             ${sources}
             ${HEADERS} )
target_link_libraries( taiyi_utilities fc )
target_include_directories( taiyi_utilities
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      taiyi_utilities PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   taiyi_utilities

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/taiyi/utilities" )

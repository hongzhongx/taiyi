file(GLOB HEADERS "*.hpp")

find_package( Perl )
find_package( Doxygen )

if( PERL_FOUND AND DOXYGEN_FOUND AND NOT "${CMAKE_GENERATOR}" STREQUAL "Ninja" )
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile )
  add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/doxygen/perlmod/DoxyDocs.pm
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                      COMMAND ${DOXYGEN_EXECUTABLE}
                      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile ${CMAKE_CURRENT_SOURCE_DIR}/xuanpin.hpp )

  add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp
                      COMMAND PERLLIB=${CMAKE_CURRENT_BINARY_DIR} ${PERL_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_api_documentation.pl ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp.new
                      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp.new ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp
                      COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp.new
                      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/generate_api_documentation.pl ${CMAKE_CURRENT_BINARY_DIR}/doxygen/perlmod/DoxyDocs.pm )
else()
  # no perl and doxygen, generate the best docs we can at runtime from reflection
  add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp
                      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/api_documentation_standin.cpp ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp
                      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/api_documentation_standin.cpp )
endif()

add_library( taiyi_xuanpin xuanpin.cpp remote_node_api.cpp ansi.cpp ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp ${HEADERS} )
target_link_libraries( taiyi_xuanpin PRIVATE taiyi_net taiyi_chain taiyi_protocol taiyi_utilities fc baiyujing_api_plugin ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
target_include_directories( taiyi_xuanpin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      taiyi_xuanpin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

if(MSVC)
  set_source_files_properties( xuanpin.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   taiyi_xuanpin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/taiyi/xuanpin" )

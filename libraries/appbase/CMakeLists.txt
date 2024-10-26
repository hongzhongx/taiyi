# Defines AppBase library target.
cmake_minimum_required( VERSION 3.8 )
project( AppBase )

file(GLOB HEADERS "./*.hpp")

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")

# CMP0167: The FindBoost module is removed.
# New in CMake 3.30. https://cmake.org/cmake/help/latest/policy/CMP0167.html
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 OLD)
endif()

SET(BOOST_COMPONENTS)
LIST(APPEND BOOST_COMPONENTS thread date_time system filesystem chrono program_options unit_test_framework locale)
set( Boost_USE_STATIC_LIBS ON CACHE STRING "ON or OFF" )
FIND_PACKAGE(Boost 1.58 REQUIRED COMPONENTS ${BOOST_COMPONENTS})

if( APPLE )
  # Apple Specific Options Here
  message( STATUS "Configuring AppBase on OS X" )
  set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++14 -stdlib=libc++ -Wall -Wno-conversion -Wno-deprecated-declarations" )
else( APPLE )
  # Linux Specific Options Here
  message( STATUS "Configuring AppBase on Linux" )
  set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++14 -Wall" )
  set( rt_library rt )
  set( pthread_library pthread)
  if ( FULL_STATIC_BUILD )
    set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
  endif ( FULL_STATIC_BUILD )
endif( APPLE )


if(ENABLE_COVERAGE_TESTING)
    SET(CMAKE_CXX_FLAGS "--coverage ${CMAKE_CXX_FLAGS}")
endif()

add_library( appbase
             application.cpp
             ${HEADERS}
           )

target_link_libraries( appbase ${Boost_LIBRARIES})

target_include_directories( appbase
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" ${Boost_INCLUDE_DIR})

INSTALL( TARGETS
   appbase

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/appbase" )

OPTION( APPBASE_EXAMPLES "build appbase examples projects (ON OR OFF)" ON )
MESSAGE( STATUS "APPBASE_EXAMPLES: ${APPBASE_EXAMPLES}" )
if( APPBASE_EXAMPLES )
  add_subdirectory( examples )
endif()

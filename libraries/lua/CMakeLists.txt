PROJECT(lua)
CMAKE_MINIMUM_REQUIRED(VERSION 3.8)

# flags
# if (${CMAKE_COMPILER_IS_GNUCXX})
# 	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
# endif()
# if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
# 	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
# endif()

if( APPLE )
    set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -D_XOPEN_SOURCE=0" )
endif()

add_subdirectory( lib )
#add_subdirectory( lua )
#add_subdirectory( luac )


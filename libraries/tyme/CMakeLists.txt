cmake_minimum_required(VERSION 3.20)

project(tyme CXX)

file(GLOB SOURCES "./*.cpp")
file(GLOB HEADERS "./*.h")

ADD_LIBRARY(tyme
    SHARED
    ${SOURCES}
    ${HEADERS} )

target_link_libraries( tyme
    fc )

target_include_directories( tyme 
    PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}" 
    "${CMAKE_SOURCE_DIR}/libraries" )

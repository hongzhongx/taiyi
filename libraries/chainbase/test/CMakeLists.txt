file(GLOB UNIT_TEST_SOURCES "*.cpp")
file(GLOB UNIT_TEST_HEADERS "*.hpp")
add_executable( chainbase_mira_test ${UNIT_TEST_SOURCES} ${UNIT_TEST_HEADERS} )
target_link_libraries( chainbase_mira_test mira fc chainbase taiyi_utilities ${PLATFORM_SPECIFIC_LIBS} )

target_include_directories( chainbase_mira_test
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/libraries"
        ${Boost_INCLUDE_DIR}
)

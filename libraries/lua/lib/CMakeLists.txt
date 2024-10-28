file(GLOB SOURCES "./*.c")
file(GLOB HEADERS "./*.h")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}"
                    /usr/include
                    )
IF(DEFINE_VERSION)
    ADD_DEFINITIONS(-DDEFINE_VERSION)
ENDIF(DEFINE_VERSION)

ADD_LIBRARY(lua ${SOURCES} ${HEADERS})

target_link_libraries( lua PUBLIC m)
target_include_directories( lua PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" )

install( TARGETS
   lua

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

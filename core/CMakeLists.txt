set(COCO_DOCUMENT_ROOT ${CMAKE_INSTALL_PREFIX}/share/coco)
add_definitions(-DCOCO_DOCUMENT_ROOT="${COCO_DOCUMENT_ROOT}")

set(CORE_SOURCE_FILE ${CMAKE_CURRENT_LIST_DIR}/src/core.cpp
                     ${CMAKE_CURRENT_LIST_DIR}/src/register.cpp
                     ${CMAKE_CURRENT_LIST_DIR}/src/web_server.cpp
    )
set(CORE_INCLUDE_FILE ${CMAKE_CURRENT_LIST_DIR}/include/coco/core.h
                      ${CMAKE_CURRENT_LIST_DIR}/include/coco/register.h
                      ${CMAKE_CURRENT_LIST_DIR}/include/coco/core_impl.hpp
                      ${CMAKE_CURRENT_LIST_DIR}/include/coco/web_server.h
    )
set(UTIL_INCLUDE_FILE ${CMAKE_CURRENT_LIST_DIR}/include/coco/util/impl.hpp
                      ${CMAKE_CURRENT_LIST_DIR}/include/coco/util/logging.h
                      ${CMAKE_CURRENT_LIST_DIR}/include/coco/util/timing.h)

set(DEPS_SOURCE_FILE ${MONGOOSE_SOURCE_FILE}
                    ${JSON_SOURCE_FILE}
    )
set(DEPS_INCLUDE_FILE ${MONGOOSE_INCLUDE_FILE}
                     ${JSON_INCLUDE_FILE}
    )

if(WIN32)
    add_definitions(-DWIN32)
endif()

include_directories(include)

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

include_directories(${CMAKE_SOURCE_DIR}/extern)

option(STATIC_BUILD "build static libraries" OFF)
if(STATIC_BUILD)
    add_library(${PROJECT_NAME} ${CORE_SOURCE_FILE} ${CORE_INCLUDE_FILE} ${UTIL_INCLUDE_FILE}
                                ${DEPS_SOURCE_FILE} ${DEPS_INCLUDE_FILE})
else()
    add_library(${PROJECT_NAME} SHARED ${CORE_SOURCE_FILE} ${CORE_INCLUDE_FILE} ${UTIL_INCLUDE_FILE}
                                       ${DEPS_SOURCE_FILE} ${DEPS_INCLUDE_FILE})
endif()

set(DEPS_LIB dl)

target_link_libraries(${PROJECT_NAME} ${DEPS_LIB})


# INSTALLING THE LIBRARY AND THE INCLUDE FILES

install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/)
install(TARGETS coco DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/)

install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coco)
install(FILES ${CORE_INCLUDE_FILE} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coco)
install(FILES ${UTIL_INCLUDE_FILE} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coco/util)

file(GLOB WEB_EDITOR_FILES LIST_DIRECTORIES=false "${CMAKE_SOURCE_DIR}/cocoaine/*.css"
                                                  "${CMAKE_SOURCE_DIR}/cocoaine/*.js"
                                                  "${CMAKE_SOURCE_DIR}/cocoaine/*.html"
    )
install(DIRECTORY DESTINATION ${COCO_DOCUMENT_ROOT})
install(FILES ${WEB_EDITOR_FILES} DESTINATION ${COCO_DOCUMENT_ROOT})

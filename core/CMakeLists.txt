set(COCO_DOCUMENT_ROOT ${CMAKE_INSTALL_PREFIX}/share/coco)
add_definitions(-DCOCO_DOCUMENT_ROOT="${COCO_DOCUMENT_ROOT}")

set(CORE_SOURCE_FILE ${CMAKE_CURRENT_LIST_DIR}/src/execution.cpp
                     ${CMAKE_CURRENT_LIST_DIR}/src/task.cpp
                     ${CMAKE_CURRENT_LIST_DIR}/src/connection.cpp
                     ${CMAKE_CURRENT_LIST_DIR}/src/register.cpp
    )
set(CORE_INCLUDE_FILE ${CMAKE_CURRENT_LIST_DIR}/include/coco/task_impl.hpp
                      ${CMAKE_CURRENT_LIST_DIR}/include/coco/connection_impl.hpp

                      ${CMAKE_CURRENT_LIST_DIR}/include/coco/execution.h
                      ${CMAKE_CURRENT_LIST_DIR}/include/coco/task.h
                      ${CMAKE_CURRENT_LIST_DIR}/include/coco/connection.h
                      ${CMAKE_CURRENT_LIST_DIR}/include/coco/register.h
    )
set(UTIL_INCLUDE_FILE ${CMAKE_CURRENT_LIST_DIR}/include/coco/util/generics.hpp
                      ${CMAKE_CURRENT_LIST_DIR}/include/coco/util/memory.hpp
                      ${CMAKE_CURRENT_LIST_DIR}/include/coco/util/accesses.hpp
                      ${CMAKE_CURRENT_LIST_DIR}/include/coco/util/logging.h
                      ${CMAKE_CURRENT_LIST_DIR}/include/coco/util/timing.h
                      ${CMAKE_CURRENT_LIST_DIR}/include/coco/util/threading.h
        ${CMAKE_CURRENT_LIST_DIR}/include/coco/util/linux_sched.h)
set(WEB_SOURCE_FILE  ${CMAKE_CURRENT_LIST_DIR}/src/web_server.cpp
    )
set(WEB_INCLUDE_FILE  ${CMAKE_CURRENT_LIST_DIR}/include/coco/web_server/web_server.h
    )

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
                                ${WEB_SOURCE_FILE} ${WEB_INCLUDE_FILE}
                                ${DEPS_SOURCE_FILE} ${DEPS_INCLUDE_FILE})
else()
    add_library(${PROJECT_NAME} SHARED ${CORE_SOURCE_FILE} ${CORE_INCLUDE_FILE} ${UTIL_INCLUDE_FILE}
                                       ${WEB_SOURCE_FILE} ${WEB_INCLUDE_FILE}
                                       ${DEPS_SOURCE_FILE} ${DEPS_INCLUDE_FILE})
endif()

if(NOT WIN32)
set(DEPS_LIB dl)
else()
set(DEPS_LIB wsock32)
endif()

target_link_libraries(${PROJECT_NAME} ${DEPS_LIB})


# INSTALLING THE LIBRARY AND THE INCLUDE FILES

install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/)
install(TARGETS coco DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/)

if (INSTALL_INCLUDE)
    install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coco)
    install(FILES ${CORE_INCLUDE_FILE} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coco)
    install(FILES ${UTIL_INCLUDE_FILE} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coco/util)
    install(FILES ${WEB_INCLUDE_FILE} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coco/web_server)
endif()

option(INSTALL_WEB_FILES "Install the web server files" ON)
if (INSTALL_WEB_FILES)
    file(GLOB WEB_EDITOR_FILES LIST_DIRECTORIES=false "${CMAKE_SOURCE_DIR}/web_viewer/*.css"
                                                      "${CMAKE_SOURCE_DIR}/web_viewer/*.js"
                                                      "${CMAKE_SOURCE_DIR}/web_viewer/*.html"
        )
    file(GLOB WEB_EDITOR_IMAGES LIST_DIRECTORIES=false "${CMAKE_SOURCE_DIR}/web_viewer/images/*.png"
        )
    install(DIRECTORY DESTINATION ${COCO_DOCUMENT_ROOT})
    install(DIRECTORY DESTINATION ${COCO_DOCUMENT_ROOT}/images)
    install(FILES ${WEB_EDITOR_FILES} DESTINATION ${COCO_DOCUMENT_ROOT})
    install(FILES ${WEB_EDITOR_IMAGES} DESTINATION ${COCO_DOCUMENT_ROOT}/images)
endif()

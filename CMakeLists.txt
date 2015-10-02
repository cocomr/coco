cmake_minimum_required(VERSION 2.8)
Project (coco)

set(PROJECT_VERSION "1.0.1")
string(REGEX MATCHALL "[0-9]" PROJECT_VERSION_PARTS "${PROJECT_VERSION}")
list(GET PROJECT_VERSION_PARTS 0 PROJECT_VERSION_MAJOR)
list(GET PROJECT_VERSION_PARTS 1 PROJECT_VERSION_MINOR)
list(GET PROJECT_VERSION_PARTS 2 PROJECT_VERSION_PATCH)
set(PROJECT_SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_LOG_DYN_LINK=1 -std=c++11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
include_directories(include)
include_directories(extern)


if(WIN32)
add_definitions(-DWIN32)
endif()

set(SOURCE_FILE src/coco_core.cpp
                src/coco_register.cpp
    )
set(INCLUDE_FILE include/coco/coco_core.h
                 include/coco/coco_register.h
                 include/coco/coco_impl.hpp
                 include/coco/coco_core_impl.hpp
    )

set(SOURCERT_FILE src/coco_launcher.cpp
    )
set(INCLUDERT_FILE include/coco/coco_launcher.h
    )

set(UTIL_SOURCE_FILE src/util/coco_profiling.cpp
                     src/util/coco_logging.cpp
                     src/util/coco_timing.cpp
    )
set(UTIL_INCLUDE_FILE include/coco/util/coco_profiling.h
                      include/coco/util/coco_logging.h
                      include/coco/util/coco_timing.h
    )

set(XML_SOURCE_FILE extern/tinyxml2/tinyxml2.cpp
    )
set(XML_INCLUDE_FILE extern/tinyxml2/tinyxml2.h
    )

#set(COCO_FILES ${SOURCE_FILE} ${INCLUDE_FILE} 
#               ${UTIL_SOURCE_FILE} ${UTIL_INCLUDE_FILE}
#    )
set(COCO_FILES ${SOURCE_FILE} ${UTIL_SOURCE_FILE}
   )
#set(COCORT_FILES ${SOURCERT_FILE} ${INCLUDERT_FILE}
#                 ${XML_SOURCE_FILE} ${XML_INCLUDE_FILE}
#    )
set(COCORT_FILES ${SOURCERT_FILE} ${XML_SOURCE_FILE}
    )

option(STATIC_BUILD "build static libraries" OFF)
if(STATIC_BUILD)
    add_library(coco_util ${UTIL_SOURCE_FILE}
        )
    add_library(coco ${COCO_FILES}
        )
    add_library(cocort ${COCORT_FILES}
        )
else()
    add_library(coco_util SHARED ${UTIL_SOURCE_FILE}
        )
    add_library(coco SHARED ${COCO_FILES}
        )
    add_library(cocort SHARED ${COCORT_FILES}
        )
endif()

target_link_libraries(coco coco_util dl)
target_link_libraries(cocort coco coco_util dl)


option(PROFILING "Insert profiling call" OFF)
if(PROFILING)
    add_definitions(-DPROFILING)
endif()

option(BUILD_SAMPLES "build the provided sample" OFF)
if(BUILD_SAMPLES)
    add_subdirectory(samples)
endif(BUILD_SAMPLES)

find_package(Doxygen)
if(DOXYGEN_FOUND)
  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif(DOXYGEN_FOUND)

set(CMAKE_INSTALL_PREFIX /usr/local)
install(TARGETS coco DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/)
install(TARGETS cocort DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/)

install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coco)
install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coco/util)
install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coco/tinyxml2)

install(FILES include/coco/coco.h ${INCLUDE_FILE} ${INCLUDERT_FILE} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coco)
install(FILES ${UTIL_INCLUDE_FILE} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coco/util)
install(FILES extern/tinyxml2/tinyxml2.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coco/tinyxml2)

CONFIGURE_FILE("${PROJECT_SOURCE_DIR}/config.cmake.in" "${PROJECT_BINARY_DIR}/Find${PROJECT_NAME}.cmake")
INSTALL(FILES "${PROJECT_BINARY_DIR}/Find${PROJECT_NAME}.cmake" DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/)


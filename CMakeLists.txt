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

set(CMAKE_INSTALL_PREFIX /usr/local)

set(XML_SOURCE_FILE ${CMAKE_CURRENT_LIST_DIR}/extern/tinyxml2/tinyxml2.cpp
    )
set(XML_INCLUDE_FILE ${CMAKE_CURRENT_LIST_DIR}/extern/tinyxml2/tinyxml2.h
    )

include_directories(.)
include_directories(src)
include_directories(extern)

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

if(WIN32)
add_definitions(-DWIN32)
endif()

option(PROFILING "Insert profiling call" OFF)
if(PROFILING)
    add_definitions(-DPROFILING)
endif()

option(STATIC_BUILD "build static libraries" OFF)

add_subdirectory(util)
add_subdirectory(src)
add_subdirectory(launcher)

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

CONFIGURE_FILE("${PROJECT_SOURCE_DIR}/config.cmake.in" "${PROJECT_BINARY_DIR}/Find${PROJECT_NAME}.cmake")
INSTALL(FILES "${PROJECT_BINARY_DIR}/Find${PROJECT_NAME}.cmake" DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/)


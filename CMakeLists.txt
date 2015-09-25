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

#set(MYBOOST -L/opt/local/lib -lboost_system-mt -lboost_log_setup-mt -lboost_log-mt -lboost_thread-mt)
find_package(Boost REQUIRED)
include_directories(Boost_INCLUDE_DIRS)

include_directories(src)
include_directories(extern)

set(SOURCE_FILE src/coco_core.cpp
                src/coco_profiling.cpp
                src/coco_launcher.cpp
                src/coco_logging.cpp
                src/coco_timing.cpp
    )
set(INCLUDE_FILE src/coco_core.hpp
                 src/coco_impl.hpp
                 src/coco_profiling.h
                 src/coco_launcher.h
                 src/coco_logging.h
                 src/coco_timing.h
    )

 set(EXTERN_SOURCE_FILE extern/tinyxml2/tinyxml2.cpp)
 set(EXTERN_INCLUDE_FILE extern/tinyxml2/tinyxml2.h)

option(STATIC_BUILD "build static libraries" OFF)
if(STATIC_BUILD)
    add_library(coco ${SOURCE_FILE} ${EXTERN_SOURCE_FILE} ${INCLUDE_FILE})
else()
    add_library(coco SHARED ${SOURCE_FILE} ${EXTERN_SOURCE_FILE} ${INCLUDE_FILE})
endif()
target_link_libraries(coco dl)

option(PROFILING "Insert profiling call" OFF)
if(PROFILING)
    add_definitions(-DPROFILING)
endif()


option(BUILD_SAMPLES "build the provided sample" OFF)
if(BUILD_SAMPLES)
    if(STATIC_BUILD)
        add_library(cococ1 STATIC samples/cococ1.cpp)
        target_link_libraries(cococ1 coco)
        add_library(cococ2 STATIC samples/cococ2.cpp)
        target_link_libraries(cococ2 coco)
        add_executable(cocoapp samples/cocoapp.cpp)
        add_executable(cocoapp2 samples/cocoapp2.cpp)

        set(COCOFORCE "-u _EzTask1_spec -u _EzTask2_spec")
        #set(CMAKE_LINKER_FLAGS ${})
        target_link_libraries(cocoapp coco -lpthread cococ1 cococ2 ${COCOFORCE})
        target_link_libraries(cocoapp2 coco -lpthread cococ1 cococ2 ${COCOFORCE})
    else()
        add_library(cococ1 SHARED samples/cococ1.cpp)
        target_link_libraries(cococ1 coco)
        add_library(cococ2 SHARED samples/cococ2.cpp)
        target_link_libraries(cococ2 coco)
        add_executable(cocoapp samples/cocoapp.cpp)
        target_link_libraries(cocoapp coco -lpthread)
        add_executable(cocoapp2 samples/cocoapp2.cpp)
        target_link_libraries(cocoapp2 coco -lpthread)
    endif()
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
install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coco)
install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coco/tinyxml2)
install(FILES include/coco.h ${INCLUDE_FILE} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coco)
install(FILES extern/tinyxml2/tinyxml2.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coco/tinyxml2)
CONFIGURE_FILE("${PROJECT_SOURCE_DIR}/config.cmake.in" "${PROJECT_BINARY_DIR}/Find${PROJECT_NAME}.cmake")
INSTALL(FILES "${PROJECT_BINARY_DIR}/Find${PROJECT_NAME}.cmake" DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/)
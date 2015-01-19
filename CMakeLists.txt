
#function(append name value)
#	set(${name} ${${name}} ${value} PARENT_SCOPE)
#endfunction()
#add_compile_options(-std=c++11)
#append(CMAKE_CXX_FLAGS -std=c++11)
#add_definitions(-DDBOOST_LOG_DYN_LINK=1)

cmake_minimum_required(VERSION 2.8)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_LOG_DYN_LINK=1 -std=c++11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")

set(MYBOOST -L/opt/local/lib -lboost_system-mt -lboost_log_setup-mt -lboost_log-mt -lboost_thread-mt)
include_directories(${PROJECT_SOURCE_DIR}/extern/lzfx-0.1)
include_directories(${PROJECT_SOURCE_DIR}/extern)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    include_directories(/opt/local/include)
endif()

include_directories(src)
include_directories(tinyxml2/)

set(SOURCE_FILE src/ezoro.cpp
                src/coco_profiling.cpp
                tinyxml2/tinyxml2.cpp)

option(STATIC_BUILD "build static libraries" OFF)
if(STATIC_BUILD)
    add_library(ezoro ${SOURCE_FILE})
else()
    add_library(ezoro SHARED ${SOURCE_FILE})
endif()
target_link_libraries(ezoro dl)

option(BUILD_SAMPLES "build the provided sample" OFF)
if(BUILD_SAMPLES)
    if(STATIC_BUILD)
        add_library(ezoroc1 STATIC samples/ezoroc1.cpp)
        target_link_libraries(ezoroc1 ezoro)
        add_library(ezoroc2 STATIC samples/ezoroc2.cpp)
        target_link_libraries(ezoroc2 ezoro)
        add_executable(ezoroapp samples/ezoroapp.cpp)
        add_executable(ezoroapp2 samples/ezoroapp2.cpp)

        set(COCOFORCE "-u _EzTask1_spec -u _EzTask2_spec")
        #set(CMAKE_LINKER_FLAGS ${})
        target_link_libraries(ezoroapp ezoro -lpthread ezoroc1 ezoroc2 ${COCOFORCE})
        target_link_libraries(ezoroapp2 ezoro -lpthread ezoroc1 ezoroc2 ${COCOFORCE})
    else()
        add_library(ezoroc1 SHARED samples/ezoroc1.cpp)
        target_link_libraries(ezoroc1 ezoro)
        add_library(ezoroc2 SHARED samples/ezoroc2.cpp)
        target_link_libraries(ezoroc2 ezoro)
        add_executable(ezoroapp samples/ezoroapp.cpp)
        target_link_libraries(ezoroapp ezoro -lpthread)
        add_executable(ezoroapp2 samples/ezoroapp2.cpp)
        target_link_libraries(ezoroapp2 ezoro -lpthread)
    endif()
endif(BUILD_SAMPLES)

find_package(Doxygen)
if(DOXYGEN_FOUND)
  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif(DOXYGEN_FOUND)




#function(append name value)
#	set(${name} ${${name}} ${value} PARENT_SCOPE)
#endfunction()
#add_compile_options(-std=c++11)
#append(CMAKE_CXX_FLAGS -std=c++11)
#add_definitions(-DDBOOST_LOG_DYN_LINK=1)

cmake_minimum_required(VERSION 2.8)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_LOG_DYN_LINK=1 -std=c++11")

set(MYBOOST -L/opt/local/lib -lboost_system-mt -lboost_log_setup-mt -lboost_log-mt -lboost_thread-mt)
include_directories(${PROJECT_SOURCE_DIR}/extern/lzfx-0.1)
include_directories(${PROJECT_SOURCE_DIR}/extern)

include_directories(/opt/local/include)
include_directories(src)
include_directories(tinyxml2/)

add_library(ezoro SHARED src/ezoro.cpp
                         tinyxml2/tinyxml2.cpp)
target_link_libraries(ezoro dl)

option(BUILD_APP "build the provided app" OFF)
if(BUILD_APP)
    add_library(ezoroc1 SHARED app/ezoroc1.cpp)
    target_link_libraries(ezoroc1 ezoro)

    add_library(ezoroc2 SHARED app/ezoroc2.cpp)
    target_link_libraries(ezoroc2 ezoro)

    add_executable(ezoroapp app/ezoroapp.cpp)
    target_link_libraries(ezoroapp ezoro)
endif(BUILD_APP)

find_package(Doxygen)
if(DOXYGEN_FOUND)
  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif(DOXYGEN_FOUND)


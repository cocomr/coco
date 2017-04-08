cmake_minimum_required(VERSION 2.8)
Project (coco)

if(WIN32)   
add_definitions(-DWIN32)
endif()

#The following is needed if MINGW has NO posix threads
#if(MINGW)   
#add_definitions(-DMINGW_STD_THREADS)
#include_directories(extern/mingw-std-threads)
#endif()

# Set Project version
set(PROJECT_VERSION "0.5.4")
string(REGEX MATCHALL "[0-9]" PROJECT_VERSION_PARTS "${PROJECT_VERSION}")
list(GET PROJECT_VERSION_PARTS 0 PROJECT_VERSION_MAJOR)
list(GET PROJECT_VERSION_PARTS 1 PROJECT_VERSION_MINOR)
list(GET PROJECT_VERSION_PARTS 2 PROJECT_VERSION_PATCH)
set(PROJECT_SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}-${PROJECT_VERSION_PATCH}")
message(STATUS "Making version ${PROJECT_SOVERSION}")
# Set compiler options
IF(NOT CMAKE_BUILD_TYPE )
   SET( CMAKE_BUILD_TYPE "Debug" )
ENDIF()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -DBOOST_LOG_DYN_LINK=1 -std=c++11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")

# Specify install directory
if(NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX /usr/local)
endif()
message(STATUS Install Prefix ${CMAKE_INSTALL_PREFIX})

add_definitions(-DCOCO_BUILDING)

option(INSTALL_INCLUDE "Install also the include files" ON)
# Compiling library and executable
add_subdirectory(extern)
add_subdirectory(core)
add_subdirectory(launcher)
add_subdirectory(include)
# Building samples
option(BUILD_SAMPLES "build the provided sample" ON)
if(BUILD_SAMPLES)
    add_subdirectory(samples)
endif(BUILD_SAMPLES)

# Building documentation
find_package(Doxygen)
if(DOXYGEN_FOUND)
  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/doc/Doxyfile.in
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif(DOXYGEN_FOUND)


MESSAGE( STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}" )

# Creating Findcoco.cmake
CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/cmake/config.cmake.in" "${PROJECT_BINARY_DIR}/Find${PROJECT_NAME}.cmake")
INSTALL(FILES "${PROJECT_BINARY_DIR}/Find${PROJECT_NAME}.cmake"
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/)

# uninstall target
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)


# Stuff to create a debian package
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_DEBIAN_PACKAGE_VERSION ${PROJECT_SOVERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Filippo Brizzi <fi.brizzi@sssup.it>")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Coco Library")

# Set the correct architecture for the deb
set(COMMAND "dpkg --print-architecture")
execute_process(COMMAND dpkg --print-architecture COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE)
if(ARCHITECTURE)
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${ARCHITECTURE})
endif()
include(CPack)


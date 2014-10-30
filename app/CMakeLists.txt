cmake_minimum_required(VERSION 2.8)
Project (cocoARapp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11") 
set(CMAKE_BUILD_TYPE Release)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    #include_directories(/opt/local/include/)
endif()

include_directories(${EZORO_INCLUDE_DIR})
find_library(EZORO_LIBRARY ezoro)

add_executable(cocoARapp cocoARapp.cpp)

if(STATIC_BUILD)
    target_link_libraries(cocoARapp ${EZORO_LIBRARY} -lpthread ${COCOAR_LIBRARIES} ${COCOFORCE})
else()
    target_link_libraries(cocoARapp ${EZORO_LIBRARY} -lpthread)
endif()

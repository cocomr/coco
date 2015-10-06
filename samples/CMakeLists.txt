include_directories(../src/)

if(STATIC_BUILD)
    add_library(cococ1 STATIC cococ1.cpp)
    add_library(cococ2 STATIC cococ2.cpp)
else()
    add_library(cococ1 SHARED cococ1.cpp)
    add_library(cococ2 SHARED cococ2.cpp)
endif()

target_link_libraries(cococ1 coco)
target_link_libraries(cococ2 coco)


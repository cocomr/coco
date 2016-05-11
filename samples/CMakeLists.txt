include_directories(../core/)

if(STATIC_BUILD)
    add_library(component_1 STATIC component_1.cpp)
    add_library(component_2 STATIC component_2.cpp)
else()
    add_library(component_1 SHARED component_1.cpp)
    add_library(component_2 SHARED component_2.cpp)
endif()

target_link_libraries(component_1 coco)
target_link_libraries(component_2 coco)


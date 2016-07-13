include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/core/include)

add_library(component_1 SHARED ${CMAKE_CURRENT_LIST_DIR}/src/component_1.cpp)
add_library(component_2 SHARED ${CMAKE_CURRENT_LIST_DIR}/src/component_2.cpp)
add_library(pipeline_comps SHARED ${CMAKE_CURRENT_LIST_DIR}/src/pipeline_comps.cpp)

add_dependencies(component_1 coco)
target_link_libraries(component_1 coco)
add_dependencies(component_2 coco)
target_link_libraries(component_2 coco)
add_dependencies(pipeline_comps coco)
target_link_libraries(pipeline_comps coco)


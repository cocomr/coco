

set(LAUNCHER_SOURCE_FILE ${CMAKE_CURRENT_LIST_DIR}/src/graph_launcher.cpp
                         ${CMAKE_CURRENT_LIST_DIR}/src/xml_creator.cpp
                         ${CMAKE_CURRENT_LIST_DIR}/src/xml_parser.cpp
                         ${CMAKE_CURRENT_LIST_DIR}/src/graph_loader.cpp
                         ${CMAKE_CURRENT_LIST_DIR}/src/library_parser.cpp
                         ${CMAKE_CURRENT_LIST_DIR}/src/legacy/loader.cpp
						 ${XML_SOURCE_FILE}

)
    
set(LAUNCHER_INCLUDE_FILE ${CMAKE_CURRENT_LIST_DIR}/include/legacy/loader.h
                          ${CMAKE_CURRENT_LIST_DIR}/include/xml_creator.h
                          ${CMAKE_CURRENT_LIST_DIR}/include/graph_spec.h
                          ${CMAKE_CURRENT_LIST_DIR}/include/input_parser.h
                          ${CMAKE_CURRENT_LIST_DIR}/include/xml_parser.h
                          ${CMAKE_CURRENT_LIST_DIR}/include/graph_loader.h
                          ${CMAKE_CURRENT_LIST_DIR}/include/library_parser.h
						  ${XML_INCLUDE_FILE}
)

include_directories(include)
include_directories(${CMAKE_SOURCE_DIR}/core/include)


find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

set(DEPS coco ${Boost_LIBRARIES} dl pthread)

include_directories(${CMAKE_SOURCE_DIR}/extern)
include_directories(${CMAKE_SOURCE_DIR}/core)

add_executable(coco_launcher ${LAUNCHER_SOURCE_FILE} ${LAUNCHER_INCLUDE_FILE})
add_dependencies(coco_launcher coco)
target_link_libraries(coco_launcher ${DEPS})

install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/)
install(TARGETS coco_launcher DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/)



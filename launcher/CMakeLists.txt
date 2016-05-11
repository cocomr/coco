set(LAUNCHER_SOURCE_FILE ${CMAKE_CURRENT_LIST_DIR}/loader.cpp
                         #${CMAKE_CURRENT_LIST_DIR}/launcher.cpp
                         ${CMAKE_CURRENT_LIST_DIR}/xml_creator.cpp
                         ${CMAKE_CURRENT_LIST_DIR}/xml_parser.cpp
                         ${CMAKE_CURRENT_LIST_DIR}/graph_loader.cpp
						 ${XML_SOURCE_FILE}
    )
set(LAUNCHER_INCLUDE_FILE ${CMAKE_CURRENT_LIST_DIR}/loader.h
                          ${CMAKE_CURRENT_LIST_DIR}/xml_creator.h
                          ${CMAKE_CURRENT_LIST_DIR}/graph_spec.h
                          ${CMAKE_CURRENT_LIST_DIR}/input_parser.h
                          ${CMAKE_CURRENT_LIST_DIR}/xml_parser.h
                          ${CMAKE_CURRENT_LIST_DIR}/graph_loader.h
						  ${XML_INCLUDE_FILE}
    )
set(XML_SOURCE_FILE )
set(DEPS coco ${Boost_LIBRARIES} dl pthread)
message(${DEPS})
include_directories(../core)

add_executable(coco_launcher ${CMAKE_CURRENT_LIST_DIR}/launcher.cpp
                             ${LAUNCHER_SOURCE_FILE} ${LAUNCHER_INCLUDE_FILE})
add_executable(coco_graph_launcher ${CMAKE_CURRENT_LIST_DIR}/graph_launcher.cpp
                                   ${LAUNCHER_SOURCE_FILE} ${LAUNCHER_INCLUDE_FILE})

add_dependencies(coco_launcher coco)
target_link_libraries(coco_launcher ${DEPS})

add_dependencies(coco_graph_launcher coco)
target_link_libraries(coco_graph_launcher ${DEPS})

install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/)
install(TARGETS coco_launcher DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/)

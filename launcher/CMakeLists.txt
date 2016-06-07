set(COCO_DOCUMENT_ROOT ${CMAKE_INSTALL_PREFIX}/share/coco)
add_definitions(-DCOCO_DOCUMENT_ROOT="${COCO_DOCUMENT_ROOT}")

set(LAUNCHER_SOURCE_FILE ${CMAKE_CURRENT_LIST_DIR}/graph_launcher.cpp
                         ${CMAKE_CURRENT_LIST_DIR}/xml_creator.cpp
                         ${CMAKE_CURRENT_LIST_DIR}/xml_parser.cpp
                         ${CMAKE_CURRENT_LIST_DIR}/graph_loader.cpp
                         ${CMAKE_CURRENT_LIST_DIR}/library_parser.cpp
                         ${CMAKE_CURRENT_LIST_DIR}/legacy/loader.cpp
						 ${XML_SOURCE_FILE}
                         ${CMAKE_CURRENT_LIST_DIR}/web_server.cpp
                         ${MONGOOSE_SOURCE_FILE}
                         ${JSON_SOURCE_FILE}
)
    
set(LAUNCHER_INCLUDE_FILE ${CMAKE_CURRENT_LIST_DIR}/legacy/loader.h
                          ${CMAKE_CURRENT_LIST_DIR}/xml_creator.h
                          ${CMAKE_CURRENT_LIST_DIR}/graph_spec.h
                          ${CMAKE_CURRENT_LIST_DIR}/input_parser.h
                          ${CMAKE_CURRENT_LIST_DIR}/xml_parser.h
                          ${CMAKE_CURRENT_LIST_DIR}/graph_loader.h
                          ${CMAKE_CURRENT_LIST_DIR}/library_parser.h
						  ${XML_INCLUDE_FILE}
                          ${CMAKE_CURRENT_LIST_DIR}/web_server.h
                          ${MONGOOSE_INCLUDE_FILE}
                          ${JSON_INCLUDE_FILE}
)

set(DEPS coco ${Boost_LIBRARIES} dl pthread)

include_directories(../core)

add_executable(coco_launcher ${LAUNCHER_SOURCE_FILE} ${LAUNCHER_INCLUDE_FILE})
add_dependencies(coco_launcher coco)
target_link_libraries(coco_launcher ${DEPS})

install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/)
install(TARGETS coco_launcher DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/)

file(GLOB WEB_EDITOR_FILES RELATIVE ${CMAKE_SOURCE_DIR} "${CMAKE_SOURCE_DIR}/cocoaine/*.css"
                                                        "${CMAKE_SOURCE_DIR}/cocoaine/*.js"
                                                        "${CMAKE_SOURCE_DIR}/cocoaine/*.html"
    )

message(${WEB_EDITOR_FILES})
message(${COCO_DOCUMENT_ROOT})
install(DIRECTORY DESTINATION ${COCO_DOCUMENT_ROOT})
install(FILES "${CMAKE_SOURCE_DIR}/${WEB_EDITOR_FILES}" DESTINATION ${COCO_DOCUMENT_ROOT})

set(LAUNCHER_SOURCE_FILE ${CMAKE_CURRENT_LIST_DIR}/loader.cpp
						 ${CMAKE_CURRENT_LIST_DIR}/launcher.cpp
						 ${XML_SOURCE_FILE}
    )
set(LAUNCHER_INCLUDE_FILE ${CMAKE_CURRENT_LIST_DIR}/loader.h
						  ${XML_INCLUDE_FILE}
    )
set(XML_SOURCE_FILE )
set(DEPS coco coco_util boost_program_options dl pthread)

add_executable(coco_launcher ${LAUNCHER_SOURCE_FILE} ${LAUNCHER_INCLUDE_FILE})

add_dependencies(coco_launcher coco coco_util)
target_link_libraries(coco_launcher ${DEPS})

install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/)
install(TARGETS coco_launcher DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/)
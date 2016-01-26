set(CORE_SOURCE_FILE ${CMAKE_CURRENT_LIST_DIR}/core.cpp
                     ${CMAKE_CURRENT_LIST_DIR}/register.cpp
    )
set(CORE_INCLUDE_FILE ${CMAKE_CURRENT_LIST_DIR}/core.h
                      ${CMAKE_CURRENT_LIST_DIR}/register.h
                      ${CMAKE_CURRENT_LIST_DIR}/impl.hpp
                      ${CMAKE_CURRENT_LIST_DIR}/core_impl.hpp
    )
set(DEPS dl)

if(STATIC_BUILD)
    add_library(coco ${CORE_SOURCE_FILE} ${CORE_INCLUDE_FILE})
else()
	add_library(coco SHARED ${CORE_SOURCE_FILE} ${CORE_INCLUDE_FILE})
endif()

#add_dependencies(coco)
target_link_libraries(coco ${DEPS})

install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/)
install(TARGETS coco DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/)

if(INSTALL_INCLUDE)
	install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coco)
	install(FILES coco.h ${CORE_INCLUDE_FILE} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coco)
endif()

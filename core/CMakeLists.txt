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
    add_library(${PROJECT_NAME} ${CORE_SOURCE_FILE} ${CORE_INCLUDE_FILE})
else()
	add_library(${PROJECT_NAME} SHARED ${CORE_SOURCE_FILE} ${CORE_INCLUDE_FILE})
endif()


#SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES        # create *nix style library versions + symbolic links
#  DEFINE_SYMBOL DSO_EXPORTS
#  VERSION ${PROJECT_VERSION}
#  SOVERSION ${PROJECT_SOVERSION}
#  CLEAN_DIRECT_OUTPUT 1           # allow creating static and shared libs without conflicts
#  OUTPUT_NAME "${PROJECT_NAME}${PROJECT_DLLVERSION}"  # avoid conflicts between library and binary target names
#)

target_link_libraries(${PROJECT_NAME} ${DEPS})

install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/)
install(TARGETS coco DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/)
#install(TARGETS ${PROJECT_NAME}
#  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/ PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
#)

#if(${INSTALL_INCLUDE})
#message(STATUS "SALVATEMI!")
	install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coco)
	install(FILES coco.h ${CORE_INCLUDE_FILE} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coco)
#endif()

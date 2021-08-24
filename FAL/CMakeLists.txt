target_sources(${PROJECT_NAME}
    PRIVATE
    # {{BEGIN_TARGET_SOURCES}}
    ${CMAKE_CURRENT_LIST_DIR}/Port/fal_port.c
    ${CMAKE_CURRENT_LIST_DIR}/Source/fal_flash.c
    ${CMAKE_CURRENT_LIST_DIR}/Source/fal_partition.c
    ${CMAKE_CURRENT_LIST_DIR}/Source/fal.c

    # {{END_TARGET_SOURCES}}
)

# Add the Common/inc directory to the project include list
target_include_directories(${PROJECT_NAME} 
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/Cfg
    ${CMAKE_CURRENT_LIST_DIR}/Port
    ${CMAKE_CURRENT_LIST_DIR}/Source
)

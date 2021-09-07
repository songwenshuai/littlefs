target_sources(${PROJECT_NAME}
    PRIVATE
    # {{BEGIN_TARGET_SOURCES}}
    # {{END_TARGET_SOURCES}}
)

# Add the Common/inc directory to the project include list
target_include_directories(${PROJECT_NAME} 
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/Source
)

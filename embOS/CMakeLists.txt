target_sources(${PROJECT_NAME}
    PRIVATE
    # {{BEGIN_TARGET_SOURCES}}
    # ${CMAKE_CURRENT_LIST_DIR}/Port/littlefs_fal_port.c
    ${CMAKE_CURRENT_LIST_DIR}/Setup/OS_Error.c
    ${CMAKE_CURRENT_LIST_DIR}/Setup/RTOSInit.c
    ${CMAKE_CURRENT_LIST_DIR}/Setup/UDP_Sim_Process.c
    ${CMAKE_CURRENT_LIST_DIR}/SEGGER/SEGGER_RTT_printf.c
    ${CMAKE_CURRENT_LIST_DIR}/SEGGER/SEGGER_RTT.c
    ${CMAKE_CURRENT_LIST_DIR}/SEGGER/SEGGER_SYSVIEW_Config_embOS_Win32.c
    ${CMAKE_CURRENT_LIST_DIR}/SEGGER/SEGGER_SYSVIEW_embOS.c
    ${CMAKE_CURRENT_LIST_DIR}/SEGGER/SEGGER_SYSVIEW.c
    ${CMAKE_CURRENT_LIST_DIR}/Application/OS_Mutexes.c

    # {{END_TARGET_SOURCES}}
)

# Properties->Linker->Input->Additional Dependencies
link_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/Lib
)

# Add the Common/inc directory to the project include list
target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/Inc
    ${CMAKE_CURRENT_LIST_DIR}/SEGGER
)

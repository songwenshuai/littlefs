SET(CMAKE_SYSTEM_NAME       Windows)
SET(CMAKE_SYSTEM_PROCESSOR  x86_64)

SET(WIN32_FLAGS             "")

SET(CMAKE_C_FLAGS           "${WIN32_FLAGS} "                           CACHE INTERNAL "c compiler flags")
SET(CMAKE_CXX_FLAGS         "${WIN32_FLAGS} -fno-rtti -fno-exceptions"  CACHE INTERNAL "cxx compiler flags")
SET(CMAKE_ASM_FLAGS         "${WIN32_FLAGS} -x assembler-with-cpp"      CACHE INTERNAL "asm compiler flags")
SET(CMAKE_EXE_LINKER_FLAGS  "${WIN32_FLAGS} ${LD_FLAGS}"                CACHE INTERNAL "exe link flags")

# this makes the test compiles use static library option so that we don't need to pre-set linker flags and scripts
# SET(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
SET(CMAKE_TRY_COMPILE_TARGET_TYPE EXECUTABLE)
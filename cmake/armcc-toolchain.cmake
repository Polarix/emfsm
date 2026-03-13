# ARM Compiler 5/6 工具链文件
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# 检测编译器版本
if("${CMAKE_C_COMPILER}" MATCHES "armclang")
    # ARM Compiler 6
    set(ARMCC_VERSION 6)
    set(CMAKE_C_COMPILER_ID "ARMCC")
    set(CMAKE_C_COMPILER_VERSION 6)
    
    # ARM Compiler 6 标志
    set(ARMCC_COMMON_FLAGS "-mcpu=cortex-m3 -mthumb --target=arm-arm-none-eabi")
    set(ARMCC_C_FLAGS "${ARMCC_COMMON_FLAGS} -xc -std=c99")
    set(ARMCC_LINK_FLAGS "${ARMCC_COMMON_FLAGS} -Wl,--gc-sections")
else()
    # ARM Compiler 5
    set(ARMCC_VERSION 5)
    set(CMAKE_C_COMPILER_ID "ARMCC")
    set(CMAKE_C_COMPILER_VERSION 5)
    
    # ARM Compiler 5 标志
    set(ARMCC_COMMON_FLAGS "--cpu Cortex-M3 --thumb --c99")
    set(ARMCC_C_FLAGS "${ARMCC_COMMON_FLAGS}")
    set(ARMCC_LINK_FLAGS "${ARMCC_COMMON_FLAGS} --scatter \"memory.scat\"")
endif()

# 设置CMake变量
set(CMAKE_C_FLAGS_INIT "${ARMCC_C_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${ARMCC_LINK_FLAGS}")

# 输出格式
set(CMAKE_EXECUTABLE_SUFFIX ".axf")

# 搜索模式
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
# ARM GCC 工具链文件
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# 工具链路径（根据实际情况修改）
set(TOOLCHAIN_PATH "/usr/local/gcc-arm-none-eabi")
set(CMAKE_C_COMPILER "${TOOLCHAIN_PATH}/bin/arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PATH}/bin/arm-none-eabi-g++")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PATH}/bin/arm-none-eabi-gcc")
set(CMAKE_OBJCOPY "${TOOLCHAIN_PATH}/bin/arm-none-eabi-objcopy")
set(CMAKE_OBJDUMP "${TOOLCHAIN_PATH}/bin/arm-none-eabi-objdump")
set(CMAKE_SIZE "${TOOLCHAIN_PATH}/bin/arm-none-eabi-size")
set(CMAKE_AR "${TOOLCHAIN_PATH}/bin/arm-none-eabi-ar")

# 编译标志
set(CMAKE_C_FLAGS "-mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_C_FLAGS_RELEASE "-Os")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-Os -g")

set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections -nostartfiles")

# 搜索模式
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
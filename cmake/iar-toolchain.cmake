# IAR 工具链文件
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# IAR编译器路径（根据实际情况修改）
set(IAR_PATH "C:/Program Files/IAR Systems/Embedded Workbench 9.0/arm")
set(CMAKE_C_COMPILER "${IAR_PATH}/bin/iccarm.exe")
set(CMAKE_CXX_COMPILER "${IAR_PATH}/bin/iccarm.exe")
set(CMAKE_ASM_COMPILER "${IAR_PATH}/bin/iasmarm.exe")
set(CMAKE_AR "${IAR_PATH}/bin/iarchive.exe")

# IAR特定的编译选项
set(IAR_COMMON_FLAGS "--cpu Cortex-M3 --dlib_config normal --no_cse --no_unroll --no_inline --no_code_motion --no_tbaa --no_clustering --no_scheduling --debug --endian little --cpu_mode thumb --silent -e --fpu=None")
set(IAR_C_FLAGS "${IAR_COMMON_FLAGS} --diag_suppress=Pa050")
set(IAR_ASM_FLAGS "${IAR_COMMON_FLAGS}")
set(IAR_LINK_FLAGS "-f \"${IAR_PATH}/config/linkerscript.icf\"")

# 设置CMake变量
set(CMAKE_C_FLAGS_INIT "${IAR_C_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${IAR_ASM_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${IAR_LINK_FLAGS}")

# IAR输出格式
set(CMAKE_EXECUTABLE_SUFFIX ".out")

# 搜索模式
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
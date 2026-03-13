# MinGW 工具链文件（如果需要作为交叉编译工具链）
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# 指定MinGW工具链路径（根据实际情况修改）
set(MINGW_PATH "C:/mingw64")
set(CMAKE_C_COMPILER "${MINGW_PATH}/bin/x86_64-w64-mingw32-gcc.exe")
set(CMAKE_CXX_COMPILER "${MINGW_PATH}/bin/x86_64-w64-mingw32-g++.exe")
set(CMAKE_RC_COMPILER "${MINGW_PATH}/bin/x86_64-w64-mingw32-windres.exe")

# 编译标志
set(CMAKE_C_FLAGS "-m64")
set(CMAKE_CXX_FLAGS "-m64")

# 链接标志
set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++ -mconsole")

# 搜索模式
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Windows特定定义
add_definitions(-D_WIN32_WINNT=0x0600)
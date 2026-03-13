cmake_minimum_required(VERSION 3.10)
project(fsm_framework C)

# 设置C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# 检测Windows平台和MinGW
if(WIN32 AND MINGW)
    message(STATUS "Building with MinGW on Windows")
    set(FSM_MINGW TRUE)
    
    # MinGW特定设置
    add_definitions(-D_WIN32_WINNT=0x0600)  # Windows Vista或更高版本
    set(CMAKE_RC_COMPILER_INIT windres)
    
    # 设置MinGW编译选项
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-format")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()

# 项目配置
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(FSM_ENABLE_DEBUG "Enable debug features" ON)
option(FSM_BUILD_TESTS "Build test executable" ON)
option(FSM_BUILD_LED_EXAMPLES "Build led blink examples" ON)
option(FSM_BUILD_BMS_EXAMPLES "Build BMS examples" ON)

# 定义FSM库
add_library(fsm
    src/fsm.c
)

# 设置包含目录
target_include_directories(fsm
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 添加编译定义
if(FSM_ENABLE_DEBUG)
    target_compile_definitions(fsm PUBLIC FSM_ENABLE_DEBUG)
    message(STATUS "FSM debug features enabled")
endif()

# 设置库属性
set_target_properties(fsm PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    OUTPUT_NAME fsm
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

# 创建测试可执行文件
if(FSM_BUILD_TESTS)
    add_executable(fsm_test
        tests/test_fsm.c
    )
    
    target_include_directories(fsm_test
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    target_link_libraries(fsm_test fsm)
    
    # MinGW特定：避免控制台闪退
    if(FSM_MINGW)
        set_target_properties(fsm_test PROPERTIES
            LINK_FLAGS "-mconsole"
        )
    endif()
    
    # 添加测试
    if(CMAKE_CROSSCOMPILING)
        message(STATUS "Cross-compiling: skipping test registration")
    else()
        enable_testing()
        add_test(NAME fsm_test COMMAND fsm_test)
    endif()
endif()

# 创建LED闪烁控制示例
if(FSM_BUILD_LED_EXAMPLES)
    add_executable(led_example
        examples/led_example.c
    )
    
    target_include_directories(led_example
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    target_link_libraries(led_example fsm)
    
    # MinGW特定：避免控制台闪退
    if(FSM_MINGW)
        set_target_properties(led_example PROPERTIES
            LINK_FLAGS "-mconsole"
        )
    endif()
endif()

# 创建BMS示例
if(FSM_BUILD_BMS_EXAMPLES)
    add_executable(bms_example
        examples/bms_example.c
    )
    
    target_include_directories(bms_example
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    target_link_libraries(bms_example fsm)
    
    # MinGW特定：避免控制台闪退
    if(FSM_MINGW)
        set_target_properties(bms_example PROPERTIES
            LINK_FLAGS "-mconsole"
        )
    endif()
endif()

# 安装配置（MinGW下可选）
if(NOT FSM_MINGW)
    install(TARGETS fsm
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
    
    install(DIRECTORY include/ DESTINATION include/fsm)
    
    # 导出包配置
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/fsm-config-version.cmake"
        VERSION 1.0.0
        COMPATIBILITY AnyNewerVersion
    )
    
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/fsm-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/fsm-config.cmake"
        INSTALL_DESTINATION lib/cmake/fsm
    )
    
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/fsm-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/fsm-config-version.cmake"
        DESTINATION lib/cmake/fsm
    )
    
    # 导出目标
    export(TARGETS fsm
        FILE "${CMAKE_CURRENT_BINARY_DIR}/fsm-targets.cmake"
    )
    
    install(EXPORT fsm-targets
        FILE fsm-targets.cmake
        DESTINATION lib/cmake/fsm
    )
endif()

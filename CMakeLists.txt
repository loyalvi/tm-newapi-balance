cmake_minimum_required(VERSION 3.15)
project(tm-newapi-balance LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(tm-newapi-balance SHARED
    dllmain.cpp
    Plugin.h
    Plugin.cpp
    BalanceItem.h
    BalanceItem.cpp
    PluginInterface.h
)

target_link_libraries(tm-newapi-balance PRIVATE winhttp)

# 导出符号定义
target_compile_definitions(tm-newapi-balance PRIVATE UNICODE _UNICODE)

# 强制 MSVC 以 UTF-8 读取源文件并以 UTF-8 生成执行字符集，避免 CI 环境中文乱码
if(MSVC)
    target_compile_options(tm-newapi-balance PRIVATE /utf-8)
endif()

# 输出到 build/bin 方便复制
set_target_properties(tm-newapi-balance PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

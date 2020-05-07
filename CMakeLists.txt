# CMake 最低版本号要求
cmake_minimum_required (VERSION 2.8)
# 项目信息
project (DEMO3)
# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_SRCS 变量
aux_source_directory(. DIR_SRCS)


# 引入库文件
LINK_LIBRARIES(-lm)
LINK_LIBRARIES(-lpthread)


# 添加头文件路径
include_directories(include)

# 添加 math 子目录
add_subdirectory(src)

# 指定生成目标
add_executable(server server.cc)
add_executable(client client.cc)
# 添加链接库
target_link_libraries(server Funcs)
target_link_libraries(client Funcs)
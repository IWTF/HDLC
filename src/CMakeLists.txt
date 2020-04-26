# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_LIB_SRCS 变量
aux_source_directory(. DIR_LIB_SRCS)

# 引入库文件
LINK_LIBRARIES(-lm)

# 添加头文件路径
include_directories(../include/)

# 生成链接库
add_library (Funcs ${DIR_LIB_SRCS})
# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_LIB_SRCS 变量
aux_source_directory(. DIR_LIB_SRCS)

# 引入库文件
LINK_LIBRARIES(-lm)
# LINK_LIBRARIES(-lmath), 与上面等价，引入任何库文件，都可以 -l + 头文件名
# 如： -lpthread
# !!!另外，使用LINK_LIBRARIES是全局引入，即其他文件再需要使用该头文件都不需要再写
# 全局引入意味着：即使这句放在根目录下的CMake里，mathxxx.c也不会报错

# 添加头文件路径
include_directories(../include)

# 生成链接库
add_library (MathFunctions ${DIR_LIB_SRCS})
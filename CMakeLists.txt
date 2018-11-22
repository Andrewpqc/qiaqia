#设置项目名称
PROJECT(system)

#要求CMake的最低版本为2.8
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

ADD_COMPILE_OPTIONS(-std=c++11)

#添加头文件搜索路径
INCLUDE_DIRECTORIES(/usr/local/include)

#添加库文件搜索路径
LINK_DIRECTORIES(/usr/local/lib /usr/lib)


ADD_LIBRARY(errorFunc utils/error_functions.c)

ADD_EXECUTABLE(qiaqia_server server/servermain.cpp server/server.hpp)

ADD_EXECUTABLE(qiaqia_client client/clientmain.cpp client/client.h
            client/client.cpp)

TARGET_LINK_LIBRARIES(qiaqia_server errorFunc)
TARGET_LINK_LIBRARIES(qiaqia_client errorFunc)
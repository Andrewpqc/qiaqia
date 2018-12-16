#设置项目名称
PROJECT(qiaqia)

#要求CMake的最低版本为2.8
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

ADD_COMPILE_OPTIONS(-std=c++11 -O3)

#添加头文件搜索路径
INCLUDE_DIRECTORIES(. ./utils)

#添加库文件搜索路径
LINK_DIRECTORIES(/usr/local/lib /usr/lib)



ADD_EXECUTABLE(qiaqia_server server/servermain.cpp)

ADD_EXECUTABLE(qiaqia_client client/clientmain.cpp)

TARGET_LINK_LIBRARIES(qiaqia_server pthread)
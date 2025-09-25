cmake_minimum_required(VERSION 3.31)
project(ConnectionPool)

set(CMAKE_CXX_STANDARD 20)

# 1. 配置 MySQL 头文件路径
include_directories(/opt/homebrew/opt/mysql-client/include/mysql)

# 2. 配置 MySQL 库文件路径
link_directories(/opt/homebrew/opt/mysql-client/lib)

add_executable(ConnectionPool
        main.cpp
        Connection.cpp
        #practice/CommonConnectionPool.cpp
        #practice/ConnectionPool.cpp
        ConnectionPool.cpp)

# 4. 链接 MySQL 客户端库
target_link_libraries(ConnectionPool mysqlclient)
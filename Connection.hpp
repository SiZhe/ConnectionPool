#pragma once
#include "public.hpp"

/*
    实现MySQL数据库的增删改查
*/
class Connection {
public:
    // 初始化数据库连接
    Connection();
    // 释放数据库连接资源
    ~Connection();
    // 连接数据库
    bool connect(std::string ip,
        unsigned short port,
        std::string user,
        std::string password,
        std::string dbname);
    // 更新操作 insert、delete、update
    bool update(std::string sql);
    // 查询操作 select
    MYSQL_RES* query(std::string sql);

    // 刷新空闲时间点
    void refreshAliveTime();

    // 返回存活时间
    clock_t getAliveTime() const;
private:
    // 表示和MySQL Server的一条连接
    MYSQL *_conn;
    std::clock_t _aliveTime; // 记录进入空闲状态的起始存活时间
};


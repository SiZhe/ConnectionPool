#pragma once
#include "../public.hpp"
#include "../Connection.hpp"

class ConnectionPool {
public:
    static ConnectionPool* getConnectionPool();
    std::shared_ptr<Connection>  getConnection();
private:
    ~ConnectionPool();
    //懒汉单例模式
    ConnectionPool();

    bool loadSqlConfig();

    void producer();

    void scanner();
private:
    // 数据库设置
    std::string _ip;
    unsigned short _port;
    std::string _username;
    std::string _password;
    std::string _dbname;

    // 连接池设置
    unsigned int _initSize;
    unsigned int _maxSize;
    std::atomic_int _connectionCnt {0};
    // 最大空闲时间单位秒
    unsigned int _maxIdleTime;
    // 连接超时时间单位毫秒
    unsigned int _connectionTimeOut;

    std::queue<Connection*> _connectionQueue;

    // 多线程
    std::mutex mtx;
    std::condition_variable cv;
};
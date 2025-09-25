#pragma once
#include "../public.hpp"
#include "../Connection.hpp"

class CommonConnectionPool {
public:
    static CommonConnectionPool* getConnectionPool();
    // 给外界提供可用的连接的接口
    std::shared_ptr<Connection> getConnection();
    // 运行在独立的线程中，专门负责生产新连接
    void produceConnectionTask();
    // 扫描多余的空间时间大于maxIdleTime进行回收
    void scannerConnectionTask();
private:
    CommonConnectionPool();

    bool loadConfigFile(); // 从配置文件中加载配置项

    std::string _ip; // mysql的ip地址
    unsigned short _port; // mysql的端口号
    std::string _username; // mysql的用户名
    std::string _password; // mysql的登录密码
    std::string _dbname; // mysql的数据库名称

    int _initSize; // 连接池的初始连接量
    int _maxSize; // 连接池的最大连接量
    int _maxIdleTime; // 连接池的最大空闲时间
    int _connectionTimeOut; // 连接池的超时连接时间

    std::queue<Connection*> _connectionQueue; // 存储 mysql
    std::mutex _queueMutex; // 维护连接队列线程安全的互斥锁
    std::atomic_int _connectionCnt;

    std::condition_variable cv; // 设置条件变量，用于连接生产线程和消费线程的通信
};
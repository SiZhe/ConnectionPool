#include "Connection.hpp"

Connection::Connection()
{
        _conn = mysql_init(nullptr);
}

Connection::~Connection()
{
    if (_conn != nullptr)
        mysql_close(_conn);
}

// 连接数据库
bool Connection::connect(
    std::string ip,
    unsigned short port,
    std::string user,
    std::string password,
    std::string dbname)
{
    MYSQL *p = mysql_real_connect(_conn, ip.c_str(), user.c_str(),
    password.c_str(), dbname.c_str(), port, nullptr, 0);
    return p != nullptr;
}

// 更新操作 insert、delete、update
bool Connection::update(std::string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG("更新失败:" + sql);
        return false;
    }
    return true;
}

// 查询操作 select
MYSQL_RES* Connection::query(std::string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG("查询失败:" + sql);
        return nullptr;
    }
    return mysql_use_result(_conn);
}

void Connection::refreshAliveTime() {
    _aliveTime = clock();
}

clock_t Connection::getAliveTime() const{
    return clock()-_aliveTime;
}

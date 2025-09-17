#include "public.hpp"
#include "Connection.hpp"
#include "CommonConnectionPool.hpp"

int main() {
    CommonConnectionPool *cp = CommonConnectionPool::getConnectionPool();

    std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();

    for (int i = 0 ; i < 1000 ; ++i) {
        //5000:43.7289s
        /*
        Connection conn;
        std::string sql = "insert into user(name,age,sex) values('lisizhe','23','M')";
        conn.connect("127.0.0.1",3306,"root","sql002522","connectionPool");
        conn.update(sql);
         */

        //1000:0.113741s
        //5000:0.447567s
        //10000:1.08505s
        CommonConnectionPool *cp = CommonConnectionPool::getConnectionPool();
        std::shared_ptr<Connection> sp = cp->getConnection();
        std::string sql = "insert into user(name,age,sex) values('lisizhe','23','M')";
        sp->update(sql);
    }

    std::chrono::system_clock::time_point endTime = std::chrono::system_clock::now();

    std::chrono::duration<double> diff = endTime - startTime;
    std::cout << diff.count() << "s" << std::endl;
    return 0;
}
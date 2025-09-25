#include "../CommonConnectionPool.hpp"

CommonConnectionPool::CommonConnectionPool() {
    // 加载配置项
    if (!loadConfigFile()) {
        return;
    }

    // 创建初始数量的连接
    for (int i = 0 ; i < _initSize ; ++i) {
        Connection* p = new Connection();
        p->connect(_ip,_port,_username,_password,_dbname);
        _connectionQueue.push(p);
        _connectionCnt++;
    }

    // 启动一个新的线程，作为连接生产者
    std::thread produce(std::bind(&CommonConnectionPool::produceConnectionTask,this));
    produce.detach();
    // 启动一个新的线程，扫描多余的空间时间大于maxIdleTime进行回收
    std::thread scanner(std::bind(&CommonConnectionPool::scannerConnectionTask,this));
    scanner.detach();
}


// 线程安全的懒汉单例函数接口
CommonConnectionPool *CommonConnectionPool::getConnectionPool() {
    static CommonConnectionPool pool = CommonConnectionPool();
    return &pool;
}

bool CommonConnectionPool::loadConfigFile() {
    FILE *pf = fopen("../mysql.ini","r");

    if (pf == nullptr) {
        LOG("mysql.ini file is not exist!!!");
        return false;
    }

    while (!feof(pf)) {
        char line[1024] = {0};
        fgets(line,1024,pf);
        std::string str = line;
        int idx = str.find('=',0);

        if (idx == -1) {
            continue;
        }

        int endIdx = str.find('\n',idx);

        std::string key = str.substr(0,idx);
        std::string value = str.substr(idx+1,endIdx-idx-1);

        // std::cout << key << ":" << value << std::endl;

        if (key == "ip") {
            _ip = value;
        } else if (key == "port") {
            _port = static_cast<unsigned short>(std::stoul(value));
        } else if (key == "username") {
            _username = value;
        } else if (key == "password") {
            _password = value;
        } else if (key == "dbname") {
            _dbname = value;
        } else if (key == "initSize") {
            _initSize = std::stoi(value);
        } else if (key == "maxSize") {
            _maxSize = std::stoi(value);
        } else if (key == "maxIdleTime") {
            _maxIdleTime = std::stoi(value);
        } else if (key == "connectionTimeOut") {
            _connectionTimeOut = std::stoi(value);
        }
    }
    return true;
}

void CommonConnectionPool::produceConnectionTask() {
    for (;;) {
        std::unique_lock<std::mutex> lock(_queueMutex);
        while (!_connectionQueue.empty()) {
            cv.wait(lock); // 队列不空，线程进入等待状态
        }
        // 连接数量没有到达上限，继续创建新的连接
        if (_connectionCnt < _maxSize) {
            Connection* p = new Connection();
            p->connect(_ip,_port,_username,_password,_dbname);
            p->refreshAliveTime(); // 刷新空闲时间
            _connectionQueue.push(p);
            _connectionCnt++;
        }
        // 通知消费者线程可以消费连接
        cv.notify_all();
    }
}

void CommonConnectionPool::scannerConnectionTask() {
    for (;;) {
        // 通过sleep模拟定时效果
        std::this_thread::sleep_for(std::chrono::seconds(_maxIdleTime));
        // 扫描整个队列，释放多余连接
        std::unique_lock<std::mutex> lock(_queueMutex);
        while (_connectionCnt > _initSize) {
            Connection* p = _connectionQueue.front();
            if (p->getAliveTime() > std::chrono::seconds(_maxIdleTime)) {
                _connectionQueue.pop();
                _connectionCnt--;
                delete p;//调用connection的析构函数close
            }else {
                break; // 队头的没有超过maxIdleTime,那么后面都没超过
            }
        }
    }
}

std::shared_ptr<Connection> CommonConnectionPool::getConnection() {
    std::unique_lock<std::mutex> lock(_queueMutex);

    while (_connectionQueue.empty()) {
        if ( std::cv_status::timeout == cv.wait_for(lock,std::chrono::milliseconds(_connectionTimeOut))) {
            if (_connectionQueue.empty()) {
                LOG("获取空闲连接超时......获取连接失败!");
                return nullptr;
            }
        }
    }

    /*
     shared_ptr的析构函数会直接delete，等于调用connection的析构函数的close，
     这里要自定义shared_ptr的析构函数，把connection直接归还到queue中
     */

    std::shared_ptr<Connection> sp(_connectionQueue.front(),
        [&](Connection *pcon) {
            // 这里在服务器中应用线程调用，要考虑队列线程安全
            std::unique_lock<std::mutex> lock(_queueMutex);
            pcon->refreshAliveTime(); // 刷新空闲时间
            _connectionQueue.push(pcon);
        });

    _connectionQueue.pop();

    // 消费完后通知其他线程可以消费
    cv.notify_all();

    return sp;
}
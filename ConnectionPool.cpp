#include "ConnectionPool.hpp"

ConnectionPool::ConnectionPool() {
    if(!loadSqlConfig()) {
        return;
    }

    for (int i = 0 ; i < _initSize; ++i) {
        Connection* cn = new Connection();
        cn->connect(_ip,_port,_username,_password,_dbname);
        _connectionQueue.push(cn);
        _connectionCnt++;
    }

    std::thread producerThread(&ConnectionPool::producer,this);
    producerThread.detach();
    std::thread scannerThread(&ConnectionPool::scanner,this);
    scannerThread.detach();
}

ConnectionPool* ConnectionPool::getConnectionPool() {
    static ConnectionPool pool = ConnectionPool();
    return &pool;
}

ConnectionPool::~ConnectionPool() {
    while (!_connectionQueue.empty()) {
        Connection* cn = _connectionQueue.front();
        _connectionQueue.pop();
        delete cn;
    }
}


bool ConnectionPool::loadSqlConfig() {
    std::fstream file;
    file.open("../mysql.ini",std::ios::in);

    if (!file) {
        LOG("Unable to open file!");
        return false;
    }

    std::string line;

    while(getline(file,line)) {
        if (line.empty()) {
            continue;
        }

        int idx = line.find('=',0);
        if (idx == -1) {
            continue;
        }

        std::string key = line.substr(0,idx);
        std::string value = line.substr(idx+1);
        //std::cout << key << " : " << value << std::endl;

        if (key == "ip") {
            this->_ip = value;
        } else if (key == "port") {
            this->_port = static_cast<unsigned short>(std::stoi(value));
        } else if (key == "username") {
            this->_username = value;
        } else if (key == "password") {
            this->_password = value;
        } else if (key == "dbname") {
            this->_dbname = value;
        } else if (key == "initSize") {
            this->_initSize = static_cast<unsigned int>(std::stoi(value));
        } else if (key == "maxSize") {
            this->_maxSize = static_cast<unsigned int>(std::stoi(value));
        } else if (key == "maxIdleTime") {
            this->_maxIdleTime = static_cast<unsigned int>(std::stoi(value));
        } else if (key == "connectionTimeOut") {
            this->_connectionTimeOut = static_cast<unsigned int>(std::stoi(value));
        } else {
            LOG("Unknown key: " + key);
        }
    }
    return true;
}

void ConnectionPool::producer() {
    for (;;) {
        std::unique_lock<std::mutex> lck(mtx);

        while (!_connectionQueue.empty()) {
            // 如果队列不为空就等待
            cv.wait(lck);
        }

        if (_connectionCnt < _maxSize) {
            Connection* cn = new Connection();
            cn->connect(_ip,_port,_username,_password,_dbname);
            _connectionQueue.push(cn);
            _connectionCnt++;
        }

        cv.notify_all();
    }
}

void ConnectionPool::scanner() {
    for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(_maxIdleTime/4));
        std::unique_lock<std::mutex> lck(mtx);
        while (_connectionCnt > _initSize) {
            if (_connectionQueue.front()->getAliveTime() > std::chrono::seconds(_maxIdleTime)) {
                Connection* cn = _connectionQueue.front();
                _connectionQueue.pop();
                _connectionCnt--;
                delete cn;
            }else {
                break;
            }
        }
    }
}


std::shared_ptr<Connection> ConnectionPool::getConnection() {
    std::unique_lock<std::mutex> lck(mtx);

    while (_connectionQueue.empty()) {
        std::cv_status flag = cv.wait_for(lck,std::chrono::milliseconds(_connectionTimeOut));
        if (flag == std::cv_status::timeout) {
            if(_connectionQueue.empty()) {
                LOG("获取空闲连接超时......获取连接失败!");
                return nullptr;
            }
        }
    }

    Connection* c = _connectionQueue.front();
    _connectionQueue.pop();

    std::shared_ptr<Connection> cn(c,[&](Connection* c) {
        std::unique_lock<std::mutex> lock(mtx);
        c->refreshAliveTime();
        _connectionQueue.push(c);
    });

    if (_connectionQueue.empty()) {
        cv.notify_all();
    }

    return cn;
}
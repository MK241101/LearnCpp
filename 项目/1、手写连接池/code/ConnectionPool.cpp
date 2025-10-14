#include "ConnectionPool.h"
#include "public.h"
#include <fstream>  // 包含ifstream头文件


bool ConnectionPool::loadConfigFile()
{
    std::ifstream ifs("mysql.ini");  // 打开配置文件
    if (!ifs.is_open())  
    {
        LOG("mysql配置文件不存在");
        return false;
    }

    std::string line;
    while (std::getline(ifs, line))  // getline自动忽略换行符，不包含在line中
    {
        // 查找等号位置
        size_t idx = line.find('=');
        if (idx == std::string::npos)  // 找不到等号则跳过此行
        {
            continue;
        }

        // 分割键和值（去除可能的空格）
        std::string key = line.substr(0, idx);
        std::string value = line.substr(idx + 1);

        // 处理键值对（trim掉前后可能的空格，增强健壮性）
        // 简化版：直接匹配键名赋值
        if (key == "ip")
        {
            ip = value;
        }
        else if (key == "port")
        {
            port = std::stoi(value);  // 使用C++的stoi替代atoi，更安全
        }
        else if (key == "userName")
        {
            userName = value;
        }
        else if (key == "passWord")
        {
            passWord = value;
        }
        else if (key == "dbName")
        {
            dbName = value;  
        }
        else if (key == "initSize")
        {
            initSize = std::stoi(value);
        }
        else if (key == "maxSize")
        {
            maxSize = std::stoi(value);
        }
        else if (key == "maxIdleTime")
        {
            maxIdleTime = std::stoi(value);
        }
        else if (key == "connectionTimeOut")
        {
            connectionTimeOut = std::stoi(value);
        }
    }

    ifs.close();  // 关闭文件流
    return true;
}

// 连接池构造函数
ConnectionPool::ConnectionPool()
{
    if (!loadConfigFile())
    {
        return;
    }

    // 创建初始数量的连接
    for (int i = 0; i < initSize; ++i)
    {
        Connection* p = new Connection();
        p->connect(ip, port, userName, passWord, dbName);
        p->refreshAliveTime();      //刷新开始空闲的起始时间
        
        connectionQue.push(p);
        connectionCount++;
    }

    thread produce(bind(&ConnectionPool::produceConnectionTask, this));  // 生产者线程
    produce.detach();

    thread scanner(bind(&ConnectionPool::scannerConnectionTask, this));  // 扫描线程
    scanner.detach();

}


void ConnectionPool::produceConnectionTask()
{
    for(;;)
    { 
        unique_lock<mutex> lock(queueMutex);
        while (!connectionQue.empty())
        {
            cv.wait(lock);   //队列不为空（有空闲连接），生产者无需工作，进入等待状态
        }

        if (connectionCount < maxSize)   //连接未达到上限，就继续创建新的连接块
        {
            Connection* p = new Connection();
            p->connect(ip, port, userName, passWord, dbName);
            p->refreshAliveTime();
            connectionQue.push(p);
            connectionCount++;
        }

        cv.notify_all();    //通知消费线程
    }

}

// 从连接池中获取一个数据库连接（Connection对象）
shared_ptr<Connection> ConnectionPool::getConnection()
{
    unique_lock<mutex> lock(queueMutex);
    while (connectionQue.empty())
    {
        if (cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(connectionTimeOut)))
        {
            if (connectionQue.empty())
            {
                LOG("获取空闲连接块失败");
                return nullptr;
            } 
        }
            
    }
    // 用队列前端的连接创建shared_ptr，自定义删除器（不销毁连接，而是放回队列）
    shared_ptr<Connection> sp(connectionQue.front(),
        [&](Connection* pcon) {
            unique_lock<mutex> lock(queueMutex);  // 归还时加锁，保证线程安全
            pcon->refreshAliveTime();  // 刷新连接的存活时间（标记为“刚刚归还”）
            connectionQue.push(pcon);  // 将连接放回空闲队列
        }); 
    connectionQue.pop();
    if (connectionQue.empty())
    {
        cv.notify_all();
    }
    return sp;
}


void ConnectionPool::scannerConnectionTask()
{
    for (;;)
    {
        this_thread::sleep_for(chrono::seconds(maxIdleTime));

        unique_lock<mutex>lock(queueMutex);    //扫描整个队列，释放多余的连接
        while (connectionCount > initSize)
        {
            Connection* p = connectionQue.front();
            if (p->getAliveeTime() >= (maxIdleTime) * 1000)
            {
                connectionQue.pop();
                connectionCount--;
                delete p;
            }
            else
            {
                break;
            }
        }


    }

}
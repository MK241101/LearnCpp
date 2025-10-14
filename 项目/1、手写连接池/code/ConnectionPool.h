#pragma once
#include <string>
#include <queue>
#include "Connection.h"
#include <mutex>
#include<atomic>
#include<thread>
#include<memory>
#include<functional>
#include<condition_variable>


using namespace std;

class ConnectionPool
{
public:
	static ConnectionPool* getConnectionPool()   // 懒汉式单例模式
	{
		static ConnectionPool pool;
		return &pool;
	}
	shared_ptr<Connection> getConnection();

private:
	ConnectionPool();   // 单例类，构造函数私有化
	bool loadConfigFile();
	void produceConnectionTask();   //负责生产新连接
	void scannerConnectionTask();   //扫描连接

private:
	string ip;
	unsigned short port;   //mysql的端口号 3306
	string userName;       //用户名
	string passWord;       //密码
	string dbName;         //连接的数据库名称
	int initSize;          //连接池初始容量
	int maxSize;           //连接池最大容量
	int maxIdleTime;       //连接池最大空闲时间
	int connectionTimeOut; //连接池获取连接的超时时间 

	queue<Connection*> connectionQue;   // 连接池队列
	mutex queueMutex;					// 连接池队列安全锁
	atomic_int connectionCount;			// 记录创建的连接块的总数量
	condition_variable cv;				// 生产线程与消费线程之间进行通信
};
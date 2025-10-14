#pragma once
#include <mysql.h>
#include <string>
#include <ctime>
using namespace std;
// 实现MySql数据库的操作

class Connection
{
public:
	Connection();
	~Connection();
	
	// 连接数据库
	bool connect(string ip,
		unsigned short port,
		string user,
		string password,
		string dbname);

	bool update(string sql);   // 更新操作 insert、delete、update
	
	MYSQL_RES* query(string sql);   // 查询操作 select

	void refreshAliveTime() { alivetime = clock(); }   // 刷新连接的起始的空闲时间点
	
	clock_t getAliveeTime()const { return clock() - alivetime; }   // 返回存活的时间

private:
	MYSQL*  conn;            // 表示和MySQL Server的一条连接
	clock_t alivetime;       // 记录进入空闲状态后的起始存活时间
};


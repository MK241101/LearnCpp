
#include "public.h"
#include "Connection.h"
#include <iostream>
using namespace std;

Connection::Connection()
{
	conn = mysql_init(nullptr);    // 初始化数据库连接
}

Connection::~Connection()
{
	if (conn != nullptr)         // 释放数据库连接资源
		mysql_close(conn);
}

bool Connection::connect(string ip, unsigned short port,
	string username, string password, string dbname)
{
	// 连接数据库
	MYSQL* p = mysql_real_connect(conn, ip.c_str(), username.c_str(),
		password.c_str(), dbname.c_str(), port, nullptr, 0);
	return p != nullptr;
}

bool Connection::update(string sql)     // 更新操作 insert、delete、update
{
	if (mysql_query(conn, sql.c_str()))    
	{
		LOG("更新失败:" + sql);
		return false;
	}
	return true;
}

MYSQL_RES* Connection::query(string sql)    // 查询操作 select
{
	if (mysql_query(conn, sql.c_str()))
	{
		LOG("查询失败:" + sql);
		return nullptr;
	}
	return mysql_use_result(conn);
}
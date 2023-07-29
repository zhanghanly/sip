#ifndef DB_CONNECTION_POOL_H
#define DB_CONNECTION_POOL_H

#include <cstdint>
#include <string>
#include <memory>
#include <mutex>
#include <map>
#include "DataBaseHandler.h"

class DBConnectionPool {
public:
	~DBConnectionPool();

	static DBConnectionPool* get_instance(void);
	void init_pool(void);
	/*for HGET*/
	std::string fetch_answer_sync(const std::string&, const std::string&, const std::string&);
	/*for HGETALL*/
	std::map<std::string, std::string> fetch_kv_answer_sync(const std::string&, const std::string&);
	/*for PUBLISH, HDEL*/
	void fetch_no_answer_sync(const std::string&, const std::string&, const std::string&);
	/*for HSET*/
	ReplyValue fetch_no_answer_sync(const std::string&, const std::string&, const std::string&, const std::string&);
	/*for mysql*/
	ReplyValue write_record(const std::string&);

private:
	DBConnectionPool();
	
	std::mutex redis_mut_;
	std::shared_ptr<DataBaseHandler> DBHandler_;
	std::shared_ptr<DataBaseHandler> mysql_handler_;
};

#endif
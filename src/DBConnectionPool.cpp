#include "DBConnectionPool.h"
#include "spdlog/spdlog.h"

DBConnectionPool::DBConnectionPool() : DBHandler_(std::make_shared<RedisHandler>()) {
}

DBConnectionPool::~DBConnectionPool() {
}

DBConnectionPool* DBConnectionPool::get_instance() {
	static DBConnectionPool DBConnectionPool;
	return &DBConnectionPool;
}

void DBConnectionPool::init_pool(void) {
	DBHandler_->connect();
}

std::string DBConnectionPool::fetch_answer_sync(const std::string& command, 
												const std::string& hash_name, 
												const std::string& key) {
	ReplyValue reply_value;
	std::string answer;
	{
		std::lock_guard<std::mutex> lg(redis_mut_);
		reply_value = DBHandler_->write_data(command, hash_name.c_str(), key.c_str());	
	}
	if (reply_value.success && reply_value.type == ResultType::STRING) {
		return reply_value.str_result;
	} else {
		return answer;
	}
} 
	
std::map<std::string, std::string> DBConnectionPool::fetch_kv_answer_sync(const std::string& command, 
																		  const std::string& hash_name) {
	ReplyValue reply_value;
	std::map<std::string, std::string> kv_result;
	{
		std::lock_guard<std::mutex> lg(redis_mut_);
		reply_value = DBHandler_->write_data(command, hash_name.c_str());	
	}
	if (reply_value.success && reply_value.type == ResultType::KEY_VALUE) {
		return reply_value.kv_result;
	} else {
		return kv_result;
	}
}
	
ReplyValue DBConnectionPool::fetch_no_answer_sync(const std::string& command, 
											const std::string& hash_name, 
											const std::string& key, 
											const std::string& value) {
	std::lock_guard<std::mutex> lg(redis_mut_);
	return DBHandler_->write_data(command, hash_name.c_str(), key.c_str(), value.c_str());
}

void DBConnectionPool::fetch_no_answer_sync(const std::string& command, 
											const std::string& hash_name, 
											const std::string& key) {
	std::lock_guard<std::mutex> lg(redis_mut_);
	DBHandler_->write_data(command, hash_name.c_str(), key.c_str());
}

ReplyValue DBConnectionPool::write_record(const std::string& sql) {
	std::lock_guard<std::mutex> lg(redis_mut_);
	return mysql_handler_->write_data(sql);
}

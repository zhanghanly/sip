#include <vector>
#include "DataBaseHandler.h"
#include "ServerConfig.h"
#include "spdlog/spdlog.h"

DataBaseHandler::~DataBaseHandler() {
}

RedisHandler::~RedisHandler() {
	if (redis_ctx_) {
		redisFree(redis_ctx_);
	}
}

void RedisHandler::connect() {
    std::string host = ServerConfig::get_instance()->redis_host();
    int port = ServerConfig::get_instance()->redis_port();
    std::string password = ServerConfig::get_instance()->redis_password();

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    redis_ctx_ = redisConnectWithTimeout(host.c_str(), port, timeout);
    if (redis_ctx_ == nullptr || redis_ctx_->err) {
        if (redis_ctx_) {
            spdlog::error("Connection error: {}", redis_ctx_->errstr);
            redisFree(redis_ctx_);
			redis_ctx_ = nullptr;
		} else {
            spdlog::error("Connection error: can't allocate redis context");
        }
        exit(1);
    }

    if (!password.empty()) {
        //std::string auth = std::string("auth ") + password;
        write_data("auth", password.c_str());
    }
}

ReplyValue RedisHandler::write_data(const std::string& command, ...) {
    ReplyValue reply_value;
	redisReply* reply;
	va_list var;

	if (command == "publish" || command == "PUBLISH" ||
	    command == "hdel" || command == "HDEL" ||
		command == "hget" || command == "HGET")  {
		va_start(var, command);
		const char* topic_or_hash = va_arg(var, const char*);
		const char* data = va_arg(var, const char*);
		va_end(var);
		/*execute redis command*/
		reply = (redisReply*)redisCommand(redis_ctx_, "%s %s %s", command.c_str(), topic_or_hash, data);
		//spdlog::info("command={} topic_or_hash={} data={}", command, topic_or_hash, data);
	} else if (command == "hgetall" || command == "HGETALL" ||
	           command == "auth" || command == "AUTH") {
		va_start(var, command);
		const char* hash_name = va_arg(var, const char*);
		va_end(var);
		/*execute redis command*/
		reply = (redisReply*)redisCommand(redis_ctx_, "%s %s", command.c_str(), hash_name);
	} else if (command == "hset" || command == "HSET" || 
	           command == "hincrby" || command == "HINCRBY") {
		va_start(var, command);
		const char* hash_name = va_arg(var, const char*);
		const char* key = va_arg(var, const char*);
		const char* value = va_arg(var, const char*);
		va_end(var);
		/*execute redis command*/
		reply = (redisReply*)redisCommand(redis_ctx_, "%s %s %s %s", command.c_str(), hash_name, key, value);
	}
	
    if (reply && reply->type != REDIS_REPLY_ERROR) {
		if (reply->type == REDIS_REPLY_STRING) {
			reply_value.type = ResultType::STRING;
			reply_value.str_result = reply->str;
		
		} else if (reply->type == REDIS_REPLY_INTEGER) {
			reply_value.type = ResultType::INTEGER;
			reply_value.int_result = reply->integer;

		} else if (reply->type == REDIS_REPLY_ARRAY) {
			reply_value.type = ResultType::KEY_VALUE;
            for (int i=0; i < reply->elements;) {
                //vec.push_back(reply->element[i]->str);
				reply_value.kv_result[reply->element[i]->str] = reply->element[i+1]->str;
				i += 2;
			}
		}
		reply_value.success = true;
    
	} else if (reply && reply->type == REDIS_REPLY_ERROR) {
        spdlog::error("redis comman {} is failed to execute", reply->str);
		reply_value.success = false;		
	}

	if (reply) {
        freeReplyObject(reply);
	}

	return reply_value;
}

void RedisHandler::close(void) {
}


#include "MessageQueue.h"
#include "spdlog/spdlog.h"
#include "ServerConfig.h"
#include "nlohmann/json.hpp"

MessageQueue::MessageQueue() {
}

MessageQueue::~MessageQueue() {
	if (work_thread_.joinable()) {
		work_thread_.join();
	}
}

void MessageQueue::start(void) {
	work_thread_ = std::thread(MessageQueue::subscribe_topic, this);
}

void MessageQueue::message_callback(redisAsyncContext *c, void *reply, void *privdata) {
	redisReply *r = (redisReply*)reply;
	if (!reply) return;
	if (r->type == REDIS_REPLY_ARRAY) {
		for (int j = 0; j < r->elements; j++) {
			if (r->element[j]->str) {
				for (auto iter = ((MessageQueue*)privdata)->message_recvers_.begin(); 
				          iter != ((MessageQueue*)privdata)->message_recvers_.end(); iter++) {
					if (!strncmp(r->element[j+1]->str, (*iter)->topic().c_str(), strlen(r->element[j+1]->str)) &&
					     r->element[j + 2]->str) {
						(*iter)->recv_message(r->element[j + 2]->str);
					}
				}
				break;
			}	
		}                                                                                     
	}   
}

void MessageQueue::subscribe_topic(MessageQueue* arg) {
	struct event_base* base = event_base_new();

	std::string host = ServerConfig::get_instance()->redis_host();
	int port = ServerConfig::get_instance()->redis_port();

	redisAsyncContext* c = redisAsyncConnect(host.c_str(), port);
	if (c->err) {                                                                             
		spdlog::error("error: {}", c->errstr);                                                     
		return;
	}                                                                                         
	std::string password = ServerConfig::get_instance()->redis_password();
	redisLibeventAttach(c, base);
	if (!password.empty()) {
		std::string auth_cmd = "auth " + password;
		redisAsyncCommand(c, MessageQueue::message_callback, (void*)arg, auth_cmd.c_str());                             
	}                                                             
	/*subscribe the topics of register*/
	for (auto iter = arg->message_recvers_.begin(); iter != arg->message_recvers_.end(); iter++) {
		std::string redis_cmd = "SUBSCRIBE " + (*iter)->topic();
		redisAsyncCommand(c, MessageQueue::message_callback, (void*)arg, redis_cmd.c_str());                             
	}
	event_base_dispatch(base);
}

void MessageQueue::register_msg_recver(MessageRecvHandle* recver) {
	if (recver) {
		message_recvers_.push_back(recver);
	}
}
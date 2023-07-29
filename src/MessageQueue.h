#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <string>
#include <list>
#include <cstdint>
#include <ctime>
#include <thread>
#include <map>
#include <memory>
#include <vector>
#include <sstream>
#include "redis/hiredis.h"
#include "redis/async.h"
#include "redis/adapters/libevent.h"


class MessageRecvHandle {
public:
	virtual ~MessageRecvHandle() {}
	virtual void recv_message(const std::string& msg) = 0;
	virtual std::string topic(void) = 0;
};

class MessageQueue {
public:
	MessageQueue();
	~MessageQueue();
	void start(void);
	static void message_callback(redisAsyncContext *c, void *reply, void *privdata);
	static void subscribe_topic(MessageQueue*);
	void register_msg_recver(MessageRecvHandle*);	

private:
	std::thread work_thread_;
	std::list<MessageRecvHandle*> message_recvers_;
};


#endif
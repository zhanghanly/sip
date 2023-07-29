#ifndef TCP_CHANNEL_H
#define TCP_CHANNEL_H

#include <functional>
#include <memory>
#include "TCPBuffer.h"

enum class EventType {
	read,
	write,
    timeout
};

enum class TCPChannelType {
	add,
	remove,
	update
};

class TCPChannel {
public:
	TCPChannel(int fd, 
			   EventType events, 
			   void* data, 
		       std::function<void(void*)> read_callback, 
               std::function<void(void*)> write_callback,
               std::function<void(void*)> error_callback) 
             : fd_(fd),
	           data_(data),
	           event_type_(events),
	           read_callback_(read_callback),
	           write_callback_(write_callback),
	           error_callback_(error_callback) {}

	int fd_;
	void* data_;
	EventType event_type_;
	TCPChannelType channel_type_;
	bool write_able_;
	std::function<void(void*)> read_callback_;
	std::function<void(void*)> write_callback_;
	std::function<void(void*)> error_callback_;
};

typedef std::shared_ptr<TCPChannel> TCPChannelSp;

#endif
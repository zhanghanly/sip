#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include <list>
#include <mutex>
#include <thread>
#include "TCPChannel.h"
#include "Dispatcher.h"

class EventLoop {
public:
    EventLoop();
    ~EventLoop();
    void add_channel_event(TCPChannelSp);
    void update_channel_event(TCPChannelSp);
    void remove_channel_event(TCPChannelSp);
    void start_loop();

private:
    std::mutex lst_mut_;    
	std::thread work_thread_;
	std::shared_ptr<Dispatcher> dispatcher_;
    std::list<TCPChannelSp> channel_lst_;
};



#endif
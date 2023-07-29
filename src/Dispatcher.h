#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <memory>
#include <map>
#include "TCPChannel.h"

class Dispatcher {
public:	
    virtual ~Dispatcher();
    virtual void init() = 0;
    virtual void add(TCPChannelSp) = 0;
    virtual void remove(TCPChannelSp) = 0;
    virtual void update(TCPChannelSp) = 0;
    virtual void clear() = 0;
    virtual void dispatch() = 0;
};


class SelectDispatcher : public Dispatcher {
public:	
	~SelectDispatcher();
	virtual void init() override;
    virtual void add(TCPChannelSp) override;
    virtual void remove(TCPChannelSp) override;
    virtual void update(TCPChannelSp) override;
    virtual void clear() override;
    virtual void dispatch() override;
};


class PollDispatcher : public Dispatcher {
public:	
	~PollDispatcher();
    virtual void init() override;
    virtual void add(TCPChannelSp) override;
    virtual void remove(TCPChannelSp) override;
    virtual void update(TCPChannelSp) override;
    virtual void clear() override;
    virtual void dispatch() override;
};


class EpollDispatcher : public Dispatcher {
public:	
	~EpollDispatcher();
	EpollDispatcher();
	virtual void init() override;
    virtual void add(TCPChannelSp) override;
    virtual void remove(TCPChannelSp) override;
    virtual void update(TCPChannelSp) override;
    virtual void clear() override;
    virtual void dispatch() override;

private:
    int epoll_fd_;
	int max_event_nums_; 
	struct epoll_event* epoll_event_;
    std::map<int, TCPChannelSp> fd_channel_;
};

#endif
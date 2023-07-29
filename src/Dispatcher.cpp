#include "Dispatcher.h"
#include "Common.h"
#include "spdlog/spdlog.h"

Dispatcher::~Dispatcher() {
}

EpollDispatcher::~EpollDispatcher() {
}

EpollDispatcher::EpollDispatcher() : max_event_nums_(1024) {
}

void EpollDispatcher::init() {
	epoll_fd_ = epoll_create1(0);
	if (epoll_fd_ < 0) {
		spdlog::error("create epoll fd error");
	}
	epoll_event_ = new epoll_event[max_event_nums_];
}

void EpollDispatcher::add(TCPChannelSp channel) {
    int fd = channel->fd_;
	int events = 0;
	if (channel->event_type_ == EventType::read) { 
		events = events | EPOLLIN;
    }
	if (channel->event_type_ == EventType::write) {
		events = events | EPOLLOUT;
    }
	struct epoll_event event;
	event.data.fd = fd;
	event.events = events;
	//event.events = events | EPOLLET;  default is condition trigger 
	if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) == -1) {
		spdlog::error("epoll_ctl add  fd failed, reason={}", strerror(errno));
    }
	fd_channel_[fd] = channel;
}

void EpollDispatcher::remove(TCPChannelSp channel) {
    int fd = channel->fd_;
	int events = 0;
	if (channel->event_type_ == EventType::read) {
		events = events | EPOLLIN;
    }
	if (channel->event_type_ == EventType::write) {
		events = events | EPOLLOUT;
    }
	struct epoll_event event;
	event.data.fd = fd;
	event.events = events;
	//event.events = events | EPOLLET;
	if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &event) == -1) {
		spdlog::error("epoll_ctl add  fd failed, reason={}", strerror(errno));
    }
	/*close fd, if remove fd first, will make a error*/
	close(fd);
	fd_channel_.erase(fd);
}

void EpollDispatcher::update(TCPChannelSp channel) {
    int fd = channel->fd_;
	int events = 0;
	if (channel->event_type_ == EventType::read) {
		events = events | EPOLLIN;
    }
	if (channel->event_type_ == EventType::write) {
		events = events | EPOLLOUT;
    }
	struct epoll_event event;
	event.data.fd = fd;
	event.events = events;
	//event.events = events | EPOLLET;
	if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event) == -1) {
		spdlog::error("epoll_ctl add  fd failed, reason={}", strerror(errno));
    }
	fd_channel_[fd] = channel;
}

void EpollDispatcher::clear() {
	//for (au)
}

void EpollDispatcher::dispatch() {
	if (fd_channel_.empty()) {
		return;
	}
	int n = epoll_wait(epoll_fd_, epoll_event_, max_event_nums_, -1);
	for (int i = 0; i < n; i++) {
		/*error event*/
		if ((epoll_event_[i].events & EPOLLERR) || (epoll_event_[i].events & EPOLLHUP)) {
			spdlog::error("epoll error event");
			TCPChannelSp channel = fd_channel_[epoll_event_[i].data.fd];
			channel->error_callback_(channel->data_);
			
			close(epoll_event_[i].data.fd);
			continue;
		}
		/*read event*/
		if (epoll_event_[i].events & EPOLLIN) {
			TCPChannelSp channel = fd_channel_[epoll_event_[i].data.fd];
			channel->read_callback_(channel->data_);
			//if (channel->write_event_is_enabled()) {
			//	channel->write_callback_(channel->data_);
			//}
		}
		/*write event*/
		if (epoll_event_[i].events & EPOLLOUT) {
			TCPChannelSp channel = fd_channel_[epoll_event_[i].data.fd];
			channel->write_callback_(channel->data_);
		}
	}
}
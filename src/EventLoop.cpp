#include <chrono>
#include "EventLoop.h"

EventLoop::EventLoop() {
	dispatcher_ = std::make_shared<EpollDispatcher>();
	work_thread_ = std::thread(&EventLoop::start_loop, this);
}

EventLoop::~EventLoop() {
	if (work_thread_.joinable()) {
		work_thread_.join();
	}
}

void EventLoop::add_channel_event(TCPChannelSp channel) {
    channel->channel_type_ = TCPChannelType::add;
    std::lock_guard<std::mutex> lg(lst_mut_);
    channel_lst_.push_back(channel);
}

void EventLoop::update_channel_event(TCPChannelSp channel) {
    channel->channel_type_ = TCPChannelType::update;
    std::lock_guard<std::mutex> lg(lst_mut_);
    channel_lst_.push_back(channel);
}

void EventLoop::remove_channel_event(TCPChannelSp channel) {
    channel->channel_type_ = TCPChannelType::remove;
    std::lock_guard<std::mutex> lg(lst_mut_);
    channel_lst_.push_back(channel);
}

void EventLoop::start_loop() {
	dispatcher_->init();
	while (true) { 
		{
			std::lock_guard<std::mutex> lg(lst_mut_);
			while (!channel_lst_.empty()) {
				TCPChannelSp channel = channel_lst_.front();
				switch (channel->channel_type_)
				{
				case TCPChannelType::add:
					dispatcher_->add(channel);
					break;
				case TCPChannelType::update:
					dispatcher_->update(channel);
					break;
				case TCPChannelType::remove:
					dispatcher_->remove(channel);
					break;
				default:
					break;
				}
				channel_lst_.pop_front();
			}
		}
		dispatcher_->dispatch();
		//std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}
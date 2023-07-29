#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <list>
#include <memory>


class SignalHandler {
public:
	virtual ~SignalHandler() {}
	virtual void process() = 0;
};


class SignalProcessor {
public:
	void register_signal_processor(std::shared_ptr<SignalHandler>); 


private:
	std::list<std::shared_ptr<SignalHandler>> signal_processor_lst_;

};


#endif
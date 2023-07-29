#include "SignalHandler.h"


void SignalProcessor::register_signal_processor(std::shared_ptr<SignalHandler> processor) {
	if (processor) {
		signal_processor_lst_.push_back(processor);
	}
}
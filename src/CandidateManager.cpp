#include "CandidateManager.h"

namespace msg {
    
CandidateManager* CandidateManager::instance() {
    static CandidateManager manager;
    return &manager;
}

void CandidateManager::set_candidate_service(CandidateServiceSp service) {
    if (service) {
        service_ = service;
    }
}

CandidateSp CandidateManager::start_stream_task(const std::string& deviceid) {
    auto task = std::make_shared<StreamTask>();
    task->deviceid = deviceid;
    task->channelid = deviceid;

    return service_->distribute_stream_task(task);
}

CandidateSp CandidateManager::query_stream_task(const std::string& deviceid) {
    return service_->query_stream_status(deviceid);
}

void CandidateManager::stop_stream_running(const std::string& deviceid) {



}


void CandidateManager::query_candidate_info(const std::string& id) {




}
    
void CandidateManager::close_stream_audio(const std::string& deviceid) {



}


void CandidateManager::open_stream_audio(const std::string& deviceid) {


}

}

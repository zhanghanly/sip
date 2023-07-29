#ifndef CANDIDATE_MANAGER_H
#define CANDIDATE_MANAGER_H

#include <string>
#include <memory>
#include <map>
#include "CandidateService.h"

namespace msg {

/*candidate service api level*/
class CandidateManager {
public:
    static CandidateManager* instance();
    void set_candidate_service(CandidateServiceSp);
    CandidateSp start_stream_task(const std::string&);
    CandidateSp query_stream_task(const std::string&);
    void stop_stream_running(const std::string&);
    void query_candidate_info(const std::string&);
    void close_stream_audio(const std::string&);
    void open_stream_audio(const std::string&);

private:
    CandidateServiceSp service_;
};

}

#endif
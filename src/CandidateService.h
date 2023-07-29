#ifndef CANDIDATE_SERVICE_H
#define CANDIDATE_SERVICE_H

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <map>
#include <set>
#include "UDPServerHandler.h"
#include "nlohmann/json.hpp"

namespace msg {

enum class Result {
	OK,
	BAD
};

/*the message came form candidate*/
struct Message {
	Message() {}
	void reset(void);
	bool is_register(void);
	bool is_keepalive(void);
	bool is_reinvite(void);
	bool is_record(void);
	bool is_record_play(void);
	bool is_record_speed(void);
	bool is_stream(void);
	bool is_cancel(void);
	bool is_result_ok(void);
	void put_value(const std::string&, const std::string&);
	std::string fetch_value(const std::string&);

	std::string method;
	std::string directin;
	std::string from;
	std::string to;
	std::string call_id;
	std::string expires;
	std::string identification;
	std::string result;
	std::map<std::string, std::string> body;
};

typedef std::shared_ptr<Message> MessageSp;

/*build and parse msg between sip and rtp*/
class MessageStack {
public:
	using json = nlohmann::json;
	/*convert json request or response message to Message*/
	MessageSp parse_json_msg(const std::string&);
	/*construct register request*/
	void req_register(MessageSp, json&);
	/*construct reinvite request*/
	void req_reinvite(MessageSp, json&);
	/*construct keepalive request*/
	void req_keepalive(MessageSp, json&);
	/*construct stream request*/
	void req_stream(MessageSp, json&);
	/*construct register resposne*/
	void resp_register(MessageSp, Result, json&);
	/*construct keepalive resposne*/
	void resp_keepalive(MessageSp, Result, json&);
	/*construct reinvite resposne*/
	void resp_reinvite(MessageSp, Result, json&);
	/*construct record resposne*/
	void resp_record(MessageSp, Result, json&);
	/*construct stream response*/
	void resp_stream(MessageSp, Result, json&);
};

typedef std::shared_ptr<MessageStack> MessageStackSp;

enum class CandidateStatus {
	CANDIDATE_REGISTER_OK,
	CANDIDATE_KEEPALIVE_OK,
	CANDIDATE_REINVITE_OK,
	CANDIDATE_STREAM_OK,
	CANDIDATE_UNKNOWN
};

struct StreamTask {
    bool start; 
	bool open_audio;
	uint32_t ssrc;
    std::string deviceid;
    std::string channelid;
    uint32_t recv_rtp_timeout;
    uint32_t max_push_durations;
};

typedef std::shared_ptr<StreamTask> TaskSp;

struct CandidateInfo {
	std::string public_ip;
	std::string recv_rtp_port;
	uint32_t rtmp_port;
	uint32_t wss_port;
	uint32_t flv_port;
	uint32_t webrtc_port;
};

typedef std::shared_ptr<CandidateInfo> CandidateSp;

class CandidateSession {
public:
	int push_stream_nums(void);
	
	/*candidate parameter*/
	std::time_t register_ts;
	std::time_t keepalive_ts;
	std::time_t reinvite_ts;
	int best_push_stream_nums;
	int max_push_stream_nums;
	int keepalive_timeout_interval;
	int max_keepalive_timeout_times;
	std::string recv_rtp_port;
	std::string recv_rtp_public_ip;
	std::string identification;
	std::map<std::string, TaskSp> stream_task;
	/*candidate network address*/
	std::string peer_ip;
	int peer_port;
	sockaddr from;
	int fromlen;
	uint32_t http_flv_port;
	uint32_t webrtc_port;
	MessageSp msg;
	/*candidate state*/
	CandidateStatus register_status;
	CandidateStatus keepalive_status;
	CandidateStatus stream_status;
};

typedef std::shared_ptr<CandidateSession> CSessionSp;


class CandidateService : public UdpDataHandler {
public:
	CandidateService();
	~CandidateService();
	/*interface for loop thread*/
	void cycle(void);
	/*interface for udp handler*/
	virtual bool on_udp_packet(const sockaddr* from, const int fromlen, char* buf, int nb_buf);
	virtual void set_udp_sock_fd(int fd) { udp_sock_fd_ = fd; }    
	std::string fetch_candidate_message(const std::string&);
	CandidateSp distribute_stream_task(TaskSp);
	CandidateSp query_stream_status(const std::string&);
	void stop_stream_task(const std::string&);

private:
	void on_udp_msg(std::string, int, std::string, sockaddr* from, int fromlen);
	/*response or request message from candidate*/	
	void response_register(MessageSp, Result, sockaddr* from, int fromlen);
	void response_keepalive(MessageSp, Result, sockaddr* from, int fromlen);
	void response_reinvite(MessageSp, Result, sockaddr* from, int fromlen);
	void response_record(MessageSp, Result, sockaddr* from, int fromlen);
	void request_stream(CSessionSp, const std::string&);
	int send_message(sockaddr* f, int l, const std::string&);
	
	CSessionSp find_min_stream_candidate(void);
	nlohmann::json dump_candidate_to_json(const std::string&);

	std::mutex mut_;
	std::thread work_thread_;
	int udp_sock_fd_;
	MessageStackSp msg_stack_;
	/*ip:port->CandidateSession*/
	std::map<std::string, CSessionSp> rtp_candidates_;
};

typedef std::shared_ptr<CandidateService> CandidateServiceSp;

}

#endif
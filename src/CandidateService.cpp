#include "CandidateService.h"
#include "spdlog/spdlog.h"
#include "ServerConfig.h"
#include "SipTime.h"
#include "SipManager.h"
#include "PublicFunc.h"
#include "DBConnectionPool.h"

namespace msg {

using json = nlohmann::json;

std::string get_candidate_status_str(CandidateStatus status) {
	switch (status) {
		case CandidateStatus::CANDIDATE_KEEPALIVE_OK:
			return "CANDIDATE_KEEPALIVE_OK";
		case CandidateStatus::CANDIDATE_REGISTER_OK:
			return "CANDIDATE_REGISTER_OK";
		case CandidateStatus::CANDIDATE_REINVITE_OK:
			return "CANDIDATE_REINVITE_OK";
		case CandidateStatus::CANDIDATE_STREAM_OK:
			return "CANDIDATE_STREAM_OK";
		default:
			return "CANDIDATE_UNKNOWN";
	}
}

CandidateStatus get_candidate_status(const std::string& status) {
	if (status == "CANDIDATE_KEEPALIVE_OK") {
		return CandidateStatus::CANDIDATE_KEEPALIVE_OK;
	} else if (status == "CANDIDATE_REGISTER_OK") {
		return CandidateStatus::CANDIDATE_REGISTER_OK;
	} else if (status == "CANDIDATE_REINVITE_OK") {
		return CandidateStatus::CANDIDATE_REINVITE_OK;
	} else if (status == "CANDIDATE_STREAM_OK") {
		return CandidateStatus::CANDIDATE_STREAM_OK;
	} else {
		return CandidateStatus::CANDIDATE_STREAM_OK;
	}
}


void Message::reset(void) {
	method.clear();
	directin.clear();
	from.clear();
	to.clear();
	call_id.clear();
	expires.clear();
	identification.clear();
	result.clear();
	body.clear();
}

bool Message::is_register(void) {
	return method == "RTP_REGISTER";
}

bool Message::is_keepalive(void) {
	return method == "RTP_KEEPALIVE";
}

bool Message::is_reinvite(void) {
	return method == "RTP_REINVITE";
}

bool Message::is_record(void) {
	return method == "RTP_RECORD";
}

bool Message::is_record_play(void) {
	return method == "RTP_RECORD_PLAY";
}

bool Message::is_record_speed(void) {
	return method == "RTP_RECORD_SPEED";
}

bool Message::is_stream(void) {
	return method == "RTP_STREAM";
}

bool Message::is_cancel(void) {
	return method == "RTP_CANCEL";
}

bool Message::is_result_ok(void) {
	return result == "OK";
}

void Message::put_value(const std::string& key, const std::string& value) {
	body[key] = value;
}

std::string Message::fetch_value(const std::string& key) {
	std::string value;
	if (body.find(key) != body.end()) {
		value = body[key];
	} 
	return value;
}


MessageSp MessageStack::parse_json_msg(const std::string& json_str) {
    MessageSp msg = std::make_shared<Message>();
    spdlog::info("begin to parse json={}", json_str);
	try {
		json j = json::parse(json_str);
		for (auto& iter : j.items()) {
			if (iter.key() == "Method") {
				msg->method = iter.value(); 
			} else if (iter.key() == "Direction") {
				msg->directin = iter.value();
			} else if (iter.key() == "From") {
				msg->from = iter.value();
			} else if (iter.key() == "To") {
				msg->to = iter.value();
			} else if (iter.key() == "Call_id") {
				msg->call_id = iter.value();
			} else if (iter.key() == "Identification") {
				msg->identification = iter.value();
			} else if (iter.key() == "Result") {
				msg->result = iter.value();
			} else if (iter.key() == "Expires") {
				msg->expires = iter.value();
			} else if (iter.key() == "Body") {
				for (auto& item : iter.value().items()) {
					msg->body[item.key()] = item.value();
				}
			}
		}
	} catch(std::runtime_error& e) {
		msg->reset();
	} catch(std::exception& e) {
		msg->reset();
	}
	return msg;
}

void MessageStack::req_register(MessageSp msg, json& j) {
	/*
	{
		"Method" : "RTP_REGISTER",
		"Direction" : "request",
		"From" : "candidate",
		"To" : "sip",
		"Expires" : "",
		"Call_id" : "",
		"Identification" : ""
		"Body" : { 
			"recv_rtp_public_ip" : "",
			"recv_rtp_port" : "",
			"keepalive_timeout" : "",
			"best_push_stream_nums" : "",
			"max_push_stream_nums" : "",
		}
	}	
	*/
	std::string recv_rtp_public_ip = msg->fetch_value("recv_rtp_public_ip"); 	
	std::string recv_rtp_port = msg->fetch_value("recv_rtp_port");
	std::string keepalive_timeout = msg->fetch_value("keepalive_timeout"); 	
	std::string best_push_stream_nums = msg->fetch_value("best_push_stream_nums"); 	
	std::string max_push_stream_nums = msg->fetch_value("max_push_stream_nums"); 	
	msg->call_id = generate_random_str(12);
	j["Method"] = "RTP_REGISTER";
	j["Direction"] = "request";
	j["From"] = "candidate";
	j["To"] = "sip";
	j["Expires"] = "3600";
	j["Call_id"] = msg->call_id;
	j["Identification"] = msg->identification;
	j["Body"]["recv_rtp_public_ip"] = recv_rtp_public_ip;
	j["Body"]["recv_rtp_port"] = recv_rtp_port;
	j["Body"]["keepalive_timeout"] = keepalive_timeout;
	j["Body"]["best_push_stream_nums"] = best_push_stream_nums;
	j["Body"]["max_push_stream_nums"] = max_push_stream_nums;
}

void MessageStack::req_reinvite(MessageSp msg, json& j) {
	/*
	{
		"Method" : "RTP_REINVITE",
		"Direction" : "request",
		"From" : "candidate",
		"To" : "sip",
		"Call_id" : "",
		"Identification" : ""
		"Body" : { 
			"deviceid" : ""
		}
	}	
	*/
	std::string deviceid = msg->fetch_value("deviceid"); 	
	msg->call_id = generate_random_str(12);
	j["Method"] = "RTP_REINVITE";
	j["Direction"] = "request";
	j["From"] = "candidate";
	j["To"] = "sip";
	j["Call_id"] = msg->call_id;
	j["Identification"] = msg->identification;
	j["Body"][ "deviceid"] = deviceid;
}

void MessageStack::req_keepalive(MessageSp msg, json& j) {
	/*
	{
		"Method" : "RTP_KEEPALIVE",
		"Direction" : "request",
		"From" : "candidate",
		"To" : "sip",
		"Call_id" : "",
		"Identification" : ""
	}	
	*/
	msg->call_id = generate_random_str(12);
	j["Method"] = "RTP_KEEPALIVE";
	j["Direction"] ="request"; 
	j["From"]  =  "candidate"; 
	j["To"]  = "sip"; 
	j["Call_id"] = msg->call_id ;
	j["Identification"] = msg->identification;
}

void MessageStack::req_stream(MessageSp msg, json& j) {
	/*
	{
		"Method" : "RTP_STREAM",
		"Direction" : "request",
		"From" : "sip",
		"To" : "candidate",
		"Call_id" : "",
		"Identification" : ""
		"Body" : { 
			"deviceid" : ""
		}
	}	
	*/
    msg->call_id = generate_random_str(12);
	std::string deviceid = msg->fetch_value("deviceid");
	j["Method"] = "RTP_STREAM";
	j["Direction"] = "request";
	j["From"] = "sip";
	j["To"] = "candidate";
	j["Call_id"] = msg->call_id;
	j["Identification"] = msg->identification;
	j["Body"]["deviceid"] = deviceid;
}

void MessageStack::resp_record(MessageSp msg, Result result, json& j) {
	/*
	{
		"Method" : "RTP_KEEPALIVE",
		"Direction" : "response",
		"From" : "sip",
		"To" : "candidate",
		"Call_id" : "",
		"Identification" : "",
		"Result" : "OK"
	}	
	*/
	std::string deviceid = msg->fetch_value("deviceid");
	std::string result_val;
	if (result == Result::OK) {
		result_val = "OK";
	} else {
		result_val = "BAD";
	}
	j["Method"] = msg->method;
	j["Direction"] = "response";
	j["From"] = "sip";
	j["To"] = "candidate";
	j["Call_id"] = msg->call_id;
	j["Identification"] = msg->identification;
	j["Result"] = result_val;
	j["Body"]["deviceid"] = deviceid;
}

void MessageStack::resp_register(MessageSp msg, Result result, json& j) {
	/*
	{
		"Method" : "RTP_REGISTER",
		"Direction" : "response",
		"From" : "sip",
		"To" : "candidate",
		"Call_id" : "",
		"Identification" : "",
		"Result" : "OK"
	}	
	*/
	std::string result_val;
	if (result == Result::OK) {
		result_val = "OK";
	} else {
		result_val = "BAD";
	}
	j["Method"] = msg->method;
	j["Direction"] = "response";
	j["From"] = "sip";
	j["To"] = "candidate";
	j["Call_id"] = msg->call_id;
	j["Identification"] = msg->identification;
	j["Result"] = result_val;
}

void MessageStack::resp_keepalive(MessageSp msg, Result result, json& j) {
	/*
	{
		"Method" : "RTP_KEEPALIVE",
		"Direction" : "response",
		"From" : "sip",
		"To" : "candidate",
		"Call_id" : "",
		"Identification" : "",
		"Result" : "OK"
	}	
	*/
	std::string result_val;
	if (result == Result::OK) {
		result_val = "OK";
	} else {
		result_val = "BAD";
	}
	j["Method"] = msg->method;
	j["Direction"] = "response";
	j["From"] = "sip";
	j["To"] = "candidate";
	j["Call_id"] = msg->call_id;
	j["Identification"] = msg->identification;
	j["Result"] = result_val;
}

void MessageStack::resp_reinvite(MessageSp msg, Result result, json& j) {
	/*
	{
		"Method" : "RTP_KEEPALIVE",
		"Direction" : "response",
		"From" : "sip",
		"To" : "candidate",
		"Call_id" : "",
		"Identification" : "",
		"Result" : "OK"
	}	
	*/
	std::string deviceid = msg->fetch_value("deviceid");
	std::string result_val;
	if (result == Result::OK) {
		result_val = "OK";
	} else {
		result_val = "BAD";
	}
	j["Method"] = msg->method;
	j["Direction"] = "response";
	j["From"] = "sip";
	j["To"] = "candidate";
	j["Call_id"] = msg->call_id;
	j["Identification"] = msg->identification;
	j["Result"] = result_val;
	j["Body"]["deviceid"] = deviceid;
}

void MessageStack::resp_stream(MessageSp msg, Result result, json& j) {
	/*
	{
		"Method" : "RTP_STREAM",
		"Direction" : "response",
		"From" : "sip",
		"To" : "candidate",
		"Call_id" : "",
		"Identification" : "",
		"Result" : "OK"
	}	
	*/
	std::string deviceid = msg->fetch_value("deviceid");
	std::string result_val;
	if (result == Result::OK) {
		result_val = "OK";
	} else {
		result_val = "BAD";
	}
	j["Method"] = msg->method;
	j["Direction"] = "response";
	j["From"] = "candidate";
	j["To"] = "sip";
	j["Call_id"] = msg->call_id;
	j["Identification"] = msg->identification;
	j["Result"] = result_val;
	j["Body"]["deviceid"] = deviceid;
}


int CandidateSession::push_stream_nums(void) {
	return stream_task.size();
}


CandidateService::CandidateService() {
	msg_stack_ = std::make_shared<MessageStack>();
	work_thread_ = std::thread(&CandidateService::cycle, this);
}

CandidateService::~CandidateService() {
	if (work_thread_.joinable()) {
		work_thread_.join();
	}
}

bool CandidateService::on_udp_packet(const sockaddr* from, const int fromlen, char* buf, int nb_buf) {
	char address_string[64];
	char port_string[16];
	if (getnameinfo(from, fromlen, 
		(char*)&address_string, sizeof(address_string),
		(char*)&port_string, sizeof(port_string),
		NI_NUMERICHOST|NI_NUMERICSERV)) {
		spdlog::warn("candidate: bad address");
		return false;
	}
	std::string peer_ip = std::string(address_string);
	int peer_port = atoi(port_string);

	std::string recv_msg(buf, nb_buf);
	on_udp_msg(peer_ip, peer_port, recv_msg, (sockaddr*)from, fromlen);
	return true;
}

void CandidateService::on_udp_msg(std::string peer_ip, int peer_port, std::string recv_msg, sockaddr* from, int fromlen) {
	MessageSp msg = msg_stack_->parse_json_msg(recv_msg);
	if (!msg) {
		spdlog::error("recieve a bad msg from ip={}, port={}", peer_ip, peer_port);
		return;
	}
	std::string candidate = msg->identification;
	if (msg->is_register()) {
		spdlog::info("receive RTP_GEGISTER message id={}", msg->identification);
		/*if candidate crash and register*/
		if (rtp_candidates_.find(candidate) != rtp_candidates_.end()) {
			for (auto& item : rtp_candidates_[candidate]->stream_task) {
				sip::SipManager::instance()->send_bye(item.first, item.first);
			}
		}
		/*the basic information of candidate*/
		std::string recv_rtp_public_ip = msg->fetch_value("recv_rtp_public_ip");  
		std::string recv_rtp_port = msg->fetch_value("recv_rtp_port");  
		std::string keepalive_timeout = msg->fetch_value("keepalive_timeout");  
		std::string max_push_stream_nums = msg->fetch_value("max_push_stream_nums");  
		std::string best_push_stream_nums = msg->fetch_value("best_push_stream_nums");  
		if (!recv_rtp_public_ip.empty() &&
			!recv_rtp_port.empty() && check_digit_validity(recv_rtp_port) &&
			!keepalive_timeout.empty() && check_digit_validity(keepalive_timeout) &&
			!max_push_stream_nums.empty() && check_digit_validity(max_push_stream_nums) &&
			!best_push_stream_nums.empty() && check_digit_validity(best_push_stream_nums)) {
			rtp_candidates_[candidate] = std::make_shared<CandidateSession>();
			rtp_candidates_[candidate]->register_ts = get_system_timestamp();
			rtp_candidates_[candidate]->identification = msg->identification;
			rtp_candidates_[candidate]->recv_rtp_public_ip = recv_rtp_public_ip;
			rtp_candidates_[candidate]->recv_rtp_port = recv_rtp_port;
			rtp_candidates_[candidate]->keepalive_timeout_interval = std::stoi(keepalive_timeout);
			rtp_candidates_[candidate]->max_push_stream_nums = std::stoi(max_push_stream_nums);
			rtp_candidates_[candidate]->best_push_stream_nums = std::stoi(best_push_stream_nums);
			rtp_candidates_[candidate]->register_status = CandidateStatus::CANDIDATE_REGISTER_OK;
			rtp_candidates_[candidate]->keepalive_status = CandidateStatus::CANDIDATE_KEEPALIVE_OK;
			/*update keepalive time*/
			rtp_candidates_[candidate]->keepalive_ts = get_system_timestamp();
			rtp_candidates_[candidate]->from = *from;
			rtp_candidates_[candidate]->fromlen = fromlen;
			/*response register*/
			response_register(msg, Result::OK, from, fromlen);
		} else {
			/*The information of register is bad*/
			response_register(msg, Result::BAD, from, fromlen);
		}

	} else if (msg->is_keepalive()) {
		spdlog::info("recieve RTP_KEEPALIVE message id={}", msg->identification);
		if (rtp_candidates_.find(candidate) != rtp_candidates_.end() &&
			rtp_candidates_[candidate]->register_status == CandidateStatus::CANDIDATE_REGISTER_OK) {
			/*update candidate keepalive time*/
			rtp_candidates_[candidate]->keepalive_ts = get_system_timestamp();
			/*update candidate address*/
			rtp_candidates_[candidate]->from = *from;
			rtp_candidates_[candidate]->fromlen = fromlen;
			/*response keepalive*/
			response_keepalive(msg, Result::OK, from, fromlen);
		} else {
			/*not found the infomation of this candidate and response BAD*/
			response_keepalive(msg, Result::BAD, from, fromlen);
		}

	} else if (msg->is_stream()) {
		spdlog::info("recieve RTP_STREAM message id={}", msg->identification);
		if (rtp_candidates_.find(candidate) != rtp_candidates_.end()) {
			std::string stream_deviceid = msg->fetch_value("deviceid");
			if (msg->is_result_ok() && check_digit_validity(stream_deviceid)) {
				spdlog::info("recieve rtp stream ack deviceid={}, id={}", stream_deviceid, msg->identification);
			}
		}

	} else {
		spdlog::warn("unknown message type id={}", msg->identification);
	}
}

CandidateSp CandidateService::distribute_stream_task(TaskSp task) {
	CandidateSp candidate = std::make_shared<CandidateInfo>();
	{
		std::lock_guard<std::mutex> lg(mut_);
		for (auto& item : rtp_candidates_) {
			if (item.second->stream_task.find(task->deviceid) !=
				item.second->stream_task.end()) {
				candidate->public_ip = item.second->recv_rtp_public_ip;
				candidate->recv_rtp_port = item.second->recv_rtp_port;
				candidate->flv_port = item.second->http_flv_port;
				candidate->webrtc_port = item.second->webrtc_port;

				return candidate;
			}
		}

		CSessionSp session = find_min_stream_candidate();
		if (!session) {
			spdlog::error("not found candidate for deviceid={}", task->deviceid);
			return nullptr;
		}
		session->stream_task[task->deviceid] = task;
		candidate->public_ip = session->recv_rtp_public_ip;
		candidate->recv_rtp_port = session->recv_rtp_port;
		candidate->flv_port = session->http_flv_port;
		candidate->webrtc_port = session->webrtc_port;

		spdlog::info("found candidate={} for deviceid={}", candidate->public_ip, task->deviceid);
	}

	return candidate;
}
	
CandidateSp CandidateService::query_stream_status(const std::string& deviceid) {
	CandidateSp candidate = std::make_shared<CandidateInfo>();
	{	
		std::lock_guard<std::mutex> lg(mut_);
		for (auto& item : rtp_candidates_) {
			if (item.second->stream_task.find(deviceid) !=
				item.second->stream_task.end()) {
				candidate->public_ip = item.second->recv_rtp_public_ip;
				candidate->recv_rtp_port = item.second->recv_rtp_port;
				candidate->flv_port = item.second->http_flv_port;
				candidate->webrtc_port = item.second->webrtc_port;

				return candidate;
			}
		}
	}
	return nullptr;
}

void CandidateService::stop_stream_task(const std::string& deviceid) {
	std::lock_guard<std::mutex> lg(mut_);
	for (auto& item : rtp_candidates_) {
		auto iter = item.second->stream_task.find(deviceid);
		if (iter != item.second->stream_task.end()) {
			spdlog::info("delete deviceid={} from candidate={}", 
			             deviceid, item.second->recv_rtp_public_ip);
			item.second->stream_task.erase(iter);
		}
	}
}

void CandidateService::cycle(void) {
    while (true) {
        /*check offline rtp candidate*/
        for (auto iter = rtp_candidates_.begin(); iter != rtp_candidates_.end();) {
            std::time_t now = get_system_timestamp();
            if (now - (iter->second)->keepalive_ts > (iter->second)->keepalive_timeout_interval) {
				spdlog::info("candiate {} timeout, remove it", iter->first);
				iter = rtp_candidates_.erase(iter);
			
			} else {
                iter++;
            }
        }
        
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

CSessionSp CandidateService::find_min_stream_candidate(void) {
    spdlog::info("rtp_candidates_.size={}", rtp_candidates_.size());
    if (!rtp_candidates_.empty()) {
        auto iter = rtp_candidates_.begin();
        CSessionSp sp = iter->second; 
        ++iter;

        for (; iter != rtp_candidates_.end(); iter++) {
            if (iter->second->push_stream_nums() < sp->push_stream_nums()) {
                sp = iter->second;
            }
        }
        if (sp->push_stream_nums() >= sp->best_push_stream_nums) {
            sp.reset();
		}
        return sp;
    } else {
        return nullptr;
    }
}

json CandidateService::dump_candidate_to_json(const std::string& identfication) {
    CSessionSp candidate_sp;
	if (rtp_candidates_.find(identfication) == rtp_candidates_.end()) {
		return "";
	} else {
		candidate_sp = rtp_candidates_[identfication];
	}
	json j;
	j["identification"] = candidate_sp->identification;
	j["push_stream_nums"] = std::to_string(candidate_sp->push_stream_nums());
	j["best_push_stream_nums"] = std::to_string(candidate_sp->best_push_stream_nums);
	j["max_push_stream_nums"] = std::to_string(candidate_sp->max_push_stream_nums);
	j["register_time"] = std::to_string(candidate_sp->register_ts);
	j["last_keepalive_time"] = std::to_string(candidate_sp->keepalive_ts);
	j["keepalive_timeout_interval"] = std::to_string(candidate_sp->keepalive_timeout_interval);
	j["max_keepalive_timeout_times"] = std::to_string(candidate_sp->max_keepalive_timeout_times);
	j["recv_rtp_port"] = candidate_sp->recv_rtp_port;
	j["recv_rtp_public_ip"] = candidate_sp->recv_rtp_public_ip;
	j["register_status"] = get_candidate_status_str(candidate_sp->register_status);
	j["keepalive_status"] = get_candidate_status_str(candidate_sp->keepalive_status);
	j["stream_status"] = get_candidate_status_str(candidate_sp->stream_status);
	for (auto item : candidate_sp->stream_task) {	
		j["device_ids"].push_back(item.first);
	}
	return j;
}

std::string CandidateService::fetch_candidate_message(const std::string& identification) {
	json j;
	if (identification == "all" || identification == "ALL") {
		for (auto item : rtp_candidates_) {
			j["candidates"].push_back(dump_candidate_to_json(item.first));
		}
	} else {
		j["candidate"] = dump_candidate_to_json(identification);
	}
		
	return j.dump();
}

void CandidateService::response_register(MessageSp msg, Result result, sockaddr* from, int fromlen) {
	json j;
	msg_stack_->resp_register(msg, result, j);
	send_message(from, fromlen, j.dump());
}

void CandidateService::response_keepalive(MessageSp msg, Result result, sockaddr* from, int fromlen) {
	json j;
	msg_stack_->resp_keepalive(msg, result, j);
	send_message(from, fromlen, j.dump());
}

void CandidateService::response_reinvite(MessageSp msg, Result result, sockaddr* from, int fromlen) {
	json j;
	msg_stack_->resp_reinvite(msg, result, j);
	send_message(from, fromlen, j.dump());
}

void CandidateService::request_stream(CSessionSp candidate, const std::string& deviceid) {
	json j;
	MessageSp msg = std::make_shared<Message>();
	msg->identification = candidate->identification;
	msg->put_value("deviceid", deviceid);
	msg_stack_->req_stream(msg, j);
	candidate->msg = msg;
	send_message(&candidate->from, candidate->fromlen, j.dump());
}

int CandidateService::send_message(sockaddr* from, int fromlen, const std::string& data) {
    int ret = sendto(udp_sock_fd_, (char*)data.c_str(), (int)data.size(), 0, from, fromlen);
    if (ret <= 0) {
        spdlog::error("send candidate msg failed, {}", strerror(ret));
    }
    return ret;
}

}
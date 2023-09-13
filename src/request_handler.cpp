#include <fstream>
#include <sstream>
#include <string>
#include "mime_types.h"
#include "reply.h"
#include "request.h"
#include "PublicFunc.h"
#include "request_handler.h"
#include "spdlog/spdlog.h"
#include "SipManager.h"
#include "nlohmann/json.hpp"
#include "CandidateManager.h"

using json = nlohmann::json;

namespace http {
namespace server {

void request_handler::handle_request(const request& req, reply& rep) {
	spdlog::info("req.uri={}", req.uri); 
	std::vector<std::string> result = string_split(req.uri, "?");
	std::vector<std::string> cmd_vector = string_split(result[0], "/");
	if (cmd_vector.size() < 4) {
		spdlog::error("url format error, body={}", req.body);
		rep = reply::stock_reply(reply::bad_request);
		return;
	}
	std::string cmd = cmd_vector[3];

	if (result[0].find("/api/sip/deviceControl/") != std::string::npos) {
		result = string_split(result[1], "&");
		if (result.size() != 4){
			spdlog::error("url parameter error");
			rep = reply::stock_reply(reply::bad_request);
			return;
		}
		std::string deviceID;
		std::string direction;
		std::string channelid;
		int speed = -1;

		for (auto iter = result.begin(); iter != result.end(); iter++) {
			std::vector<std::string> tmp = string_split(*iter, "=");
			if (tmp.size() != 2) {
				spdlog::error("url parameter error");
				rep = reply::stock_reply(reply::bad_request);
				return;
			}
			if (tmp[0] == "deviceid" || tmp[0] == "DEVICEID") {
				deviceID = tmp[1];
			} else if (tmp[0] == "direction" || tmp[0] == "DIRECTION") {
				direction = tmp[1];
			} else if (tmp[0] == "channelid" || tmp[0] == "CHANNELID") {
				channelid = tmp[1];
			} else if (tmp[0] == "speed" || tmp[0] == "SPEED") {
				speed = atoi(tmp[1].c_str());
			} else {
				spdlog::error("url parameter error");
				rep = reply::stock_reply(reply::bad_request);
				return;
			}
		}
		if (deviceID.empty() || direction.empty() || speed == -1){
			spdlog::error("url parameter error");
			rep = reply::stock_reply(reply::bad_request);
			return;
		}
		if (cmd == "ptz") {
			spdlog::info("sending pte message, deviceid={}, channelid={}, direction={}, speed={}",
						deviceID, channelid, direction, speed);
			sip::SipManager::instance()->send_ptz(deviceID, channelid, direction, speed);

		} else if(cmd == "fi") {
			spdlog::info("sending fi message, deviceid={}, channelid={}, direction={}, speed={}",
						deviceID, channelid, direction, speed);
			
			sip::SipManager::instance()->send_fi(deviceID, channelid, direction, speed);

		} else {
			spdlog::error("cmd type error");
			rep = reply::stock_reply(reply::bad_request);
			return;
		}
	} else if (result[0].find("/api/sip/deviceStream/") != std::string::npos) {
		std::string deviceID;
		std::string channelid;

		result = string_split(result[1], "&");
		if (result.size() != 2) {
			rep = reply::stock_reply(reply::bad_request);
			return;
		}

		for (auto iter = result.begin(); iter != result.end(); iter++) {
			std::vector<std::string> tmp = string_split(*iter, "=");
			if (tmp.size() != 2) {
				spdlog::error("url parameter error");
				rep = reply::stock_reply(reply::bad_request);
				return;
			}

			if (tmp[0] == "deviceid" || tmp[0] == "DEVICEID") {
				deviceID = tmp[1];
			} else if (tmp[0] == "channelid" || tmp[0] == "CHANNELID") {
				channelid = tmp[1];                          
			} else {
				spdlog::error("url parameter error");
				rep = reply::stock_reply(reply::bad_request);
				return;
			} 
		}

		if (cmd == "invite") {
			spdlog::info("sendign invite message, deviceid={} channelid={}", deviceID, channelid);
			//sip::SipManager::instance()->send_invite(deviceID, channelid);
		} else if (cmd == "bye") {  
			spdlog::info("sending bye message, deviceid={} channelid={}", deviceID, channelid);
			sip::SipManager::instance()->send_bye(deviceID, channelid);
		} else {
			spdlog::error("cmd type error");
			rep = reply::stock_reply(reply::bad_request);
			return;
		}        
	} else if (result[0].find("/api/sip/deviceMessage/") != std::string::npos) {
		result = string_split(result[1], "=");
		if (result.size() != 2 || (result[0] != "deviceid" && result[0] != "DEVICEID")) {
			rep = reply::stock_reply(reply::bad_request);
			return;
		}
		std::string json_str = sip::SipManager::instance()->fetch_device_message(result[1]);
		// Fill out the reply to be sent to the client.
		rep.status = reply::ok;
		rep.content.append(json_str.c_str(), json_str.size());
		rep.headers.resize(2);
		rep.headers[0].name = "Content-Length";
		rep.headers[0].value = std::to_string(rep.content.size());
		rep.headers[1].name = "Content-Type";
		rep.headers[1].value = "application/json";

	} else if (result[0].find("/api/sip/startPushStream") != std::string::npos) { 
		result = string_split(result[1], "=");
		if (result.size() != 2 || (result[0] != "deviceid" && result[0] != "DEVICEID")) {
			rep = reply::stock_reply(reply::bad_request);
			return;
		}
		
		json j;
		std::string deviceid = result[1];	
		msg::CandidateSp candidate = msg::CandidateManager::instance()->query_stream_task(deviceid);
		if (!candidate) {
			candidate = msg::CandidateManager::instance()->start_stream_task(deviceid);
			if (!candidate) {
				rep = reply::stock_reply(reply::bad_request);
				return;
			}
		}
		sip::SipManager::instance()->send_invite(deviceid, deviceid, 
			                                     candidate->public_ip, 
												 candidate->recv_rtp_port);

		std::string http_flv_address = "http://";
		http_flv_address += candidate->public_ip;
		http_flv_address += ":18080/yuexin/";
		http_flv_address += deviceid;
		http_flv_address += ".flv";
		
		std::string webrtc_address = "webrtc://";
		webrtc_address += candidate->public_ip;
		webrtc_address += "/yuexin/";
		webrtc_address += deviceid;
		
		j["status"] = "successful";
		j["http_flv"] = http_flv_address;
		j["webrtc"] = webrtc_address;

		rep.status = reply::ok;
		rep.content.append(j.dump().c_str(), j.dump().size());
		rep.headers.resize(2);
		rep.headers[0].name = "Content-Length";
		rep.headers[0].value = std::to_string(rep.content.size());
		rep.headers[1].name = "Content-Type";
		rep.headers[1].value = "application/json";

	} else if (result[0].find("/api/sip/deviceStatus") != std::string::npos) { 
		result = string_split(result[1], "=");
		if (result.size() != 2 || (result[0] != "deviceid" && result[0] != "DEVICEID")) {
			rep = reply::stock_reply(reply::bad_request);
			return;
		}
		
		json j;
		std::string deviceid = result[1];	
		j["status"] = sip::SipManager::instance()->query_device_status(deviceid);

		rep.status = reply::ok;
		rep.content.append(j.dump().c_str(), j.dump().size());
		rep.headers.resize(2);
		rep.headers[0].name = "Content-Length";
		rep.headers[0].value = std::to_string(rep.content.size());
		rep.headers[1].name = "Content-Type";
		rep.headers[1].value = "application/json";
	
	} else if (result[0].find("/api/sip/stopPushStream") != std::string::npos) { 
		result = string_split(result[1], "=");
		if (result.size() != 2 || (result[0] != "deviceid" && result[0] != "DEVICEID")) {
			rep = reply::stock_reply(reply::bad_request);
			return;
		}
		std::string deviceid = result[1];	
		sip::SipManager::instance()->send_bye(deviceid, deviceid); 
		msg::CandidateManager::instance()->stop_stream_running(deviceid);

	} else if (result[0].find("/api/sip/onlineDeviceNums") != std::string::npos) { 
		int nums = sip::SipManager::instance()->query_online_device_nums();

		json j;
		std::string deviceid = result[1];	
		j["nums"] = std::to_string(nums); 

		rep.status = reply::ok;
		rep.content.append(j.dump().c_str(), j.dump().size());
		rep.headers.resize(2);
		rep.headers[0].name = "Content-Length";
		rep.headers[0].value = std::to_string(rep.content.size());
		rep.headers[1].name = "Content-Type";
		rep.headers[1].value = "application/json";
	
	} else if (result[0].find("/api/sip/candidateMessage/") != std::string::npos) {
		//result = string_split(result[1], "=");
		//if (result.size() != 2 || (result[0] != "identification" && result[0] != "IDENTIFICATION")) {
		//	rep = reply::stock_reply(reply::bad_request);
		//	return;
		//}
		//std::string json_str = msg::CandidateManager::instance()->candidate_message(result[1]);
		//// Fill out the reply to be sent to the client.
		//rep.status = reply::ok;
		//rep.content.append(json_str.c_str(), json_str.size());
		//rep.headers.resize(2);
		//rep.headers[0].name = "Content-Length";
		//rep.headers[0].value = std::to_string(rep.content.size());
		//rep.headers[1].name = "Content-Type";
		//rep.headers[1].value = "application/json";

	} else if (result[0].find("/api/sip/deviceCurd/") != std::string::npos) {
		result = string_split(result[1], "=");
		if (result.size() != 2 || (result[0] != "deviceid" && result[0] != "DEVICEID")) {
			rep = reply::stock_reply(reply::bad_request);
			return;
		}

		sip::SipManager::instance()->delete_device(result[1]);

	} else {
		spdlog::info("result[0]={}", result[0]);
		rep = reply::stock_reply(reply::bad_request);
		return;
	}
	rep.status = reply::ok;
}

bool request_handler::url_decode(const std::string& in, std::string& out) {
	out.clear();
	out.reserve(in.size());
	for (std::size_t i = 0; i < in.size(); ++i) {
		if (in[i] == '%') {
			if (i + 3 <= in.size()) {
				int value = 0;
				std::istringstream is(in.substr(i + 1, 2));
				if (is >> std::hex >> value) {
					out += static_cast<char>(value);
					i += 2;
				}
				else {
					return false;
				}
			}
			else {
				return false;
			}
		}
		else if (in[i] == '+') {
			out += ' ';
		}
		else {
			out += in[i];
		}
	}
	return true;
}

} // namespace server
} // namespace http

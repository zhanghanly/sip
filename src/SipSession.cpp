#include <list>
#include <set>
#include <utility>
#include "SipSession.h"
#include "spdlog/spdlog.h"
#include "SipService.h"
#include "ServerConfig.h"
#include "PublicFunc.h"
#include "nlohmann/json.hpp"

namespace sip {

using json = nlohmann::json;

SipSession::SipSession(SipService* c, std::shared_ptr<SipRequest> r) {
    servcie_ = c;
    req_ = std::make_shared<SipRequest>();
    req_->copy(r);
    session_id_ = req_->sip_auth_id;
    reg_expires_ = 3600;
    register_status_ = SipSessionUnkonw;
    alive_status_ = SipSessionUnkonw;
    invite_status_ = SipSessionUnkonw;
    register_time_ = 0;
    alive_time_ = 0;
    invite_time_ = 0;
    query_catalog_time_ = 0;
    peer_ip_ = "";
    peer_port_ = 0;
    fromlen_ = 0;
    sip_cseq_ = 100;
    item_list_sumnum_ = 0;
    set_recv_rtp_address_flag_ = false;
	sess_build_time_ = get_system_timestamp();
	last_query_record_time_ = sess_build_time_ - 30*24*60*60;

	work_thread_ = std::thread(&SipSession::cycle, this);
}

void SipSession::set_invite_status(SipSessionStatusType s) { 
    invite_status_ = s;
    for (auto iter = device_list_.begin(); iter != device_list_.end(); iter++) {
        iter->second->invite_status = s;
    } 
}

SipSession::~SipSession() {
	destroy();
}

void SipSession::destroy() {
    /*destory all device*/
    std::map<std::string, Gb28181Device*>::iterator it;
    for (it = device_list_.begin(); it != device_list_.end(); ++it) {
        delete it->second;
    }
    device_list_.clear();
}

void SipSession::cycle(void) {
	/*send invite to camera*/
	//if (register_status_ == SipSessionRegisterOk &&
	//	alive_status_ == SipSessionAliveOk &&
	//	set_recv_rtp_address_flag_) {
	//	std::list<std::string> auto_play_list;

	//	spdlog::info("_device_list size={}", device_list_.size());
	//	std::map<std::string, Gb28181Device*>::iterator it;
	//	for (it = device_list_.begin(); it != device_list_.end(); it++) {
	//		Gb28181Device *device = it->second;
	//		std::string chid = it->first;

	//		if (device->invite_status == SipSessionInviteOk) continue;

	//		/*update device invite time*/
	//		std::time_t invite_duration = 0;
	//		if (device->invite_time != 0) {
	//			invite_duration = get_system_timestamp() - device->invite_time;
	//		}

	//		if (device->invite_status == SipSessionTrying &&
	//			invite_duration > 180) {
	//			device->invite_status = SipSessionUnkonw;
	//		}

	//		/*offline or already invite device does not need to send invite*/
	//		if (device->device_status != "ON" || 
	//			device->invite_status != SipSessionUnkonw) continue;

	//		auto_play_list.push_back(chid);
	//	
	//		/*update the invite time of device*/
	//		device->invite_time = get_system_timestamp();
	//	} 
	//	spdlog::info("auto_play_list size={}", auto_play_list.size());

	//	/*auto send sip invite and create stream chennal*/
	//	while(auto_play_list.size() > 0) {
	//		std::string chid = auto_play_list.front();
	//		auto_play_list.pop_front();
	
	//		std::shared_ptr<SipRequest> req = std::make_shared<SipRequest>();
	//		req->sip_auth_id = session_id_;
	//		req->chid = chid;
	//		req->invite_param.ip = recv_rtp_ip_;
	//		req->invite_param.port = recv_rtp_port_;
	//		req->invite_param.ssrc = generate_ssrc(session_id_);
	//		req->invite_param.tcp_flag = false;
	//		spdlog::info("_session_id={}", session_id_);  
	//		//send invite to device, req push av stream
	//		bool err = servcie_->send_invite(req);
	//		if (!err) {
	//			continue;
	//		} 
	//		//the same device can't be sent too fast. the device can't handle it
	//		set_recv_rtp_address_flag_ = false;
	//	
	//		spdlog::info("gb28181: {} clients device={} send invite", session_id_, chid);
	//	}
	//}
	/*if (_register_status == SipSessionRegisterOk &&
		reg_duration > _reg_expires){
		spdlog::info("gb28181: sip session={} register expire", _session_id);
		//break;
	}*/
	while (true) {
		if (register_status_ == SipSessionRegisterOk && alive_status_ == SipSessionAliveOk) {
			std::time_t alive_time = get_system_timestamp();
			if (alive_time - alive_time_ > ServerConfig::get_instance()->sip_keepalive_timeout()) {
				spdlog::error("gb28181: sip session={} keepalive timeout", session_id_);
				alive_status_ = SipSessionUnkonw;
				register_status_ = SipSessionUnkonw;
				clear_device_list();
			} 
			//spdlog::info("gb28181: sip session={} keepalive normal", session_id_);
		}

		/*query device channel*/
		if (alive_status_ == SipSessionAliveOk && register_status_ == SipSessionRegisterOk /*&&
			query_duration >= config->sip_query_catalog_interval*/) {
			std::shared_ptr<SipRequest> req = std::make_shared<SipRequest>();
			req->sip_auth_id = session_id_;
			std::time_t query_catalog_time = get_system_timestamp(); 
			std::time_t query_catalog_interval = ServerConfig::get_instance()->sip_catalog_interval();
			/*send catalog*/
			if (!query_catalog_time_ || query_catalog_time - query_catalog_time_ >= query_catalog_interval) {
				bool err = servcie_->send_query_catalog(req);
				if (!err) {
					spdlog::error("gb28181: sip query catalog error");
					return;
				}
				spdlog::info("send query catalog message");
				/*update catalog time*/
				query_catalog_time_ = query_catalog_time;
			}
			/*send record_Info*/
			//if (last_query_record_time_ < sess_build_time_) {
			//	std::string start_time = get_system_iso_time_str(last_query_record_time_);
			//	last_query_record_time_ += 24 * 60 * 60;
			//	std::string end_time = get_system_iso_time_str(last_query_record_time_);
			//	servcie_->send_query_record_info(req, start_time, end_time);

			//} else {
			//	sess_build_time_ = get_system_timestamp();
			//	last_query_record_time_ = sess_build_time_ - 30*24*60*60;
			//}
			//std::map<std::string, Gb28181Device*>::iterator it;
			//for (it = device_list_.begin(); it != device_list_.end(); it++) {
			//	Gb28181Device *device = it->second;
			//	std::string chid = it->first;
			
			//	std::time_t invite_duration = get_system_timestamp() - device->invite_time;

			//	if (device->invite_status != SipSessionTrying &&
			//		device->invite_status != SipSessionInviteOk) {
			//		invite_duration = 0;
			//	}
			//}
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}


void SipSession::update_device_list(std::map<std::string, std::string> lst) {
    std::map<std::string, std::string>::iterator it;
    for (it = lst.begin(); it != lst.end(); ++it) {
        std::string id = it->first;
        std::string status = it->second;

        if (device_list_.find(id) == device_list_.end()) {
            Gb28181Device *device = new Gb28181Device();
            device->device_id = id;
            if (status.find(",") != std::string::npos) {
                device->device_status = status.substr(0,status.find(","));
                device->device_name = status.substr(status.find(",")+1);
            } else {
                device->device_status = status;
                device->device_name = "NONAME";
            }
            device->invite_status = SipSessionUnkonw;
            device->invite_time = 0;
            device_list_[id] = device;

        } else {
            Gb28181Device *device = device_list_[id];
            if (status.find(",") != std::string::npos) {
                device->device_status = status.substr(0,status.find(","));
                device->device_name = status.substr(status.find(",")+1);
            } else {
                device->device_status = status;
                device->device_name = "NONAME";
            }
        }

    }
}

void SipSession::update_record_info(std::vector<std::map<std::string, std::string>>& item_list) {
	for (auto& item : item_list) {
		if (item.find("StartTime") == item.end() || 
			item.find("EndTime") == item.end()) {
			continue;
		}	
		std::string date;
		std::pair<std::string, std::string> duration;
		std::vector<std::string> res = string_split(item["StartTime"], "T");
		if (res.size() != 2) {
			continue;
		}
		date = res[0];	
		duration.first = res[1];
		res = string_split(item["EndTime"], "T");
		if (res.size() != 2) {
			continue;
		}
		duration.second = res[1];
		record_info_[date].push_back(duration);
	}
}

void SipSession::clear_device_list() {
    /*destory all devices*/
    std::map<std::string, Gb28181Device*>::iterator it;
    for (it = device_list_.begin(); it != device_list_.end(); ++it) {
        delete it->second;
    }

    device_list_.clear();
}

Gb28181Device* SipSession::get_device_info(std::string chid) {
	//for (auto& item : device_list_) {
	//	spdlog::info("deviceid={}", item.first);
	//}
	
	if (device_list_.find(chid) != device_list_.end()){
        return device_list_[chid];
    }
    return nullptr;
}

void SipSession::insert_device(const std::string& deviceid, Gb28181Device* device) {
	if (!deviceid.empty() && device) {
		device_list_[deviceid] = device;
	}
}

bool SipSession::contain_should_invite_device(void) {
	for (auto iter : device_list_) {
		if (iter.second->invite_status == SipSessionUnkonw) {
			return true;
		}
	}
	return false;
}

std::string SipSession::dump_record_info(void) {
	json j;
	for (auto& item : record_info_) {
		std::set<std::map<std::string, std::string>> vec;
		for (auto& sub_item : item.second) {
			std::map<std::string, std::string> mp;
			mp["start_time"] = sub_item.first;
			mp["end_time"] = sub_item.second;
			vec.insert(mp);
		}
		j[item.first] = vec;
	}
	return j.dump();
}

} // namespace sip
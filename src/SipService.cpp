#include "SipService.h"
#include "ServerConfig.h"
#include "spdlog/spdlog.h"
#include "PublicFunc.h"
#include "SipManager.h"
#include "DBConnectionPool.h"

namespace sip {

using json = nlohmann::json;

static std::string get_local_host_from_via(const std::string& via) {
    std::vector<std::string> vs = string_split(via, " ");
    if (vs.size() != 2) {
        spdlog::error("via format error");
        return "";
    }
    vs = string_split(vs[1], ":");
    if (vs.size() != 2) {
        spdlog::error("via format error");
        return "";
    }

    return vs[0];
}


SipService::SipService() : sip_sp_(std::make_shared<SipStack>()) {
	//work_thread_ = std::thread(SipService::loop_session, this);
}

SipService::~SipService() {
	//if (work_thread_.joinable()) {
	//	work_thread_.join();
	//}
}

bool SipService::on_udp_packet(const sockaddr* from, const int fromlen, char* buf, int nb_buf) {
    char address_string[64];
    char port_string[16];
    if(getnameinfo(from, fromlen, 
                   (char*)&address_string, sizeof(address_string),
                   (char*)&port_string, sizeof(port_string),
                   NI_NUMERICHOST|NI_NUMERICSERV)) {
        spdlog::warn("gb28181: bad address");
        return false;
    }
    std::string peer_ip = std::string(address_string);
    int peer_port = atoi(port_string);

    std::string recv_msg(buf, nb_buf);
	bool err = on_udp_sip(peer_ip, peer_port, recv_msg, (sockaddr*)from, fromlen);
	if (!err) {
		//spdlog::warn("gb28181: process udp");
		return false;
	}
    return true;
}

bool SipService::on_udp_sip(std::string peer_ip, int peer_port, std::string recv_msg, sockaddr* from, int fromlen) {
    bool err;
    RegStatus status = RegInit;

    int recv_len = recv_msg.size();
    char* recv_data = (char*)recv_msg.c_str();

    //spdlog::info("gb28181: request peer({}, {}) nbbuf={}", peer_ip, peer_port, recv_len);
    //spdlog::info("gb28181: request recv message={}", recv_data);
    if (recv_len < 10) {
        return false;
    }
    std::shared_ptr<SipRequest> req = std::make_shared<SipRequest>();
    if ((err = sip_sp_->parse_request(req, recv_data, recv_len)) != true) {
        spdlog::error("parse sip request");
        return false;
    }
    req->peer_ip = peer_ip;
    req->peer_port = peer_port;

    std::string session_id = req->sip_auth_id;

    if (req->is_register()) {
        std::vector<std::string> serial = string_split(string_replace(req->uri,"sip:", ""), "@");
        if (serial.empty()){
            spdlog::error("no server serial in sip message");
            return false;
        }
        spdlog::info("gb28181: {} method={}, uri={}, version={} expires={}", 
            req->get_cmdtype_str(), req->method,
            req->uri, req->version, req->expires);

        send_status(req, from, fromlen, status);
        if (status == RegPass) {
            std::shared_ptr<SipSession> sip_session;
            if (!(err = fetch_or_create_sip_session(req, sip_session))) {
                spdlog::error("create sip session error!");
                return false;
            }
            sip_session->set_request(req);
    
            if (req->expires) {
                sip_session->set_register_status(SipSessionRegisterOk);
                //sip_session->set_alive_status(SipSessionUnkonw);
                //sip_session->set_invite_status(SipSessionUnkonw);
            } else {
                sip_session->set_register_status(SipSessionUnkonw);
                sessions[req->sip_auth_id]->clear_device_list();
            }
            sip_session->set_register_time(get_system_timestamp());
            sip_session->set_reg_expires(req->expires);
            sip_session->set_sockaddr((sockaddr)*from);
            sip_session->set_sockaddr_len(fromlen);
            sip_session->set_peer_ip(peer_ip);
            sip_session->set_peer_port(peer_port);
        }       
    } else if (req->is_message()) {
        std::shared_ptr<SipSession> sip_session = fetch(session_id);
        if (!sip_session || sip_session->register_status() == SipSessionUnkonw) {
            spdlog::warn("gb28181: {} client not registered", req->sip_auth_id);
            return false;
        }
        //reponse status 
        if (req->cmdtype == SipCmdRequest) {
            send_status(req, from, fromlen, status);
            sip_session->set_alive_status(SipSessionAliveOk);
            sip_session->set_alive_time(get_system_timestamp());
            sip_session->set_sockaddr((sockaddr)*from);
            sip_session->set_sockaddr_len(fromlen);
            sip_session->set_peer_port(peer_port);
            sip_session->set_peer_ip(peer_ip);
            //update device list
            if (req->device_list_map.size() > 0) {
                sip_session->update_device_list(req->device_list_map);
            }
            if (!strcasecmp(req->content_type.c_str(),"application/manscdp+xml")
                && req->xml_body_map.find("Response@CmdType") != req->xml_body_map.end()
                && req->xml_body_map["Response@CmdType"] == "Catalog") {
                if (req->xml_body_map.find("Response@SumNum") != req->xml_body_map.end()) {
                    sip_session->item_list_sumnum_ = atoi(req->xml_body_map["Response@SumNum"].c_str());
                }
                std::vector<std::map<std::string, std::string> >::iterator it;
                for (it = req->item_list.begin(); it != req->item_list.end(); ++it) {
                    std::map<std::string, std::string> device = *it;
                    std::map<std::string, std::map<std::string, std::string> >::iterator it2 = sip_session->item_list.find(device["DeviceID"]);
                    if (it2 != sip_session->item_list.end()) {
                        sip_session->item_list.erase(it2);
                        sip_session->item_list[device["DeviceID"]] = device;
                    } else {
                        sip_session->item_list[device["DeviceID"]] = device;
                    }
                }
				
            } else if (!strcasecmp(req->content_type.c_str(),"application/manscdp+xml")
					   && req->xml_body_map.find("Response@CmdType") != req->xml_body_map.end()	
		 			   && req->xml_body_map["Response@CmdType"] == "RecordInfo") {
				/*update RecordInfo*/
				sip_session->update_record_info(req->item_list);

			} else {    
                /*keepalive*/
                sip_session->set_local_host(get_local_host_from_via(req->via));
                sip_session->set_alive_time(get_system_timestamp());
            }
        }
       
    } else if (req->is_invite()) {
        std::shared_ptr<SipSession> sip_session = fetch(req->sip_auth_id);
        spdlog::info("gb28181: {} method={}, uri={}, version={}", 
            		 req->get_cmdtype_str(), req->method, req->uri, req->version);
        if (!sip_session) {
            spdlog::error("gb28181: call_id {} not map {} client ", req->call_id, req->sip_auth_id);
            return false;
        }
        // sip_session->set_sockaddr((sockaddr)*from);
        // sip_session->set_sockaddr_len(fromlen);
        if (sip_session->register_status() == SipSessionUnkonw ||
            sip_session->alive_status() == SipSessionUnkonw) {
            spdlog::warn("gb28181: {} client not registered or not alive", req->sip_auth_id);
            return false;
        }
        if (req->cmdtype == SipCmdRespone) {
            spdlog::info("gb28181: INVITE response {} client status={}", req->sip_auth_id, req->status);

            if (req->status == "200") {
                spdlog::info("gb28181: device unique id is {}@{}", sip_session->session_id(), req->sip_auth_id);
                // so update ssrc to the y line in source realm '200 OK' response
                // actually, we should do this all the time
                send_ack(req, from, fromlen);
                Gb28181Device* device = sip_session->get_device_info(req->sip_auth_id);
                if (device) {
					if (req->call_id == device->req_inivate->call_id) { 
						device->invite_status = SipSessionInviteOk;
						device->req_inivate->copy(req);
						device->invite_time = get_system_timestamp();
						/*save this session to cache*/
						//dump_session_to_cache(sip_session);
					
					} else if (req->call_id == device->req_record->call_id) {
						device->record_status = SipSessionInviteOk;
						device->req_record->copy(req);
					
					}
                }
            } else if (req->status == "100") {
                //send_ack(req, from, fromlen);
                Gb28181Device* device = sip_session->get_device_info(req->sip_auth_id);
                if (device) {
					if (req->call_id == device->req_inivate->call_id) { 
						device->req_inivate->copy(req);
						device->invite_status = SipSessionTrying;
						device->invite_time = get_system_timestamp();
					} else if (req->call_id == device->req_record->call_id) {
						device->req_record->copy(req);
						device->record_status = SipSessionTrying;
					}
				}
            
			} else {
                Gb28181Device* device = sip_session->get_device_info(req->sip_auth_id);
                if (device) {
					if (req->call_id == device->req_inivate->call_id) { 
						device->req_inivate->copy(req);
						device->invite_status = SipSessionUnkonw;
						spdlog::warn("invite_status = SipSessionUnkonw session_id={}", sip_session->session_id());
						device->invite_time = get_system_timestamp();
					} else if (req->call_id == device->req_record->call_id) {
						device->req_inivate->copy(req);
						device->invite_status = SipSessionUnkonw;
						spdlog::warn("record_status = SipSessionUnkonw session_id={}", sip_session->session_id());
						device->invite_time = get_system_timestamp();
					}
				}
            }
        }
       
    } else if (req->is_bye()) {
        spdlog::info("gb28181: {} method={}, uri={}, version={}", 
            req->get_cmdtype_str(), req->method, req->uri, req->version);
      
        send_status(req, from, fromlen, status);

        //SipSession* sip_session = fetch_session_by_callid(req->call_id);
        std::shared_ptr<SipSession> sip_session = fetch(req->sip_auth_id);
        if (!sip_session) {
            spdlog::error("gb28181: call_id {} not map {} client ", req->call_id, req->sip_auth_id);
            return false;
        }
        if (req->cmdtype == SipCmdRespone) {
            spdlog::info("gb28181: BYE  {} client status={}", req->sip_auth_id, req->status);

            if (req->status == "200") {
                Gb28181Device *device = sip_session->get_device_info(req->sip_auth_id);
                if (device) {
                    device->invite_status = SipSessionUnkonw;
                    spdlog::warn("device->invite_status = SipSessionUnkonw {} {}", __LINE__,  __FUNCTION__ );
                    device->invite_time = get_system_timestamp();
                }
            
			} else {
                //TODO:fixme
                Gb28181Device* device = sip_session->get_device_info(req->sip_auth_id);
                if (device) {
                    device->invite_status = SipSessionUnkonw;
                    spdlog::warn("device->invite_status = SipSessionUnkonw {} {}", __LINE__,  __FUNCTION__ );
                    device->invite_time = get_system_timestamp();
                }
            }
        }
    } else {
        spdlog::warn("gb28181: ingor request method={}", req->method);
    }

    return true;
}

int  SipService::send_message(sockaddr* from, int fromlen, std::stringstream& ss) {
    std::string str = ss.str();
    //spdlog::info("gb28181: send_message:{}", str);
    int ret = sendto(udp_sock_fd_, (char*)str.c_str(), (int)str.length(), 0, from, fromlen);
    if (ret <= 0) {
        spdlog::error("gb28181: send_message falid ({})", ret);
    }
    
    return ret;
}

int SipService::send_ack(std::shared_ptr<SipRequest> req, sockaddr *f, int l) {
    std::stringstream ss;
    
    req->host = ServerConfig::get_instance()->sip_wai_ip();
    req->host_port = ServerConfig::get_instance()->sip_port();
    req->realm = ServerConfig::get_instance()->sip_realm();
    req->serial = ServerConfig::get_instance()->sip_serial();
    req->chid = req->sip_auth_id;

    sip_sp_->req_ack(ss, req);
    return send_message(f, l, ss);
}

int SipService::send_status(std::shared_ptr<SipRequest> req,  sockaddr *f, int l, RegStatus& status) {
    std::stringstream ss;
    
    req->host =  ServerConfig::get_instance()->sip_wai_ip();
    req->host_port = ServerConfig::get_instance()->sip_port();
    req->realm = ServerConfig::get_instance()->sip_realm();
    req->serial = ServerConfig::get_instance()->sip_serial();

    sip_sp_->resp_status(ss, req, status);
    return send_message(f, l, ss);
}


bool SipService::send_invite(std::shared_ptr<SipRequest> req) {
    bool err;
    std::shared_ptr<SipSession> sip_session = fetch(req->sip_auth_id);
    if (!sip_session) {
        spdlog::error("sip session not exist, deviceid={}", req->sip_auth_id);
        return false;
    }
    //if you are inviting or succeed in invite, 
    //you cannot invite again. you need to 'bye' and try again
    Gb28181Device* device = sip_session->get_device_info(req->chid);
    if (!device || device->device_status != "ON") {
        spdlog::error("sip device channel offline, channelid={}", req->chid);
        return false;
    }
    if (device->invite_status == SipSessionTrying ||
        device->invite_status == SipSessionInviteOk) {
        spdlog::error("sip device channel inviting");
        return false;   
    }
    req->host =  ServerConfig::get_instance()->sip_wai_ip();
    req->host_port = ServerConfig::get_instance()->sip_port();
    req->realm = ServerConfig::get_instance()->sip_realm();
    req->serial = ServerConfig::get_instance()->sip_serial();
    req->seq = sip_session->sip_cseq();

    SipRequest register_req = sip_session->request();
    req->to_realm = register_req.to_realm;
    req->from_realm = ServerConfig::get_instance()->sip_realm();
   
    std::stringstream ss;
    sip_sp_->req_invite(ss, req);
   
    sockaddr addr = sip_session->sockaddr_from();
    if (send_message(&addr, sip_session->sockaddr_fromlen(), ss) <= 0) {
        spdlog::error("sip device invite failed");
        return false;
    }
    //prame branch, from_tag, to_tag, call_id, 
    //The parameter of 'bye' must be the same as 'invite'
    device->req_inivate->copy(req);
    device->invite_time = get_system_timestamp();
    device->invite_status = SipSessionTrying;

    return true;
}

bool SipService::send_bye(std::shared_ptr<SipRequest> req, std::string chid) {
    bool err;
    std::shared_ptr<SipSession> sip_session = fetch(req->sip_auth_id);
    if (!sip_session) {
        spdlog::error("sip session not exist deviceid={}", req->sip_auth_id);
        return false;
    }
    Gb28181Device* device = sip_session->get_device_info(chid);
    if (!device) {
        spdlog::error("sip device channel not exist");
        return false;
    }
    device->invite_status = SipSessionUnkonw;
    //prame branch, from_tag, to_tag, call_id, 
    //The parameter of 'bye' must be the same as 'invite'
    //SipRequest r = sip_session->request();
    req->copy(device->req_inivate);

    req->host = ServerConfig::get_instance()->sip_wai_ip();
    req->host_port = ServerConfig::get_instance()->sip_port();
    req->realm = ServerConfig::get_instance()->sip_realm();
    req->serial = ServerConfig::get_instance()->sip_serial();
    req->chid = chid;
    req->seq = sip_session->sip_cseq();
    
    SipRequest register_req = sip_session->request();
    req->to_realm = register_req.to_realm;
    req->from_realm = ServerConfig::get_instance()->sip_realm();
    //get protocol stack 
    std::stringstream ss;
    sip_sp_->req_bye(ss, req);
   
    sockaddr addr = sip_session->sockaddr_from();
    if (send_message(&addr, sip_session->sockaddr_fromlen(), ss) <= 0) {
        spdlog::error("sip bye failed");
        return false;
    }
	/*save this session to cache*/
	//dump_session_to_cache(sip_session);
    return true;
}

bool SipService::send_sip_raw_data(std::shared_ptr<SipRequest> req,  std::string data) {
    std::shared_ptr<SipSession> sip_session = fetch(req->sip_auth_id);
    if (!sip_session) {
        spdlog::error("sip session no exist");
        return false;
    }
    std::stringstream ss;
    ss << data;

    sockaddr addr = sip_session->sockaddr_from();
    if (send_message(&addr, sip_session->sockaddr_fromlen(), ss) <= 0) {
        spdlog::error("sip raw data failed");
        return false;
    }
    return true;
}

bool SipService::send_query_catalog(std::shared_ptr<SipRequest> req) {
    std::shared_ptr<SipSession> sip_session = fetch(req->sip_auth_id);
    if (!sip_session) {
        spdlog::error("sip session not exist, deviceid={}", req->sip_auth_id);
        return false;
    }
    req->host = ServerConfig::get_instance()->sip_wai_ip();
    req->host_port = ServerConfig::get_instance()->sip_port();
    req->realm = ServerConfig::get_instance()->sip_realm();
    req->serial = ServerConfig::get_instance()->sip_serial();
    req->chid = req->sip_auth_id;
    req->seq = sip_session->sip_cseq();
    //get protocol stack 
    std::stringstream ss;
    sip_sp_->req_query_catalog(ss, req);

    return send_sip_raw_data(req, ss.str());
}

bool SipService::send_query_record_info(std::shared_ptr<SipRequest> req, 
										const std::string& start_time, 
										const std::string& end_time) {
    std::shared_ptr<SipSession> sip_session = fetch(req->sip_auth_id);
    if (!sip_session) {
        spdlog::error("sip session not exist, deviceid={}", req->sip_auth_id);
        return false;
    }
    req->host = ServerConfig::get_instance()->sip_wai_ip();
    req->host_port = ServerConfig::get_instance()->sip_port();
    req->realm = ServerConfig::get_instance()->sip_realm();
    req->serial = ServerConfig::get_instance()->sip_serial();
    req->chid = req->sip_auth_id;
    req->seq = sip_session->sip_cseq();
    //get protocol stack 
    std::stringstream ss;
    sip_sp_->req_query_record_info(ss, req, start_time, end_time);

    return send_sip_raw_data(req, ss.str());
}

bool SipService::send_record_invite(std::shared_ptr<SipRequest> req) {
    bool err;
    std::shared_ptr<SipSession> sip_session = fetch(req->sip_auth_id);
    if (!sip_session) {
        spdlog::error("sip session not exist, deviceid={}", req->sip_auth_id);
        return false;
    }
    //if you are inviting or succeed in invite, 
    //you cannot invite again. you need to 'bye' and try again
    Gb28181Device* device = sip_session->get_device_info(req->chid);
    if (!device || device->device_status != "ON") {
        spdlog::error("sip device channel offline");
        return false;
    }
    if (device->record_status  == SipSessionTrying ||
        device->record_status  == SipSessionInviteOk) {
        spdlog::error("sip device channel record inviting");
        return false;   
    }
    req->host =  ServerConfig::get_instance()->sip_wai_ip();
    req->host_port = ServerConfig::get_instance()->sip_port();
    req->realm = ServerConfig::get_instance()->sip_realm();
    req->serial = ServerConfig::get_instance()->sip_serial();
    req->seq = sip_session->sip_cseq();

    SipRequest register_req = sip_session->request();
    req->to_realm = register_req.to_realm;
    req->from_realm = ServerConfig::get_instance()->sip_realm();
   
    std::stringstream ss;
    sip_sp_->req_record(ss, req);
    sockaddr addr = sip_session->sockaddr_from();
    if (send_message(&addr, sip_session->sockaddr_fromlen(), ss) <= 0) {
        spdlog::error("sip device invite failed");
        return false;
    }
    //prame branch, from_tag, to_tag, call_id, 
    //The parameter of 'bye' must be the same as 'invite'
    device->req_record->copy(req);
    device->record_status = SipSessionTrying;

    return true;
}

bool SipService::send_record_start(std::shared_ptr<SipRequest> req) {
    std::shared_ptr<SipSession> sip_session = fetch(req->sip_auth_id);
    if (!sip_session) {
        spdlog::error("sip session not exist, deviceid={}", req->sip_auth_id);
        return false;
    }
    Gb28181Device* device = sip_session->get_device_info(req->chid);
    if (!device) {
        spdlog::error("sip device channel not exist");
        return false;
    }
	if (device->record_status != SipSessionInviteOk) {
		return false;
	}
    //prame branch, from_tag, to_tag, call_id, 
    //The parameter of 'bye' must be the same as 'invite'
    //SipRequest r = sip_session->request();
    req->copy(device->req_record);
    req->host = ServerConfig::get_instance()->sip_wai_ip();
    req->host_port = ServerConfig::get_instance()->sip_port();
    req->realm = ServerConfig::get_instance()->sip_realm();
    req->serial = ServerConfig::get_instance()->sip_serial();
    req->seq = sip_session->sip_cseq();
    
    SipRequest register_req = sip_session->request();
    req->to_realm = register_req.to_realm;
    req->from_realm = ServerConfig::get_instance()->sip_realm();
    //get protocol stack 
    std::stringstream ss;
    sip_sp_->req_record_start(ss, req);

    return send_sip_raw_data(req, ss.str());
}

bool SipService::send_record_stop(std::shared_ptr<SipRequest> req) {
    std::shared_ptr<SipSession> sip_session = fetch(req->sip_auth_id);
    if (!sip_session) {
        spdlog::error("sip session not exist, deviceid={}", req->sip_auth_id);
        return false;
    }
    Gb28181Device* device = sip_session->get_device_info(req->chid);
    if (!device) {
        spdlog::error("sip device channel not exist");
        return false;
    }
	if (device->record_status != SipSessionInviteOk) {
		return false;
	}
    //prame branch, from_tag, to_tag, call_id, 
    //The parameter of 'bye' must be the same as 'invite'
    //SipRequest r = sip_session->request();
    req->copy(device->req_record);
    req->host = ServerConfig::get_instance()->sip_wai_ip();
    req->host_port = ServerConfig::get_instance()->sip_port();
    req->realm = ServerConfig::get_instance()->sip_realm();
    req->serial = ServerConfig::get_instance()->sip_serial();
    req->seq = sip_session->sip_cseq();
    
    SipRequest register_req = sip_session->request();
    req->to_realm = register_req.to_realm;
    req->from_realm = ServerConfig::get_instance()->sip_realm();
    //get protocol stack 
    std::stringstream ss;
    sip_sp_->req_record_stop(ss, req);

    return send_sip_raw_data(req, ss.str());
}

bool SipService::send_record_seek(std::shared_ptr<SipRequest> req, const std::string& range) {
    std::shared_ptr<SipSession> sip_session = fetch(req->sip_auth_id);
    if (!sip_session) {
        spdlog::error("sip session not exist, deviceid={}", req->sip_auth_id);
        return false;
    }
    Gb28181Device* device = sip_session->get_device_info(req->chid);
    if (!device) {
        spdlog::error("sip device channel not exist");
        return false;
    }
	if (device->record_status != SipSessionInviteOk) {
		return false;
	}
    //prame branch, from_tag, to_tag, call_id, 
    //The parameter of 'bye' must be the same as 'invite'
    //SipRequest r = sip_session->request();
    req->copy(device->req_record);
    req->host = ServerConfig::get_instance()->sip_wai_ip();
    req->host_port = ServerConfig::get_instance()->sip_port();
    req->realm = ServerConfig::get_instance()->sip_realm();
    req->serial = ServerConfig::get_instance()->sip_serial();
    req->seq = sip_session->sip_cseq();
    
    SipRequest register_req = sip_session->request();
    req->to_realm = register_req.to_realm;
    req->from_realm = ServerConfig::get_instance()->sip_realm();
    //get protocol stack 
    std::stringstream ss;
    sip_sp_->req_record_seek(ss, req, range);

    return send_sip_raw_data(req, ss.str());
}

bool SipService::send_record_fast(std::shared_ptr<SipRequest> req, const std::string& scale) {
    std::shared_ptr<SipSession> sip_session = fetch(req->sip_auth_id);
    if (!sip_session) {
        spdlog::error("sip session not exist, deviceid={}", req->sip_auth_id);
        return false;
    }
    Gb28181Device* device = sip_session->get_device_info(req->chid);
    if (!device) {
        spdlog::error("sip device channel not exist");
        return false;
    }
	if (device->record_status != SipSessionInviteOk) {
		return false;
	}
	//prame branch, from_tag, to_tag, call_id, 
    //The parameter of 'bye' must be the same as 'invite'
    //SipRequest r = sip_session->request();
    req->copy(device->req_record);
    req->host = ServerConfig::get_instance()->sip_wai_ip();
    req->host_port = ServerConfig::get_instance()->sip_port();
    req->realm = ServerConfig::get_instance()->sip_realm();
    req->serial = ServerConfig::get_instance()->sip_serial();
    req->seq = sip_session->sip_cseq();
    
    SipRequest register_req = sip_session->request();
    req->to_realm = register_req.to_realm;
    req->from_realm = ServerConfig::get_instance()->sip_realm();
    //get protocol stack 
    std::stringstream ss;
    sip_sp_->req_record_fast(ss, req, scale);

    return send_sip_raw_data(req, ss.str());
}

bool SipService::send_record_teardown(std::shared_ptr<SipRequest> req) {
    std::shared_ptr<SipSession> sip_session = fetch(req->sip_auth_id);
    if (!sip_session) {
        spdlog::error("sip session not exist, deviceid={}", req->sip_auth_id);
        return false;
    }
    Gb28181Device* device = sip_session->get_device_info(req->chid);
    if (!device) {
        spdlog::error("sip device channel not exist");
        return false;
    }
	if (device->record_status == SipSessionUnkonw) {
		return false;
	}
	device->record_status = SipSessionUnkonw;
    //prame branch, from_tag, to_tag, call_id, 
    //The parameter of 'bye' must be the same as 'invite'
    //SipRequest r = sip_session->request();
    req->copy(device->req_record);
    req->host = ServerConfig::get_instance()->sip_wai_ip();
    req->host_port = ServerConfig::get_instance()->sip_port();
    req->realm = ServerConfig::get_instance()->sip_realm();
    req->serial = ServerConfig::get_instance()->sip_serial();
    req->seq = sip_session->sip_cseq();
    
    SipRequest register_req = sip_session->request();
    req->to_realm = register_req.to_realm;
    req->from_realm = ServerConfig::get_instance()->sip_realm();
    //get protocol stack 
    std::stringstream ss;
    sip_sp_->req_record_teardown(ss, req);

    return send_sip_raw_data(req, ss.str());
}

bool SipService::send_ptz(std::shared_ptr<SipRequest> req, std::string chid, 
						  std::string cmd, uint8_t speed, int priority) {
    bool err;
    std::shared_ptr<SipSession> sip_session = fetch(req->sip_auth_id);
    if (!sip_session) {
        spdlog::error("sip session not exist, deviceid={}", req->sip_auth_id);
        return false;
    }
    Gb28181Device* device = sip_session->get_device_info(chid);
    if (!device) {
        spdlog::error("sip device channel not exist");
        return false;
    }
    if (sip_session->register_status() != SipSessionRegisterOk) {
        spdlog::error("sip device channel not inviting");
        return false;   
    }
    //prame branch, from_tag, to_tag, call_id, 
    //The parameter of 'bye' must be the same as 'invite'
    //SipRequest r = sip_session->request();
    req->copy(sip_session->register_req());
    req->host = ServerConfig::get_instance()->sip_wai_ip();
    req->host_port = ServerConfig::get_instance()->sip_port();
    req->realm = ServerConfig::get_instance()->sip_realm();
    req->serial = ServerConfig::get_instance()->sip_serial();
    req->chid = chid;
    req->seq = sip_session->sip_cseq();
    
    SipPtzCmdType ptzcmd = SipPtzCmdRight;
    const char *ss_cmd = cmd.c_str();
    if (!strcasecmp(ss_cmd, "stop")) {
        ptzcmd = SipPtzCmdStop;
    } else if (!strcasecmp(ss_cmd, "right")) {
        ptzcmd = SipPtzCmdRight;
    } else if (!strcasecmp(ss_cmd, "left")) {
        ptzcmd = SipPtzCmdLeft;
    } else if (!strcasecmp(ss_cmd, "down")) {
        ptzcmd = SipPtzCmdDown;
    } else if (!strcasecmp(ss_cmd, "up")) {
        ptzcmd = SipPtzCmdUp;
    } else if (!strcasecmp(ss_cmd, "zoomout")) {
        ptzcmd = SipPtzCmdZoomOut;
    } else if (!strcasecmp(ss_cmd, "zoomin")) {    
        ptzcmd = SipPtzCmdZoomIn;
    } else if (!strcasecmp(ss_cmd, "rightUp")) {
        ptzcmd = SipPtzCmdRightUp;
    } else if (!strcasecmp(ss_cmd, "rightDown")) {
        ptzcmd = SipPtzCmdRightDown;
    } else if (!strcasecmp(ss_cmd, "leftUp")) {
        ptzcmd = SipPtzCmdLeftUp;
    } else if (!strcasecmp(ss_cmd, "leftDown")) {
        ptzcmd = SipPtzCmdLeftDown;
    } else {
        spdlog::error("sip ptz cmd no support");
        return false;  
    }
    if (speed < 0 || speed > 0xFF) {
        spdlog::error("sip ptz cmd speed out of range");
        return false;  
    }
    if (priority <= 0 ) {
        priority = 5;
    }
    //get protocol stack 
    std::stringstream ss;
    sip_sp_->req_ptz(ss, req, ptzcmd, speed, priority);
   
    sockaddr addr = sip_session->sockaddr_from();
    if (send_message(&addr, sip_session->sockaddr_fromlen(), ss) <= 0) {
        spdlog::error("sip ptz failed");
        return false;
    }

    return true;
}

bool SipService::send_fi(std::shared_ptr<SipRequest> req, std::string chid,
						 std::string cmd, uint8_t speed) {
    bool err;
    std::shared_ptr<SipSession> sip_session = fetch(req->sip_auth_id);
    if (!sip_session) {
        spdlog::error("sip session not exist, deviceid={}", req->sip_auth_id);
        return false;
    }
    Gb28181Device* device = sip_session->get_device_info(chid);
    if (!device) {
        spdlog::error("sip device channel not exist");
        return false;
    }
    if (sip_session->register_status() != SipSessionRegisterOk) {
        spdlog::error("sip device channel not inviting");
        return false;   
    }
    //prame branch, from_tag, to_tag, call_id, 
    //The parameter of 'bye' must be the same as 'invite'
    //SipRequest r = sip_session->request();
    req->copy(sip_session->register_req());
    req->host = ServerConfig::get_instance()->sip_wai_ip();
    req->host_port = ServerConfig::get_instance()->sip_port();
    req->realm = ServerConfig::get_instance()->sip_realm();
    req->serial = ServerConfig::get_instance()->sip_serial();
    req->chid = chid;
    req->seq = sip_session->sip_cseq();
    
    SipFiCmdType ficmd = SipFiCmdStop;
    const char *ss_cmd = cmd.c_str();
    if (!strcasecmp(ss_cmd, "stop")) {
        ficmd = SipFiCmdStop;
    } else if (!strcasecmp(ss_cmd, "IrisBig")) {
        ficmd = SipFiCmdIrisBig;
    } else if (!strcasecmp(ss_cmd, "IrisSmall")) {
        ficmd = SipFiCmdIrisSmall;
    } else if (!strcasecmp(ss_cmd, "FocusFar")) {
        ficmd = SipFiCmdFocusFar;
    } else if (!strcasecmp(ss_cmd, "FocusNear")) {
        ficmd = SipFiCmdFocusNear;
    } else {
        spdlog::error("sip ptz cmd no support");
        return false;  
    }
    if (speed < 0 || speed > 0xFF){
        spdlog::error("sip ptz cmd speed out of range");
        return false;  
    }
    //get protocol stack 
    std::stringstream ss;
    sip_sp_->req_fi(ss, req, ficmd, speed);
   
    sockaddr addr = sip_session->sockaddr_from();
    if (send_message(&addr, sip_session->sockaddr_fromlen(), ss) <= 0)
    {
        spdlog::error("sip ptz failed");
        return false;
    }

    return true;
}

bool SipService::fetch_or_create_sip_session(std::shared_ptr<SipRequest> req, std::shared_ptr<SipSession>& sip_session) {
    std::shared_ptr<SipSession> sess_sp;
    if ((sess_sp = fetch(req->sip_auth_id)) != nullptr) {
        sip_session = sess_sp;
        return true;
    }
    sess_sp = std::make_shared<SipSession>(this, req);
	{
		std::lock_guard<std::mutex> lg(session_mut_);
		sessions[req->sip_auth_id] = sess_sp;
	}
	sip_session = sess_sp;
    
    return true;
}

std::shared_ptr<SipSession> SipService::fetch(std::string sid) {
	std::lock_guard<std::mutex> lg(session_mut_);
    std::map<std::string, std::shared_ptr<SipSession>>::iterator it = sessions.find(sid);
    if (it == sessions.end()) {
        return nullptr;
    } else {
        return it->second;
    }
}

void SipService::remove_session(const std::string& deviceid) {
    if (sessions.find(deviceid) != sessions.end()) {
        sessions[deviceid]->set_register_status(SipSessionUnkonw);
        sessions[deviceid]->set_alive_status(SipSessionUnkonw);
        sessions[deviceid]->clear_device_list();
    }
}

json SipService::dump_device_message_to_json(const std::string& deviceid) {
	json j;
	std::shared_ptr<SipSession> sess = fetch(deviceid);
	if (!sess) {
		j["deviceid"] = deviceid;
		j["error"] = "device not build session";
		return j;
	} 
	Gb28181Device* device = sess->get_device_info(deviceid);
	if (!device) {
		j["deviceid"] = deviceid;
		j["error"] = "please wait a miute, not send catalog to device";
		return j;
	} 
	j["deviceid"] = deviceid;
	j["channelid"] = device->device_id;
	j["remote_ip"] = sess->peer_ip();
	j["remote_port"] = sess->peer_port();
	j["local_host"] = sess->local_host();
	j["device_status"] = device->device_status;
	j["register_status"] = get_sip_session_status_str(sess->register_status());
	j["keepalive_status"] = get_sip_session_status_str(sess->alive_status());
	j["invite_status"] = get_sip_session_status_str(device->invite_status);
	j["register_time"] = get_iso_time_by_ts(sess->register_time());
	j["query_catalog_time"] = get_iso_time_by_ts(sess->query_catalog_time());
	j["keepalive_time"] = get_iso_time_by_ts(sess->alive_time());
	j["invite_time"] = get_iso_time_by_ts(device->invite_time);

	return j;
}

std::string SipService::dump_all_devices_message_to_json(void) {
	json j;
	for (auto item : sessions) {
		j["devices"].push_back(dump_device_message_to_json(item.first));
	}

    return j.dump();
}

int SipService::unset_address_session_nums(void) {
    int unset_nums = 0;

    for (auto iter = sessions.begin(); iter != sessions.end(); iter++) {
        if (!iter->second->recv_rtp_address_flag() &&
		    iter->second->register_status() == SipSessionRegisterOk && 
			iter->second->alive_status() == SipSessionAliveOk) {
            unset_nums++;
        }
    }

    return unset_nums;
}

bool SipService::is_session_online(const std::string& deviceid) {
    std::lock_guard<std::mutex> lg(session_mut_);
    if (sessions.find(deviceid) == sessions.end()) {
        return false;
    }
    if (sessions[deviceid]->register_status() == SipSessionRegisterOk && 
	    sessions[deviceid]->alive_status() == SipSessionAliveOk) {
        return true;
    }
    return false;
}

//std::list<std::string> SipService::fetch_ready_invite_deviceid(void) {
//	std::list<std::string> invite_device_lst;
//	std::lock_guard<std::mutex> lg(session_mut_);
//	for (auto iter = sessions.begin(); iter != sessions.end(); iter++) {
//        if (iter->second->contain_should_invite_device() &&   
//		    iter->second->register_status() == SipSessionRegisterOk && 
//			iter->second->alive_status() == SipSessionAliveOk) {
//            invite_device_lst.push_back(iter->second->session_id());
//        }
//    }
//	return invite_device_lst;
//}

//void SipService::loop_session(SipService* arg) {
//	while (true) {
//		for (auto iter = arg->sessions.begin(); iter != arg->sessions.end();) {
//			/*send catalog or invite or check offline*/
//			(iter->second)->cycle();
//			/*session is offline*/
//			if ((iter->second)->register_status() == SipSessionUnkonw &&
//				(iter->second)->alive_status() == SipSessionUnkonw) {
//				/*update cache sessions*/
//				DBConnectionPool::get_instance()->fetch_answer_sync("HDEL", "smd:sip_session", iter->second->session_id());
//				{
//					std::lock_guard<std::mutex> lg(arg->session_mut_);
//					spdlog::warn("session_id={} is time out, delete it", (iter->second)->session_id());
//					iter = arg->sessions.erase(iter);
//				}
//				/*update cache sessions*/
//			} else {
//				iter++;
//			}
//		}
//		std::this_thread::sleep_for(std::chrono::milliseconds(200));
//	}
//}

//void SipService::dump_session_to_cache(std::shared_ptr<SipSession> sess) {
//    Gb28181Device* device = sess->get_device_info(sess->session_id());
//	if (!device) {
//		return;
//	}
//	json j; 
//	j["session_id"] = sess->session_id();
//	j["peer_ip"] =  sess->peer_ip();
//	j["peer_port"] = std::to_string(sess->peer_port());
//	j["local_host"] = sess->local_host();
//	j["register_status"] = get_sip_session_status_str(sess->register_status());
//	j["keepalive_status"] = get_sip_session_status_str(sess->alive_status());
//	j["register_time"] = std::to_string(sess->register_time());
//	j["query_catalog_time"] = std::to_string(sess->query_catalog_time());
//	j["keepalive_time"] = std::to_string(sess->alive_time()) ;
//	j["sip_cseq"] = std::to_string(sess->sip_cseq());
//	j["recv_rtp_address_flag"] = std::to_string(sess->recv_rtp_address_flag());
//	j["recv_rtp_ip"] = sess->recv_rtp_ip();
//	j["recv_rtp_port"] = std::to_string(sess->recv_rtp_port());
//	j["register_request"]["via"] = sess->register_req()->via; 
//	j["register_request"]["from"] = sess->register_req()->from; 
//	j["register_request"]["to"] = sess->register_req()->to;
//	j["register_request"]["from_tag"] = sess->register_req()->from_tag;
//	j["register_request"]["to_tag"] = sess->register_req()->to_tag;
//	j["register_request"]["branch"] = sess->register_req()->branch;
//	j["register_request"]["call_id"] = sess->register_req()->call_id;
//	j["device"]["deviceid"] = device->device_id;
//	j["device"]["device_status"] = device->device_status;
//	j["device"]["invite_status"] = get_sip_session_status_str(device->invite_status);
//	j["device"]["invite_time"] = std::to_string(device->invite_time);
//	j["device"]["invite_branch"] = device->req_inivate->branch;
//	j["device"]["invite_from_tag"] = device->req_inivate->from_tag;
//	j["device"]["invite_to_tag"] = device->req_inivate->to_tag;
//	j["device"]["invite_call_id"] = device->req_inivate->call_id;
//	j["sockaddr"]["sa_family"] = std::to_string(sess->sockaddr_from().sa_family);
//	j["sockaddr"]["addr_len"] = std::to_string(sess->sockaddr_fromlen());
//	for (int i = 0; i < sizeof(sess->sockaddr_from().sa_data); i++) {
//		j["sockaddr"]["sa_data"].push_back(std::to_string(sess->sockaddr_from().sa_data[i]));
//	}
//
//	DBConnectionPool::get_instance()->fetch_no_answer_sync("hset", "smd:sip_session", sess->session_id(), j.dump());	 	
//}

//void SipService::load_sessions_from_cache(void) {
//	std::map<std::string, std::string> session_json;
//	//std::string query_sessions_command = "HGETALL smd:sip_session";
//	session_json = DBConnectionPool::get_instance()->fetch_kv_answer_sync("HGETALL", "smd:sip_session");	 	
//
//	std::lock_guard<std::mutex> lg(session_mut_);
//	for (auto session : session_json) {
//		if (!check_digit_validity(session.first)) {
//			continue;
//		}
//		sessions[session.first] = std::make_shared<SipSession>(this, std::make_shared<SipRequest>());
//		
//		try {
//			json j = json::parse(session.second);
//
//			for (auto& iter : j.items()) {
//				if (iter.key() == "session_id") {
//					sessions[session.first]->set_session_id(iter.value()); 
//				} else if (iter.key() == "peer_ip") {
//					sessions[session.first]->set_peer_ip(iter.value());
//				} else if (iter.key() == "peer_port") {
//					sessions[session.first]->set_peer_port(std::stoi(std::string(iter.value())));
//				} else if (iter.key() == "local_host") {
//					sessions[session.first]->set_local_host(iter.value());
//				} else if (iter.key() == "register_status") {
//					sessions[session.first]->set_register_status(get_sip_session_status(iter.value()));
//				} else if (iter.key() == "keepalive_status") {
//					sessions[session.first]->set_alive_status(get_sip_session_status(iter.value()));
//				} else if (iter.key() == "register_time") {
//					sessions[session.first]->set_register_time(std::stoi(std::string(iter.value())));
//				} else if (iter.key() == "keepalive_time") {
//					//sessions[session.first]->set_alive_time(std::stol(std::string(iter.value())));
//					sessions[session.first]->set_alive_time(get_system_timestamp());
//				} else if (iter.key() == "query_catalog_time") {
//					sessions[session.first]->set_query_catalog_time(std::stol(std::string(iter.value())));
//				} else if (iter.key() == "sip_cseq") {
//					sessions[session.first]->set_sip_seq(std::stoi(std::string(iter.value())));
//				} else if (iter.key() == "recv_rtp_address_flag") {
//					sessions[session.first]->set_recv_rtp_address_flag(std::stoi(std::string(iter.value())));
//				} else if (iter.key() == "recv_rtp_ip") {
//					sessions[session.first]->set_recv_rtp_ip(iter.value());
//				} else if (iter.key() == "recv_rtp_port") {
//					sessions[session.first]->set_recv_rtp_port(std::stoi(std::string(iter.value())));
//				} else if (iter.key() == "sockaddr") {
//					sockaddr from;	
//					
//					for (auto& item : iter.value().items()) {
//						if (item.key() == "sa_family") {
//							from.sa_family = std::stoi(std::string(item.value()));
//						} else if (item.key() == "sa_data") {
//							std::vector<std::string> sa_data_vec = item.value();
//							/*escape stack overflow*/
//							if (sizeof(from.sa_data) == sa_data_vec.size()) {
//								for (int i = 0; i < sa_data_vec.size(); i++) {
//									from.sa_data[i] = std::stoi(sa_data_vec[i]);
//								}
//							}
//						} else if (item.key() == "addr_len") {
//							sessions[session.first]->set_sockaddr_len(std::stoi(std::string(item.value())));
//						}
//					}
//					sessions[session.first]->set_sockaddr(from);	
//				} else if (iter.key() == "register_request") {
//					std::shared_ptr<SipRequest> req_sp = std::make_shared<SipRequest>();
//					
//					for (auto& item : iter.value().items()) {
//						if (item.key() == "via") {
//							req_sp->via = item.value();
//						} else if (item.key() == "from") {
//							req_sp->from = item.value();
//						} else if (item.key() == "to") {
//							req_sp->to = item.value();
//						} else if (item.key() == "from_tag") {
//							req_sp->from_tag = item.value();
//						} else if (item.key() == "to_tag") {
//							req_sp->to_tag = item.value();
//						} else if (item.key() == "call_id") {
//							req_sp->call_id = item.value();
//						} else if (item.key() == "branch") {
//							req_sp->branch = item.value();
//						} else if (item.key() == "realm") {
//							req_sp->realm = item.value();
//						}
//					}
//					sessions[session.first]->set_request(req_sp);	
//				} else if (iter.key() == "device") {
//					Gb28181Device* device = new Gb28181Device();
//					
//					for (auto& item : iter.value().items()) {
//						if (item.key() == "deviceid") {
//							device->device_id = item.value();
//						} else if (item.key() == "device_status") {
//							device->device_status = item.value();
//						} else if (item.key() == "invite_status") {
//							device->invite_status = get_sip_session_status(item.value());
//						} else if (item.key() == "invite_time") {
//							device->invite_time = std::stol(std::string(item.value())); 								
//						} else if (item.key() == "invite_branch") {
//							device->req_inivate->branch = item.value();
//						} else if (item.key() == "invite_from_tag") {
//							device->req_inivate->from_tag = item.value();
//						} else if (item.key() == "invite_to_tag") {
//							device->req_inivate->to_tag = item.value();
//						} else if (item.key() == "invite_call_id") {
//							device->req_inivate->call_id = item.value();
//						}
//					}
//					sessions[session.first]->insert_device(device->device_id, device); 
//				}
//			}
//		} catch(std::runtime_error& e) {
//			sessions.erase(session.first);
//		} catch(std::exception& e) {
//			sessions.erase(session.first);
//		}
//	}

} //namespace sip
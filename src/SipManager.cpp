#include <stdexcept>
#include "SipManager.h"
#include "ServerConfig.h"
#include "PublicFunc.h"
#include "spdlog/spdlog.h"
#include "DBConnectionPool.h"
#include "SipTime.h"

namespace sip {

using json = nlohmann::json;

SipManager::SipManager() {
}

SipManager::~SipManager() {
}    

SipManager* SipManager::instance() {
    static SipManager sipManager;
    return &sipManager;
}

void SipManager::send_ptz(const std::string& deviceid, const std::string& channelid, const std::string& direction, int speed) {
    std::shared_ptr<sip::SipRequest> req = std::make_shared<sip::SipRequest>();
    req->sip_auth_id = deviceid;

    sip_service_->send_ptz(req, channelid, direction, speed, 0);
}

void SipManager::send_fi(const std::string& deviceid, const std::string& channelid, const std::string& direction, int speed) {
    std::shared_ptr<sip::SipRequest> req = std::make_shared<sip::SipRequest>();
    req->sip_auth_id = deviceid;
    
    sip_service_->send_fi(req, channelid, direction, speed);
}

void SipManager::send_invite(const std::string& deviceid, const std::string& channelid,
                             const std::string& recv_rtp_ip, const std::string& recv_rtp_port) {
    std::shared_ptr<sip::SipRequest> req = std::make_shared<sip::SipRequest>();
    req->sip_auth_id = deviceid;
	req->chid = channelid;
	req->invite_param.ip = recv_rtp_ip ;
	req->invite_param.port = atoi(recv_rtp_port.c_str());
	req->invite_param.ssrc = generate_ssrc(deviceid);
	req->invite_param.tcp_flag = false;

    sip_service_->send_invite(req);
}

void SipManager::send_bye(const std::string& deviceid, const std::string& channelid) {
    std::shared_ptr<sip::SipRequest> req = std::make_shared<sip::SipRequest>();
    req->sip_auth_id = deviceid;
    
    sip_service_->send_bye(req, channelid);
}

void SipManager::delete_device(const std::string& deviceid) {
    sip_service_->remove_session(deviceid);
}
    
void SipManager::reload_conf_file() {
    ServerConfig::get_instance()->init_conf("../conf/server.conf");
}

int SipManager::query_online_device_nums() {
    return sip_service_->online_session_nums();
}

std::string SipManager::query_device_status(const std::string& deviceid) {
    return sip_service_->is_session_online(deviceid) ? "online" : "offline";
}

std::string SipManager::fetch_device_message(const std::string& deviceid) {
	json j;
	if (deviceid == "all" || deviceid == "ALL") {
        return sip_service_->dump_all_devices_message_to_json(); 
    } else {
        return sip_service_->dump_device_message_to_json(deviceid).dump();
    }
}

}
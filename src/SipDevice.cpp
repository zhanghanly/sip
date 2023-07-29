#include "SipDevice.h"

namespace sip {

std::string get_sip_session_status_str(SipSessionStatusType status) {
    switch(status) {
        case SipSessionRegisterOk:
            return "RegisterOk";
        case SipSessionAliveOk:
            return "AliveOk";
        case SipSessionInviteOk:
            return "InviteOk";
        case SipSessionTrying:
            return "InviteTrying";
        case SipSessionBye:
            return "InviteBye";
        case SipSessionUnRegister:
            return "UnRegister";
        default:
            return "Unknow";
    }
}

SipSessionStatusType get_sip_session_status(const std::string& status) {
	if (status == "RegisterOk") {
		return SipSessionRegisterOk;
	} else if (status == "AliveOk") {
		return SipSessionAliveOk;
	} else if (status == "InviteOk") {
		return SipSessionInviteOk;
	} else if (status == "InviteTrying") {
		return SipSessionTrying;
	} else if (status == "InviteBye") {
		return SipSessionBye;
	} else if (status == "UnRegister") {
		return SipSessionUnRegister;
	} else {
		return SipSessionUnkonw;
	}
}

Gb28181Device::Gb28181Device() {
    device_id = ""; 
    device_name = "";
    invite_status = SipSessionUnkonw;
    invite_time = 0;
    device_status = "";
    req_inivate = std::make_shared<SipRequest>();
	record_status = SipSessionUnkonw;
    req_record = std::make_shared<SipRequest>();
}

Gb28181Device::~Gb28181Device(){
}

} // namespace sip
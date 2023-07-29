#ifndef SIP_DEVICE_H
#define SIP_DEVICE_h

#include <string>
#include "SipStack.h"
#include "SipTime.h"

namespace sip {

enum SipSessionStatusType {
     SipSessionUnkonw = 0,
     SipSessionRegisterOk = 1,
     SipSessionAliveOk = 2,
     SipSessionInviteOk = 3,
     SipSessionTrying = 4,
     SipSessionBye = 5,
     SipSessionUnRegister = 6
};

/*convert SipSessionStatusType to describe string*/
std::string get_sip_session_status_str(SipSessionStatusType);
/*convert describe string to SipSessionStatusType*/
SipSessionStatusType get_sip_session_status(const std::string&);

class Gb28181Device {
public:
    Gb28181Device();
    ~Gb28181Device();
    
	std::string device_id;
    std::string device_name;
    std::string device_status;
	/*for live invite stream*/
    SipSessionStatusType  invite_status; 
    std::time_t  invite_time;
    std::shared_ptr<SipRequest>  req_inivate;
	/*for record stream*/
    SipSessionStatusType  record_status; 
    std::shared_ptr<SipRequest>  req_record;
};

} //namespace sip

#endif
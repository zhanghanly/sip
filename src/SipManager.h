#ifndef SIP_MANAGER_H
#define SIP_MANAGER_H

#include <string>
#include <map>
#include <set>
#include <list>
#include <thread>
#include <mutex>
#include "SipService.h"

namespace sip {

class SipManager {
public:
    ~SipManager();
    static SipManager* instance();
    /*reload configure file*/
    void reload_conf_file();
    /*the interface is uesed to be invoked by http*/
    void set_sip_service(std::shared_ptr<sip::SipService> ss) { sip_service_ = ss; }
    void send_ptz(const std::string&, const std::string&, const std::string&, int);
    void send_fi(const std::string&, const std::string&, const std::string&, int);
    void send_invite(const std::string&, const std::string&, const std::string&, const std::string&);
    void send_bye(const std::string&, const std::string&);
    void delete_device(const std::string&);
    /*online or offline*/
    std::string query_device_status(const std::string&);
    std::string fetch_device_message(const std::string&);

private:
    SipManager();
    std::shared_ptr<sip::SipService> sip_service_;
};

}

#endif
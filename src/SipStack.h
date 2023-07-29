#ifndef SIP_STACK_H
#define SIP_STACK_H

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include <ctime>

namespace sip {

#define SIP_RTSP_CRLF "\r\n"
#define SIP_RTSP_CRLFCRLF "\r\n\r\n"

// SIP methods
#define SIP_METHOD_REGISTER       "REGISTER"
#define SIP_METHOD_MESSAGE        "MESSAGE"
#define SIP_METHOD_INVITE         "INVITE"
#define SIP_METHOD_ACK            "ACK"
#define SIP_METHOD_BYE            "BYE"

// SIP-Version
#define SIP_VERSION "SIP/2.0"
#define SIP_USER_AGENT "sip_rtp"

#define SIP_PTZ_START 0xA5


enum SipCmdType {
    SipCmdRequest=0,
    SipCmdRespone=1
};

enum SipPtzCmdType {
    SipPtzCmdStop = 0x00,
    SipPtzCmdRight = 0x01,
    SipPtzCmdLeft  = 0x02,
    SipPtzCmdDown  = 0x04,
    SipPtzCmdUp    = 0x08,
    SipPtzCmdZoomIn  = 0x10,
    SipPtzCmdZoomOut   = 0x20,
    SipPtzCmdRightUp = 0x01 | 0x08   ,
    SipPtzCmdRightDown = 0x01 | 0x04,
    SipPtzCmdLeftUp  = 0x02 | 0x08,
    SipPtzCmdLeftDown  = 0x02 | 0x04
};

enum SipFiCmdType {
    SipFiCmdStop = 0x00,
    SipFiCmdIrisBig = 0x04,
    SipFiCmdIrisSmall = 0x08,
    SipFiCmdFocusFar = 0x01,
    SipFiCmdFocusNear = 0x02
};

enum RegStatus {
    RegInit,
    RegForbidden,
    RegUnauthorized,
    RegPass
};

std::string srs_sip_get_utc_date();

class SipRequest {
public:
    //sip header member
    std::string method;
    std::string uri;
    std::string version;
    std::string status;

    std::string via;
    std::string from;
    std::string to;
    std::string from_tag;
    std::string to_tag;
    std::string branch;
    
    std::string call_id;
    long seq;

    std::string contact;
    std::string user_agent;

    std::string content_type;
    long content_length;

    long expires;
    int max_forwards;

    std::string www_authenticate;
    std::string authorization;

    std::string chid;

    std::map<std::string, std::string> xml_body_map;
    std::map<std::string, std::string> device_list_map;
    // add an item_list, you can do a lot of other things
    // used by DeviceList, Alarmstatus, RecordList in "GB/T 28181—2016"
    std::vector<std::map<std::string, std::string>> item_list;

    std::string serial;
    std::string realm;
    std::string sip_auth_id;
    std::string sip_auth_pwd;
    std::string sip_username;
    std::string peer_ip;
    int peer_port;
    std::string host;
    int host_port;
    SipCmdType cmdtype;

    std::string from_realm;
    std::string to_realm;
    uint32_t y_ssrc;

	struct InviteParam {
		std::string ip;	
		int port;
		uint32_t ssrc;
		bool tcp_flag;
		std::string channelid;
		std::time_t start_time;
		std::time_t end_time;	
	} invite_param;

public:
    SipRequest();
    ~SipRequest();
public:
    bool is_register();
    bool is_invite();
    bool is_message();
    bool is_ack();
    bool is_bye();
   
    void copy(std::shared_ptr<SipRequest> src);
public:
    std::string get_cmdtype_str();
};

// The gb28181 sip protocol stack.
class SipStack {
public:
    SipStack();
    ~SipStack();
public:
    bool parse_request(std::shared_ptr<SipRequest> preq, const char *recv_msg, int nb_buf);
protected:
    bool do_parse_request(std::shared_ptr<SipRequest> req, const char *recv_msg);
    bool parse_xml(std::string xml_msg, std::map<std::string, std::string> &json_map, 
				   std::vector<std::map<std::string, std::string> > &item_list);

private:
    //response from
    std::string get_sip_from(std::shared_ptr<SipRequest> req);
    //response to
    std::string get_sip_to(std::shared_ptr<SipRequest> req);
    //response via
    std::string get_sip_via(std::shared_ptr<SipRequest> req);
    bool verify_ipc_authority(const std::string&);

public:
    //response:  request sent by the sip-agent, wait for sip-server response
    void resp_status(std::stringstream& ss, std::shared_ptr<SipRequest>, RegStatus& status);
    void resp_keepalive(std::stringstream& ss, std::shared_ptr<SipRequest>);
    //request:  request sent by the sip-server, wait for sip-agent response
    void req_invite(std::stringstream& ss, std::shared_ptr<SipRequest>); 
    void req_record(std::stringstream& ss, std::shared_ptr<SipRequest>); 
    void req_ack(std::stringstream& ss, std::shared_ptr<SipRequest>);
    void req_bye(std::stringstream& ss, std::shared_ptr<SipRequest>);
    void req_401_unauthorized(std::stringstream& ss, std::shared_ptr<SipRequest>);
    void req_403_forbidden(std::stringstream& ss, std::shared_ptr<SipRequest>);
    void req_query_catalog(std::stringstream& ss, std::shared_ptr<SipRequest>);
    void req_query_record_info(std::stringstream& ss, std::shared_ptr<SipRequest>, const std::string&, const std::string&);
    void req_record_start(std::stringstream& ss, std::shared_ptr<SipRequest>);
    void req_record_stop(std::stringstream& ss, std::shared_ptr<SipRequest>);
    void req_record_seek(std::stringstream& ss, std::shared_ptr<SipRequest>, const std::string&);
    void req_record_fast(std::stringstream& ss, std::shared_ptr<SipRequest>, const std::string&);
    void req_record_teardown(std::stringstream& ss, std::shared_ptr<SipRequest>);
    void req_ptz(std::stringstream& ss, std::shared_ptr<SipRequest>, uint8_t cmd, uint8_t speed,  int priority);
    void req_fi(std::stringstream& ss, std::shared_ptr<SipRequest>, uint8_t cmd, uint8_t speed);
};

} //namespace sip

#endif
#ifndef SIP_SERVICE_H
#define SIP_SERVICE_H

#include <thread>
#include <mutex>
#include "SipSession.h"
#include "DataBaseHandler.h"
#include "UDPServerHandler.h"
#include "nlohmann/json.hpp"

namespace sip {

class SipService : public UdpDataHandler {
public:
    SipService();
    virtual ~SipService();

    // Interface ISrsUdpHandler
public:
    virtual bool on_udp_packet(const sockaddr* from, const int fromlen, char* buf, int nb_buf);
    virtual void set_udp_sock_fd(int fd) { udp_sock_fd_ = fd; }    

private:
    void destroy();
    bool on_udp_sip(std::string host, int port, std::string recv_msg, sockaddr* from, int fromlen);

public:
    int send_message(sockaddr* f, int l, std::stringstream& ss);
    int send_ack(std::shared_ptr<SipRequest> req, sockaddr *f, int l);
    int send_status(std::shared_ptr<SipRequest> req, sockaddr *f, int l, RegStatus&);
    bool send_invite(std::shared_ptr<SipRequest> req);
    bool send_bye(std::shared_ptr<SipRequest> req, std::string chid);
    bool send_query_catalog(std::shared_ptr<SipRequest> req);
    bool send_record_invite(std::shared_ptr<SipRequest> req);
    bool send_record_start(std::shared_ptr<SipRequest> req);
    bool send_record_stop(std::shared_ptr<SipRequest> req);
    bool send_record_seek(std::shared_ptr<SipRequest> req, const std::string&);
    bool send_record_fast(std::shared_ptr<SipRequest> req, const std::string&);
    bool send_record_teardown(std::shared_ptr<SipRequest> req);
    bool send_query_record_info(std::shared_ptr<SipRequest> req, const std::string&, const std::string&);
    bool send_ptz(std::shared_ptr<SipRequest> req, std::string chid, std::string cmd, uint8_t speed, int priority);
    bool send_fi(std::shared_ptr<SipRequest> req, std::string chid, std::string cmd, uint8_t speed);

    // The SIP command is transmitted through HTTP API, 
    // and the body content is transmitted to the device, 
    // mainly for testing and debugging, For example, here is HTTP body:
    // BYE sip:34020000001320000003@3402000000 SIP/2.0
    // Via: SIP/2.0/UDP 39.100.155.146:15063;rport;branch=z9hG4bK34205410
    // From: <sip:34020000002000000001@3402000000>;tag=512355410
    // To: <sip:34020000001320000003@3402000000>;tag=680367414
    // Call-ID: 200003304
    // CSeq: 21 BYE
    // Max-Forwards: 70
    // User-Agent: sip_rtp
    // Content-Length: 0
    //
    //
    bool send_sip_raw_data(std::shared_ptr<SipRequest> req, std::string data);
    //bool query_sip_session(std::string sid,  SrsJsonArray* arr);
    //bool query_device_list(std::string sid,  SrsJsonArray* arr);
    bool is_session_online(const std::string&);
    int online_session_nums();

public:
    bool fetch_or_create_sip_session(std::shared_ptr<SipRequest> req,  std::shared_ptr<SipSession>& sip_session);
    std::shared_ptr<SipSession> fetch(std::string id);
    void remove_session(const std::string&);
    nlohmann::json dump_device_message_to_json(const std::string&);
    std::string dump_all_devices_message_to_json();
    std::string dump_session_record_to_json(const std::string&);
    int unset_address_session_nums(void);
	std::list<std::string> fetch_ready_invite_deviceid();
    void reset_session_recv_address_flag(const std::string&);
	/*save session to redis when sip id down*/
	void dump_session_to_cache(std::shared_ptr<SipSession>);
	void load_sessions_from_cache(void);
	/*update the state of session and send invite and catatlog*/
	//static void loop_session(SipService*);

private:
    std::shared_ptr<SipStack> sip_sp_;
    std::map<std::string, std::shared_ptr<SipSession>> sessions;
	int udp_sock_fd_;
	//std::thread work_thread_;
	std::mutex session_mut_;
};

} //namespace sip

#endif
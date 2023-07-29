#ifndef SIP_SESSION_H
#define SIP_SESSION_H

#include <string>
#include <map>
#include <vector>
#include <thread>
#include <list>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "SipDevice.h"

namespace sip {

class SipService;

class SipSession {
public:
    SipSession(SipService *c, std::shared_ptr<SipRequest> r);
    virtual ~SipSession();

private:
    void destroy();

public:
    void set_register_status(SipSessionStatusType s) { register_status_ = s;}
    void set_alive_status(SipSessionStatusType s) { alive_status_ = s;}
    void set_invite_status(SipSessionStatusType s);
    void set_register_time(std::time_t t) { register_time_ = t;}
    void set_alive_time(std::time_t t) { alive_time_ = t;}
    void set_invite_time(std::time_t t) { invite_time_ = t;}
    void set_query_catalog_time(std::time_t t) { query_catalog_time_ = t; }
    //void set_recv_rtp_time(std::time_t t) { _recv_rtp_time = t;}
    void set_reg_expires(int e) { reg_expires_ = 3600;}
    void set_peer_ip(std::string i) { peer_ip_ = i;}
    void set_peer_port(int o) { peer_port_ = o;}
    void set_sockaddr(sockaddr  f) { from_ = f;}
    void set_sockaddr_len(int l) { fromlen_ = l;}
    void set_request(std::shared_ptr<SipRequest> r) { req_->copy(r);}
    void set_local_host(const std::string& local_host) { local_host_ = local_host; }
    void set_recv_rtp_address(const std::string&, int);
    void set_recv_rtp_ip(const std::string& ip) { recv_rtp_ip_ = ip; }
    void set_recv_rtp_port(int port) { recv_rtp_port_ = port; }
	void set_session_id(const std::string& session_id) { session_id_ = session_id; }
	void set_sip_seq(int sip_seq) { sip_cseq_ = sip_seq; }

    std::string local_host() { return local_host_; }
    SipSessionStatusType register_status() { return register_status_;}
    SipSessionStatusType alive_status() { return  alive_status_;}
    SipSessionStatusType invite_status() { return  invite_status_;}
    std::time_t register_time() { return  register_time_;}
    std::time_t alive_time() { return alive_time_;}
    std::time_t invite_time() { return invite_time_;}
    std::time_t query_catalog_time() { return query_catalog_time_; }
    //std::time_t recv_rtp_time() { return _recv_rtp_time;}
    int reg_expires() { return reg_expires_;}
    std::string peer_ip() { return peer_ip_;}
    int peer_port() { return peer_port_;}
    sockaddr  sockaddr_from() { return from_;}
    int sockaddr_fromlen() { return fromlen_;}
    SipRequest request() { return *req_;}
    std::shared_ptr<SipRequest> register_req() { return req_;}
    int sip_cseq() { return sip_cseq_++;}
    bool recv_rtp_address_flag() { return set_recv_rtp_address_flag_;}
	std::string recv_rtp_ip() { return recv_rtp_ip_; }
	int recv_rtp_port() { return recv_rtp_port_; }

    std::string session_id() { return session_id_;}
    std::map<std::string, std::map<std::string, std::string> > item_list;
    int item_list_sumnum_;
public:
    void update_device_list(std::map<std::string, std::string> devlist);
    void update_record_info(std::vector<std::map<std::string, std::string>>&);
    void clear_device_list();
    Gb28181Device* get_device_info(std::string chid);
    void insert_device(const std::string&, Gb28181Device*); 
	bool contain_should_invite_device();
    void cycle(void);
	std::string dump_record_info();

private:
    SipService *servcie_;
    std::string session_id_;
    SipSessionStatusType register_status_;
    SipSessionStatusType alive_status_;
    SipSessionStatusType invite_status_;
    std::time_t register_time_;
    std::time_t alive_time_;
    std::time_t invite_time_;
    std::time_t reg_expires_;
    std::time_t query_catalog_time_;
    std::string peer_ip_;
    int peer_port_;
    std::string local_host_;
    sockaddr  from_;
    int fromlen_;
    std::shared_ptr<SipRequest> req_;
    std::map<std::string, Gb28181Device*> device_list_;
    //std::map<std::string, int> _device_status;
    int sip_cseq_;
    std::string recv_rtp_ip_;
    int recv_rtp_port_;
    bool set_recv_rtp_address_flag_;
	std::time_t sess_build_time_;
	std::time_t last_query_record_time_;
	//std::map<std::string, std::set<std::string>> record_info_;
	typedef std::list<std::pair<std::string, std::string>> list_pair;
	std::map<std::string, list_pair> record_info_;
    std::thread work_thread_;
};

} //namespace sip


#endif

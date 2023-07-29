#include <signal.h>
#include "ServerConfig.h"
#include "SipService.h"
#include "server.h"
#include "GlobalCacheConf.h"
#include "SipManager.h"
#include "UDPServerHandler.h"
#include "DBConnectionPool.h"
#include "PublicFunc.h"
#include "CandidateService.h"
#include "CandidateManager.h"

int main(int argc, char** argv) {
	/*run as daemon*/
	if (!daemon(1, 1)) {
		/*deal SIGPIPE*/
		signal(SIGPIPE, signal_handler);
		/*init log configure*/
		log_init();
		/*load configure file*/
		ServerConfig::get_instance()->init_conf();
		/*init DBConnectionPool*/
		DBConnectionPool::get_instance()->init_pool();
		/*dispach subscribe message*/
		std::shared_ptr<MessageQueue> mq = std::make_shared<MessageQueue>(); 
		ServerType server_type = ServerConfig::get_instance()->judge_server_type();
		/*open sip service*/
		std::shared_ptr<UDPServerHandler> server_handle = std::make_shared<UDPSipServer>();
		std::shared_ptr<sip::SipService> sip_service = std::make_shared<sip::SipService>();
		/*load sip session from redis*/
		//sip_service->load_sessions_from_cache();
		server_handle->set_udp_data_handler(sip_service);
		sip::SipManager::instance()->set_sip_service(sip_service);
		/*start sip service*/
		server_handle->start();
		/*open msg service*/	
		std::shared_ptr<UDPServerHandler> server_handle_msg = std::make_shared<UDPMsgServer>();
		std::shared_ptr<msg::CandidateService> msg_service = std::make_shared<msg::CandidateService>();
		/*load candidate session from redis*/
		server_handle_msg->set_udp_data_handler(msg_service);
		msg::CandidateManager::instance()->set_candidate_service(msg_service);
		/*start msg service*/
		server_handle_msg->start();
		/*open http server*/
		http::server::server ser("0.0.0.0", ServerConfig::get_instance()->sip_http_port());
		ser.run();
	}        
	
	return 0;
}
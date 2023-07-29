#include "UDPServerHandler.h"
#include "spdlog/spdlog.h"
#include "ServerConfig.h"

ClientHandler::~ClientHandler() {
}


UDPClient::~UDPClient() {
}

bool UDPClient::connect(const std::string& ip, int port) {
	/*create udp socket fd*/
	udp_sock_fd_ = socket(AF_INET, SOCK_DGRAM, 0);  
	if (udp_sock_fd_ < 0) {  
		spdlog::error("create udp socket failed, ip={}, port={}", ip, port);
		return false;  
	}  
	/*set remote address*/  
	memset(&addr_, 0, sizeof(addr_));  
	addr_.sin_family = AF_INET;
	addr_.sin_addr.s_addr = inet_addr(ip.c_str());
	addr_.sin_port = htons(port);
	addr_len_ = sizeof(addr_);
	
	return true;
}

void UDPClient::send_data(const std::string& data) {
	int send_num = sendto(udp_sock_fd_, data.c_str(), data.size(), 0, (struct sockaddr*)&addr_, addr_len_);  
	if (send_num < 0) {  
		spdlog::error("sendto data failed on udp socket");  
	}  
}

void UDPClient::set_recv_timeout(time_t sec) {
	struct timeval timeout; 
	timeout.tv_sec = sec;
	timeout.tv_usec = 0;
	if (setsockopt(udp_sock_fd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
		spdlog::error("set recv timeout failed for udp socket");
	}
}

std::string UDPClient::recv_data(void) {
	char recv_buf[MAX_UDP_PACKET_SIZE];
	struct sockaddr_in peer_addr;
	socklen_t peer_len;
	
	//int ret = recvfrom(udp_sock_fd_, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&addr_, &addr_len_);
	int ret = recvfrom(udp_sock_fd_, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&peer_addr, &peer_len);
	if (ret == EWOULDBLOCK || ret == EAGAIN) {
		spdlog::error("recv data timeout on udp socket");
		return "";
	} else if (ret < 0) {
		//spdlog::error("recv data failed on udp socket, {}", strerror(ret));  
		return "";
	}
	return std::string(recv_buf, ret);
}

void UDPClient::close(void) {
	/*udp do nothing*/
}

UdpDataHandler::UdpDataHandler() {
}

UdpDataHandler::~UdpDataHandler() {
}


UDPServerHandler::UDPServerHandler() {
}

UDPServerHandler::~UDPServerHandler() {
}


void UDPServerHandler::set_udp_data_handler(std::shared_ptr<UdpDataHandler> handler) {
	udp_data_handler_ = handler;
} 

void UDPServerHandler::create_udp_socket(int port) {
	server_sock_fd_ = socket(PF_INET, SOCK_DGRAM, 0);
	if (server_sock_fd_ == -1) {
		spdlog::error("create UDP socket error!");
	        return;
	}
	memset(&server_addr_, 0, sizeof(server_addr_));
	server_addr_.sin_family = AF_INET;
	server_addr_.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr_.sin_port = htons(port);
 
	if (bind(server_sock_fd_, (struct sockaddr*)&server_addr_, sizeof(server_addr_)) == -1) {
		spdlog::error("udp bind error!");
		return;
	}
	if (udp_data_handler_) {
		udp_data_handler_->set_udp_sock_fd(server_sock_fd_);
	}
}

void UDPServerHandler::cycle(UDPServerHandler* arg) {
	struct sockaddr_in client_addr;
	char message[MAX_UDP_PACKET_SIZE];
	socklen_t client_addr_sz;
	client_addr_sz = sizeof(client_addr);
	
	while(true) {
		int str_len = recvfrom(arg->server_sock_fd_, message, MAX_UDP_PACKET_SIZE, 0, (struct sockaddr*)&client_addr, &client_addr_sz);
		if (arg->udp_data_handler_) {
			arg->udp_data_handler_->on_udp_packet((struct sockaddr*)&client_addr, client_addr_sz, message, str_len);
		}
	}
}

UDPSipServer::UDPSipServer() {
}

UDPSipServer::~UDPSipServer() {
	if (work_thread_.joinable()) {
		work_thread_.join();
	}
}

void UDPSipServer::start(void) {
	create_udp_socket(ServerConfig::get_instance()->sip_port());
	work_thread_ = std::thread(UDPServerHandler::cycle, this);
}

void UDPSipServer::close(void) {
}

UDPRtpServer::UDPRtpServer() {
}

UDPRtpServer::~UDPRtpServer() {
	if (work_thread_.joinable()) {
		work_thread_.join();
	}
}

void UDPRtpServer::start(void) {
	create_udp_socket(ServerConfig::get_instance()->rtp_fix_recv_stream_port());
	work_thread_ = std::thread(UDPServerHandler::cycle, this);
}

void UDPRtpServer::close(void) {
}

UDPMsgServer::UDPMsgServer() {
}

UDPMsgServer::~UDPMsgServer() {
	if (work_thread_.joinable()) {
		work_thread_.join();
	}
}

void UDPMsgServer::start(void) {
	create_udp_socket(ServerConfig::get_instance()->candidate_port());
	work_thread_ = std::thread(UDPServerHandler::cycle, this);
}

void UDPMsgServer::close(void) {
}
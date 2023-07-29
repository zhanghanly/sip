#ifndef UDP_SERVER_HANDLER_H
#define UDP_SERVER_HANDLER_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <functional>
#include <memory>
#include <thread>
#include <string>

#define MAX_UDP_PACKET_SIZE  65535

/*udp or tcp client*/
class ClientHandler {
public:
	virtual ~ClientHandler();
	virtual bool connect(const std::string&, int) = 0;
	virtual void send_data(const std::string&) = 0;
	virtual void set_recv_timeout(time_t) = 0;
	virtual std::string recv_data(void) = 0;
	virtual void close(void) = 0;
};

typedef std::shared_ptr<ClientHandler> ClientSp;

/*udp client*/
class UDPClient : public ClientHandler {
public:
	~UDPClient();
	bool connect(const std::string&, int) override;
	void send_data(const std::string&) override;
	void set_recv_timeout(time_t) override;
	std::string recv_data(void) override;
	void close(void) override;

private:
	int udp_sock_fd_;
	socklen_t addr_len_;  
	struct sockaddr_in addr_;  
};


class UdpDataHandler {
public:
	UdpDataHandler();
	virtual ~UdpDataHandler();
	virtual bool on_udp_packet(const sockaddr* from, const int fromlen, char* buf, int nb_buf) = 0;
	virtual void set_udp_sock_fd(int fd) = 0;
};


class UDPServerHandler {
public:
	UDPServerHandler();
	virtual ~UDPServerHandler();
	void set_udp_data_handler(std::shared_ptr<UdpDataHandler>);
	void create_udp_socket(int);
	virtual void start(void) = 0;
	virtual void close(void) = 0;
	static void cycle(UDPServerHandler*);

protected:
	std::shared_ptr<UdpDataHandler> udp_data_handler_;
	std::thread work_thread_;
	struct sockaddr_in server_addr_;
	int server_sock_fd_;
};

class UDPSipServer : public UDPServerHandler {
public:
	UDPSipServer();
	~UDPSipServer();
	void start(void) override;
	void close(void) override;
};

class UDPRtpServer : public UDPServerHandler {
public:
	UDPRtpServer();
	~UDPRtpServer();
	void start(void) override;
	void close(void) override;
};

class UDPMsgServer : public UDPServerHandler {
public:
	UDPMsgServer();
	~UDPMsgServer();
	void start(void) override;
	void close(void) override;
};

#endif
#ifndef TCP_BUFFER_H
#define TCP_BUFFER_H

#include <memory>

constexpr const char* CRLF = "\r\n";

class TCPBuffer {
public:
	TCPBuffer(void);
	~TCPBuffer(void);
	void buffer_append(const void* data, int size);
	void buffer_append_string(const char* data);
	void buffer_append_char(char data);
	int buffer_socket_read(int fd);
	char buffer_read_char(void);
	char* buffer_find_CRLF(void);
	int buffer_readable_size(void);
	void make_room(int size);
	int buffer_writeable_size(void);
	int buffer_front_spare_size(void);

	char* data_;
	int total_size_;
	int write_index_;
	int read_index_;
};

typedef std::shared_ptr<TCPBuffer> TCPBufferSp;

#endif

#include <new>		  //for  std::nothrow
#include <stdexcept>
#include <exception>
#include <sys/uio.h>  //for readv
#include <cstring>    //for memcpy
#include "TCPBuffer.h"

constexpr int buffer_init_size = 65536;

TCPBuffer::TCPBuffer(void) {
	data_ = new char[buffer_init_size];
	total_size_ = buffer_init_size;
	read_index_ = 0;
	write_index_ = 0;
}

TCPBuffer::~TCPBuffer(void) {
	if (data_) {
		delete[] data_;
	}
}

void TCPBuffer::buffer_append(const void* data, int size) {
	if (data != nullptr) {
		make_room(size);
		memcpy(data_ + write_index_, data, size);
		write_index_ += size;
	}
}

void TCPBuffer::buffer_append_string(const char* data) {
	if (data != nullptr) {
		int size = strlen(data);
		buffer_append(data, size);
	}
}

void TCPBuffer::buffer_append_char(char data) {
	make_room(1);
	data_[write_index_++] = data;
}

int TCPBuffer::buffer_socket_read(int fd) {
	char additional_buffer[buffer_init_size];
	struct iovec vec[2];
	int max_writable = buffer_writeable_size();
	vec[0].iov_base = data_ + write_index_;
	vec[0].iov_len = max_writable;
	vec[1].iov_base = additional_buffer;
	vec[1].iov_len = sizeof(additional_buffer);
	int result = readv(fd, vec, 2);
	if (result < 0) {
		return -1;
	} else if (result <= max_writable) {
		write_index_ += result;
	} else {
		write_index_ = total_size_;
		buffer_append(additional_buffer, result - max_writable);
	}
	return result;
}

char TCPBuffer::buffer_read_char(void) {
	char c = data_[read_index_];
	read_index_++;
	return c;
}

char* TCPBuffer::buffer_find_CRLF(void) {
	char *crlf = (char*)memmem(data_ + read_index_, buffer_readable_size(), CRLF, 2);
	return crlf;
}

void TCPBuffer::make_room(int size) {
	if (buffer_writeable_size() >= size) {
		return;
	}
	if (buffer_front_spare_size() + buffer_writeable_size() >= size) {
		int readable = buffer_readable_size();
		int i;
		for (i = 0; i < readable; i++) {
			memcpy(data_ + i, data_ + read_index_ + i, 1);
		}
		read_index_ = 0;
		write_index_ = readable;
	}
	else {
		void *tmp = realloc(data_, total_size_ + size);
		if (tmp == nullptr) {
			return;
		}
		data_ = (char*)tmp;
		total_size_ += size;
	}
}

int TCPBuffer::buffer_writeable_size(void) {
	return total_size_ - write_index_;
}

int TCPBuffer::buffer_readable_size(void) {
	return write_index_ - read_index_;
}

int TCPBuffer::buffer_front_spare_size(void) {
	return read_index_;
}
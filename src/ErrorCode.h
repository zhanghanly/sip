#ifndef ERROR_CODE_H
#define ERROR_CODE_H

#include <string>

//c++11 style
enum class ErrorCode {
	NO_ERROR,
	RECV_RTP_VIDEO_ERROR,
	RECV_RTP_AUDIO_ERROR,
	TIMEOUT_ERROR,
	PUSH_STREAM_ERROR,
	NO_DISK_SPACE_AVAILABLE,
	BAD_ADDRESS,
	DECODE_RTP_ERROR,
	CONNECT_ERROR
};

std::string err2str(ErrorCode);


#endif
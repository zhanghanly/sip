#include "ErrorCode.h"


std::string err2str(ErrorCode errCode) {
	switch(errCode){
		case ErrorCode::NO_ERROR:
			return "ok";
		case ErrorCode::PUSH_STREAM_ERROR:
			return "push stream error";
		case ErrorCode::RECV_RTP_AUDIO_ERROR:
			return "recv rtp audio error";
		case ErrorCode::RECV_RTP_VIDEO_ERROR:
			return "recv rtp video error";
		case ErrorCode::TIMEOUT_ERROR:
			return "timeout error";
		case ErrorCode::NO_DISK_SPACE_AVAILABLE:
			return "no disk space available";
		case ErrorCode::BAD_ADDRESS:
			return "bad address";
		case ErrorCode::DECODE_RTP_ERROR:
			return "decode rtp error";
		case ErrorCode::CONNECT_ERROR:
		 	return "connect error";

		default:
			return "unknown type error";

	}
}
#include "HttpRequest.h"
#include "PublicFunc.h"

#define INIT_REQUEST_HEADER_SIZE 128

constexpr const char* HTTP10 = "HTTP/1.0";
constexpr const char* HTTP11 = "HTTP/1.1";
constexpr const char* KEEP_ALIVE = "Keep-Alive";
constexpr const char* CLOSE = "close";
 
HttpRequest::HttpRequest(void) : current_state(HttpRequestState::REQUEST_STATUS),
								 request_headers_number(0) {}

void HttpRequest::http_request_add_header(const std::string& key, 
										  const std::string& value) {
	request_headers[key] = value;
}

std::string HttpRequest::http_request_get_header(const std::string& rhs) {
	return request_headers[rhs];
}

int HttpRequest::http_request_close_connection(void) {
	std::string value = http_request_get_header("Connection");

	if (value.c_str() != nullptr && strncmp(value.c_str(), CLOSE, strlen(CLOSE)) == 0) {
		return 1;
	}
	if (version.size() != 0 &&
		strncmp(version.c_str(), HTTP10, strlen(HTTP10)) == 0 &&
		strncmp(value.c_str(), KEEP_ALIVE, strlen(KEEP_ALIVE)) == 1) {
		return 1;
	}
	return 0;
}

void HttpRequest::http_request_reset(void) {
	current_state = HttpRequestState::REQUEST_STATUS;
	request_headers.clear();
	request_headers_number = 0;
}

void HttpRequest::parse_url(void) {
	/*eg: /api/sip/deviceRecord/info?deviceid=34020000001320000105&channelid=34020000001320000105*/
	std::vector<std::string> vec = string_split(url, "?");
	if (vec.size() != 2) {
		return;
	}
	vec = string_split(vec[1], "&");
	for (auto& item : vec) {
		std::vector<std::string> res = string_split(item, "=");
		if (res.size() != 2) {
			continue;
		}
		request_parameters[res[0]] = res[1];
	}
}

bool HttpRequest::parameter_exist(const std::string& key) {
	return request_parameters.find(key) != request_parameters.end();
}

std::string HttpRequest::fetch_parameter_value(const std::string& key) {
	if (request_parameters.find(key) != request_parameters.end()) {
		return request_parameters[key];
	} 
	return "";
}
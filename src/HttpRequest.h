#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <map>
#include <cstdint>
#include <ctime>
#include <memory>
#include <cstring>

enum class HttpRequestState {
	REQUEST_STATUS,
	REQUEST_HEADERS,
	REQUEST_BODY,
	REQUEST_DONE
};

enum class HttpRequestType {
	STATIC_REQUEST,
	HLS_INDEX_REQUEST,
	HLS_SLICE_REQUEST,
	FLV_REQUEST,
	UNKNOWN_REQUEST
}; 

class HttpRequest {
public:
	HttpRequest(void);
	std::string http_request_get_header(const std::string&);
	void http_request_add_header(const std::string&, const std::string&);
	int http_request_close_connection(void);
	void http_request_reset(void);
	void parse_url(void);
	bool parameter_exist(const std::string&);
	std::string fetch_parameter_value(const std::string&);

	std::string version;
	std::string method;
	std::string url;
	HttpRequestState current_state;
	std::map<std::string, std::string> request_headers;
	std::map<std::string, std::string> request_parameters;
	int request_headers_number;
};

typedef std::shared_ptr<HttpRequest> HttpRequestSp;

#endif
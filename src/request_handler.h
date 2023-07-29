#ifndef HTTP_REQUEST_HANDLER_H
#define HTTP_REQUEST_HANDLER_H

#include <string>

namespace http {
namespace server {

struct reply;
struct request;

/// The common handler for all incoming requests.
class request_handler
{
public:
  request_handler() = default;
  request_handler(const request_handler&) = delete;
  request_handler& operator=(const request_handler&) = delete;

  /// Construct with a directory containing files to be served.
  explicit request_handler(const std::string& doc_root);

  /// Handle a request and produce a reply.
  void handle_request(const request& req, reply& rep);

private:
  /// Perform URL-decoding on a string. Returns false if the encoding was
  /// invalid.
  static bool url_decode(const std::string& in, std::string& out);
};

// srs http callback, srs-monitor forward srs-live use
struct LiveSrsHttpCallback {
  std::string action;
  std::string client_id;
  std::string ip;
  std::string vhost;
  // app equal enterpriseid
  std::string app;
  // stream equal cameraid
  std::string stream;
};

} // namespace server
} // namespace http

#endif // HTTP_REQUEST_HANDLER_HPP

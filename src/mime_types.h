#ifndef HTTP_MIME_TYPES_H
#define HTTP_MIME_TYPES_H

#include <string>

namespace http {
namespace server {
namespace mime_types {

/// Convert a file extension into a MIME type.
std::string extension_to_type(const std::string& extension);

} // namespace mime_types
} // namespace server
} // namespace http

#endif // HTTP_MIME_TYPES_HPP

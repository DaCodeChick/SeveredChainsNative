#include "util/status.hpp"

#include <string_view>

namespace sc
{
std::string_view ToString(Status status)
{
  switch (status)
  {
    case Status::Ok:
      return "Ok";
    case Status::Error:
      return "Error";
    case Status::InvalidArgument:
      return "InvalidArgument";
    case Status::NotFound:
      return "NotFound";
    case Status::NotSupported:
      return "NotSupported";
    case Status::OutOfMemory:
      return "OutOfMemory";
    case Status::Timeout:
      return "Timeout";
    case Status::AlreadyExists:
      return "AlreadyExists";
    case Status::WouldBlock:
      return "WouldBlock";
  }
  return "Unknown";
}

} // namespace sc
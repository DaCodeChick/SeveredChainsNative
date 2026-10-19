#include <psx/core/error.hpp>

#include <string_view>

namespace psx
{
std::string_view NameOf(Error error)
{
  switch (error)
  {
    case Error::Ok:
      return "Ok";
    case Error::Failed:
      return "Failed";
    case Error::InvalidArgument:
      return "InvalidArgument";
    case Error::NotSupported:
      return "NotSupported";
    case Error::OutOfMemory:
      return "OutOfMemory";
    case Error::IoOpenFailed:
      return "IoOpenFailed";
    case Error::IoReadFailed:
      return "IoReadFailed";
    case Error::DecodeMalformed:
      return "DecodeMalformed";
    case Error::DecodeTruncated:
      return "DecodeTruncated";
  }
  return "Unknown";
}

} // namespace psx
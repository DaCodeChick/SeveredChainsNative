#include <psx/core/version.hpp>

#include <format>
#include <string>
#include <string_view>

namespace psx
{
std::string_view VersionString()
{
  static const std::string version = std::format("{}.{}.{}", kMajorVersion, kMinorVersion, kPatchVersion);
  return version;
}

} // namespace psx
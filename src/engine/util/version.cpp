#include "util/version.hpp"

#include <string_view>

#ifndef LOD_VERSION
#  define LOD_VERSION "0.0.0"
#endif
#ifndef LOD_COMMIT_HASH
#  define LOD_COMMIT_HASH "unknown"
#endif
#ifndef LOD_BUILD_TIMESTAMP
#  define LOD_BUILD_TIMESTAMP "unknown"
#endif
#ifndef LOD_CHANNEL
#  define LOD_CHANNEL "development"
#endif

namespace lod
{
namespace
{
constexpr VersionInfo kVersionInfo{LOD_VERSION, LOD_COMMIT_HASH, LOD_BUILD_TIMESTAMP, LOD_CHANNEL};
} // namespace

const VersionInfo& GetVersionInfo()
{
  return kVersionInfo;
}

bool IsReleaseChannel()
{
  return GetVersionInfo().channel == "release";
}

} // namespace lod
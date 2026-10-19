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
const VersionInfo& GetVersionInfo()
{
  // Process-lifetime instance (constant after construction).
  static const VersionInfo info{LOD_VERSION, LOD_COMMIT_HASH, LOD_BUILD_TIMESTAMP, LOD_CHANNEL};
  return info;
}

bool IsReleaseChannel()
{
  return GetVersionInfo().channel == "release";
}

} // namespace lod
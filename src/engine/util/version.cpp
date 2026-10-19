#include "util/version.hpp"

#include <string_view>

#ifndef SC_VERSION
#  define SC_VERSION "0.0.0"
#endif
#ifndef SC_COMMIT_HASH
#  define SC_COMMIT_HASH "unknown"
#endif
#ifndef SC_BUILD_TIMESTAMP
#  define SC_BUILD_TIMESTAMP "unknown"
#endif
#ifndef SC_CHANNEL
#  define SC_CHANNEL "development"
#endif

namespace sc::util
{
namespace
{
constexpr VersionInfo kVersionInfo{SC_VERSION, SC_COMMIT_HASH, SC_BUILD_TIMESTAMP, SC_CHANNEL};
} // namespace

const VersionInfo& GetVersionInfo()
{
  return kVersionInfo;
}

bool IsReleaseChannel()
{
  return GetVersionInfo().channel == "release";
}

} // namespace sc::util
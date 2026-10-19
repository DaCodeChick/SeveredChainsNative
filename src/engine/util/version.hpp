#pragma once

#include <string_view>

namespace lod
{
/// Build-time version information (build-system.md §5: injected by CMake —
/// no `__DATE__`/`__TIME__` in code).
struct VersionInfo
{
  std::string_view version;        ///< Semantic version (e.g. "0.1.0").
  std::string_view commit;         ///< Git commit hash (short) or "unknown".
  std::string_view buildTimestamp; ///< ISO 8601 UTC configure time.
  std::string_view channel;        ///< "development" or "release" (roadmap M1
                                   ///< Version release-channel gating).
};

/// The build's version info (compile-time injected).
///
/// @return A reference to a process-lifetime struct; safe to store.
[[nodiscard]] const VersionInfo& GetVersionInfo();

/// Release-channel gating (true only for release builds).
///
/// @return true when built in the release channel.
[[nodiscard]] bool IsReleaseChannel();

} // namespace lod
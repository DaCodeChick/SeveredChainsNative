#pragma once

#include <string_view>

namespace psx
{
/// Library major version (incremented for ABI-breaking changes).
inline constexpr int kMajorVersion = 0;

/// Library minor version (incremented for additive changes).
inline constexpr int kMinorVersion = 1;

/// Library patch version (incremented for bug fixes).
inline constexpr int kPatchVersion = 0;

/// The library version as a stable "MAJOR.MINOR.PATCH" string.
///
/// @return A `std::string_view` backed by a process-lifetime static string
///         (e.g. "0.1.0"); safe to store.
[[nodiscard]] std::string_view VersionString();

} // namespace psx
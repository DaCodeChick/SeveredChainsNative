#pragma once

#include <string_view>

namespace psx
{
/// Library major version (incremented for ABI-breaking changes).
inline constexpr int MajorVersion = 0;

/// Library minor version (incremented for additive changes).
inline constexpr int MinorVersion = 1;

/// Library patch version (incremented for bug fixes).
inline constexpr int PatchVersion = 0;

/// The library version as a stable "MAJOR.MINOR.PATCH" string.
///
/// @return A `std::string_view` backed by a process-lifetime static string
///         (e.g. "0.1.0"); safe to store.
[[nodiscard]] std::string_view VersionString();

} // namespace psx
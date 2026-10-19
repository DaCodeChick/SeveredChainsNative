#pragma once

#include <cstdint>
#include <string_view>

namespace sc
{
/// Engine-wide status codes (error-handling.md §2).
///
/// Stable, documented, safe across the plugin ABI. Domain ranges: 100s
/// io/asset, 200s decode, 300s gpu, 400s audio, 500s save, 600s mod, 700s
/// script, 800s plugin.
enum class Status : std::int32_t
{
  Ok = 0,
  /// Unspecified error (see diagnostics for details).
  Error,
  /// An argument was outside its documented domain.
  InvalidArgument,
  /// A file/asset/handle was not found.
  NotFound,
  /// Malformed-format-but-recognized / missing feature.
  NotSupported,
  /// A buffer allocation failed.
  OutOfMemory,
  /// An operation did not complete in time.
  Timeout,
  /// The named resource already exists.
  AlreadyExists,
  /// A non-blocking operation would block.
  WouldBlock,
};

/// Human-readable name for a status code (diagnostics).
///
/// @param status The status code.
/// @return A `std::string_view` backed by a process-lifetime static string.
[[nodiscard]] std::string_view ToString(Status status);

} // namespace sc
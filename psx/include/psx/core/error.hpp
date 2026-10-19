#pragma once

#include <cstdint>
#include <string_view>

namespace psx
{
/// psx library error codes (error-handling.md taxonomy; psx-library.md §5).
///
/// Decoders return `std::expected<T, psx::Error>`; each module extends this
/// enum with specific codes in its documented range (malformed-class
/// granularity per testing.md §2.11).
enum class Error : std::int32_t
{
  Ok = 0,
  /// Unspecified failure (see diagnostics for details).
  Failed,
  /// An argument was outside its documented domain.
  InvalidArgument,
  /// The operation exists but cannot be performed in this context.
  NotSupported,
  /// A buffer allocation failed.
  OutOfMemory,
  // io (100s)
  /// A file or stream could not be opened.
  IoOpenFailed = 100,
  /// A file or stream read failed.
  IoReadFailed,
  // decode (200s)
  /// Input is structurally malformed (recognized format, bad content).
  DecodeMalformed = 200,
  /// Input is shorter than the format declares (truncated).
  DecodeTruncated,
};

/// Human-readable name for an error code (diagnostics).
///
/// @param error The error code.
/// @return A `std::string_view` backed by a process-lifetime static string.
[[nodiscard]] std::string_view NameOf(Error error);

} // namespace psx
#pragma once

#include <format>
#include <mutex>
#include <string_view>

namespace sc::util
{
/// Diagnostic log level (parity with the reference's log4j levels).
enum class LogLevel : int
{
  Trace = 0,
  Debug,
  Info,
  Warning,
  Error
};

/// Name of a log level (diagnostics).
///
/// @param level The level.
/// @return A `std::string_view` backed by a process-lifetime static string.
[[nodiscard]] std::string_view NameOf(LogLevel level);

/// Process-wide diagnostic logger (roadmap M1 diagnostics: leveled, tagged
/// "marker", timestamped — line format matches the reference log4j style:
/// `[time] LEVEL [marker] message`).
///
/// All subsystem logging goes through the logger (guidelines §3). Formatted
/// entries take a *literal* format string (`std::format_string`), so a
/// mismatched format is a compile error. Thread-safe: output is serialized
/// by an internal mutex; formatting happens before locking.
class Logger
{
public:
  /// The process logger instance.
  static Logger& Instance();

  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  /// Sets the minimum level that is emitted.
  ///
  /// @param level Minimum level (lower levels are dropped).
  void SetLevel(LogLevel level);

  /// The current minimum level.
  [[nodiscard]] LogLevel Level() const;

  /// Logs a formatted message.
  ///
  /// @param level Message level.
  /// @param tag Subsystem marker (e.g. "window", "app").
  /// @param format A **string-literal** format string (`std::format`
  ///               syntax); compile-checked against `args`.
  /// @param args Format arguments.
  template <typename... Args>
  void Log(LogLevel level, std::string_view tag, std::format_string<Args...> format, Args&&... args)
  {
    LogMessage(level, tag, std::format(format, std::forward<Args>(args)...));
  }

  /// Logs an already-built message (use when the text is dynamic).
  ///
  /// @param level Message level.
  /// @param tag Subsystem marker.
  /// @param message The message text.
  void LogMessage(LogLevel level, std::string_view tag, std::string_view message);

private:
  Logger() = default;
  mutable std::mutex mutex_; // mutable: internal synchronization (const queries lock it)
  LogLevel level_ = LogLevel::Info;
};

} // namespace sc::util
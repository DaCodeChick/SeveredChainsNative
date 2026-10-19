#pragma once

#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>

namespace lod
{
/// Diagnostic log level.
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
/// "marker", timestamped — line format `[   1234ms] INFO  [marker] message`).
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

  /// Attaches a log file sink (append mode; parity: the reference's
  /// `debug.log`). Lines go to console **and** the file.
  ///
  /// @param file The log file path (created/truncated-on-first-use).
  /// @note Thread: Main thread (call before logging starts).
  void AttachFile(const std::filesystem::path& file);

  /// Detaches the log file sink.
  /// @note Thread: Main thread.
  void DetachFile();

  /// The attached log file path (empty when none) — the crash handler
  /// appends its report here.
  [[nodiscard]] std::string_view LogFilePath() const;

  /// Appends raw pre-formatted text to the log file only (the crash
  /// handler's crash report; console output uses the normal levels).
  ///
  /// @param text The text (already formatted, newline-terminated lines).
  /// @note Thread: Crash-handler safe only via the raw fd path (see
  ///         `crash.cpp`); regular calls are mutex-serialized.
  void AppendFileText(std::string_view text);

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

  /// Zero-argument overload (preferred by overload resolution for argument-less
  /// calls). A plain `string_view` avoids instantiating the empty variadic
  /// pack — Apple Clang 21 + libc++ miscompile that instantiation inside
  /// `std::function`'s immediate-call check (false "not a constant
  /// expression" when a lambda body logs with no format arguments).
  void Log(LogLevel level, std::string_view tag, std::string_view message) { LogMessage(level, tag, message); }

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
  std::string logFile_;
  std::ofstream logStream_;
};

} // namespace lod
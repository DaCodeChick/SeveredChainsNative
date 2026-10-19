#include "util/log.hpp"

#include <chrono>
#include <format>
#include <print>
#include <string>
#include <string_view>

namespace lod
{
namespace
{
/// Monotonic origin for log timestamps (milliseconds since first use).
const std::chrono::steady_clock::time_point Origin = std::chrono::steady_clock::now();
} // namespace

std::string_view NameOf(LogLevel level)
{
  switch (level)
  {
    case LogLevel::Trace:
      return "TRACE";
    case LogLevel::Debug:
      return "DEBUG";
    case LogLevel::Info:
      return "INFO";
    case LogLevel::Warning:
      return "WARN";
    case LogLevel::Error:
      return "ERROR";
  }
  return "?????";
}

Logger& Logger::Instance()
{
  static Logger instance;
  return instance;
}

void Logger::SetLevel(LogLevel level)
{
  std::scoped_lock lock(mutex_);
  level_ = level;
}

LogLevel Logger::Level() const
{
  std::scoped_lock lock(mutex_);
  return level_;
}

void Logger::AttachFile(const std::filesystem::path& file)
{
  std::scoped_lock lock(mutex_);
  logStream_.close();
  logFile_ = file.string();
  logStream_.open(file, std::ios::out | std::ios::app);
}

void Logger::DetachFile()
{
  std::scoped_lock lock(mutex_);
  logStream_.close();
  logFile_.clear();
}

std::string_view Logger::LogFilePath() const
{
  std::scoped_lock lock(mutex_);
  return logFile_;
}

void Logger::AppendFileText(std::string_view text)
{
  std::scoped_lock lock(mutex_);
  if (logStream_.is_open())
  {
    logStream_ << text;
    logStream_.flush();
  }
}

void Logger::LogMessage(LogLevel level, std::string_view tag, std::string_view message)
{
  if (level < Level())
  {
    return;
  }
  const auto now = std::chrono::steady_clock::now();
  const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - Origin).count();
  // Line format: [   1234ms] INFO  [tag] message.
  const std::string line = std::format("[{:>7}ms] {:<5} [{}] {}\n", elapsedMs, NameOf(level), tag, message);
  std::print("{}", line);
  AppendFileText(line);
}

} // namespace lod
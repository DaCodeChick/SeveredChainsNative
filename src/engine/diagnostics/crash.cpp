#include "diagnostics/crash.hpp"

#include "util/log.hpp"
#include "util/version.hpp"

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#if defined(__linux__) || defined(__APPLE__)
#  include <execinfo.h>
#  include <fcntl.h>
#  include <time.h>
#  include <unistd.h>

#  include <csignal>
#  include <sys/stat.h>
#  define LOD_CRASH_POSIX 1
#elif defined(_WIN32)
#  include <windows.h>
#  define LOD_CRASH_WINDOWS 1
#endif

namespace lod
{
namespace
{
constexpr const char* kSeparator = "----------------------------------------------------------------------------\n";
constexpr const char* kDiscordLine1 =
  "Please copy this crash log and send it to us in the Player Help channel in the Legend of Dragoon Discord "
  "server.\n";
constexpr const char* kDiscordLine2 = "https://discord.gg/legendofdragoon\n";
constexpr const char* kStackHeader = "Stack trace:\n";
constexpr int kMaxBacktraceFrames = 64;

/// The version line (compile-time literals only — async-signal-safe; the
/// macros are injected by CMake as string literals, build-system.md §5).
constexpr const char* kVersionLine =
  "Severed Chains " LOD_VERSION " commit " LOD_COMMIT_HASH " built " LOD_BUILD_TIMESTAMP "\n";
// (The raw string concatenation keeps this a single literal — no runtime
// allocation in the signal path.)
// NOLINTBEGIN(readability-identifier-naming) — crash-handler locals use the
// conventional lowercase names of the POSIX APIs they wrap (signalNumber,
// offset, value); the PascalCase convention is relaxed here because these
// are C-API-shaped parameters, not project-owned identifiers.

/// Writes a string to an fd (async-signal-safe; loops over partial writes).
/// The result is intentionally discarded by callers (a failed write in a
/// crash context must never abort the crash report).
bool WriteFd(int fd, const char* text)
{
  const std::size_t size = std::strlen(text);
  std::size_t written = 0;
  while (written < size)
  {
    const auto result = ::write(fd, text + written, size - written);
    if (result <= 0)
    {
      return false;
    }
    written += static_cast<std::size_t>(result);
  }
  return true;
}

/// The fixed crash block (parity fields; M1 has no mod system — the
/// loaded-mods sections are the documented empty list).
void WriteCrashBlock(int fd, const char* signalName, int signalNumber, const char* detail)
{
  WriteFd(fd, kSeparator);
  WriteFd(fd, "Crash detected\n");
  WriteFd(fd, kVersionLine);
  WriteFd(fd, "Loaded mods: (none)\n");
  WriteFd(fd, "All mods: (none)\n");
  WriteFd(fd, "\n");
  WriteFd(fd, kDiscordLine1);
  WriteFd(fd, kDiscordLine2);

  // "Message: SIGSEGV (signal 11)" — small fixed stack buffer (signal-safe).
  char message[160];
  int offset = 0;
  const char* prefix = "Message: ";
  while (*prefix != '\0' && offset < static_cast<int>(sizeof(message)) - 2)
  {
    message[offset++] = *prefix++;
  }
  for (const char* p = signalName; *p != '\0' && offset < static_cast<int>(sizeof(message)) - 16; ++p)
  {
    message[offset++] = *p;
  }
  // "(signal N)" — manual integer format (signal-safe).
  if (signalNumber > 0)
  {
    char digits[12];
    int digitsSize = 0;
    int value = signalNumber;
    if (value == 0)
    {
      digits[digitsSize++] = '0';
    }
    while (value > 0)
    {
      digits[digitsSize++] = static_cast<char>('0' + value % 10);
      value /= 10;
    }
    message[offset++] = '(';
    message[offset++] = 's';
    message[offset++] = 'i';
    message[offset++] = 'g';
    message[offset++] = 'n';
    message[offset++] = 'a';
    message[offset++] = 'l';
    message[offset++] = ' ';
    for (int i = digitsSize - 1; i >= 0; --i)
    {
      message[offset++] = digits[i];
    }
    message[offset++] = ')';
  }
  message[offset] = '\0';
  WriteFd(fd, message);
  if (detail[0] != '\0')
  {
    WriteFd(fd, "\n");
    WriteFd(fd, detail);
  }
  WriteFd(fd, "\n");
  WriteFd(fd, kSeparator);
  WriteFd(fd, kStackHeader);

  // Best-effort backtrace (not strictly async-signal-safe; the standard
  // approach — a failed backtrace must never mask the report).
#if defined(__linux__)
  {
    void* frames[kMaxBacktraceFrames];
    const int count = backtrace(frames, kMaxBacktraceFrames);
    if (count > 0)
    {
      backtrace_symbols_fd(frames, count, fd);
    }
  }
#elif defined(__APPLE__)
  {
    void* frames[kMaxBacktraceFrames];
    const int count = backtrace(frames, kMaxBacktraceFrames);
    char** symbols = backtrace_symbols(frames, count);
    for (int i = 0; i < count; ++i)
    {
      if (symbols != nullptr)
      {
        WriteFd(fd, symbols[i]);
      }
      else
      {
        WriteFd(fd, "<frame>");
      }
      WriteFd(fd, "\n");
    }
  }
#endif
  WriteFd(fd, kDiscordLine1);
  WriteFd(fd, kDiscordLine2);
}

/// Builds the CRASHLOG file name (UTC epoch digits; the reference uses a
/// LocalDateTime stamp — the native format is documented as CRASHLOG-<utc>.log).
void CrashLogName(char* out, std::size_t size)
{
  const time_t now = time(nullptr);
  std::size_t offset = 0;
  out[offset++] = 'C';
  out[offset++] = 'R';
  out[offset++] = 'A';
  out[offset++] = 'S';
  out[offset++] = 'H';
  out[offset++] = 'L';
  out[offset++] = 'O';
  out[offset++] = 'G';
  out[offset++] = '-';
  // Manual epoch-digit format (signal-safe; strftime is not).
  char digits[24];
  int digitsSize = 0;
  unsigned long long value = static_cast<unsigned long long>(now);
  if (value == 0)
  {
    digits[digitsSize++] = '0';
  }
  while (value > 0)
  {
    digits[digitsSize++] = static_cast<char>('0' + static_cast<int>(value % 10));
    value /= 10;
  }
  for (int i = digitsSize - 1; i >= 0 && offset < size - 5; --i)
  {
    out[offset++] = digits[i];
  }
  out[offset++] = '.';
  out[offset++] = 'l';
  out[offset++] = 'o';
  out[offset++] = 'g';
  out[offset] = '\0';
}

#if defined(LOD_CRASH_POSIX)

/// The debug.log fd kept open by Install for the handler (append mode).
int gDebugLogFd = -1;
bool gInstalled = false;

void CrashSignalHandler(int signalNumber, siginfo_t* /*info*/, void* /*context*/)
{
  // The signal-name table is platform-ordered (macOS has SIGEMT at 7 and
  // SIGBUS at 10; Linux has no EMT — index = signal number - 1).
#  if defined(__APPLE__)
  static const char* kNames[] = {
    "SIGHUP",  "SIGINT",   "SIGQUIT", "SIGILL",  "SIGTRAP", "SIGABRT",   "SIGEMT",   "SIGFPE",   "SIGKILL",
    "SIGBUS",  "SIGSEGV",  "SIGSYS",  "SIGPIPE", "SIGALRM", "SIGTERM",   "SIGUSR1",  "SIGUSR2",  "SIGCHLD",
    "SIGCONT", "SIGSTOP",  "SIGTSTP", "SIGTTIN", "SIGTTOU", "SIGIO",     "SIGXCPU",  "SIGXFSZ",  "SIGVTALRM",
    "SIGPROF", "SIGWINCH", "SIGINFO", "SIGUSR1", "SIGUSR2", "SIGTHREAD", "SIGFIDEL", "SIGRTMIN", "SIGRTMAX"};
#  else
  static const char* kNames[] = {
    "SIGHUP",  "SIGINT",  "SIGQUIT", "SIGILL",  "SIGTRAP", "SIGABRT",   "SIGBUS",  "SIGFPE",   "SIGKILL", "SIGSEGV",
    "SIGSYS",  "SIGPIPE", "SIGALRM", "SIGTERM", "SIGUSR1", "SIGUSR2",   "SIGCHLD", "SIGCONT",  "SIGSTOP", "SIGTSTP",
    "SIGTTIN", "SIGTTOU", "SIGIO",   "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGINFO", "SIGUSR2"};
#  endif
  const char* name = "SIGNAL";
  if (signalNumber > 0 && signalNumber <= 31)
  {
    name = kNames[signalNumber - 1];
  }

  // 1) Timestamped CRASHLOG file.
  char path[64];
  CrashLogName(path, sizeof(path));
  const int crashFd = ::open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (crashFd >= 0)
  {
    WriteCrashBlock(crashFd, name, signalNumber, "");
    ::close(crashFd);
  }
  // 2) Append the same block to debug.log (parity: the reference's crash log
  //    is a copy of the application log).
  if (gDebugLogFd >= 0)
  {
    WriteCrashBlock(gDebugLogFd, name, signalNumber, "");
  }
  // 3) Stderr.
  WriteCrashBlock(2, name, signalNumber, "");
  ::_exit(1); // Parity: System.exit(1) after the crash log.
}

#endif // LOD_CRASH_POSIX

} // namespace

void CrashHandler::Install(std::string_view logFile)
{
#if defined(LOD_CRASH_POSIX)
  if (gInstalled)
  {
    return;
  }
  // Keep debug.log open for the handler (append; created if absent).
  if (!logFile.empty())
  {
    const std::string logPath(logFile);
    gDebugLogFd = ::open(logPath.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
  }
  for (const int signal : {SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT})
  {
    struct sigaction action{};
    action.sa_sigaction = &CrashSignalHandler;
    action.sa_flags = SA_SIGINFO | SA_RESETHAND; // default behavior after one crash
    sigemptyset(&action.sa_mask);
    sigaction(signal, &action, nullptr);
  }
  gInstalled = true;
#endif
}

bool CrashHandler::Installed()
{
#if defined(LOD_CRASH_POSIX)
  return gInstalled;
#else
  return false;
#endif
}

std::string CrashHandler::WriteCrashReport(std::string_view signalName, std::string_view detail)
{
  // Non-signal path (tests / diagnostics): build the report, append to
  // debug.log (when the logger has a file sink), write the CRASHLOG file.
  std::string report;
  report += kSeparator;
  report += "Crash detected\n";
  const auto& version = GetVersionInfo();
  report += "Severed Chains " + std::string(version.version) + " commit " + std::string(version.commit) + " built " +
            std::string(version.buildTimestamp) + "\n";
  report += "Loaded mods: (none)\n";
  report += "All mods: (none)\n";
  report += "\n";
  report += kDiscordLine1;
  report += kDiscordLine2;
  report += "Message: " + std::string(signalName);
  if (!detail.empty())
  {
    report += "\n" + std::string(detail);
  }
  report += "\n";
  report += kSeparator;
  report += kStackHeader;
#if defined(LOD_CRASH_POSIX)
  void* frames[kMaxBacktraceFrames];
  const int count = backtrace(frames, kMaxBacktraceFrames);
  char** symbols = backtrace_symbols(frames, count);
  for (int i = 0; i < count; ++i)
  {
    report += symbols != nullptr ? symbols[i] : "<frame>";
    report += "\n";
  }
#endif
  report += kDiscordLine1;
  report += kDiscordLine2;

  Logger::Instance().AppendFileText(report);

  char path[64];
  CrashLogName(path, sizeof(path));
#if defined(_WIN32)
  // Windows (M1: best effort — no Windows runner smoke yet): CreateFile/WriteFile.
  const HANDLE handle = CreateFileA(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (handle == INVALID_HANDLE_VALUE)
  {
    return {};
  }
  DWORD written = 0;
  WriteFile(handle, report.data(), static_cast<DWORD>(report.size()), &written, nullptr);
  CloseHandle(handle);
  return std::string(path);
#else
  // Reuse the fd path (the file is small; one write loop).
  const int fd = ::open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0)
  {
    return {};
  }
  std::size_t written = 0;
  while (written < report.size())
  {
    const auto result = ::write(fd, report.data() + written, report.size() - written);
    if (result <= 0)
    {
      break;
    }
    written += static_cast<std::size_t>(result);
  }
  ::close(fd);
  return std::string(path);
#endif
}

} // namespace lod

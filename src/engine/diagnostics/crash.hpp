#pragma once

#include <filesystem>
#include <string_view>

namespace lod
{
/// Crash handler + crash log (roadmap M1 diagnostics; parity: the
/// reference `Main.main` top-level crash path — a crash block with
/// version/commit/timestamp + loaded-mods sections (empty in M1) +
/// exception message + stack trace, written to `debug.log` and copied to a
/// timestamped `CRASHLOG-<utc>.log`, exit code 1).
///
/// The crash report layout (the "byte-compatible fields"):
/// ```
/// ----------------------------------------------------------------------------
/// Crash detected
/// Severed Chains <version> commit <commit> built <timestamp>
/// Loaded mods: (none)
/// All mods: (none)
///
/// Please copy this crash log and send it to us in the Player Help channel
/// in the Legend of Dragoon Discord server.
/// https://discord.gg/legendofdragoon
/// Message: <signal name> (signal <N>)
/// ----------------------------------------------------------------------------
/// Stack trace:
/// <backtrace — best effort>
/// Please copy this crash log and send it to us in the Player Help channel
/// in the Legend of Dragoon Discord server.
/// https://discord.gg/legendofdragoon
/// ```
///
/// POSIX: async-signal-safe `write()`-only handler (no locks, no heap, no
/// `std` I/O); the version/commit/timestamp come from compile-time macros so
/// no dynamic memory is touched. The backtrace is best effort (backtrace()
/// is not strictly async-signal-safe but is the standard approach).
/// Windows: `SetUnhandledExceptionFilter` (best effort; not exercised in M1
/// CI — no Windows runner smoke yet).
class CrashHandler
{
public:
  /// Installs crash handling. Idempotent.
  ///
  /// @param logFile The `debug.log` path the crash report is appended to
  ///                ("" = no file sink, stderr + CRASHLOG only).
  /// @note Thread: **Main thread only** — install before any other subsystem
  ///         (SDL init in the app).
  static void Install(std::string_view logFile = {});

  /// Whether a handler is installed.
  [[nodiscard]] static bool Installed();

  /// Writes a crash report (used by the signal/SEH handler; also exposed
  /// for tests — `--crash-test` style injected failures).
  ///
  /// @param signalName The signal/exception name (e.g. "SIGSEGV").
  /// @param detail One line of detail (e.g. the faulting address).
  /// @return The `CRASHLOG-<utc>.log` path written (empty on failure).
  /// @note Thread: Signal context: POSIX path uses only async-signal-safe calls;
  ///         the returned path is a fixed-size stack buffer — do not store
  ///         the reference after the handler returns.
  [[nodiscard]] static std::string WriteCrashReport(std::string_view signalName, std::string_view detail);
};

} // namespace lod

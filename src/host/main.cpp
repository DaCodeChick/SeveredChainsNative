// severed-chains — thin host (build-system.md §1): argument parsing + App +
// exit code. All behavior lives in scengine.
#include "app/app.hpp"
#include "util/log.hpp"
#include "util/status.hpp"
#include "util/version.hpp"

#include <SDL3/SDL.h>

#include <charconv>
#include <cstdlib>
#include <print>
#include <string_view>
#include <system_error>

namespace
{
void PrintUsage(std::string_view programName)
{
  std::println("usage: {} [options]", programName);
  std::println("  --frames N      auto-quit after N frames (-1: run until quit)");
  std::println("  --headless      use the dummy video driver (CI / smoke runs)");
  std::println("  --log-level L   trace|debug|info|warning|error (default: info)");
  std::println("  --version       print build version info and exit");
  std::println("  --help          show this help");
}

sc::util::LogLevel ParseLogLevel(std::string_view text)
{
  if (text == "trace")
  {
    return sc::util::LogLevel::Trace;
  }
  if (text == "debug")
  {
    return sc::util::LogLevel::Debug;
  }
  if (text == "info")
  {
    return sc::util::LogLevel::Info;
  }
  if (text == "warning")
  {
    return sc::util::LogLevel::Warning;
  }
  if (text == "error")
  {
    return sc::util::LogLevel::Error;
  }
  return sc::util::LogLevel::Info;
}
} // namespace

int main(int argc, char** argv)
{
  sc::app::AppOptions options;
  bool headless = false;

  for (int i = 1; i < argc; ++i)
  {
    const std::string_view arg = argv[i];
    if (arg == "--help")
    {
      PrintUsage(argv[0]);
      return EXIT_SUCCESS;
    }
    if (arg == "--version")
    {
      const auto& version = sc::util::GetVersionInfo();
      std::println("Severed Chains {} (commit {}, built {}, channel {})", version.version, version.commit,
                   version.buildTimestamp, version.channel);
      return EXIT_SUCCESS;
    }
    if (arg == "--headless")
    {
      headless = true;
      continue;
    }
    if (arg == "--log-level")
    {
      if (i + 1 >= argc)
      {
        std::println("error: --log-level requires a value");
        return EXIT_FAILURE;
      }
      options.logLevel = ParseLogLevel(argv[++i]);
      continue;
    }
    if (arg == "--frames")
    {
      if (i + 1 >= argc)
      {
        std::println("error: --frames requires an integer value");
        return EXIT_FAILURE;
      }
      const std::string_view value = argv[++i];
      const auto [end, ec] = std::from_chars(value.data(), value.data() + value.size(), options.frameCount);
      if (ec != std::errc{})
      {
        std::println("error: --frames value is not an integer: '{}'", value);
        return EXIT_FAILURE;
      }
      continue;
    }
    std::println("error: unknown option '{}'", arg);
    PrintUsage(argv[0]);
    return EXIT_FAILURE;
  }

  if (headless)
  {
    // Must be set before SDL_Init (consumed by the video subsystem).
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
  }

  sc::app::App app;
  const sc::Status status = app.Run(options);
  return status == sc::Status::Ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
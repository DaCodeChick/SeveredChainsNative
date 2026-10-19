// Crash handler tests: the crash report contains the parity fields
// (version/commit/timestamp line, empty loaded-mods sections, the Discord
// contact lines, the message line, and the stack-trace section), and
// install is idempotent. Runs in a scratch directory (CRASHLOG files land
// in the current working directory).
#include "lod_test.hpp"

#include <cstdio>
#include <diagnostics/crash.hpp>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <util/version.hpp>

using namespace lod;

namespace
{
std::string ReadFile(const std::filesystem::path& file)
{
  std::ifstream in(file, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}
} // namespace

LOD_TEST(CrashReportFields, "crash")
{
  const auto oldDir = std::filesystem::current_path();
  const auto dir = std::filesystem::temp_directory_path() / "lod-sc-tests" / "crash";
  std::filesystem::create_directories(dir);
  std::filesystem::current_path(dir);
  LOD_REQUIRE(std::filesystem::equivalent(std::filesystem::current_path(), dir));

  const std::string path = CrashHandler::WriteCrashReport("SIGSEGV", "injected fault at 0x0");
  LOD_REQUIRE(!path.empty());
  const auto file = dir / path;
  LOD_REQUIRE(std::filesystem::exists(file));
  const std::string content = ReadFile(file);

  // Parity fields (Main.java crash log layout).
  LOD_REQUIRE(content.find("Crash detected") != std::string::npos);
  const auto& version = GetVersionInfo();
  const std::string versionLine = std::format("Severed Chains {} commit {}", version.version, version.commit);
  LOD_REQUIRE(content.find(versionLine) != std::string::npos);
  LOD_REQUIRE(content.find("Loaded mods: (none)") != std::string::npos);
  LOD_REQUIRE(content.find("All mods: (none)") != std::string::npos);
  LOD_REQUIRE(content.find("https://discord.gg/legendofdragoon") != std::string::npos);
  LOD_REQUIRE(content.find("Message: SIGSEGV") != std::string::npos);
  LOD_REQUIRE(content.find("injected fault at 0x0") != std::string::npos);
  LOD_REQUIRE(content.find("Stack trace:") != std::string::npos);
  // The separator rule.
  LOD_REQUIRE(content.find("----------------------------------------------------------------------------") !=
              std::string::npos);

  // Idempotent install.
  CrashHandler::Install();
  CrashHandler::Install();

  std::filesystem::current_path(oldDir);
  return true;
}

LOD_TEST(CrashInstallReports, "crash")
{
  CrashHandler::Install();
  LOD_REQUIRE(CrashHandler::Installed());
  return true;
}
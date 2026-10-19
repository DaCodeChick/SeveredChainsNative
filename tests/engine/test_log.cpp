// Logger tests (M1 diagnostics: levels, tags, type-safe formatting).
#include "lod_test.hpp"

#include <util/log.hpp>

using lod::Logger;
using lod::LogLevel;

LOD_TEST(LogLevelNames, "log")
{
  LOD_REQUIRE(lod::NameOf(LogLevel::Trace) == "TRACE");
  LOD_REQUIRE(lod::NameOf(LogLevel::Debug) == "DEBUG");
  LOD_REQUIRE(lod::NameOf(LogLevel::Info) == "INFO");
  LOD_REQUIRE(lod::NameOf(LogLevel::Warning) == "WARN");
  LOD_REQUIRE(lod::NameOf(LogLevel::Error) == "ERROR");
  return true;
}

LOD_TEST(LoggerLevelFiltering, "log")
{
  Logger::Instance().SetLevel(LogLevel::Warning);
  LOD_REQUIRE(Logger::Instance().Level() == LogLevel::Warning);
  // Emission below the level is dropped without error (side-effect free).
  Logger::Instance().Log(LogLevel::Debug, "test", "dropped message {}", 1);
  Logger::Instance().SetLevel(LogLevel::Info);
  LOD_REQUIRE(Logger::Instance().Level() == LogLevel::Info);
  return true;
}

LOD_TEST(LoggerFormatsMessages, "log")
{
  // The type-safe formatted path: literal format string checked at compile
  // time (guidelines §3); emission is side-effect free on the observable
  // state.
  Logger::Instance().SetLevel(LogLevel::Trace);
  Logger::Instance().Log(LogLevel::Info, "test", "formatted {} and {} with {}", "alpha", 42, 3.5);
  Logger::Instance().LogMessage(LogLevel::Info, "test", std::string_view("prebuilt message"));
  Logger::Instance().SetLevel(LogLevel::Info);
  return true;
}
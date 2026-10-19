// Logger tests (M1 diagnostics: levels, tags, type-safe formatting).
#include "sc_test.hpp"

#include <util/log.hpp>

using sc::util::Logger;
using sc::util::LogLevel;

SC_TEST(LogLevelNames, "log")
{
  SC_REQUIRE(sc::util::NameOf(LogLevel::Trace) == "TRACE");
  SC_REQUIRE(sc::util::NameOf(LogLevel::Debug) == "DEBUG");
  SC_REQUIRE(sc::util::NameOf(LogLevel::Info) == "INFO");
  SC_REQUIRE(sc::util::NameOf(LogLevel::Warning) == "WARN");
  SC_REQUIRE(sc::util::NameOf(LogLevel::Error) == "ERROR");
  return true;
}

SC_TEST(LoggerLevelFiltering, "log")
{
  Logger::Instance().SetLevel(LogLevel::Warning);
  SC_REQUIRE(Logger::Instance().Level() == LogLevel::Warning);
  // Emission below the level is dropped without error (side-effect free).
  Logger::Instance().Log(LogLevel::Debug, "test", "dropped message {}", 1);
  Logger::Instance().SetLevel(LogLevel::Info);
  SC_REQUIRE(Logger::Instance().Level() == LogLevel::Info);
  return true;
}

SC_TEST(LoggerFormatsMessages, "log")
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
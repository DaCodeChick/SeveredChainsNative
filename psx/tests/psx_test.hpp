#pragma once
// psx test harness — self-contained, single header, no dependencies, no
// exceptions (build-system.md §2).
//
// psx is an independent subproject (psx-library.md §1) that may be extracted
// to its own repository, so its test suite does not depend on the host
// repository's test framework. The API shape mirrors
// `tests/framework/sc_test.hpp` so test files port 1:1 between the two
// harnesses.

#include <print>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace psx::test
{
/// A registered test case.
struct Case
{
  std::string name; ///< Unique test name.
  std::string tags; ///< Comma-separated filter tags (or "").
  bool (*fn)();     ///< Test body; returns true when all checks passed.
};

/// Global test-case registry (populated by static initializers).
class Registry
{
public:
  /// The process registry instance.
  static Registry& Instance();

  /// Registers a case (static-initializer call only).
  void Add(std::string name, std::string tags, bool (*fn)());

  /// The registered cases, in registration order.
  const std::vector<Case>& Cases() const;

private:
  Registry() = default;
  std::vector<Case> cases_;
};

inline Registry& Registry::Instance()
{
  static Registry instance;
  return instance;
}

inline void Registry::Add(std::string name, std::string tags, bool (*fn)())
{
  Instance().cases_.push_back(Case{std::move(name), std::move(tags), fn});
}

inline const std::vector<Case>& Registry::Cases() const
{
  return cases_;
}

/// Per-test-run counters.
struct Context
{
  std::string testName; ///< Currently running test name.
  int checks = 0;       ///< Total checks executed so far.
  int failures = 0;     ///< Failed checks so far.
  std::string section;  ///< Current PSX_SECTION label (or "").
};

/// The current test context (single-threaded runner).
inline Context& Current()
{
  static Context instance;
  return instance;
}

/// Records one check; returns the (bool) condition.
inline bool Check(bool condition, std::string_view expr, int line)
{
  Context& ctx = Current();
  ++ctx.checks;
  if (!condition)
  {
    ++ctx.failures;
    const std::string section = ctx.section.empty() ? "" : " " + ctx.section;
    std::print("[  FAILED  ] {} (line {}){} {}\n", ctx.testName, line, section, expr);
  }
  return condition;
}

/// Records an unconditional failure with a message.
inline bool Fail(std::string_view message, int line)
{
  Context& ctx = Current();
  ++ctx.checks;
  ++ctx.failures;
  std::print("[  FAILED  ] {} (line {}) {}\n", ctx.testName, line, message);
  return false;
}

/// Static-initializer registrar (constructed by PSX_TEST).
struct Registrar
{
  Registrar(std::string name, std::string tags, bool (*fn)())
  {
    Registry::Instance().Add(std::move(name), std::move(tags), fn);
  }
};

/// Runs all registered cases (optional substring filter on name or tags).
/// @return 0 when no case failed, 1 otherwise.
inline int RunAll(int argc, char** argv)
{
  std::string filter;
  for (int i = 1; i < argc; ++i)
  {
    filter = argv[i];
  }
  int run = 0;
  int failed = 0;
  for (const Case& c : Registry::Instance().Cases())
  {
    if (!filter.empty() && c.name.find(filter) == std::string::npos && c.tags.find(filter) == std::string::npos)
    {
      continue;
    }
    Context& ctx = Current();
    ctx.testName = c.name;
    ctx.checks = 0;
    ctx.failures = 0;
    ctx.section.clear();
    const bool ok = c.fn();
    ++run;
    if (ok)
    {
      std::print("[       OK ] {} ({} checks)\n", c.name, ctx.checks);
    }
    else
    {
      ++failed;
      std::print("[  FAILED ] {} ({}/{} checks failed)\n", c.name, ctx.failures, ctx.checks);
    }
  }
  std::print("== {} test(s) run, {} failed ==\n", run, failed);
  return failed == 0 ? 0 : 1;
}
} // namespace psx::test

// --- Registration and check macros (guidelines §8.1: the only sanctioned
//     macro use is registration sugar; checks are ordinary function calls) ---

#define PSX_TEST_CONCAT_IMPL(a, b) a##b
#define PSX_TEST_CONCAT(a, b) PSX_TEST_CONCAT_IMPL(a, b)

/// Declares a test case. `Tags` is a comma-separated tag string (or "").
#define PSX_TEST(Name, Tags)                                                                                           \
  static bool PSX_TEST_CONCAT(psx_test_fn_, Name)();                                                                   \
  static ::psx::test::Registrar PSX_TEST_CONCAT(psx_test_reg_, Name){#Name, Tags,                                      \
                                                                     &PSX_TEST_CONCAT(psx_test_fn_, Name)};            \
  static bool PSX_TEST_CONCAT(psx_test_fn_, Name)()

/// Checks that a condition is true; a failure does not stop the test.
#define PSX_CHECK(cond) ::psx::test::Check(static_cast<bool>(cond), #cond, __LINE__)

/// Checks that two values compare equal.
#define PSX_CHECK_EQ(a, b) ::psx::test::Check((a) == (b), #a " == " #b, __LINE__)

/// Checks that two values compare unequal.
#define PSX_CHECK_NE(a, b) ::psx::test::Check((a) != (b), #a " != " #b, __LINE__)

/// Checks that a condition is true; on failure the test stops (returns false).
#define PSX_REQUIRE(cond)                                                                                              \
  do                                                                                                                   \
  {                                                                                                                    \
    if (!::psx::test::Check(static_cast<bool>(cond), #cond, __LINE__))                                                 \
    {                                                                                                                  \
      return false;                                                                                                    \
    }                                                                                                                  \
  } while (false)

/// Labels the current section in failure diagnostics.
#define PSX_SECTION(label)                                                                                             \
  do                                                                                                                   \
  {                                                                                                                    \
    ::psx::test::Current().section = (label);                                                                          \
  } while (false)
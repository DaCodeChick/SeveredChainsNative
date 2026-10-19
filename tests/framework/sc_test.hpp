#pragma once

// Minimal dependency-free test framework (testing.md §1): Catch-like,
// no exceptions — checks are bool-based, a failed REQUIRE ends the test
// case via early return, registration uses static initializers (guidelines
// §8.1: sanctioned registration sugar — the only macro use).

#include <print>
#include <string>
#include <string_view>
#include <vector>

namespace sc::test
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
  [[nodiscard]] const std::vector<Case>& Cases() const;

private:
  Registry() = default;
  std::vector<Case> cases_;
};

/// Per-test-run counters.
struct Context
{
  std::string testName; ///< Currently running test name.
  int checks = 0;       ///< Total checks executed so far.
  int failures = 0;     ///< Failed checks so far.
  std::string section;  ///< Current SC_SECTION label (or "").
};

/// The current test context (single-threaded runner).
Context& Current();

/// Records a check and reports a failure.
///
/// @param condition The evaluated condition.
/// @param expression The source text of the condition (for the report).
/// @param file Captured source file.
/// @param line Captured source line.
/// @return true if the condition held.
[[nodiscard]] bool Check(bool condition, std::string_view expression, std::string_view file, int line);

/// Runs all registered cases (optionally filtered by a name/tag substring).
///
/// @param argc Argument count (argv[1] is an optional filter).
/// @param argv Arguments.
/// @return 0 all passed, 1 at least one failed, 2 no case matched.
int RunAll(int argc, char** argv);

} // namespace sc::test

namespace
{
/// Static-initializer registration (no macro-free spelling stays readable —
/// guidelines §8.1).
struct TestRegistration
{
  TestRegistration(std::string name, std::string tags, bool (*fn)())
  {
    sc::test::Registry::Instance().Add(std::move(name), std::move(tags), fn);
  }
};
} // namespace

#define SC_TEST(name, tags)                                                                                            \
  static bool Test_##name();                                                                                           \
  namespace                                                                                                            \
  {                                                                                                                    \
  const TestRegistration kRegistration_##name{#name, tags, &Test_##name};                                              \
  }                                                                                                                    \
  static bool Test_##name()

#define SC_CHECK(cond) (void)sc::test::Check(static_cast<bool>(cond), #cond, __FILE__, __LINE__)
#define SC_REQUIRE(cond)                                                                                               \
  if (!sc::test::Check(static_cast<bool>(cond), #cond, __FILE__, __LINE__))                                            \
  return false
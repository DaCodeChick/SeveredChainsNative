// Implementation of the test framework (testing.md §1; sc_test.hpp).
#include "sc_test.hpp"

#include <string_view>

namespace sc::test
{
Registry& Registry::Instance()
{
  static Registry registry;
  return registry;
}

void Registry::Add(std::string name, std::string tags, bool (*fn)())
{
  cases_.push_back(Case{std::move(name), std::move(tags), fn});
}

const std::vector<Case>& Registry::Cases() const
{
  return cases_;
}

Context& Current()
{
  static Context context;
  return context;
}

bool Check(bool condition, std::string_view expression, std::string_view file, int line)
{
  Context& context = Current();
  ++context.checks;
  if (!condition)
  {
    ++context.failures;
    std::print("    [check failed] {}:{}: {}\n", file, line, expression);
  }
  return condition;
}

int RunAll(int argc, char** argv)
{
  const std::string_view filter = argc > 1 ? std::string_view(argv[1]) : std::string_view{};
  int runCount = 0;
  int failCount = 0;
  for (const Case& c : Registry::Instance().Cases())
  {
    if (!filter.empty() && std::string_view(c.name).find(filter) == std::string_view::npos &&
        std::string_view(c.tags).find(filter) == std::string_view::npos)
    {
      continue;
    }
    ++runCount;
    Context& context = Current();
    context.testName = c.name;
    context.checks = 0;
    context.failures = 0;
    std::print("[ RUN      ] {}\n", c.name);
    const bool ok = c.fn();
    if (ok)
    {
      std::print("[       OK ] {} ({} checks)\n", c.name, context.checks);
    }
    else
    {
      std::print("[     FAIL ] {} ({} of {} checks failed)\n", c.name, context.failures, context.checks);
      ++failCount;
    }
  }
  std::print("== {} test(s) run, {} failed ==\n", runCount, failCount);
  if (runCount == 0)
  {
    return 2;
  }
  return failCount == 0 ? 0 : 1;
}

} // namespace sc::test
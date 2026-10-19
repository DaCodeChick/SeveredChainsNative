// Scheduler tests (port of the reference fixed-rate Action — the input /
// render pacing primitive): 60Hz period, deadline advancement, rate
// reprogramming, and skip-to-catch-up resync. Uses an injected clock.
#include "lod_test.hpp"

#include <cstdint>
#include <platform/scheduler.hpp>

using lod::ScheduledAction;

namespace
{
constexpr std::int64_t SixtyHzPeriod = 16'666'666;    // 1e9 / 60
constexpr std::int64_t OneTwentyHzPeriod = 8'333'333; // 1e9 / 120
} // namespace

LOD_TEST(SchedulerPeriod, "scheduler")
{
  std::int64_t now = 0;
  int ticks = 0;
  ScheduledAction action([&ticks] { ++ticks; }, 60, [&now] { return now; });
  LOD_REQUIRE(action.ExpectedFps() == 60);
  LOD_REQUIRE(action.NanosPerTick() == SixtyHzPeriod);
  // The constructor advances one period (parity).
  LOD_REQUIRE(!action.IsReady());
  LOD_REQUIRE(action.NanosUntilNextRun() == SixtyHzPeriod);
  return true;
}

LOD_TEST(SchedulerTicks, "scheduler")
{
  std::int64_t now = 0;
  int ticks = 0;
  ScheduledAction action([&ticks] { ++ticks; }, 60, [&now] { return now; });

  // One tick per period at the deadline.
  now = SixtyHzPeriod;
  LOD_REQUIRE(action.IsReady());
  action.Tick();
  LOD_REQUIRE(ticks == 1);
  LOD_REQUIRE(!action.IsReady());
  now = SixtyHzPeriod * 2;
  action.Tick();
  LOD_REQUIRE(ticks == 2);

  // No double tick between deadlines.
  now = SixtyHzPeriod * 2 + 1000;
  action.Tick();
  LOD_REQUIRE(ticks == 2);

  // A few periods ahead: ticks once per call (no burst — parity: the
  // reference ticks at most once per loop iteration; the resync path below
  // handles the multi-period-late case).
  now = SixtyHzPeriod * 3;
  action.Tick();
  LOD_REQUIRE(ticks == 3);
  now = SixtyHzPeriod * 4;
  action.Tick();
  LOD_REQUIRE(ticks == 4);
  return true;
}

LOD_TEST(SchedulerRateChange, "scheduler")
{
  std::int64_t now = 0;
  int ticks = 0;
  ScheduledAction action([&ticks] { ++ticks; }, 60, [&now] { return now; });
  action.SetExpectedFps(120);
  LOD_REQUIRE(action.ExpectedFps() == 120);
  LOD_REQUIRE(action.NanosPerTick() == OneTwentyHzPeriod);
  // The deadline is not reset on reprogram (parity).
  now = SixtyHzPeriod;
  LOD_REQUIRE(action.IsReady());
  action.Tick();
  LOD_REQUIRE(ticks == 1);
  // Next deadline advances by the new period.
  LOD_REQUIRE(!action.IsReady());
  now = SixtyHzPeriod + OneTwentyHzPeriod;
  LOD_REQUIRE(action.IsReady());
  action.Tick();
  LOD_REQUIRE(ticks == 2);
  return true;
}

LOD_TEST(SchedulerCatchUpResync, "scheduler")
{
  std::int64_t now = 0;
  int ticks = 0;
  ScheduledAction action([&ticks] { ++ticks; }, 60, [&now] { return now; });
  // Run one tick, then stall far more than two periods behind.
  now = SixtyHzPeriod;
  action.Tick();
  LOD_REQUIRE(ticks == 1);
  now += 100'000'000LL; // > 2 * SixtyHzPeriod late
  LOD_REQUIRE(action.IsReady());
  action.Tick();
  LOD_REQUIRE(ticks == 2);
  // Resynced: the next deadline is exactly one (new) period from now.
  LOD_REQUIRE(action.NanosUntilNextRun() == SixtyHzPeriod);
  return true;
}

LOD_TEST(SchedulerZeroFpsClamps, "scheduler")
{
  std::int64_t now = 0;
  ScheduledAction action([] {}, 0, [&now] { return now; });
  // 0 is clamped to 1Hz (the period must never be 0/overflow).
  LOD_REQUIRE(action.ExpectedFps() == 1);
  LOD_REQUIRE(action.NanosPerTick() == 1'000'000'000LL);
  return true;
}
#pragma once

#include <cstdint>
#include <functional>

namespace lod
{
/// Fixed-rate scheduler action (port of the reference
/// `legend.core.platform.Action` — the input + render pacing primitive):
/// a tick callback that runs at an integer Hz rate, advancing a fixed
/// nanos-per-tick deadline; if more than two periods are missed it
/// skip-to-catch-ups to the current time instead of bursting (parity: the
/// reference logs "Action running behind, skipping ticks to catch up").
///
/// The clock is injected for testability; the default is the SDL monotonic
/// clock. The rate is reprogrammable per frame by the game layer (parity:
/// the reference reprograms the input/render action rates each frame from
/// `vsyncMode` + `speedMultiplier`).
class ScheduledAction
{
public:
  /// @param tick The callback (run on the main thread at the tick rate).
  /// @param expectedFps The initial rate in Hz (must be > 0; the period is
  ///                    `1'000'000'000 / expectedFps`, integer division —
  ///                    parity).
  /// @param nowNs Monotonic time source (ns); nullptr = `SDL_GetTicksNS`.
  ScheduledAction(std::function<void()> tick, std::int32_t expectedFps, std::function<std::int64_t()> nowNs = nullptr);

  /// Reprograms the rate (parity: `setExpectedFps`).
  ///
  /// @param expectedFps The new rate in Hz (> 0).
  void SetExpectedFps(std::int32_t expectedFps);

  /// The current rate in Hz.
  [[nodiscard]] std::int32_t ExpectedFps() const;

  /// The nanos-per-tick period.
  [[nodiscard]] std::int64_t NanosPerTick() const;

  /// Runs the action if its deadline has passed (updates the deadline first,
  /// with the skip-to-catch-up resync — parity: `tick`).
  void Tick();

  /// Whether the deadline has passed.
  [[nodiscard]] bool IsReady() const;

  /// Nanos until the next run (<= 0 when ready).
  [[nodiscard]] std::int64_t NanosUntilNextRun() const;

private:
  /// Advances the deadline by one period (resyncs when more than two
  /// periods are behind).
  void UpdateTimer();

  std::function<void()> tick_;
  std::int32_t expectedFps_ = 60;
  std::int64_t nanosPerTick_ = 0;
  std::int64_t nextRunTimeNs_ = 0;
  std::function<std::int64_t()> nowNs_;
};

} // namespace lod

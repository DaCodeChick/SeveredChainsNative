#include "platform/scheduler.hpp"

#include <SDL3/SDL.h>

#include <algorithm>

namespace lod
{
namespace
{
[[nodiscard]] std::int64_t SDLNowNs()
{
  return static_cast<std::int64_t>(SDL_GetTicksNS());
}
} // namespace

ScheduledAction::ScheduledAction(std::function<void()> tick, std::int32_t expectedFps,
                                 std::function<std::int64_t()> nowNs)
    : tick_(std::move(tick)), nowNs_(std::move(nowNs))
{
  if (nowNs_ == nullptr)
  {
    nowNs_ = SDLNowNs;
  }
  nextRunTimeNs_ = nowNs_();
  SetExpectedFps(expectedFps);
  UpdateTimer(); // Parity: the constructor advances one period.
}

void ScheduledAction::SetExpectedFps(std::int32_t expectedFps)
{
  expectedFps_ = std::max(expectedFps, 1);
  nanosPerTick_ = 1'000'000'000LL / expectedFps_;
}

std::int32_t ScheduledAction::ExpectedFps() const
{
  return expectedFps_;
}

std::int64_t ScheduledAction::NanosPerTick() const
{
  return nanosPerTick_;
}

void ScheduledAction::Tick()
{
  if (IsReady())
  {
    UpdateTimer();
    if (tick_)
    {
      tick_();
    }
  }
}

bool ScheduledAction::IsReady() const
{
  return NanosUntilNextRun() <= 0;
}

std::int64_t ScheduledAction::NanosUntilNextRun() const
{
  return nextRunTimeNs_ - nowNs_();
}

void ScheduledAction::UpdateTimer()
{
  nextRunTimeNs_ += nanosPerTick_;
  if (-(nextRunTimeNs_ - nowNs_()) > nanosPerTick_ * 2)
  {
    // More than two periods behind — skip to catch up (parity: the
    // reference logs this and resyncs to now + one period).
    nextRunTimeNs_ = nowNs_() + nanosPerTick_;
  }
}

} // namespace lod

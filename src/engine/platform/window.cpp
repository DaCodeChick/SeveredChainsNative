#include "platform/window.hpp"

#include "util/log.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <string>

namespace lod
{
Window::~Window()
{
  Destroy();
}

bool Window::Create(const WindowConfig& config)
{
  if (window_ != nullptr)
  {
    return true;
  }
  const auto width = std::max(config.width, kWindowMinWidth);
  const auto height = std::max(config.height, kWindowMinHeight);
  window_ = SDL_CreateWindow(config.title.c_str(), width, height, SDL_WINDOW_RESIZABLE);
  if (window_ == nullptr)
  {
    Logger::Instance().LogMessage(LogLevel::Error, "window", "SDL_CreateWindow failed: " + std::string(SDL_GetError()));
    return false;
  }
  // Enforce the minimum window size.
  SDL_SetWindowMinimumSize(window_, kWindowMinWidth, kWindowMinHeight);
  Logger::Instance().Log(LogLevel::Info, "window", "created '{}' at {}x{}", config.title, width, height);
  return true;
}

void Window::Destroy()
{
  if (window_ == nullptr)
  {
    return;
  }
  SDL_DestroyWindow(window_);
  window_ = nullptr;
  Logger::Instance().Log(LogLevel::Info, "window", "destroyed");
}

bool Window::IsCreated() const
{
  return window_ != nullptr;
}

void Window::Show()
{
  if (window_ != nullptr)
  {
    SDL_ShowWindow(window_);
  }
}

Window::Size Window::GetSize() const
{
  int32_t width = 0;
  int32_t height = 0;
  if (window_ != nullptr)
  {
    SDL_GetWindowSize(window_, &width, &height);
  }
  return Size{width, height};
}

void Window::SetSize(int32_t width, int32_t height)
{
  if (window_ == nullptr)
  {
    return;
  }
  SDL_SetWindowSize(window_, std::max(width, kWindowMinWidth), std::max(height, kWindowMinHeight));
}

bool Window::IsFullscreen() const
{
  return window_ != nullptr && (SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN) != 0;
}

void Window::ToggleFullscreen()
{
  if (window_ == nullptr)
  {
    return;
  }
  // SDL3's unified fullscreen is per-monitor at the window's size
  // (roadmap M1: "fullscreen per-monitor").
  SDL_SetWindowFullscreen(window_, IsFullscreen() ? 0 : SDL_WINDOW_FULLSCREEN);
}

SDL_Window* Window::Handle() const
{
  return window_;
}

} // namespace lod
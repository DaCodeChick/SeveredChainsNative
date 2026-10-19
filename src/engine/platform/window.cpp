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
  const auto width = std::max(config.width, WindowMinWidth);
  const auto height = std::max(config.height, WindowMinHeight);
  // SDL_Window is an opaque C type — the handle returned by SDL is owned
  // through the unique_ptr member with its custom deleter (guidelines §5).
  window_.reset(SDL_CreateWindow(config.title.c_str(), width, height, SDL_WINDOW_RESIZABLE));
  if (window_ == nullptr)
  {
    Logger::Instance().LogMessage(LogLevel::Error, "window", "SDL_CreateWindow failed: " + std::string(SDL_GetError()));
    return false;
  }
  // Enforce the minimum window size.
  SDL_SetWindowMinimumSize(window_.get(), WindowMinWidth, WindowMinHeight);
  Logger::Instance().Log(LogLevel::Info, "window", "created '{}' at {}x{}", config.title, width, height);
  return true;
}

void Window::Destroy()
{
  if (window_ == nullptr)
  {
    return;
  }
  window_.reset(); // invokes SDLWindowDeleter (SDL_DestroyWindow)
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
    SDL_ShowWindow(window_.get());
  }
}

Window::Size Window::GetSize() const
{
  int32_t width = 0;
  int32_t height = 0;
  if (window_ != nullptr)
  {
    SDL_GetWindowSize(window_.get(), &width, &height);
  }
  return Size{width, height};
}

void Window::SetSize(int32_t width, int32_t height)
{
  if (window_ == nullptr)
  {
    return;
  }
  SDL_SetWindowSize(window_.get(), std::max(width, WindowMinWidth), std::max(height, WindowMinHeight));
}

bool Window::IsFullscreen() const
{
  return window_ != nullptr && (SDL_GetWindowFlags(window_.get()) & SDL_WINDOW_FULLSCREEN) != 0;
}

void Window::ToggleFullscreen()
{
  if (window_ == nullptr)
  {
    return;
  }
  // SDL3's unified fullscreen is per-monitor at the window's size
  // (roadmap M1: "fullscreen per-monitor").
  SDL_SetWindowFullscreen(window_.get(), IsFullscreen() ? 0 : SDL_WINDOW_FULLSCREEN);
}

SDL_Window* Window::Handle() const
{
  return window_.get(); // non-owning escape hatch (M2 GPU layer)
}

} // namespace lod
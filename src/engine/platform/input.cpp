#include "platform/input.hpp"

#include <string_view>

namespace sc::platform
{
std::string_view ToString(InputAction action)
{
  switch (action)
  {
    case InputAction::None:
      return "None";
    case InputAction::Quit:
      return "Quit";
    case InputAction::ToggleFullscreen:
      return "ToggleFullscreen";
    case InputAction::Count:
      break;
  }
  return "Unknown";
}

InputAction Input::ProcessEvent(const SDL_Event& event)
{
  switch (event.type)
  {
    case SDL_EVENT_QUIT:
      return InputAction::Quit;
    case SDL_EVENT_KEY_DOWN:
    {
      const InputAction action = ActionForKey(event.key.key);
      if (action != InputAction::None)
      {
        held_[static_cast<std::size_t>(action)] = true;
      }
      return action;
    }
    case SDL_EVENT_KEY_UP:
    {
      const InputAction action = ActionForKey(event.key.key);
      if (action != InputAction::None)
      {
        held_[static_cast<std::size_t>(action)] = false;
      }
      return InputAction::None;
    }
    default:
      return InputAction::None;
  }
}

bool Input::IsActionHeld(InputAction action) const
{
  if (action == InputAction::None || action == InputAction::Count)
  {
    return false;
  }
  return held_[static_cast<std::size_t>(action)];
}

InputAction Input::ActionForKey(SDL_Keycode key)
{
  switch (key)
  {
    case SDLK_ESCAPE:
      return InputAction::Quit;
    case SDLK_F11:
      return InputAction::ToggleFullscreen;
    default:
      return InputAction::None;
  }
}

} // namespace sc::platform
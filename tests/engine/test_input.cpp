// Input action mapping tests (M1 foundation subset; the full
// InputKey/Action binding table lands with the input-parity increment).
// Pure event-driven — no SDL init required.
#include "sc_test.hpp"

#include <SDL3/SDL.h>

#include <platform/input.hpp>

using sc::platform::Input;
using sc::platform::InputAction;

namespace
{
SDL_Event MakeKeyDownEvent(SDL_Keycode key)
{
  SDL_Event event{};
  event.type = SDL_EVENT_KEY_DOWN;
  event.key.key = key;
  return event;
}

SDL_Event MakeKeyUpEvent(SDL_Keycode key)
{
  SDL_Event event{};
  event.type = SDL_EVENT_KEY_UP;
  event.key.key = key;
  return event;
}
} // namespace

SC_TEST(InputQuitAction, "input")
{
  Input input;
  SC_REQUIRE(input.ProcessEvent(MakeKeyDownEvent(SDLK_ESCAPE)) == InputAction::Quit);
  SC_REQUIRE(input.IsActionHeld(InputAction::Quit));
  SC_REQUIRE(input.ProcessEvent(MakeKeyUpEvent(SDLK_ESCAPE)) == InputAction::None);
  SC_REQUIRE(!input.IsActionHeld(InputAction::Quit));
  return true;
}

SC_TEST(InputFullscreenToggleAction, "input")
{
  Input input;
  SC_REQUIRE(input.ProcessEvent(MakeKeyDownEvent(SDLK_F11)) == InputAction::ToggleFullscreen);
  SC_REQUIRE(input.IsActionHeld(InputAction::ToggleFullscreen));
  SC_REQUIRE(input.ProcessEvent(MakeKeyUpEvent(SDLK_F11)) == InputAction::None);
  SC_REQUIRE(!input.IsActionHeld(InputAction::ToggleFullscreen));
  return true;
}

SC_TEST(InputUnboundKey, "input")
{
  Input input;
  SC_REQUIRE(input.ProcessEvent(MakeKeyDownEvent(SDLK_A)) == InputAction::None);
  SC_REQUIRE(!input.IsActionHeld(InputAction::Quit));
  return true;
}

SC_TEST(InputQuitEvent, "input")
{
  Input input;
  SDL_Event event{};
  event.type = SDL_EVENT_QUIT;
  SC_REQUIRE(input.ProcessEvent(event) == InputAction::Quit);
  return true;
}

SC_TEST(InputActionNames, "input")
{
  SC_REQUIRE(sc::platform::ToString(InputAction::None) == "None");
  SC_REQUIRE(sc::platform::ToString(InputAction::Quit) == "Quit");
  SC_REQUIRE(sc::platform::ToString(InputAction::ToggleFullscreen) == "ToggleFullscreen");
  return true;
}
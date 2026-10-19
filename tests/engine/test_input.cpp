// Input action mapping tests (M1 foundation subset; the full
// key/button binding table lands with the input-parity increment).
// Pure event-driven — no SDL init required.
#include "lod_test.hpp"

#include <SDL3/SDL.h>

#include <platform/input.hpp>

using lod::Input;
using lod::InputAction;

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

LOD_TEST(InputQuitAction, "input")
{
  Input input;
  LOD_REQUIRE(input.ProcessEvent(MakeKeyDownEvent(SDLK_ESCAPE)) == InputAction::Quit);
  LOD_REQUIRE(input.IsActionHeld(InputAction::Quit));
  LOD_REQUIRE(input.ProcessEvent(MakeKeyUpEvent(SDLK_ESCAPE)) == InputAction::None);
  LOD_REQUIRE(!input.IsActionHeld(InputAction::Quit));
  return true;
}

LOD_TEST(InputFullscreenToggleAction, "input")
{
  Input input;
  LOD_REQUIRE(input.ProcessEvent(MakeKeyDownEvent(SDLK_F11)) == InputAction::ToggleFullscreen);
  LOD_REQUIRE(input.IsActionHeld(InputAction::ToggleFullscreen));
  LOD_REQUIRE(input.ProcessEvent(MakeKeyUpEvent(SDLK_F11)) == InputAction::None);
  LOD_REQUIRE(!input.IsActionHeld(InputAction::ToggleFullscreen));
  return true;
}

LOD_TEST(InputUnboundKey, "input")
{
  Input input;
  LOD_REQUIRE(input.ProcessEvent(MakeKeyDownEvent(SDLK_A)) == InputAction::None);
  LOD_REQUIRE(!input.IsActionHeld(InputAction::Quit));
  return true;
}

LOD_TEST(InputQuitEvent, "input")
{
  Input input;
  SDL_Event event{};
  event.type = SDL_EVENT_QUIT;
  LOD_REQUIRE(input.ProcessEvent(event) == InputAction::Quit);
  return true;
}

LOD_TEST(InputActionNames, "input")
{
  LOD_REQUIRE(lod::ToString(InputAction::None) == "None");
  LOD_REQUIRE(lod::ToString(InputAction::Quit) == "Quit");
  LOD_REQUIRE(lod::ToString(InputAction::ToggleFullscreen) == "ToggleFullscreen");
  return true;
}
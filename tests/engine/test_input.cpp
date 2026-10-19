// Input parity model tests: enum name tables, action registry, bindings,
// the action state machine (press/repeat/release), and the default
// actions/bindings table. Pure logic — no SDL init required.
#include "lod_test.hpp"

#include <algorithm>
#include <cstdint>
#include <platform/activation.hpp>
#include <platform/input.hpp>
#include <string>
#include <string_view>
#include <util/status.hpp>
#include <vector>

using namespace lod;

namespace
{
InputMods WithCtrl()
{
  InputMods mods;
  mods.flags |= static_cast<std::uint32_t>(InputMod::Ctrl);
  return mods;
}
} // namespace

LOD_TEST(ActivationNameRoundTrip, "input")
{
  // Every enum value round-trips through its on-disk name (Java constant
  // names — the reference format).
  for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(InputKey::Count); ++i)
  {
    const auto key = static_cast<InputKey>(i);
    const auto parsed = InputKeyFromName(NameOf(key));
    if (!parsed.has_value() || *parsed != key)
    {
      LOD_CHECK(false); // expression: InputKeyFromName(NameOf(key)) == key
    }
  }
  for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(InputButton::Count); ++i)
  {
    const auto button = static_cast<InputButton>(i);
    const auto parsed = InputButtonFromName(NameOf(button));
    if (!parsed.has_value() || *parsed != button)
    {
      LOD_CHECK(false); // expression: InputButtonFromName(NameOf(button)) == button
    }
  }
  for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(InputAxis::Count); ++i)
  {
    const auto axis = static_cast<InputAxis>(i);
    const auto parsed = InputAxisFromName(NameOf(axis));
    if (!parsed.has_value() || *parsed != axis)
    {
      LOD_CHECK(false); // expression: InputAxisFromName(NameOf(axis)) == axis
    }
  }
  // Modifier + direction names (format data).
  LOD_REQUIRE(InputModFromName(NameOf(InputMod::Alt)) == InputMod::Alt);
  LOD_REQUIRE(InputAxisDirectionFromName(NameOf(InputAxisDirection::Negative)) == InputAxisDirection::Negative);
  return true;
}

LOD_TEST(ActionRegistryDuplicates, "input")
{
  ActionRegistry registry;
  const auto added = registry.Register("menu_confirm", InputAction{.canBeEdited = false});
  LOD_REQUIRE(added.has_value());
  const auto duplicate = registry.Register("menu_confirm", InputAction{});
  LOD_REQUIRE(!duplicate.has_value());
  LOD_REQUIRE(duplicate.error() == Status::AlreadyExists);
  LOD_REQUIRE(registry.Find("nope") == std::nullopt);
  LOD_REQUIRE(registry.Find("menu_confirm").has_value());
  LOD_REQUIRE(registry.Find("menu_confirm")->get().canBeEdited == false);
  return true;
}

LOD_TEST(ActionBindingsMatch, "input")
{
  ActionBindings bindings;
  const Activation key = KeyActivation{InputKey::W, {}};
  const Activation ctrlKey = KeyActivation{InputKey::W, WithCtrl()};
  const Activation scancode = ScancodeActivation{InputKey::W, {}};
  const Activation button = ButtonActivation{InputButton::DPadUp};
  bindings.Add(key, "menu_up");
  bindings.Add(ctrlKey, "freecam_toggle");
  bindings.Add(scancode, "menu_up");
  bindings.Add(button, "menu_up");

  std::vector<std::string> matched;
  // Bare W (keycode path): key binding + scancode binding; Ctrl is a
  // required-modifier subset — the Ctrl+M binding does not match bare W.
  bindings.MatchKey(SDL_GetKeyFromName("W"), SDL_GetScancodeFromName("W"), InputMods{}, matched);
  LOD_REQUIRE(matched.size() == 2);
  for (const auto& id : matched)
  {
    LOD_REQUIRE(id == "menu_up");
  }

  // Ctrl+W: the Ctrl+key binding now matches too.
  matched.clear();
  bindings.MatchKey(SDL_GetKeyFromName("W"), SDL_GetScancodeFromName("W"), WithCtrl(), matched);
  LOD_REQUIRE(matched.size() == 3);

  // Button match.
  matched.clear();
  bindings.MatchButton(SDL_GAMEPAD_BUTTON_DPAD_UP, matched);
  LOD_REQUIRE(matched.size() == 1);
  LOD_REQUIRE(matched[0] == "menu_up");
  return true;
}

LOD_TEST(InputActionStateMachine, "input")
{
  // The exact reference state machine: press -> (first tick) -> delay of
  // 500ms -> REPEAT/HELD alternating every 50ms.
  InputActionState state;
  state.Press(0);
  LOD_REQUIRE(state.IsHeld());
  LOD_REQUIRE(state.IsPressed());
  LOD_REQUIRE(state.IsRepeat());

  // First two ticks settle into the delay.
  LOD_REQUIRE(!state.Repeat(0));
  LOD_REQUIRE(!state.Repeat(0));   // now in Delay
  LOD_REQUIRE(!state.IsPressed()); // Delay is held but not "pressed/repeat"
  LOD_REQUIRE(state.IsHeld());

  // 500ms delay elapses -> first repeat.
  LOD_REQUIRE(!state.Repeat(499'999'999LL));
  LOD_REQUIRE(state.Repeat(500'000'000LL)); // REPEAT, returns true
  LOD_REQUIRE(state.IsRepeat());
  // 50ms later -> HELD (no repeat event).
  LOD_REQUIRE(!state.Repeat(549'999'999LL));
  LOD_REQUIRE(!state.Repeat(550'000'000LL)); // HELD
  LOD_REQUIRE(!state.IsRepeat());
  LOD_REQUIRE(state.IsHeld());
  // 50ms later -> REPEAT again.
  LOD_REQUIRE(state.Repeat(600'000'000LL));
  LOD_REQUIRE(state.IsRepeat());

  // Release.
  state.Release();
  LOD_REQUIRE(!state.IsHeld());
  LOD_REQUIRE(!state.IsRepeat());
  LOD_REQUIRE(!state.IsPressed());

  // Re-press.
  state.Press(1'000'000'000LL);
  LOD_REQUIRE(state.IsPressed());
  return true;
}

LOD_TEST(InputActionStateAxis, "input")
{
  InputActionState state;
  state.Press(0);
  state.Axis(0.5f);
  LOD_REQUIRE(state.GetAxis() == 0.5f);
  state.Release();
  LOD_REQUIRE(state.GetAxis() == 0.0f);
  return true;
}

LOD_TEST(DefaultInputActions, "input")
{
  ActionRegistry registry;
  RegisterDefaultInputActions(registry);
  // The CoreMod parity set (menu + title + fmv + general + freecam + debug).
  const char* expected[] = {"menu_up",
                            "menu_down",
                            "menu_left",
                            "menu_right",
                            "menu_home",
                            "menu_end",
                            "menu_page_up",
                            "menu_page_down",
                            "menu_confirm",
                            "menu_back",
                            "menu_delete",
                            "menu_sort",
                            "menu_advanced",
                            "menu_help",
                            "menu_mods",
                            "menu_textbox_confirm",
                            "menu_unequip",
                            "title_update",
                            "title_convert_memcard",
                            "fmv_skip",
                            "general_toggle_fullscreen",
                            "general_speed_up",
                            "general_slow_down",
                            "general_toggle_speed",
                            "general_turbo",
                            "freecam_toggle",
                            "freecam_forward",
                            "debug_open_debugger",
                            "debug_pause"};
  for (const char* id : expected)
  {
    if (!registry.Find(id).has_value())
    {
      LOD_CHECK(false); // expression: registry.Find(id).has_value()
      continue;
    }
  }
  // Fixed vs editable vs hidden (CoreMod parity).
  LOD_REQUIRE(registry.Find("menu_confirm")->get().canBeEdited == false);
  LOD_REQUIRE(registry.Find("menu_delete")->get().canBeEdited == true);
  LOD_REQUIRE(registry.Find("menu_unequip")->get().visible == false);
  LOD_REQUIRE(registry.Find("freecam_toggle")->get().visible == false);
  return true;
}

LOD_TEST(DefaultInputBindings, "input")
{
  ActionRegistry registry;
  RegisterDefaultInputActions(registry);
  ActionBindings bindings;
  RegisterDefaultInputBindings(bindings);

  // menu_confirm: A button + Return key + Space key. Return is also bound
  // to menu_textbox_confirm and fmv_skip (CoreMod parity) — the game state
  // decides which reacts — so assert membership, not exclusivity.
  std::vector<std::string> confirm;
  bindings.MatchKey(SDLK_RETURN, SDL_SCANCODE_RETURN, InputMods{}, confirm);
  LOD_REQUIRE(std::find(confirm.begin(), confirm.end(), "menu_confirm") != confirm.end());

  std::vector<std::string> space;
  bindings.MatchKey(SDLK_SPACE, SDL_SCANCODE_SPACE, InputMods{}, space);
  LOD_REQUIRE(space.size() == 1);
  LOD_REQUIRE(space[0] == "menu_confirm");

  // DPadUp button -> menu_up.
  std::vector<std::string> dpad;
  bindings.MatchButton(SDL_GAMEPAD_BUTTON_DPAD_UP, dpad);
  LOD_REQUIRE(dpad.size() == 1);
  LOD_REQUIRE(dpad[0] == "menu_up");

  // Ctrl+M -> freecam_toggle. (The bare-M menu_mods binding also matches
  // with extra modifiers — the required-modifier subset semantics — same
  // as the reference; the game state decides which reacts.)
  std::vector<std::string> ctrlM;
  bindings.MatchKey(SDLK_M, SDL_SCANCODE_M, WithCtrl(), ctrlM);
  LOD_REQUIRE(std::find(ctrlM.begin(), ctrlM.end(), "freecam_toggle") != ctrlM.end());
  std::vector<std::string> bareM;
  bindings.MatchKey(SDLK_M, SDL_SCANCODE_M, InputMods{}, bareM);
  LOD_REQUIRE(std::find(bareM.begin(), bareM.end(), "freecam_toggle") == bareM.end());

  // F11 -> debug_pause.
  std::vector<std::string> f11;
  bindings.MatchKey(SDLK_F11, SDL_SCANCODE_UNKNOWN, InputMods{}, f11);
  LOD_REQUIRE(f11.size() == 1);
  LOD_REQUIRE(f11[0] == "debug_pause");
  return true;
}
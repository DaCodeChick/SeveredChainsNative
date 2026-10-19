// Default input actions + bindings — parity table with the reference
// `CoreMod.registerDefaultInputBindings` (IDs, flags, and every default
// binding; docs/research input parity checklist).
#include "platform/input.hpp"

#include <string>
#include <vector>

namespace lod
{
namespace
{
InputAction Fixed()
{
  return InputAction{.canBeEdited = false, .visible = true, .useMovementDeadzone = false};
}

InputAction Editable()
{
  return InputAction{.canBeEdited = true, .visible = true, .useMovementDeadzone = false};
}

InputAction Hidden()
{
  return InputAction{.canBeEdited = false, .visible = false, .useMovementDeadzone = false};
}

} // namespace

void RegisterDefaultInputActions(ActionRegistry& registry)
{
  // Menu.
  (void)registry.Register("menu_up", Fixed());
  (void)registry.Register("menu_down", Fixed());
  (void)registry.Register("menu_left", Fixed());
  (void)registry.Register("menu_right", Fixed());
  (void)registry.Register("menu_home", Fixed());
  (void)registry.Register("menu_end", Fixed());
  (void)registry.Register("menu_top", Fixed());
  (void)registry.Register("menu_bottom", Fixed());
  (void)registry.Register("menu_page_up", Fixed());
  (void)registry.Register("menu_page_down", Fixed());
  (void)registry.Register("menu_confirm", Fixed());
  (void)registry.Register("menu_back", Fixed());
  (void)registry.Register("menu_delete", Editable());
  (void)registry.Register("menu_sort", Editable());
  (void)registry.Register("menu_advanced", Editable());
  (void)registry.Register("menu_help", Editable());
  (void)registry.Register("menu_mods", Editable());
  (void)registry.Register("menu_textbox_confirm", Fixed());
  (void)registry.Register("menu_unequip", Hidden());

  // Title.
  (void)registry.Register("title_update", Fixed());
  (void)registry.Register("title_convert_memcard", Fixed());

  // FMV.
  (void)registry.Register("fmv_skip", Fixed());

  // General.
  (void)registry.Register("general_toggle_fullscreen", Editable());
  (void)registry.Register("general_speed_up", Editable());
  (void)registry.Register("general_slow_down", Editable());
  (void)registry.Register("general_toggle_speed", Editable());
  (void)registry.Register("general_turbo", Editable());

  // Freecam.
  (void)registry.Register("freecam_toggle", Hidden());
  (void)registry.Register("freecam_forward", Hidden());
  (void)registry.Register("freecam_backward", Hidden());
  (void)registry.Register("freecam_left", Hidden());
  (void)registry.Register("freecam_right", Hidden());
  (void)registry.Register("freecam_up", Hidden());
  (void)registry.Register("freecam_down", Hidden());

  // Debug.
  (void)registry.Register("debug_open_debugger", Editable());
  (void)registry.Register("debug_pause", Editable());
  (void)registry.Register("debug_frame_advance", Editable());
  (void)registry.Register("debug_frame_advance_hold", Editable());
  (void)registry.Register("debug_toggle_wireframe", Editable());
  (void)registry.Register("debug_reload_shaders", Editable());
}

void RegisterDefaultInputBindings(ActionBindings& bindings)
{
  auto key = [](InputKey key) { return KeyActivation{key, {}}; };
  auto scancode = [](InputKey key) { return ScancodeActivation{key, {}}; };
  auto button = [](InputButton button) { return ButtonActivation{button}; };
  auto axis = [](InputAxis axis, InputAxisDirection direction) { return AxisActivation{axis, direction}; };
  constexpr InputAxisDirection Positive = InputAxisDirection::Positive;
  constexpr InputAxisDirection Negative = InputAxisDirection::Negative;

  // Menu.
  bindings.Add(button(InputButton::DPadUp), "menu_up");
  bindings.Add(axis(InputAxis::LeftY, Negative), "menu_up");
  bindings.Add(scancode(InputKey::Up), "menu_up");
  bindings.Add(scancode(InputKey::W), "menu_up");
  bindings.Add(button(InputButton::DPadDown), "menu_down");
  bindings.Add(axis(InputAxis::LeftY, Positive), "menu_down");
  bindings.Add(scancode(InputKey::Down), "menu_down");
  bindings.Add(scancode(InputKey::S), "menu_down");
  bindings.Add(button(InputButton::DPadLeft), "menu_left");
  bindings.Add(axis(InputAxis::LeftX, Negative), "menu_left");
  bindings.Add(scancode(InputKey::Left), "menu_left");
  bindings.Add(scancode(InputKey::A), "menu_left");
  bindings.Add(button(InputButton::DPadRight), "menu_right");
  bindings.Add(axis(InputAxis::LeftX, Positive), "menu_right");
  bindings.Add(scancode(InputKey::Right), "menu_right");
  bindings.Add(scancode(InputKey::D), "menu_right");
  bindings.Add(axis(InputAxis::LeftTrigger, Positive), "menu_home");
  bindings.Add(key(InputKey::Home), "menu_home");
  bindings.Add(axis(InputAxis::RightTrigger, Positive), "menu_end");
  bindings.Add(key(InputKey::End), "menu_end");
  bindings.Add(button(InputButton::LeftBumper), "menu_page_up");
  bindings.Add(key(InputKey::PageUp), "menu_page_up");
  bindings.Add(button(InputButton::RightBumper), "menu_page_down");
  bindings.Add(key(InputKey::PageDown), "menu_page_down");
  bindings.Add(button(InputButton::A), "menu_confirm");
  bindings.Add(key(InputKey::Return), "menu_confirm");
  bindings.Add(key(InputKey::Space), "menu_confirm");
  bindings.Add(button(InputButton::B), "menu_back");
  bindings.Add(key(InputKey::Escape), "menu_back");
  bindings.Add(button(InputButton::X), "menu_delete");
  bindings.Add(scancode(InputKey::Q), "menu_delete");
  bindings.Add(button(InputButton::Y), "menu_sort");
  bindings.Add(key(InputKey::X), "menu_sort");
  bindings.Add(button(InputButton::Select), "menu_advanced");
  bindings.Add(key(InputKey::V), "menu_advanced");
  bindings.Add(button(InputButton::Start), "menu_help");
  bindings.Add(key(InputKey::H), "menu_help");
  bindings.Add(button(InputButton::Y), "menu_mods");
  bindings.Add(key(InputKey::M), "menu_mods");
  bindings.Add(button(InputButton::A), "menu_textbox_confirm");
  bindings.Add(key(InputKey::Return), "menu_textbox_confirm");
  bindings.Add(button(InputButton::X), "menu_unequip");
  bindings.Add(key(InputKey::Q), "menu_unequip");

  // Title.
  bindings.Add(button(InputButton::Y), "title_update");
  bindings.Add(key(InputKey::U), "title_update");
  bindings.Add(button(InputButton::X), "title_convert_memcard");
  bindings.Add(key(InputKey::C), "title_convert_memcard");

  // FMV.
  bindings.Add(key(InputKey::Return), "fmv_skip");
  bindings.Add(button(InputButton::Y), "fmv_skip");

  // General.
  bindings.Add(key(InputKey::F1), "general_toggle_fullscreen");
  bindings.Add(scancode(InputKey::Equals), "general_speed_up");
  bindings.Add(key(InputKey::KpPlus), "general_speed_up");
  bindings.Add(scancode(InputKey::Minus), "general_slow_down");
  bindings.Add(key(InputKey::KpMinus), "general_slow_down");

  // Freecam.
  {
    KeyActivation toggle{InputKey::M, InputMods{static_cast<std::uint32_t>(InputMod::Ctrl)}};
    bindings.Add(toggle, "freecam_toggle");
  }
  bindings.Add(scancode(InputKey::I), "freecam_forward");
  bindings.Add(scancode(InputKey::K), "freecam_backward");
  bindings.Add(scancode(InputKey::J), "freecam_left");
  bindings.Add(scancode(InputKey::L), "freecam_right");
  bindings.Add(scancode(InputKey::O), "freecam_up");
  bindings.Add(scancode(InputKey::U), "freecam_down");

  // Debug.
  bindings.Add(key(InputKey::F12), "debug_open_debugger");
  bindings.Add(key(InputKey::F11), "debug_pause");
  bindings.Add(key(InputKey::F9), "debug_frame_advance");
  bindings.Add(key(InputKey::F10), "debug_frame_advance_hold");
  bindings.Add(key(InputKey::F2), "debug_toggle_wireframe");
  bindings.Add(key(InputKey::F5), "debug_reload_shaders");
}

} // namespace lod
#include "config/config.hpp"
#include "config/raw_serializers.hpp"
#include "util/status.hpp"

#include <string>
#include <vector>

namespace lod
{
/// Registers the M1 engine config entries — the platform-relevant subset of
/// the reference `CoreMod` config registrations (docs/research input parity
/// checklist), with the same IDs, storage locations, defaults, and encodings:
///
/// | ID                              | type    | location | default                |
/// |---------------------------------|---------|----------|------------------------|
/// | menu_inner_deadzone             | float32 | global   | 0.5                    |
/// | menu_outer_deadzone             | float32 | global   | 0.6                    |
/// | movement_inner_deadzone         | float32 | global   | 0.2                    |
/// | movement_outer_deadzone         | float32 | global   | 0.9                    |
/// | receive_input_on_inactive_window| boolean | global   | false                  |
/// | disable_mouse_input             | boolean | global   | false                  |
/// | rumble                          | boolean | global   | true                   |
/// | rumble_intensity                | float32 | global   | 1.0                    |
/// | ignore_steam_input_mode         | enum    | global   | IGNORE_WHEN_GAMEPAD_USED |
/// | fullscreen                      | boolean | global   | false                  |
/// | monitor                         | int32   | global   | 0 (1-byte encoding)    |
/// | create_crash_save               | boolean | global   | true                   |
/// | frame_skip                      | boolean | global   | true                   |
/// | show_fps                        | boolean | global   | false                  |
/// | game_speed_multiplier           | int32   | global   | 1                      |
/// | controller_keybinds             | keybinds| campaign | (empty = built-ins)    |
std::expected<void, Status> RegisterEngineConfigs(ConfigCollection& configs)
{
  auto registerBool = [&configs](const std::string& id, bool defaultValue) -> std::expected<void, Status>
  {
    ConfigEntry entry{
      .id = id,
      .location = ConfigStorageLocation::Global,
      .defaultValue = ConfigValue{defaultValue},
      .presetValueType = "boolean",
      .presetSchemaVersion = 1,
      .serializer = &SerializeRawBool,
      .deserializer = &DeserializeRawBool,
    };
    return configs.Register(entry);
  };
  auto registerFloat = [&configs](const std::string& id, float defaultValue) -> std::expected<void, Status>
  {
    ConfigEntry entry{
      .id = id,
      .location = ConfigStorageLocation::Global,
      .defaultValue = ConfigValue{defaultValue},
      .presetValueType = "float32",
      .presetSchemaVersion = 1,
      .serializer = &SerializeRawFloat32,
      .deserializer = &DeserializeRawFloat32,
    };
    return configs.Register(entry);
  };
  auto registerInt = [&configs](const std::string& id, std::int32_t defaultValue) -> std::expected<void, Status>
  {
    ConfigEntry entry{
      .id = id,
      .location = ConfigStorageLocation::Global,
      .defaultValue = ConfigValue{defaultValue},
      .presetValueType = "int32",
      .presetSchemaVersion = 1,
      .serializer = &SerializeRawInt32,
      .deserializer = &DeserializeRawInt32,
    };
    return configs.Register(entry);
  };

  // Input / controls.
  auto result = registerFloat("menu_inner_deadzone", 0.5f);
  if (!result.has_value())
  {
    return result;
  }
  result = registerFloat("menu_outer_deadzone", 0.6f);
  if (!result.has_value())
  {
    return result;
  }
  result = registerFloat("movement_inner_deadzone", 0.2f);
  if (!result.has_value())
  {
    return result;
  }
  result = registerFloat("movement_outer_deadzone", 0.9f);
  if (!result.has_value())
  {
    return result;
  }
  result = registerBool("receive_input_on_inactive_window", false);
  if (!result.has_value())
  {
    return result;
  }
  result = registerBool("disable_mouse_input", false);
  if (!result.has_value())
  {
    return result;
  }
  result = registerBool("rumble", true);
  if (!result.has_value())
  {
    return result;
  }
  result = registerFloat("rumble_intensity", 1.0f);
  if (!result.has_value())
  {
    return result;
  }
  {
    // Enum: 1-byte length + name; valid names + default from the reference
    // IgnoreSteamInputMode enum (VIRTUAL_KEYBOARD_ID is reference-TODO and
    // not registered).
    std::vector<std::string> modes{"IGNORE_WHEN_GAMEPAD_USED", "NONE"};
    ConfigEntry entry{
      .id = "ignore_steam_input_mode",
      .location = ConfigStorageLocation::Global,
      .defaultValue = ConfigValue{std::string("IGNORE_WHEN_GAMEPAD_USED")},
      .presetValueType = "enum",
      .presetSchemaVersion = 1,
      .serializer = &SerializeRawEnum,
      .deserializer = [modes = std::move(modes)](std::span<const std::byte> data)
      { return DeserializeRawEnum(data, modes, "IGNORE_WHEN_GAMEPAD_USED"); },
    };
    result = configs.Register(entry);
    if (!result.has_value())
    {
      return result;
    }
  }

  // Graphics.
  result = registerBool("fullscreen", false);
  if (!result.has_value())
  {
    return result;
  }
  {
    // Parity: MonitorConfigEntry serializes as a single byte.
    ConfigEntry entry{
      .id = "monitor",
      .location = ConfigStorageLocation::Global,
      .defaultValue = ConfigValue{std::int32_t{0}},
      .presetValueType = "int32",
      .presetSchemaVersion = 1,
      .serializer = &SerializeRawInt8,
      .deserializer = &DeserializeRawInt8,
    };
    result = configs.Register(entry);
    if (!result.has_value())
    {
      return result;
    }
  }
  result = registerBool("frame_skip", true);
  if (!result.has_value())
  {
    return result;
  }
  result = registerBool("show_fps", false);
  if (!result.has_value())
  {
    return result;
  }

  // Gameplay / diagnostics.
  result = registerBool("create_crash_save", true);
  if (!result.has_value())
  {
    return result;
  }
  result = registerInt("game_speed_multiplier", 1);
  if (!result.has_value())
  {
    return result;
  }

  // Per-campaign.
  {
    ConfigEntry entry{
      .id = "controller_keybinds",
      .location = ConfigStorageLocation::Campaign,
      .defaultValue = ConfigValue{KeybindValue{}},
      .presetValueType = "keybinds",
      .presetSchemaVersion = 1,
      .serializer = &SerializeRawKeybinds,
      .deserializer = &DeserializeRawKeybinds,
    };
    result = configs.Register(entry);
    if (!result.has_value())
    {
      return result;
    }
  }
  return {};
}

} // namespace lod
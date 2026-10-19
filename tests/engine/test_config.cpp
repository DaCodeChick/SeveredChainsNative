// Config tests: the Tag-based global config.dcnf layout (byte goldens),
// round-trips, the per-type raw serializers, preset save/load, and the M1
// engine entry registrations.
#include "lod_test.hpp"

#include <config/config.hpp>
#include <config/raw_serializers.hpp>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace lod;

namespace
{
/// A scratch directory for the config files (per-test isolation).
std::filesystem::path ScratchDir()
{
  const std::filesystem::path dir = std::filesystem::temp_directory_path() / "lod-sc-tests" / "config";
  std::filesystem::create_directories(dir);
  return dir;
}

std::vector<std::byte> ReadBytes(const std::filesystem::path& file)
{
  std::ifstream in(file, std::ios::binary);
  const std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  std::vector<std::byte> bytes(data.size());
  std::memcpy(bytes.data(), data.data(), data.size());
  return bytes;
}

void WriteBytes(const std::filesystem::path& file, std::span<const std::byte> bytes)
{
  std::ofstream out(file, std::ios::binary | std::ios::trunc);
  out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

ConfigEntry MakeInt32(std::string id, std::int32_t value)
{
  ConfigEntry entry;
  entry.id = std::move(id);
  entry.location = ConfigStorageLocation::Global;
  entry.defaultValue = value;
  entry.presetValueType = "int32";
  entry.serializer = &SerializeRawInt32;
  entry.deserializer = &DeserializeRawInt32;
  return entry;
}
} // namespace

LOD_TEST(ConfigGlobalFileLayout, "config")
{
  // Byte golden for the global layout: int32 count, then per entry
  // (registry ID = int32 ASCII length + ASCII, int32 LE value length, raw
  // value bytes).
  ConfigCollection configs;
  LOD_REQUIRE(configs.Register(MakeInt32("foo", 100)).has_value());
  const std::filesystem::path file = ScratchDir() / "global.dcnf";
  LOD_REQUIRE(SaveGlobalConfig(configs, file).has_value());

  const auto bytes = ReadBytes(file);
  // Header: 01 00 00 00 (count) | id: 03 00 00 00 'f' 'o' 'o' | len: 04 00 00 00 | value: 64 00 00 00
  const std::uint8_t expected[] = {
    0x01, 0x00, 0x00, 0x00,           // count (LE int32)
    0x03, 0x00, 0x00, 'f',  'o', 'o', // registry ID (3-byte LE length + ASCII)
    0x04, 0x00, 0x00, 0x00,           // value length (LE int32)
    0x64, 0x00, 0x00, 0x00            // int32 value 100
  };
  LOD_REQUIRE(bytes.size() == sizeof(expected));
  bool equal = bytes.size() == sizeof(expected);
  for (std::size_t i = 0; equal && i < bytes.size(); ++i)
  {
    equal = static_cast<std::uint8_t>(bytes[i]) == expected[i];
  }
  LOD_REQUIRE(equal);
  return true;
}

LOD_TEST(ConfigGlobalRoundTrip, "config")
{
  ConfigCollection configs;
  LOD_REQUIRE(configs.Register(MakeInt32("foo", 100)).has_value());
  LOD_REQUIRE(configs.Register(MakeInt32("bar", -2)).has_value());
  configs.Set("foo", static_cast<std::int32_t>(7));

  const std::filesystem::path file = ScratchDir() / "roundtrip.dcnf";
  LOD_REQUIRE(SaveGlobalConfig(configs, file).has_value());

  ConfigCollection loaded;
  LOD_REQUIRE(loaded.Register(MakeInt32("foo", 111)).has_value()); // different default
  LOD_REQUIRE(loaded.Register(MakeInt32("bar", 222)).has_value());
  LOD_REQUIRE(LoadGlobalConfig(loaded, file).has_value());
  LOD_REQUIRE(loaded.GetInt("foo") == 7);
  LOD_REQUIRE(loaded.GetInt("bar") == -2);
  return true;
}

LOD_TEST(ConfigGlobalErrors, "config")
{
  const std::filesystem::path missing = ScratchDir() / "absent.dcnf";
  ConfigCollection configs;
  LOD_REQUIRE(configs.Register(MakeInt32("foo", 1)).has_value());
  // Missing file = no-op (parity).
  LOD_REQUIRE(LoadGlobalConfig(configs, missing).has_value());

  // Corrupted (truncated) file -> Error, collection untouched.
  const std::filesystem::path corrupt = ScratchDir() / "corrupt.dcnf";
  std::vector<std::byte> truncated{std::byte{0x02}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
                                   std::byte{0x03}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}};
  WriteBytes(corrupt, truncated);
  LOD_REQUIRE(!LoadGlobalConfig(configs, corrupt).has_value());
  LOD_REQUIRE(configs.GetInt("foo") == 1); // untouched
  return true;
}

LOD_TEST(RawSerializerGoldens, "config")
{
  // bool -> 1 byte.
  {
    const auto bytes = SerializeRawBool(ConfigValue{true});
    LOD_REQUIRE(bytes.size() == 1);
    LOD_REQUIRE(static_cast<std::uint8_t>(bytes[0]) == 1);
    const auto value = DeserializeRawBool({bytes.data(), bytes.size()});
    LOD_REQUIRE(value.has_value());
    LOD_REQUIRE(std::get<bool>(*value) == true);
  }
  // int32 -> 4-byte LE.
  {
    const auto bytes = SerializeRawInt32(ConfigValue{std::int32_t{-1}});
    LOD_REQUIRE(bytes.size() == 4);
    LOD_REQUIRE(static_cast<std::uint8_t>(bytes[0]) == 0xff);
    LOD_REQUIRE(static_cast<std::uint8_t>(bytes[3]) == 0xff);
  }
  // int8 -> 1 byte.
  {
    const auto bytes = SerializeRawInt8(ConfigValue{std::int32_t{255}});
    LOD_REQUIRE(bytes.size() == 1);
    LOD_REQUIRE(static_cast<std::uint8_t>(bytes[0]) == 0xff);
  }
  // int64 -> 8-byte LE.
  {
    const auto bytes = SerializeRawInt64(ConfigValue{std::int64_t{0x0102030405060708LL}});
    LOD_REQUIRE(bytes.size() == 8);
    LOD_REQUIRE(static_cast<std::uint8_t>(bytes[0]) == 0x08);
    LOD_REQUIRE(static_cast<std::uint8_t>(bytes[7]) == 0x01);
  }
  // enum -> 1-byte length + UTF-8 name; unknown names fall back to the
  // default (parity `enumFromBytes`).
  {
    const auto bytes = SerializeRawEnum(ConfigValue{std::string("LOW")});
    LOD_REQUIRE(bytes.size() == 4);
    LOD_REQUIRE(static_cast<std::uint8_t>(bytes[0]) == 3);
    const auto ok = DeserializeRawEnum({bytes.data(), bytes.size()}, {"LOW", "HIGH"}, "LOW");
    LOD_REQUIRE(ok.has_value());
    LOD_REQUIRE(std::get<std::string>(*ok) == "LOW");
    const std::vector<std::byte> unknown{std::byte{2}, static_cast<std::byte>('A'), static_cast<std::byte>('B'),
                                         std::byte{0}};
    const auto fallback = DeserializeRawEnum({unknown.data(), unknown.size()}, {"LOW", "HIGH"}, "LOW");
    LOD_REQUIRE(fallback.has_value());
    LOD_REQUIRE(std::get<std::string>(*fallback) == "LOW");
  }
  return true;
}

LOD_TEST(RawKeybindsGolden, "config")
{
  // One action ("a") with one key activation (key A, no mods):
  //   u16 count=1 | registryId "a" (00 00 00 01 'a') | u8 activations=1
  //   | "key" (03 'k' 'e' 'y') | "A" (01 'A') | u8 mods=0
  KeybindValue keybinds;
  keybinds.emplace_back("a", std::vector<Activation>{KeyActivation{InputKey::A, {}}});
  const auto bytes = SerializeRawKeybinds(ConfigValue{keybinds});
  const std::uint8_t expected[] = {
    0x01, 0x00,            // count (LE u16)
    0x01, 0x00, 0x00, 'a', // registry ID (3-byte LE length + ASCII)
    0x01,                  // activation count
    0x03, 'k',  'e',  'y', // "key"
    0x01, 'A',             // "A"
    0x00                   // no mods
  };
  LOD_REQUIRE(bytes.size() == sizeof(expected));
  bool equal = bytes.size() == sizeof(expected);
  for (std::size_t i = 0; equal && i < bytes.size(); ++i)
  {
    equal = static_cast<std::uint8_t>(bytes[i]) == expected[i];
  }
  LOD_REQUIRE(equal);

  // Round-trip.
  const auto value = DeserializeRawKeybinds({bytes.data(), bytes.size()});
  LOD_REQUIRE(value.has_value());
  const auto* roundTripped = std::get_if<KeybindValue>(value.has_value() ? &*value : nullptr);
  LOD_REQUIRE(roundTripped != nullptr);
  LOD_REQUIRE(roundTripped != nullptr && roundTripped->size() == 1);
  LOD_REQUIRE(roundTripped != nullptr && roundTripped->at(0).first == "a");
  LOD_REQUIRE(roundTripped != nullptr && roundTripped->at(0).second.size() == 1);
  const auto* activation = std::get_if<KeyActivation>(&roundTripped->at(0).second.at(0));
  LOD_REQUIRE(activation != nullptr);
  LOD_REQUIRE(activation != nullptr && activation->key == InputKey::A);
  return true;
}

LOD_TEST(ConfigPresetRoundTrip, "config")
{
  ConfigCollection configs;
  LOD_REQUIRE(configs.Register(MakeInt32("foo", 100)).has_value());
  configs.Set("foo", static_cast<std::int32_t>(9));

  const std::filesystem::path file = ScratchDir() / "preset.lodsc";
  LOD_REQUIRE(SavePresetConfig(configs, ConfigStorageLocation::Global, file).has_value());

  ConfigCollection loaded;
  LOD_REQUIRE(loaded.Register(MakeInt32("foo", 1)).has_value());
  LOD_REQUIRE(LoadPresetConfig(loaded, ConfigStorageLocation::Global, file).has_value());
  LOD_REQUIRE(loaded.GetInt("foo") == 9);
  return true;
}

LOD_TEST(EngineConfigRegistrations, "config")
{
  ConfigCollection configs;
  LOD_REQUIRE(RegisterEngineConfigs(configs).has_value());
  // M1 entries exist with the CoreMod-parity defaults.
  LOD_REQUIRE(configs.GetFloat("menu_inner_deadzone") == 0.5f);
  LOD_REQUIRE(configs.GetFloat("menu_outer_deadzone") == 0.6f);
  LOD_REQUIRE(configs.GetFloat("movement_inner_deadzone") == 0.2f);
  LOD_REQUIRE(configs.GetFloat("movement_outer_deadzone") == 0.9f);
  LOD_REQUIRE(configs.GetBool("create_crash_save") == true);
  LOD_REQUIRE(configs.GetBool("show_fps") == false);
  LOD_REQUIRE(configs.GetFloat("rumble_intensity") == 1.0f);
  LOD_REQUIRE(configs.GetString("ignore_steam_input_mode") == "IGNORE_WHEN_GAMEPAD_USED");

  // Full save/load round-trip of every global entry through a file.
  configs.Set("menu_inner_deadzone", 0.25f);
  const std::filesystem::path file = ScratchDir() / "engine.dcnf";
  LOD_REQUIRE(SaveGlobalConfig(configs, file).has_value());
  ConfigCollection reloaded;
  LOD_REQUIRE(RegisterEngineConfigs(reloaded).has_value());
  LOD_REQUIRE(LoadGlobalConfig(reloaded, file).has_value());
  LOD_REQUIRE(reloaded.GetFloat("menu_inner_deadzone") == 0.25f);
  return true;
}
#include "config/raw_serializers.hpp"

#include "psx/format/byte_buffer.hpp"
#include "util/status.hpp"

#include <algorithm>
#include <cstdint>
#include <string>

namespace lod
{
namespace
{
/// Serializes one activation to its wire form (parity:
/// `InputActivation.serialize` — 1-byte-length ASCII tags).
void SerializeActivation(const Activation& activation, psx::ByteWriter& writer)
{
  std::visit(
    [&writer](const auto& value)
    {
      using T = std::decay_t<decltype(value)>;
      if constexpr (std::is_same_v<T, KeyActivation> || std::is_same_v<T, ScancodeActivation>)
      {
        const std::string_view kind = std::is_same_v<T, KeyActivation> ? "key" : "scancode";
        writer.WriteAscii(kind, 1);
        writer.WriteAscii(NameOf(value.key), 1);
        std::uint8_t modCount = 0;
        for (const InputMod mod : {InputMod::Ctrl, InputMod::Alt, InputMod::Shift})
        {
          if (value.mods.Has(mod))
          {
            ++modCount;
          }
        }
        writer.WriteByte(modCount);
        for (const InputMod mod : {InputMod::Ctrl, InputMod::Alt, InputMod::Shift})
        {
          if (value.mods.Has(mod))
          {
            writer.WriteAscii(NameOf(mod), 1);
          }
        }
      }
      else if constexpr (std::is_same_v<T, ButtonActivation>)
      {
        writer.WriteAscii("button", 1);
        writer.WriteAscii(NameOf(value.button), 1);
      }
      else // AxisActivation
      {
        writer.WriteAscii("axis", 1);
        writer.WriteAscii(NameOf(value.axis), 1);
        writer.WriteAscii(NameOf(value.direction), 1);
      }
    },
    activation);
}

/// Parses one activation from its wire form.
[[nodiscard]] std::expected<Activation, Status> DeserializeActivation(psx::ByteReader& reader)
{
  const std::string kind = reader.ReadAscii(1);
  if (kind == "key" || kind == "scancode")
  {
    const auto key = InputKeyFromName(reader.ReadAscii(1));
    if (!key.has_value())
    {
      return std::unexpected(Status::NotSupported);
    }
    InputMods mods;
    const std::uint8_t modCount = reader.ReadByte();
    for (std::uint8_t i = 0; i < modCount; ++i)
    {
      const auto mod = InputModFromName(reader.ReadAscii(1));
      if (!mod.has_value())
      {
        return std::unexpected(Status::NotSupported);
      }
      mods.flags |= static_cast<std::uint32_t>(*mod);
    }
    if (kind == "key")
    {
      return KeyActivation{*key, mods};
    }
    return ScancodeActivation{*key, mods};
  }
  if (kind == "button")
  {
    const auto button = InputButtonFromName(reader.ReadAscii(1));
    if (!button.has_value())
    {
      return std::unexpected(Status::NotSupported);
    }
    return ButtonActivation{*button};
  }
  if (kind == "axis")
  {
    const auto axis = InputAxisFromName(reader.ReadAscii(1));
    const auto direction = InputAxisDirectionFromName(reader.ReadAscii(1));
    if (!axis.has_value() || !direction.has_value())
    {
      return std::unexpected(Status::NotSupported);
    }
    return AxisActivation{*axis, *direction};
  }
  return std::unexpected(Status::NotSupported);
}

} // namespace

std::vector<std::byte> SerializeRawBool(const ConfigValue& value)
{
  psx::ByteWriter writer;
  writer.WriteBool(std::holds_alternative<bool>(value) && std::get<bool>(value));
  return writer.TakeBytes();
}

std::expected<ConfigValue, Status> DeserializeRawBool(std::span<const std::byte> data)
{
  psx::ByteReader reader(data);
  return ConfigValue{reader.ReadBool()};
}

std::vector<std::byte> SerializeRawInt32(const ConfigValue& value)
{
  psx::ByteWriter writer;
  writer.WriteInt(std::holds_alternative<std::int32_t>(value) ? std::get<std::int32_t>(value) : 0);
  return writer.TakeBytes();
}

std::expected<ConfigValue, Status> DeserializeRawInt32(std::span<const std::byte> data)
{
  psx::ByteReader reader(data);
  return ConfigValue{reader.ReadInt()};
}

std::vector<std::byte> SerializeRawInt8(const ConfigValue& value)
{
  psx::ByteWriter writer;
  const std::int32_t v = std::holds_alternative<std::int32_t>(value) ? std::get<std::int32_t>(value) : 0;
  writer.WriteByte(static_cast<std::uint8_t>(v & 0xff));
  return writer.TakeBytes();
}

std::expected<ConfigValue, Status> DeserializeRawInt8(std::span<const std::byte> data)
{
  psx::ByteReader reader(data);
  return ConfigValue{static_cast<std::int32_t>(reader.ReadByte())};
}

std::vector<std::byte> SerializeRawInt64(const ConfigValue& value)
{
  psx::ByteWriter writer;
  writer.WriteLong(std::holds_alternative<std::int64_t>(value) ? std::get<std::int64_t>(value) : 0);
  return writer.TakeBytes();
}

std::expected<ConfigValue, Status> DeserializeRawInt64(std::span<const std::byte> data)
{
  psx::ByteReader reader(data);
  return ConfigValue{reader.ReadLong()};
}

std::vector<std::byte> SerializeRawFloat32(const ConfigValue& value)
{
  psx::ByteWriter writer;
  writer.WriteFloat(std::holds_alternative<float>(value) ? std::get<float>(value) : 0.0f);
  return writer.TakeBytes();
}

std::expected<ConfigValue, Status> DeserializeRawFloat32(std::span<const std::byte> data)
{
  psx::ByteReader reader(data);
  return ConfigValue{reader.ReadFloat()};
}

std::vector<std::byte> SerializeRawEnum(const ConfigValue& value)
{
  psx::ByteWriter writer;
  const std::string* name = std::get_if<std::string>(&value);
  writer.WriteAscii(name != nullptr ? *name : std::string{}, 1);
  return writer.TakeBytes();
}

std::expected<ConfigValue, Status> DeserializeRawEnum(std::span<const std::byte> data,
                                                      std::vector<std::string> validNames, std::string defaultValue)
{
  psx::ByteReader reader(data);
  const std::string name = reader.ReadAscii(1);
  if (std::find(validNames.begin(), validNames.end(), name) == validNames.end())
  {
    // Parity: `enumFromBytes(cls, bytes, defaultValue)` falls back on error.
    return ConfigValue{defaultValue};
  }
  return ConfigValue{name};
}

std::vector<std::byte> SerializeRawKeybinds(const ConfigValue& value)
{
  const KeybindValue* keybinds = std::get_if<KeybindValue>(&value);
  KeybindValue sorted = keybinds != nullptr ? *keybinds : KeybindValue{}; // sorted in place
  std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
  psx::ByteWriter writer;
  writer.WriteUShort(static_cast<std::uint16_t>(sorted.size()));
  for (const auto& [actionID, activations] : sorted)
  {
    writer.WriteRegistryId(actionID);
    writer.WriteByte(static_cast<std::uint8_t>(activations.size()));
    for (const auto& activation : activations)
    {
      SerializeActivation(activation, writer);
    }
  }
  return writer.TakeBytes();
}

std::expected<ConfigValue, Status> DeserializeRawKeybinds(std::span<const std::byte> data)
{
  if (data.empty())
  {
    return ConfigValue{KeybindValue{}};
  }
  psx::ByteReader reader(data);
  const std::uint16_t count = reader.ReadUShort();
  KeybindValue keybinds;
  keybinds.reserve(count);
  for (std::uint16_t i = 0; i < count; ++i)
  {
    const std::string actionID = reader.ReadRegistryId();
    const std::uint8_t activationCount = reader.ReadByte();
    std::vector<Activation> activations;
    activations.reserve(activationCount);
    for (std::uint8_t j = 0; j < activationCount; ++j)
    {
      auto activation = DeserializeActivation(reader);
      if (!activation.has_value())
      {
        return std::unexpected(activation.error());
      }
      activations.push_back(*activation);
    }
    // Unknown action IDs are skipped (parity: "Skipping unknown keybind"),
    // but their activations were consumed so the file keeps loading right.
    if (actionID.empty())
    {
      continue;
    }
    keybinds.emplace_back(actionID, std::move(activations));
  }
  if (reader.Failed())
  {
    return std::unexpected(Status::Error);
  }
  return ConfigValue{std::move(keybinds)};
}

} // namespace lod

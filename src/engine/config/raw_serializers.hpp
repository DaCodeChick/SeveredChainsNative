#pragma once
// Raw-value serializers for config entries (engine-internal; not a public
// API header). Parity with the reference per-entry serializer functions and
// `IoHelper` widths:
//   boolean -> 1 byte (0/1)
//   int32   -> 4-byte little-endian
//   int64   -> 8-byte little-endian
//   float32 -> 4-byte little-endian IEEE 754
//   enum    -> 1-byte length + UTF-8 name (IoHelper.enumToBytes)
//   keybinds -> u16 count + (registry ID, u8 count, activations)*
//               (ControllerKeybindsConfigEntry)
//
// These are PSX-format-style binary layouts (guidelines §9.2).

#include "config/config.hpp"

#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace lod
{
/// @name boolean raw serializer (1 byte).
/// @{
[[nodiscard]] std::vector<std::byte> SerializeRawBool(const ConfigValue& value);
[[nodiscard]] std::expected<ConfigValue, Status> DeserializeRawBool(std::span<const std::byte> data);
/// @}

/// @name int32 raw serializer (4-byte LE).
/// @{
[[nodiscard]] std::vector<std::byte> SerializeRawInt32(const ConfigValue& value);
[[nodiscard]] std::expected<ConfigValue, Status> DeserializeRawInt32(std::span<const std::byte> data);
/// @}

/// @name int8 raw serializer (1 byte; parity `MonitorConfigEntry`).
/// @{
[[nodiscard]] std::vector<std::byte> SerializeRawInt8(const ConfigValue& value);
[[nodiscard]] std::expected<ConfigValue, Status> DeserializeRawInt8(std::span<const std::byte> data);
/// @}

/// @name int64 raw serializer (8-byte LE).
/// @{
[[nodiscard]] std::vector<std::byte> SerializeRawInt64(const ConfigValue& value);
[[nodiscard]] std::expected<ConfigValue, Status> DeserializeRawInt64(std::span<const std::byte> data);
/// @}

/// @name float32 raw serializer (4-byte LE).
/// @{
[[nodiscard]] std::vector<std::byte> SerializeRawFloat32(const ConfigValue& value);
[[nodiscard]] std::expected<ConfigValue, Status> DeserializeRawFloat32(std::span<const std::byte> data);
/// @}

/// @name enum raw serializer (1-byte length + name; unknown names fall back
///      to `defaultValue`, parity `IoHelper.enumFromBytes`).
/// @{
[[nodiscard]] std::vector<std::byte> SerializeRawEnum(const ConfigValue& value);
[[nodiscard]] std::expected<ConfigValue, Status>
DeserializeRawEnum(std::span<const std::byte> data, std::vector<std::string> validNames, std::string defaultValue);
/// @}

/// @name keybinds raw serializer (u16 count + entries; unknown action IDs are
///      skipped on read, parity `ControllerKeybindsConfigEntry`).
/// @{
[[nodiscard]] std::vector<std::byte> SerializeRawKeybinds(const ConfigValue& value);
[[nodiscard]] std::expected<ConfigValue, Status> DeserializeRawKeybinds(std::span<const std::byte> data);
/// @}
} // namespace lod
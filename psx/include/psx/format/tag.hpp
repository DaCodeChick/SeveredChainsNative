#pragma once

#include "psx/core/error.hpp"
#include "psx/format/byte_buffer.hpp"

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace psx
{
/// Tag type codes — the PS1/Severed-Chains tag container format
/// (reference: `legend/core/tags/Tag.java` tag-type constants).
///
/// @note Codes are part of the on-disk format; they never change.
enum class TagType : std::uint32_t
{
  // Collections
  Map = 0,
  List = 1,

  // Primitives
  String = 10,
  Int = 11,
  Long = 12,
  Float = 13,
  Bool = 14,
  Raw = 15,

  // Complex types
  RegistryID = 30,
  Enum = 31,
};

/// Payload for `TagType::String` (UTF-8, varint byte-length prefix).
struct StringTag
{
  std::string value; ///< The UTF-8 payload.
};

/// Payload for `TagType::Int` (varint).
struct IntTag
{
  std::int32_t value = 0; ///< The signed payload.
};

/// Payload for `TagType::Long` (64-bit varint).
struct LongTag
{
  std::int64_t value = 0; ///< The signed payload.
};

/// Payload for `TagType::Float` (4-byte little-endian IEEE 754).
struct FloatTag
{
  float value = 0.0f; ///< The payload.
};

/// Payload for `TagType::Bool` (one byte, 0/1).
struct BoolTag
{
  bool value = false; ///< The payload.
};

/// Payload for `TagType::Raw` (varint byte-length prefix + bytes).
struct RawTag
{
  std::vector<std::byte> value; ///< The payload bytes.
};

/// Payload for `TagType::Enum` (UTF-8 string of the enum constant name).
struct EnumTag
{
  std::string value; ///< The enum constant name.
};

/// Payload for `TagType::RegistryID` (3-byte little-endian ASCII length +
/// ASCII; the reference `writeRegistryId` encoding).
struct RegistryIDTag
{
  std::string value; ///< The registry ID string.
};

/// Payload for `TagType::Map`: varint entry count, then per entry
/// (varint type + UTF-8 key + payload). **Insertion-ordered** — deterministic
/// output; the reference writer iterated a hash map, so byte-identical map
/// ordering with reference files is not required (keys are looked up by
/// name, order is irrelevant to readers).
/// @note Forward-declared so `Tag` can reference it (recursive variant).
struct MapTag;

/// Payload for `TagType::List`: varint entry count, then per entry
/// (varint type + payload).
/// @note Forward-declared so `Tag` can reference it (recursive variant).
struct ListTag;

/// A tag: a typed payload (the reference `Tag` interface as a variant —
/// no RTTI needed, `std::visit` dispatch). The recursive members are
/// indirected through `std::vector`.
using Tag =
  std::variant<MapTag, ListTag, StringTag, IntTag, LongTag, FloatTag, BoolTag, RawTag, RegistryIDTag, EnumTag>;

/// Payload for `TagType::Map` (complete definition — see the forward
/// declaration above for the wire layout).
struct MapTag
{
  std::vector<std::pair<std::string, Tag>> entries; ///< The entries, in insertion order.
};

/// Payload for `TagType::List` (complete definition — see the forward
/// declaration above for the wire layout).
struct ListTag
{
  std::vector<Tag> entries; ///< The entries, in insertion order.
};

/// Creates an empty tag of the given type.
///
/// @param type The tag type.
/// @return The empty tag, or `Error::DecodeMalformed` for an unknown code.
[[nodiscard]] std::expected<Tag, Error> MakeTag(TagType type);

/// The type code of a tag.
[[nodiscard]] TagType TypeOf(const Tag& tag);

/// Serializes a tag (payload only — the caller writes the type code and
/// key where the format requires them).
///
/// @param tag The tag.
/// @param writer The writer (non-owning).
void SerializeTag(const Tag& tag, ByteWriter& writer);

/// Serializes a **top-level** tag: type code + payload.
///
/// @param tag The tag.
/// @param writer The writer (non-owning).
void SerializeTagTree(const Tag& tag, ByteWriter& writer);

/// Deserializes a **top-level** tag: type code + payload.
///
/// @param reader The reader (non-owning view; the error state is checked —
///               a truncated buffer yields `Error::DecodeTruncated`).
/// @return The tag, or a decode error (`DecodeMalformed` for unknown type
///         codes, `DecodeTruncated` for short input).
[[nodiscard]] std::expected<Tag, Error> DeserializeTagTree(ByteReader& reader);

/// Finds a child of a map tag by key (accepts a `Tag` or a `MapTag`).
///
/// @param map The map (returns nullptr for a non-map or missing key).
/// @param key The child key.
/// @return The child tag, or nullptr. Lifetime: the child inside `map`.
/// @note Thread: Safe (const query).
/// @note Header template: `constexpr if` dispatch, no RTTI.
template <typename TagOrMap>
[[nodiscard]] const Tag* MapGet(const TagOrMap& map, std::string_view key)
{
  const MapTag* mapTag = nullptr;
  if constexpr (std::is_same_v<std::remove_cv_t<TagOrMap>, Tag>)
  {
    mapTag = std::get_if<MapTag>(&map);
  }
  else // TagOrMap is MapTag
  {
    mapTag = &map;
  }
  if (mapTag == nullptr)
  {
    return nullptr;
  }
  for (const auto& [name, child] : mapTag->entries)
  {
    if (name == key)
    {
      return &child;
    }
  }
  return nullptr;
}

/// Human-readable name of a tag type (diagnostics).
///
/// @param type The tag type.
/// @return A `std::string_view` backed by a process-lifetime static string.
[[nodiscard]] std::string_view NameOf(TagType type);

} // namespace psx

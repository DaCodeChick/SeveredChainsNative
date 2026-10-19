#include "psx/format/tag.hpp"

namespace psx
{
namespace
{

/// Serializes one tag's payload (no type code / key).
void SerializePayload(const Tag& tag, ByteWriter& writer)
{
  std::visit(
    [&writer](const auto& payload)
    {
      using T = std::decay_t<decltype(payload)>;
      if constexpr (std::is_same_v<T, MapTag>)
      {
        writer.WriteVarInt(static_cast<std::int32_t>(payload.entries.size()));
        for (const auto& [key, child] : payload.entries)
        {
          writer.WriteVarInt(static_cast<std::int32_t>(TypeOf(child)));
          writer.WriteString(key);
          SerializePayload(child, writer);
        }
      }
      else if constexpr (std::is_same_v<T, ListTag>)
      {
        writer.WriteVarInt(static_cast<std::int32_t>(payload.entries.size()));
        for (const auto& child : payload.entries)
        {
          writer.WriteVarInt(static_cast<std::int32_t>(TypeOf(child)));
          SerializePayload(child, writer);
        }
      }
      else if constexpr (std::is_same_v<T, StringTag>)
      {
        writer.WriteString(payload.value);
      }
      else if constexpr (std::is_same_v<T, IntTag>)
      {
        writer.WriteVarInt(payload.value);
      }
      else if constexpr (std::is_same_v<T, LongTag>)
      {
        writer.WriteVarLong(payload.value);
      }
      else if constexpr (std::is_same_v<T, FloatTag>)
      {
        writer.WriteFloat(payload.value);
      }
      else if constexpr (std::is_same_v<T, BoolTag>)
      {
        writer.WriteBool(payload.value);
      }
      else if constexpr (std::is_same_v<T, RawTag>)
      {
        writer.WriteVarInt(static_cast<std::int32_t>(payload.value.size()));
        writer.WriteBytes(payload.value);
      }
      else if constexpr (std::is_same_v<T, EnumTag>)
      {
        writer.WriteString(payload.value);
      }
      else // RegistryIDTag
      {
        writer.WriteRegistryId(payload.value);
      }
    },
    tag);
}

/// Deserializes one tag's payload (type code already consumed).
std::expected<void, Error> DeserializePayload(Tag& tag, ByteReader& reader)
{
  const auto result = std::visit(
    [&reader](auto& payload) -> std::expected<void, Error>
    {
      using T = std::decay_t<decltype(payload)>;
      if constexpr (std::is_same_v<T, MapTag>)
      {
        const std::int32_t count = reader.ReadVarInt();
        if (reader.Failed() || count < 0)
        {
          return std::unexpected(Error::DecodeTruncated);
        }
        payload.entries.clear();
        payload.entries.reserve(static_cast<std::size_t>(count));
        for (std::int32_t i = 0; i < count; ++i)
        {
          const std::uint32_t typeCode = static_cast<std::uint32_t>(reader.ReadVarInt());
          const std::string key = reader.ReadString();
          auto child = MakeTag(static_cast<TagType>(typeCode));
          if (!child.has_value())
          {
            return std::unexpected(child.error());
          }
          const auto payloadResult = DeserializePayload(*child, reader);
          if (!payloadResult.has_value())
          {
            return std::unexpected(payloadResult.error());
          }
          payload.entries.emplace_back(std::move(key), std::move(*child));
        }
        return {};
      }
      else if constexpr (std::is_same_v<T, ListTag>)
      {
        const std::int32_t count = reader.ReadVarInt();
        if (reader.Failed() || count < 0)
        {
          return std::unexpected(Error::DecodeTruncated);
        }
        payload.entries.clear();
        payload.entries.reserve(static_cast<std::size_t>(count));
        for (std::int32_t i = 0; i < count; ++i)
        {
          const std::uint32_t typeCode = static_cast<std::uint32_t>(reader.ReadVarInt());
          auto child = MakeTag(static_cast<TagType>(typeCode));
          if (!child.has_value())
          {
            return std::unexpected(child.error());
          }
          const auto payloadResult = DeserializePayload(*child, reader);
          if (!payloadResult.has_value())
          {
            return std::unexpected(payloadResult.error());
          }
          payload.entries.push_back(std::move(*child));
        }
        return {};
      }
      else if constexpr (std::is_same_v<T, StringTag>)
      {
        payload.value = reader.ReadString();
      }
      else if constexpr (std::is_same_v<T, IntTag>)
      {
        payload.value = reader.ReadVarInt();
      }
      else if constexpr (std::is_same_v<T, LongTag>)
      {
        payload.value = reader.ReadVarLong();
      }
      else if constexpr (std::is_same_v<T, FloatTag>)
      {
        payload.value = reader.ReadFloat();
      }
      else if constexpr (std::is_same_v<T, BoolTag>)
      {
        payload.value = reader.ReadBool();
      }
      else if constexpr (std::is_same_v<T, RawTag>)
      {
        const std::int32_t count = reader.ReadVarInt();
        if (reader.Failed() || count < 0)
        {
          return std::unexpected(Error::DecodeTruncated);
        }
        payload.value = reader.ReadBytes(static_cast<std::size_t>(count));
      }
      else if constexpr (std::is_same_v<T, EnumTag>)
      {
        payload.value = reader.ReadString();
      }
      else // RegistryIDTag
      {
        payload.value = reader.ReadRegistryId();
      }
      if (reader.Failed())
      {
        return std::unexpected(Error::DecodeTruncated);
      }
      return {};
    },
    tag);

  if (reader.Failed())
  {
    return std::unexpected(Error::DecodeTruncated);
  }
  return result;
}

} // namespace

std::expected<Tag, Error> MakeTag(TagType type)
{
  switch (type)
  {
    case TagType::Map:
      return MapTag{};
    case TagType::List:
      return ListTag{};
    case TagType::String:
      return StringTag{};
    case TagType::Int:
      return IntTag{};
    case TagType::Long:
      return LongTag{};
    case TagType::Float:
      return FloatTag{};
    case TagType::Bool:
      return BoolTag{};
    case TagType::Raw:
      return RawTag{};
    case TagType::RegistryID:
      return RegistryIDTag{};
    case TagType::Enum:
      return EnumTag{};
  }
  return std::unexpected(Error::DecodeMalformed);
}

TagType TypeOf(const Tag& tag)
{
  return std::visit(
    [](const auto& payload) -> TagType
    {
      using T = std::decay_t<decltype(payload)>;
      if constexpr (std::is_same_v<T, MapTag>)
      {
        return TagType::Map;
      }
      else if constexpr (std::is_same_v<T, ListTag>)
      {
        return TagType::List;
      }
      else if constexpr (std::is_same_v<T, StringTag>)
      {
        return TagType::String;
      }
      else if constexpr (std::is_same_v<T, IntTag>)
      {
        return TagType::Int;
      }
      else if constexpr (std::is_same_v<T, LongTag>)
      {
        return TagType::Long;
      }
      else if constexpr (std::is_same_v<T, FloatTag>)
      {
        return TagType::Float;
      }
      else if constexpr (std::is_same_v<T, BoolTag>)
      {
        return TagType::Bool;
      }
      else if constexpr (std::is_same_v<T, RawTag>)
      {
        return TagType::Raw;
      }
      else if constexpr (std::is_same_v<T, RegistryIDTag>)
      {
        return TagType::RegistryID;
      }
      else // EnumTag
      {
        return TagType::Enum;
      }
    },
    tag);
}

void SerializeTag(const Tag& tag, ByteWriter& writer)
{
  SerializePayload(tag, writer);
}

void SerializeTagTree(const Tag& tag, ByteWriter& writer)
{
  writer.WriteVarInt(static_cast<std::int32_t>(TypeOf(tag)));
  SerializePayload(tag, writer);
}

std::expected<Tag, Error> DeserializeTagTree(ByteReader& reader)
{
  const std::uint32_t typeCode = static_cast<std::uint32_t>(reader.ReadVarInt());
  if (reader.Failed())
  {
    return std::unexpected(Error::DecodeTruncated);
  }
  auto tag = MakeTag(static_cast<TagType>(typeCode));
  if (!tag.has_value())
  {
    return std::unexpected(tag.error());
  }
  const auto result = DeserializePayload(*tag, reader);
  if (!result.has_value())
  {
    return std::unexpected(result.error());
  }
  return *tag;
}

std::string_view NameOf(TagType type)
{
  switch (type)
  {
    case TagType::Map:
      return "map";
    case TagType::List:
      return "list";
    case TagType::String:
      return "string";
    case TagType::Int:
      return "int";
    case TagType::Long:
      return "long";
    case TagType::Float:
      return "float";
    case TagType::Bool:
      return "bool";
    case TagType::Raw:
      return "raw";
    case TagType::RegistryID:
      return "registry_id";
    case TagType::Enum:
      return "enum";
  }
  return "unknown";
}

} // namespace psx

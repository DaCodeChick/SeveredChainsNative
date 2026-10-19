// psx::format tests — byte-level golden checks + round-trips (testing.md §2:
// golden fixtures for formats).
#include "psx_test.hpp"

#include "psx/format/byte_buffer.hpp"
#include "psx/format/tag.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

using namespace psx;

namespace
{

std::string ToText(const std::vector<std::byte>& bytes)
{
  std::string text(bytes.size(), '\0');
  for (std::size_t i = 0; i < bytes.size(); ++i)
  {
    text[i] = static_cast<char>(bytes[i]);
  }
  return text;
}

std::vector<std::byte> ToBytes(const std::string& text)
{
  std::vector<std::byte> bytes(text.size());
  for (std::size_t i = 0; i < text.size(); ++i)
  {
    bytes[i] = std::byte{static_cast<unsigned char>(text[i])};
  }
  return bytes;
}

/// Golden-byte string with explicit length (a raw literal would be
/// NUL-truncated by `strlen` when compared to a `std::string`).
std::string FromBytes(const char* data, std::size_t size)
{
  return std::string(data, size);
}

} // namespace

PSX_TEST(ByteWriter_ReadPrimitivesRoundTrip, "format")
{
  ByteWriter writer;
  writer.WriteInt(-123456);
  writer.WriteUInt(0xdeadbeefu);
  writer.WriteUShort(0x1234);
  writer.WriteBool(true);
  writer.WriteBool(false);
  writer.WriteLong(-9876543210LL);
  writer.WriteFloat(1.5f);

  ByteReader reader(writer.Bytes());
  PSX_CHECK_EQ(reader.ReadInt(), -123456);
  PSX_CHECK_EQ(reader.ReadUInt(), 0xdeadbeefu);
  PSX_CHECK_EQ(reader.ReadUShort(), 0x1234);
  PSX_CHECK(reader.ReadBool());
  PSX_CHECK(!reader.ReadBool());
  PSX_CHECK_EQ(reader.ReadLong(), -9876543210LL);
  PSX_CHECK_EQ(reader.ReadFloat(), 1.5f);
  PSX_CHECK(!reader.Failed());
  return true;
}

PSX_TEST(ByteWriter_GoldenLEBytes, "format")
{
  // Golden: little-endian 32-bit int 0x01020304 -> bytes 04 03 02 01.
  ByteWriter writer;
  writer.WriteInt(0x01020304);
  PSX_CHECK_EQ(ToText(writer.Bytes()), "\x04\x03\x02\x01");
  return true;
}

PSX_TEST(VarInt_GoldenEncoding, "format")
{
  // Golden LEB128 (reference writeVarInt): 0 -> 00; 63 -> 3F; 64 -> 40
  // (fits 7 bits); 16384 -> 80 80 01; 300 -> AC 02.
  auto encode = [](std::int32_t value)
  {
    ByteWriter writer;
    writer.WriteVarInt(value);
    return ToText(writer.Bytes());
  };
  PSX_CHECK_EQ(encode(0), FromBytes("\x00", 1));
  PSX_CHECK_EQ(encode(63), FromBytes("\x3f", 1));
  PSX_CHECK_EQ(encode(64), FromBytes("\x40", 1));
  PSX_CHECK_EQ(encode(16384), FromBytes("\x80" "\x80" "\x01", 3));
  PSX_CHECK_EQ(encode(300), FromBytes("\xac" "\x02", 2));
  PSX_CHECK_EQ(encode(-1), FromBytes("\xff" "\xff" "\xff" "\xff" "\x0f", 5)); // 32-bit bit pattern
  return true;
}

PSX_TEST(VarInt_RoundTrip, "format")
{
  for (const std::int32_t value :
       {0, 1, 63, 64, 127, 128, 300, 16383, 16384, std::numeric_limits<std::int32_t>::max(), -1, -300,
        std::numeric_limits<std::int32_t>::min()})
  {
    ByteWriter writer;
    writer.WriteVarInt(value);
    ByteReader reader(writer.Bytes());
    PSX_CHECK_EQ(reader.ReadVarInt(), value);
    PSX_CHECK(!reader.Failed());
  }
  for (const std::int64_t value : {0LL, 1LL, 300LL, 1LL << 35, -1LL, 0x0123456789abcdefLL})
  {
    ByteWriter writer;
    writer.WriteVarLong(value);
    ByteReader reader(writer.Bytes());
    PSX_CHECK_EQ(reader.ReadVarLong(), value);
    PSX_CHECK(!reader.Failed());
  }
  return true;
}

PSX_TEST(Ascii_ThreeByteLength_Golden, "format")
{
  // "key" -> 03 00 00 6b 65 79
  ByteWriter writer;
  writer.WriteAscii("key");
  PSX_CHECK_EQ(ToText(writer.Bytes()), FromBytes("\x03" "\x00" "\x00" "key", 6));
  ByteReader reader(writer.Bytes());
  PSX_CHECK_EQ(reader.ReadAscii(), "key");
  return true;
}

PSX_TEST(Ascii_OneByteLength_RoundTrip, "format")
{
  ByteWriter writer;
  writer.WriteAscii("scancode", 1);
  writer.WriteAscii("", 1);
  ByteReader reader(writer.Bytes());
  PSX_CHECK_EQ(reader.ReadAscii(1), "scancode");
  PSX_CHECK_EQ(reader.ReadAscii(1), "");
  return true;
}

PSX_TEST(String_UTF8Varint_RoundTrip, "format")
{
  ByteWriter writer;
  writer.WriteString("h\u00e9llo\u2192tag"); // "héllo→tag" (UTF-8)
  ByteReader reader(writer.Bytes());
  PSX_CHECK_EQ(reader.ReadString(), "h\u00e9llo\u2192tag");
  return true;
}

PSX_TEST(RegistryId_GoldenEncoding, "format")
{
  // writeRegistryId("lod_core") -> 08 00 00 "lod_core"
  ByteWriter writer;
  writer.WriteRegistryId("lod_core");
  PSX_CHECK_EQ(ToText(writer.Bytes()), FromBytes("\x08" "\x00" "\x00" "lod_core", 11));
  ByteReader reader(writer.Bytes());
  PSX_CHECK_EQ(reader.ReadRegistryId(), "lod_core");
  return true;
}

PSX_TEST(Reader_TruncationStickyError, "format")
{
  const auto bytes = ToBytes("\x01\x02");
  ByteReader reader(bytes);
  const auto truncatedInt = reader.ReadInt(); // needs 4 bytes, only 2 present
  PSX_CHECK(truncatedInt == 0);
  PSX_CHECK(reader.Failed());
  const auto truncatedByte = reader.ReadByte(); // stays failed, returns default
  PSX_CHECK(truncatedByte == 0);
  PSX_CHECK(reader.Failed());
  return true;
}

PSX_TEST(Tag_MakeTag_UnknownCode, "format")
{
  auto unknown = MakeTag(static_cast<TagType>(99));
  PSX_CHECK(!unknown.has_value());
  PSX_CHECK_EQ(unknown.error(), Error::DecodeMalformed);
  PSX_CHECK(MakeTag(TagType::Map).has_value());
  return true;
}

PSX_TEST(Tag_MapRoundTrip, "format")
{
  MapTag map;
  map.entries.emplace_back("configId", RegistryIDTag{"controller_keybinds"});
  map.entries.emplace_back("valueType", StringTag{"boolean"});
  map.entries.emplace_back("schemaVersion", IntTag{1});
  map.entries.emplace_back("data", RawTag{ToBytes("\x01\x02\x03")});
  map.entries.emplace_back("flag", BoolTag{true});
  map.entries.emplace_back("scale", FloatTag{0.25f});
  map.entries.emplace_back("count", LongTag{1LL << 40});

  ByteWriter writer;
  SerializeTagTree(Tag{map}, writer);

  ByteReader reader(writer.Bytes());
  auto result = DeserializeTagTree(reader);
  PSX_REQUIRE(result.has_value());
  PSX_CHECK(!reader.Failed());
  const MapTag* out = std::get_if<MapTag>(&*result);
  PSX_REQUIRE(out != nullptr);
  PSX_CHECK_EQ(out->entries.size(), 7);
  const Tag* configId = MapGet(*result, "configId");
  PSX_REQUIRE(configId != nullptr);
  PSX_CHECK_EQ(std::get<RegistryIDTag>(*configId).value, "controller_keybinds");
  PSX_CHECK_EQ(std::get<StringTag>(*MapGet(*result, "valueType")).value, "boolean");
  PSX_CHECK_EQ(std::get<IntTag>(*MapGet(*result, "schemaVersion")).value, 1);
  PSX_CHECK_EQ(std::get<RawTag>(*MapGet(*result, "data")).value.size(), 3);
  PSX_CHECK(std::get<BoolTag>(*MapGet(*result, "flag")).value);
  PSX_CHECK_EQ(std::get<FloatTag>(*MapGet(*result, "scale")).value, 0.25f);
  PSX_CHECK_EQ(std::get<LongTag>(*MapGet(*result, "count")).value, 1LL << 40);
  return true;
}

PSX_TEST(Tag_NestedRoundTrip, "format")
{
  // Preset-shaped tree: map{ location: list[ map{...}, map{...} ] }
  MapTag root;
  ListTag items;
  {
    MapTag item;
    item.entries.emplace_back("id", RegistryIDTag{"fullscreen"});
    item.entries.emplace_back("value", BoolTag{true});
    items.entries.push_back(Tag{item});
  }
  {
    MapTag item;
    item.entries.emplace_back("id", RegistryIDTag{"monitor"});
    item.entries.emplace_back("value", IntTag{2});
    items.entries.push_back(Tag{item});
  }
  root.entries.emplace_back("global", Tag{items});

  ByteWriter writer;
  SerializeTagTree(Tag{root}, writer);
  ByteReader reader(writer.Bytes());
  auto result = DeserializeTagTree(reader);
  PSX_REQUIRE(result.has_value());
  const ListTag* list = std::get_if<ListTag>(MapGet(*result, "global"));
  PSX_REQUIRE(list != nullptr);
  PSX_CHECK_EQ(list->entries.size(), 2);
  const MapTag* second = std::get_if<MapTag>(&list->entries[1]);
  PSX_REQUIRE(second != nullptr);
  PSX_CHECK_EQ(std::get<RegistryIDTag>(*MapGet(*second, "id")).value, "monitor");
  PSX_CHECK_EQ(std::get<IntTag>(*MapGet(*second, "value")).value, 2);
  return true;
}

PSX_TEST(Tag_GoldenMapBytes, "format")
{
  // Golden bytes for map{ "a": int 1 }:
  //   varint(0)=map, varint(1)=1 entry, varint(11)=int, "a" (varint 1 + 'a'),
  //   varint(1)
  ByteWriter writer;
  MapTag map;
  map.entries.emplace_back("a", IntTag{1});
  SerializeTagTree(Tag{map}, writer);
  PSX_CHECK_EQ(ToText(writer.Bytes()), FromBytes("\x00" "\x01" "\x0b" "\x01" "a" "\x01", 6));
  return true;
}

PSX_TEST(Tag_TruncatedTree, "format")
{
  // "map with 2 entries" header but only one entry present.
  const auto bytes = ToBytes("\x00\x02\x0b\x01a\x01");
  ByteReader reader(bytes);
  auto result = DeserializeTagTree(reader);
  PSX_CHECK(!result.has_value());
  PSX_CHECK_EQ(result.error(), Error::DecodeTruncated);
  return true;
}
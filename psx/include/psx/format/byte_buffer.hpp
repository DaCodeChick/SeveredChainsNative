#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace psx
{
/// Append-style little-endian byte writer (the write side of the reference
/// project's `FileData` primitives — `legend/game/unpacker/FileData.java`).
///
/// All multi-byte values are little-endian, matching the reference reader.
/// This is low-level binary-format work (guidelines §9.2).
class ByteWriter
{
public:
  /// Appends one byte.
  void WriteByte(std::uint8_t value);

  /// Appends a boolean as one byte (0 or 1).
  void WriteBool(bool value);

  /// Appends a 16-bit unsigned value, little-endian.
  void WriteUShort(std::uint16_t value);

  /// Appends a 32-bit signed value, little-endian.
  void WriteInt(std::int32_t value);

  /// Appends a 32-bit unsigned value, little-endian.
  void WriteUInt(std::uint32_t value);

  /// Appends a 64-bit signed value, little-endian.
  void WriteLong(std::int64_t value);

  /// Appends a 32-bit IEEE 754 float (raw bits), little-endian.
  void WriteFloat(float value);

  /// Appends an unsigned LEB128 varint (7 bits per byte, MSB =
  /// continuation). Byte-identical to the reference writer for all
  /// non-negative values (the only domain the formats use); the bit pattern
  /// of negative 32-bit values round-trips exactly as well.
  void WriteVarInt(std::int32_t value);

  /// Appends a 64-bit varint (7 bits per byte, MSB = continuation).
  void WriteVarLong(std::int64_t value);

  /// Appends a length-prefixed ASCII string: `lengthSize` little-endian
  /// bytes of length followed by the string bytes. The reference default is
  /// 3-byte length (e.g. registry IDs); keybind activations use 1.
  ///
  /// @param value The string (ASCII range expected).
  /// @param lengthSize Length-prefix size in bytes (1 or 3).
  void WriteAscii(std::string_view value, int lengthSize = 3);

  /// Appends a UTF-8 string with a varint byte-length prefix.
  void WriteString(std::string_view value);

  /// Appends a registry ID: 3-byte little-endian ASCII length + ASCII (the
  /// reference `writeRegistryId` encoding; the empty ID encodes length 0).
  void WriteRegistryId(std::string_view id);

  /// Appends raw bytes.
  void WriteBytes(std::span<const std::byte> bytes);

  /// The bytes written so far.
  [[nodiscard]] const std::vector<std::byte>& Bytes() const;

  /// Moves out the written bytes (resets the writer).
  [[nodiscard]] std::vector<std::byte> TakeBytes();

private:
  void WriteLE(std::uint64_t value, int size);

  std::vector<std::byte> data_;
};

/// Read-only little-endian byte reader (the read side of `FileData`).
///
/// The reader carries a **sticky error state**: any out-of-range read sets
/// `Failed()` and subsequent reads return defaults without advancing. Callers
/// surface errors through `std::expected` (see `DeserializeTag`); this keeps
/// the no-exceptions policy with readable parse loops.
class ByteReader
{
public:
  /// @param data The buffer to read from (non-owning view).
  explicit ByteReader(std::span<const std::byte> data);

  /// Whether any read so far failed (truncated input).
  [[nodiscard]] bool Failed() const;

  /// Reads one byte.
  [[nodiscard]] std::uint8_t ReadByte();

  /// Reads a boolean encoded as one byte (0 or 1).
  [[nodiscard]] bool ReadBool();

  /// Reads a 16-bit unsigned value, little-endian.
  [[nodiscard]] std::uint16_t ReadUShort();

  /// Reads a 32-bit signed value, little-endian.
  [[nodiscard]] std::int32_t ReadInt();

  /// Reads a 32-bit unsigned value, little-endian.
  [[nodiscard]] std::uint32_t ReadUInt();

  /// Reads a 64-bit signed value, little-endian.
  [[nodiscard]] std::int64_t ReadLong();

  /// Reads a 32-bit IEEE 754 float (raw bits), little-endian.
  [[nodiscard]] float ReadFloat();

  /// Reads a varint (7 bits per byte, MSB = continuation).
  [[nodiscard]] std::int32_t ReadVarInt();

  /// Reads a 64-bit varint.
  [[nodiscard]] std::int64_t ReadVarLong();

  /// Reads a length-prefixed ASCII string.
  ///
  /// @param lengthSize Length-prefix size in bytes (must match the writer).
  [[nodiscard]] std::string ReadAscii(int lengthSize = 3);

  /// Reads a UTF-8 string with a varint byte-length prefix.
  [[nodiscard]] std::string ReadString();

  /// Reads a registry ID (3-byte little-endian ASCII length + ASCII).
  [[nodiscard]] std::string ReadRegistryId();

  /// Reads `count` raw bytes (empty result on failure).
  [[nodiscard]] std::vector<std::byte> ReadBytes(std::size_t count);

  /// The number of bytes consumed so far.
  [[nodiscard]] std::size_t Offset() const;

  /// The total buffer size.
  [[nodiscard]] std::size_t Size() const;

private:
  bool Check(std::size_t count);
  std::uint64_t ReadLE(int size);

  std::span<const std::byte> data_;
  std::size_t offset_ = 0;
  bool failed_ = false;
};

} // namespace psx
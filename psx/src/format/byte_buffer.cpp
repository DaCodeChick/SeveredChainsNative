#include "psx/format/byte_buffer.hpp"

#include <cstring>

namespace psx
{
void ByteWriter::WriteByte(std::uint8_t value)
{
  data_.push_back(std::byte{value});
}

void ByteWriter::WriteBool(bool value)
{
  WriteByte(value ? 1 : 0);
}

void ByteWriter::WriteUShort(std::uint16_t value)
{
  WriteLE(value, 2);
}

void ByteWriter::WriteInt(std::int32_t value)
{
  WriteLE(static_cast<std::uint32_t>(value), 4);
}

void ByteWriter::WriteUInt(std::uint32_t value)
{
  WriteLE(value, 4);
}

void ByteWriter::WriteLong(std::int64_t value)
{
  WriteLE(static_cast<std::uint64_t>(value), 8);
}

void ByteWriter::WriteFloat(float value)
{
  std::uint32_t bits = 0;
  std::memcpy(&bits, &value, sizeof(bits));
  WriteLE(bits, 4);
}

void ByteWriter::WriteVarInt(std::int32_t value)
{
  // Unsigned-shift semantics on the bit pattern (reference: `val >>>= 7`).
  std::uint32_t v = static_cast<std::uint32_t>(value);
  do
  {
    const int bits = static_cast<int>(v & 0x7f);
    v >>= 7;
    WriteByte(static_cast<std::uint8_t>(bits + (v != 0 ? 0x80 : 0)));
  } while (v != 0);
}

void ByteWriter::WriteVarLong(std::int64_t value)
{
  std::uint64_t v = static_cast<std::uint64_t>(value);
  do
  {
    const int bits = static_cast<int>(v & 0x7f);
    v >>= 7;
    WriteByte(static_cast<std::uint8_t>(bits + (v != 0 ? 0x80 : 0)));
  } while (v != 0);
}

void ByteWriter::WriteAscii(std::string_view value, int lengthSize)
{
  std::uint64_t length = value.size();
  WriteLE(length, lengthSize);
  for (const char c : value)
  {
    WriteByte(static_cast<std::uint8_t>(c));
  }
}

void ByteWriter::WriteString(std::string_view value)
{
  WriteVarInt(static_cast<std::int32_t>(value.size()));
  for (const char c : value)
  {
    WriteByte(static_cast<std::uint8_t>(c));
  }
}

void ByteWriter::WriteRegistryId(std::string_view id)
{
  WriteAscii(id, 3);
}

void ByteWriter::WriteBytes(std::span<const std::byte> bytes)
{
  data_.insert(data_.end(), bytes.begin(), bytes.end());
}

const std::vector<std::byte>& ByteWriter::Bytes() const
{
  return data_;
}

std::vector<std::byte> ByteWriter::TakeBytes()
{
  return std::move(data_);
}

void ByteWriter::WriteLE(std::uint64_t value, int size)
{
  for (int i = 0; i < size; ++i)
  {
    WriteByte(static_cast<std::uint8_t>((value >> (8 * i)) & 0xff));
  }
}

ByteReader::ByteReader(std::span<const std::byte> data) : data_(data)
{
}

bool ByteReader::Failed() const
{
  return failed_;
}

std::uint8_t ByteReader::ReadByte()
{
  if (!Check(1))
  {
    return 0;
  }
  return static_cast<std::uint8_t>(data_[offset_++]);
}

bool ByteReader::ReadBool()
{
  return ReadByte() != 0;
}

std::uint16_t ByteReader::ReadUShort()
{
  return static_cast<std::uint16_t>(ReadLE(2));
}

std::int32_t ByteReader::ReadInt()
{
  return static_cast<std::int32_t>(ReadLE(4));
}

std::uint32_t ByteReader::ReadUInt()
{
  return static_cast<std::uint32_t>(ReadLE(4));
}

std::int64_t ByteReader::ReadLong()
{
  return static_cast<std::int64_t>(ReadLE(8));
}

float ByteReader::ReadFloat()
{
  const std::uint32_t bits = static_cast<std::uint32_t>(ReadLE(4));
  float value = 0.0f;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}

std::int32_t ByteReader::ReadVarInt()
{
  std::uint32_t result = 0;
  int shift = 0;
  for (int group = 0; group < 5; ++group)
  {
    const std::uint8_t b = ReadByte();
    result |= static_cast<std::uint32_t>(b & 0x7f) << shift;
    if ((b & 0x80) == 0)
    {
      break;
    }
    shift += 7;
  }
  return static_cast<std::int32_t>(result);
}

std::int64_t ByteReader::ReadVarLong()
{
  std::uint64_t result = 0;
  int shift = 0;
  for (int group = 0; group < 10; ++group)
  {
    const std::uint8_t b = ReadByte();
    result |= static_cast<std::uint64_t>(b & 0x7f) << shift;
    if ((b & 0x80) == 0)
    {
      break;
    }
    shift += 7;
  }
  return static_cast<std::int64_t>(result);
}

std::string ByteReader::ReadAscii(int lengthSize)
{
  const std::uint64_t length = ReadLE(lengthSize);
  if (failed_ || length > data_.size() - offset_)
  {
    failed_ = true;
    return {};
  }
  std::string result(static_cast<std::size_t>(length), '\0');
  for (std::size_t i = 0; i < length; ++i)
  {
    result[i] = static_cast<char>(data_[offset_ + i]);
  }
  offset_ += length;
  return result;
}

std::string ByteReader::ReadString()
{
  const std::int32_t length = ReadVarInt();
  if (failed_ || length < 0 || static_cast<std::uint64_t>(length) > data_.size() - offset_)
  {
    failed_ = true;
    return {};
  }
  std::string result(static_cast<std::size_t>(length), '\0');
  for (std::size_t i = 0; i < static_cast<std::size_t>(length); ++i)
  {
    result[i] = static_cast<char>(data_[offset_ + i]);
  }
  offset_ += static_cast<std::size_t>(length);
  return result;
}

std::string ByteReader::ReadRegistryId()
{
  return ReadAscii(3);
}

std::vector<std::byte> ByteReader::ReadBytes(std::size_t count)
{
  if (!Check(count))
  {
    return {};
  }
  std::vector<std::byte> result(data_.begin() + offset_, data_.begin() + offset_ + count);
  offset_ += count;
  return result;
}

std::size_t ByteReader::Offset() const
{
  return offset_;
}

std::size_t ByteReader::Size() const
{
  return data_.size();
}

bool ByteReader::Check(std::size_t count)
{
  if (failed_ || count > data_.size() - offset_)
  {
    failed_ = true;
    return false;
  }
  return true;
}

std::uint64_t ByteReader::ReadLE(int size)
{
  if (!Check(static_cast<std::size_t>(size)))
  {
    return 0;
  }
  std::uint64_t result = 0;
  for (int i = 0; i < size; ++i)
  {
    result |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(data_[offset_ + i])) << (8 * i);
  }
  offset_ += static_cast<std::size_t>(size);
  return result;
}

} // namespace psx

#include "config/config.hpp"

#include "config/raw_serializers.hpp"
#include "psx/format/byte_buffer.hpp"
#include "psx/format/tag.hpp"
#include "util/log.hpp"
#include "util/status.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace lod
{
namespace
{
/// Reads a whole file into bytes (no-throw filesystem).
///
/// @param file The path.
/// @param out The destination buffer.
/// @return `Status::Ok`, `Status::NotFound`, or `Status::Error`.
[[nodiscard]] Status ReadWholeFile(const std::filesystem::path& file, std::vector<std::byte>& out)
{
  std::error_code ec;
  std::filesystem::file_status status = std::filesystem::status(file, ec);
  if (ec || !std::filesystem::exists(status))
  {
    return Status::NotFound;
  }
  std::ifstream stream(file, std::ios::binary);
  if (!stream)
  {
    return Status::Error;
  }
  stream.seekg(0, std::ios::end);
  const std::streamoff size = stream.tellg();
  stream.seekg(0, std::ios::beg);
  out.resize(static_cast<std::size_t>(size));
  if (size > 0)
  {
    stream.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(size));
  }
  if (!stream)
  {
    return Status::Error;
  }
  return Status::Ok;
}

/// Writes a whole file (creating parent directories; no-throw filesystem).
[[nodiscard]] Status WriteWholeFile(const std::filesystem::path& file, std::span<const std::byte> bytes)
{
  std::error_code ec;
  if (const std::filesystem::path parent = file.parent_path(); !parent.empty())
  {
    std::filesystem::create_directories(parent, ec);
  }
  std::ofstream stream(file, std::ios::binary | std::ios::trunc);
  if (!stream)
  {
    return Status::Error;
  }
  if (!bytes.empty())
  {
    stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  }
  stream.flush();
  return stream ? Status::Ok : Status::Error;
}

// --- Per-value-type raw serializers live in raw_serializers.hpp/.cpp ---

/// Builds a preset item map for one entry (parity:
/// `ConfigStorage.saveConfig(configs, storageLocation, MapTag)` item shape).
void AppendPresetItem(psx::ListTag& items, const ConfigEntry& entry, const ConfigValue& value)
{
  psx::MapTag item;
  item.entries.emplace_back("configId", psx::RegistryIDTag{entry.id});
  item.entries.emplace_back("valueType", psx::StringTag{entry.presetValueType});
  item.entries.emplace_back("schemaVersion", psx::IntTag{entry.presetSchemaVersion});
  std::vector<std::byte> raw = entry.serializer != nullptr ? entry.serializer(value) : std::vector<std::byte>{};
  item.entries.emplace_back("data", psx::RawTag{std::move(raw)});
  items.entries.push_back(psx::Tag{item});
}

} // namespace

std::string_view NameOf(ConfigStorageLocation location)
{
  switch (location)
  {
    case ConfigStorageLocation::Save:
      return "save";
    case ConfigStorageLocation::Campaign:
      return "campaign";
    case ConfigStorageLocation::Global:
      return "global";
  }
  return {};
}

std::expected<void, Status> ConfigCollection::Register(const ConfigEntry& entry)
{
  if (entry.id.empty() || entry.serializer == nullptr || entry.deserializer == nullptr)
  {
    return std::unexpected(Status::InvalidArgument);
  }
  if (FindEntryImpl(entry.id) != nullptr)
  {
    return std::unexpected(Status::AlreadyExists);
  }
  entries_.push_back(entry);
  values_.push_back(entry.defaultValue);
  set_.push_back(false);
  return {};
}

const ConfigEntry* ConfigCollection::FindEntry(std::string_view id) const
{
  return FindEntryImpl(id);
}

const ConfigEntry* ConfigCollection::FindEntryImpl(std::string_view id) const
{
  for (std::size_t i = 0; i < entries_.size(); ++i)
  {
    if (entries_[i].id == id)
    {
      return &entries_[i];
    }
  }
  return nullptr;
}

const ConfigValue& ConfigCollection::Get(std::string_view id) const
{
  const ConfigEntry* entry = FindEntryImpl(id);
  // Entries are always registered before use (API contract); the static
  // default covers the (caller-bug) missing case without trapping.
  static const ConfigValue missing{};
  if (entry == nullptr)
  {
    return missing;
  }
  const std::size_t index = static_cast<std::size_t>(entry - entries_.data());
  return values_[index];
}

void ConfigCollection::Set(std::string_view id, const ConfigValue& value)
{
  const ConfigEntry* entry = FindEntryImpl(id);
  if (entry == nullptr)
  {
    return;
  }
  const std::size_t index = static_cast<std::size_t>(entry - entries_.data());
  values_[index] = value;
  set_[index] = true;
}

void ConfigCollection::ClearLocation(ConfigStorageLocation location)
{
  for (std::size_t i = 0; i < entries_.size(); ++i)
  {
    if (entries_[i].location == location)
    {
      values_[i] = entries_[i].defaultValue;
      set_[i] = false;
    }
  }
}

std::span<const ConfigEntry> ConfigCollection::Entries() const
{
  return entries_;
}

bool ConfigCollection::GetBool(std::string_view id, bool fallback) const
{
  const ConfigValue* value = std::addressof(Get(id));
  const bool* stored = std::get_if<bool>(value);
  return stored != nullptr ? *stored : fallback;
}

std::int32_t ConfigCollection::GetInt(std::string_view id, std::int32_t fallback) const
{
  const ConfigValue* value = std::addressof(Get(id));
  const std::int32_t* stored = std::get_if<std::int32_t>(value);
  return stored != nullptr ? *stored : fallback;
}

std::int64_t ConfigCollection::GetLong(std::string_view id, std::int64_t fallback) const
{
  const ConfigValue* value = std::addressof(Get(id));
  const std::int64_t* stored = std::get_if<std::int64_t>(value);
  return stored != nullptr ? *stored : fallback;
}

float ConfigCollection::GetFloat(std::string_view id, float fallback) const
{
  const ConfigValue* value = std::addressof(Get(id));
  const float* stored = std::get_if<float>(value);
  return stored != nullptr ? *stored : fallback;
}

std::string ConfigCollection::GetString(std::string_view id, std::string_view fallback) const
{
  const ConfigValue* value = std::addressof(Get(id));
  const std::string* stored = std::get_if<std::string>(value);
  return stored != nullptr ? *stored : std::string(fallback);
}

const KeybindValue* ConfigCollection::GetKeybinds(std::string_view id) const
{
  return std::get_if<KeybindValue>(&Get(id));
}

std::expected<void, Status> LoadGlobalConfig(ConfigCollection& configs, const std::filesystem::path& file)
{
  Logger::Instance().Log(LogLevel::Info, "config", "loading config from {}", file.string());
  std::vector<std::byte> data;
  const Status read = ReadWholeFile(file, data);
  if (read == Status::NotFound)
  {
    configs.ClearLocation(ConfigStorageLocation::Global);
    return {};
  }
  if (read != Status::Ok)
  {
    Logger::Instance().Log(LogLevel::Warning, "config", "failed to read config file {}", file.string());
    return std::unexpected(Status::Error);
  }

  psx::ByteReader reader(data);
  const std::int32_t count = reader.ReadInt();
  if (reader.Failed() || count < 0)
  {
    Logger::Instance().Log(LogLevel::Warning, "config", "corrupt config file {}", file.string());
    return std::unexpected(Status::Error);
  }
  for (std::int32_t i = 0; i < count; ++i)
  {
    const std::string id = reader.ReadRegistryId();
    const std::int32_t length = reader.ReadInt();
    if (reader.Failed() || length < 0)
    {
      Logger::Instance().Log(LogLevel::Warning, "config", "corrupt config entry in {}", file.string());
      return std::unexpected(Status::Error);
    }
    const std::vector<std::byte> raw = reader.ReadBytes(static_cast<std::size_t>(length));
    if (reader.Failed())
    {
      return std::unexpected(Status::Error);
    }
    const ConfigEntry* entry = configs.FindEntry(id);
    if (entry == nullptr)
    {
      Logger::Instance().Log(LogLevel::Warning, "config", "unknown config ID {}", id);
      continue;
    }
    if (entry->location != ConfigStorageLocation::Global)
    {
      continue;
    }
    auto value = entry->deserializer(raw);
    if (!value.has_value())
    {
      Logger::Instance().Log(LogLevel::Warning, "config", "failed to decode config ID {}", id);
      continue;
    }
    configs.Set(id, *value);
  }
  return {};
}

std::expected<void, Status> SaveGlobalConfig(const ConfigCollection& configs, const std::filesystem::path& file)
{
  Logger::Instance().Log(LogLevel::Info, "config", "saving config to {}", file.string());
  psx::ByteWriter writer;

  // The header is an int32 count written up front, so two passes.
  std::int32_t count = 0;
  for (const ConfigEntry& entry : configs.Entries())
  {
    if (entry.location == ConfigStorageLocation::Global)
    {
      ++count;
    }
  }
  writer.WriteInt(count);
  for (const ConfigEntry& entry : configs.Entries())
  {
    if (entry.location != ConfigStorageLocation::Global)
    {
      continue;
    }
    const ConfigValue& value = configs.Get(entry.id);
    const std::vector<std::byte> raw = entry.serializer(value);
    writer.WriteRegistryId(entry.id);
    writer.WriteInt(static_cast<std::int32_t>(raw.size()));
    writer.WriteBytes(raw);
  }
  const Status write = WriteWholeFile(file, writer.Bytes());
  if (write != Status::Ok)
  {
    Logger::Instance().Log(LogLevel::Warning, "config", "failed to save config file {}", file.string());
  }
  return write == Status::Ok ? std::expected<void, Status>{} : std::unexpected(write);
}

std::expected<void, Status> LoadPresetConfig(ConfigCollection& configs, ConfigStorageLocation location,
                                             const std::filesystem::path& file)
{
  Logger::Instance().Log(LogLevel::Info, "config", "loading preset for {} from {}", NameOf(location), file.string());
  std::vector<std::byte> data;
  const Status read = ReadWholeFile(file, data);
  if (read == Status::NotFound)
  {
    return {};
  }
  if (read != Status::Ok)
  {
    return std::unexpected(Status::Error);
  }
  psx::ByteReader reader(data);
  auto root = psx::DeserializeTagTree(reader);
  if (!root.has_value())
  {
    Logger::Instance().Log(LogLevel::Warning, "config", "corrupt preset file {}", file.string());
    return std::unexpected(Status::Error);
  }
  const psx::Tag* locationTag = psx::MapGet(*root, NameOf(location));
  if (locationTag == nullptr)
  {
    return {};
  }
  const psx::ListTag* items = std::get_if<psx::ListTag>(locationTag);
  if (items == nullptr)
  {
    return std::unexpected(Status::Error);
  }
  int32_t skipped = 0;
  for (const psx::Tag& itemTag : items->entries)
  {
    const psx::MapTag* item = std::get_if<psx::MapTag>(&itemTag);
    if (item == nullptr)
    {
      ++skipped;
      continue;
    }
    const psx::Tag* idTag = psx::MapGet(*item, "configId");
    const std::string id = idTag != nullptr ? std::get<psx::RegistryIDTag>(*idTag).value : std::string{};
    const ConfigEntry* entry = configs.FindEntry(id);
    if (entry == nullptr || entry->location != location)
    {
      ++skipped;
      continue;
    }
    // Schema compatibility (parity: loadConfigValue — legacy items without
    // valueType/schemaVersion pass).
    const psx::Tag* typeTag = psx::MapGet(*item, "valueType");
    const psx::Tag* schemaTag = psx::MapGet(*item, "schemaVersion");
    if (typeTag != nullptr || schemaTag != nullptr)
    {
      const std::string valueType = typeTag != nullptr ? std::get<psx::StringTag>(*typeTag).value : std::string{};
      const std::int32_t schemaVersion = schemaTag != nullptr ? std::get<psx::IntTag>(*schemaTag).value : 0;
      if (valueType != entry->presetValueType || schemaVersion <= 0 || schemaVersion != entry->presetSchemaVersion)
      {
        Logger::Instance().Log(LogLevel::Warning, "config", "skipping preset setting {} (incompatible schema)", id);
        ++skipped;
        continue;
      }
    }
    const psx::Tag* dataTag = psx::MapGet(*item, "data");
    if (dataTag == nullptr)
    {
      ++skipped;
      continue;
    }
    const psx::RawTag& raw = std::get<psx::RawTag>(*dataTag);
    auto value = entry->deserializer(raw.value);
    if (!value.has_value())
    {
      ++skipped;
      continue;
    }
    configs.Set(id, *value);
  }
  if (skipped != 0)
  {
    Logger::Instance().Log(LogLevel::Warning, "config", "skipped {} preset setting(s) for {}", skipped,
                           NameOf(location));
  }
  return {};
}

std::expected<void, Status> SavePresetConfig(const ConfigCollection& configs, ConfigStorageLocation location,
                                             const std::filesystem::path& file)
{
  Logger::Instance().Log(LogLevel::Info, "config", "saving preset for {} to {}", NameOf(location), file.string());
  psx::MapTag root;
  psx::ListTag items;
  for (const ConfigEntry& entry : configs.Entries())
  {
    if (entry.location != location)
    {
      continue;
    }
    AppendPresetItem(items, entry, configs.Get(entry.id));
  }
  root.entries.emplace_back(std::string(NameOf(location)), psx::Tag{items});
  psx::ByteWriter writer;
  psx::SerializeTagTree(psx::Tag{root}, writer);
  const Status write = WriteWholeFile(file, writer.Bytes());
  if (write != Status::Ok)
  {
    Logger::Instance().Log(LogLevel::Warning, "config", "failed to save preset file {}", file.string());
  }
  return write == Status::Ok ? std::expected<void, Status>{} : std::unexpected(write);
}

} // namespace lod
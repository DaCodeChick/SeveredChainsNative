#pragma once

#include "platform/activation.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace lod
{
/// Engine-wide status codes.
enum class Status : std::int32_t;

/// Where a config value is stored (parity: `ConfigStorageLocation` —
/// SAVE/CAMPAIGN/GLOBAL).
enum class ConfigStorageLocation : std::int32_t
{
  /// Stored per save file (different saves all have different values).
  Save = 0,
  /// Stored per campaign file (every save in the campaign has the same value).
  Campaign = 1,
  /// Stored globally (always has the same value).
  Global = 2
};

/// Name of a storage location (diagnostics + preset file keys: "save",
/// "campaign", "global" — the reference enum constant names lowercased).
///
/// @param location The location.
/// @return A `std::string_view` backed by a process-lifetime static string.
[[nodiscard]] std::string_view NameOf(ConfigStorageLocation location);

/// @note `KeybindValue` is defined in `platform/activation.hpp` (pure input
///       data; this header re-exports it via the include).

/// A config value (type-erased; the entry's preset value type says which
/// alternative is meaningful — parity: the reference `ConfigEntry<T>`
/// generic storage).
using ConfigValue = std::variant<std::monostate, bool, std::int32_t, std::int64_t, float, std::string, KeybindValue>;

/// A config entry: registry ID, storage location, default value, preset
/// compatibility metadata, and the raw-value serializer/deserializer
/// (parity: `ConfigEntry<T>` — the reference stores per-entry
/// `Function<T, byte[]>`/`Function<byte[], T>`).
struct ConfigEntry
{
  std::string id;                                                 ///< Lowercase registry ID (id grammar).
  ConfigStorageLocation location = ConfigStorageLocation::Global; ///< Storage location.
  ConfigValue defaultValue;                                       ///< Default value.
  /// Stable preset value category (parity `getPresetValueType`): "boolean",
  /// "int32", "int64", "float32", "string", "enum", "keybinds".
  std::string presetValueType = "opaque";
  /// Positive version of this entry's preset encoding (readers require an
  /// exact match when present).
  std::int32_t presetSchemaVersion = 1;
  /// Serializes a value to its raw storage bytes.
  std::function<std::vector<std::byte>(const ConfigValue&)> serializer;
  /// Deserializes raw storage bytes to a value.
  std::function<std::expected<ConfigValue, Status>(std::span<const std::byte>)> deserializer;
};

/// Config entries + their current values (parity: `ConfigCollection`).
///
/// Registration order is preserved; values default to the entry's
/// `defaultValue` until set (or loaded from a file).
class ConfigCollection
{
public:
  /// Registers an entry.
  ///
  /// @param entry The entry (copied).
  /// @return `Status::Ok`, or `Status::AlreadyExists` for a duplicate ID.
  [[nodiscard]] std::expected<void, Status> Register(const ConfigEntry& entry);

  /// Finds an entry by ID.
  ///
  /// @param id The entry ID.
  /// @return The entry, or nullptr. Lifetime: this collection.
  [[nodiscard]] const ConfigEntry* FindEntry(std::string_view id) const;

  /// The stored value for an ID (the default when never set).
  ///
  /// @param id The entry ID (must be registered).
  /// @return The value. Lifetime: this collection.
  [[nodiscard]] const ConfigValue& Get(std::string_view id) const;

  /// Sets a value (entry must be registered; type mismatches are left
  /// stored and surface as accessor fallbacks).
  void Set(std::string_view id, const ConfigValue& value);

  /// Clears the stored (non-default) values of entries at a location.
  void ClearLocation(ConfigStorageLocation location);

  /// All registered entries, in registration order.
  [[nodiscard]] std::span<const ConfigEntry> Entries() const;

  /// Typed bool accessor (the entry's default when unset or type-mismatched).
  ///
  /// @param id The entry ID (must be registered).
  /// @param fallback The value when unset or type-mismatched.
  [[nodiscard]] bool GetBool(std::string_view id, bool fallback = false) const;
  /// Typed int32 accessor (the entry's default when unset or type-mismatched).
  ///
  /// @param id The entry ID (must be registered).
  /// @param fallback The value when unset or type-mismatched.
  [[nodiscard]] std::int32_t GetInt(std::string_view id, std::int32_t fallback = 0) const;
  /// Typed int64 accessor (the entry's default when unset or type-mismatched).
  ///
  /// @param id The entry ID (must be registered).
  /// @param fallback The value when unset or type-mismatched.
  [[nodiscard]] std::int64_t GetLong(std::string_view id, std::int64_t fallback = 0) const;
  /// Typed float32 accessor (the entry's default when unset or type-mismatched).
  ///
  /// @param id The entry ID (must be registered).
  /// @param fallback The value when unset or type-mismatched.
  [[nodiscard]] float GetFloat(std::string_view id, float fallback = 0.0f) const;
  /// Typed string accessor (the entry's default when unset or type-mismatched).
  ///
  /// @param id The entry ID (must be registered).
  /// @param fallback The value when unset or type-mismatched.
  [[nodiscard]] std::string GetString(std::string_view id, std::string_view fallback = {}) const;
  /// @return The keybinds value, or nullptr when the entry holds none.
  [[nodiscard]] const KeybindValue* GetKeybinds(std::string_view id) const;

private:
  const ConfigEntry* FindEntryImpl(std::string_view id) const;

  std::vector<ConfigEntry> entries_;
  std::vector<ConfigValue> values_; ///< Parallel to `entries_` (set = true slot means loaded/stored).
  std::vector<bool> set_{};         ///< Parallel to `entries_`.
};

/// Loads the **global** config file (Tag-based `config.dcnf` binary layout —
/// parity `ConfigStorage.loadConfig(configs, location, data)`):
/// `int32 LE count`, then per entry: registry ID (3-byte LE ASCII length +
/// ASCII), `int32 LE` value length, raw value bytes.
///
/// A missing file is a no-op (parity: clear + return); a corrupted file logs
/// and returns `Status::Error` without touching the collection.
///
/// @param configs The collection (non-owning; must already be registered).
/// @param file The config file path.
/// @return `Status::Ok` (including the no-op case), or `Status::Error`.
/// @note Thread: Main thread (file IO).
[[nodiscard]] std::expected<void, Status> LoadGlobalConfig(ConfigCollection& configs,
                                                           const std::filesystem::path& file);

/// Saves the **global** config file (entries at `ConfigStorageLocation::Global`,
/// registration order — the reference iterated a hash map; ordering within a
/// location is not load-relevant since entries are keyed by ID).
///
/// @param configs The collection (non-owning).
/// @param file The config file path (parent directories are created).
/// @return `Status::Ok`, or `Status::Error` on IO failure.
/// @note Thread: Main thread (file IO).
[[nodiscard]] std::expected<void, Status> SaveGlobalConfig(const ConfigCollection& configs,
                                                           const std::filesystem::path& file);

/// Loads a **preset** file (Tag-serialized — parity
/// `ConfigStorage.loadConfig(configs, location, MapTag)`): a top-level map
/// with one key per location (`"global"` etc.), each a list of maps
/// `{ "configId": RegistryIDTag, "valueType": StringTag, "schemaVersion":
/// IntTag, "data": RawTag }`. Entries whose `valueType`/`schemaVersion`
/// (when present) do not match the registered entry are skipped (parity:
/// the "skipped settings" behavior).
///
/// @param configs The collection (non-owning).
/// @param location The location the preset applies to.
/// @param file The preset file path (missing file = no-op).
/// @return `Status::Ok` (including the no-op case), or `Status::Error`.
/// @note Thread: Main thread (file IO).
[[nodiscard]] std::expected<void, Status> LoadPresetConfig(ConfigCollection& configs, ConfigStorageLocation location,
                                                           const std::filesystem::path& file);

/// Saves a **preset** file: all entries at `location` (defaults included),
/// with the schema metadata, in the Tag layout above.
///
/// @param configs The collection (non-owning).
/// @param location The location the preset applies to.
/// @param file The preset file path (parent directories are created).
/// @return `Status::Ok`, or `Status::Error` on IO failure.
/// @note Thread: Main thread (file IO).
[[nodiscard]] std::expected<void, Status>
SavePresetConfig(const ConfigCollection& configs, ConfigStorageLocation location, const std::filesystem::path& file);

/// Registers the M1 engine config entries — the platform-relevant subset of
/// the reference `CoreMod` config registrations, with the same IDs,
/// locations, defaults, and ranges (see docs/architecture for the parity
/// table). Idempotence: registering twice errors (use a fresh collection).
///
/// @param configs The collection (non-owning).
/// @return `Status::Ok`, or the first duplicate/invalid registration error.
[[nodiscard]] std::expected<void, Status> RegisterEngineConfigs(ConfigCollection& configs);

} // namespace lod

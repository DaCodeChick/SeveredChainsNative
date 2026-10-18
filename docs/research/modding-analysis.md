# Modding Analysis — Severed Chains (Java) Modding Architecture

Reference commit: `ab5dfe5ba6d4c07464a2f0e67985b9e77e2f7bdb`
Paths relative to `~/Documents/GitHub/Severed-Chains`.

## 1. Mod Model (verified)

- Mods are **Java JARs** in `mods/`, discovered by the external
  `org.legendofdragoon:mod-loader:4.3.5` library (`ModManager.findMods(dir, version)`).
- A mod declares `@Mod(id = "...", version = "^x.y.z")` (semver range; the engine
  matches against `Version.VERSION` — wrong versions reported via
  `MODS.getWrongVersions()` and shown as overlay notifications).
- `@EventListener`-annotated methods subscribe to the event bus (`EventManager`).
- Class-loading: standard Java classloader per mod (mod JARs on the classpath);
  failed loads tracked in `MODS.getFailedToLoad()` (modId → error), surfaced in
  the title screen and crash logs.
- **Retail content is a mod**: `legend.lodmod.LodMod` (`@Mod(id = "lod")`)
  registers all game content — characters, stats, elements, additions, items,
  equipment, spells, shops, encounters, battle actions, campaign/engine-state
  types, input actions, configs, fonts/icons. This "content as a mod" pattern is
  the template for the native game layer.
- Bundled example mod: `legend.turnorder.TurnOrderMod` (`@Mod(id = "turn_order")`)
  — subscribes to `RenderEvent` + battle events, registers a config entry
  (`ShowTurnOrderConfig`), renders a turn-order HUD, and its on-screen bounds are
  exposed to the post shader via `turn_order_bounds` (verified in
  `RenderEngine.onDraw`).

## 2. Core Extensibility Machinery (verified)

### 2.1 Registries (external mod-loader + in-repo `Registries`)

- `Registries` (core, `legend.core.Registries`) holds the engine's registry set:
  `config`, `campaignTypes`, `engineStateTypes`, `inputActions`,
  `characterTemplates`, `statTypes`, `statModTypes`, `goods`, plus mod-added
  registries discovered via `AddRegistryEvent` (mods can define **new registry
  kinds**).
- `RegistryId` = `{modId, entryId}` — globally unique content IDs.
- `MutableRegistry`/`Registry`/`RegistryEntry`/`RegistryDelegate`/`Registrar`:
  typed registries; `Registrar` in a mod class declares entries lazily
  (`CONFIG_REGISTRAR.register("addition_mode", ...)` — see
  `legend.game.modding.coremod.CoreMod`, 378 lines of core config entries).
- Boot order (`GameEngine.bootMods`): load mod classes → init event bus →
  post `AddRegistryEvent` (mods register their registries) → initialize core
  registries in dependency order (config, campaignTypes, engineStateTypes,
  inputActions, characterTemplates, statTypes, statModTypes, goods) →
  `loadingComplete` (fires registry events so consumers can read entries) →
  input bindings → lang.

### 2.2 Event bus

- `EventManager.postEvent(TypedEvent)` — synchronous, ordered listeners;
  `CancelableEvent` for vetoable events; `EventListener` annotation; per-mod
  error isolation (`onModError` → overlay notification, mod failure doesn't crash).
- In-repo event catalog (`legend.game.modding.events.*`) — representative set:
  - battle: `BattleStartedEvent`, `BattleEndedEvent`, `BattleEntityTurnEvent`,
    `MonsterStatsEvent`, `EnemyRewardsEvent`, `SpellStatsEvent`,
    `StatDisplayEvent`, `LoadDeffEvent`, `LoadEnemyEvent`, `ActiveItemEvent`,
    `ActiveSpellEvent`, `ArcherSpEvent`, `BattleMusicEvent`,
    `RegisterBattleEntityStatsEvent`, `DragonBlockStaffOn/OffEvent`
  - characters: character create/unlock events
  - config: `ConfigRegistryEvent`
  - engine: `EngineStateChangeEvent`
  - gamestate: `GameLoadedEvent`, `GameSavedEvent`, ...
  - input: `InputPressedEvent`, `InputReleasedEvent` (mod-only; the game reads
    input directly — verified: no in-game subscribers)
  - inventory: item/goods/shop/equipment/spell registry + gather events
  - scripting: `ReadGlobalFlagsEvent` (mods can read story flag state)
  - submap/worldmap: enter/exit + object events
  - `RenderEvent` (per-frame render hook — how mods draw)
- Content registration events (one per content type): `RegisterCharacterTemplatesEvent`,
  `AdditionRegistryEvent`, `ItemRegistryEvent`, `GoodsRegistryEvent`,
  `ShopRegistryEvent`, `SpellRegistryEvent`, `EquipmentRegistryEvent`,
  `EncounterRegistryEvent`, `RegisterDeffsEvent`, `RegisterPostBattleActionsEvent`,
  `RegisterBattleActionsEvent`, `GatherBattleActionsEvent`,
  `RegisterCampaignTypesEvent`, `RegisterEngineStateTypesEvent`,
  `InputActionRegistryEvent`, `RegisterAtlasTexturesEvent`, `IconMapEvent`,
  `RegisterLevelUpActionsEvent`, `ElementRegistryEvent`, `StatTypeRegistryEvent`,
  `StatModTypeRegistryEvent`, `BattleEntityTypeRegistryEvent`,
  `ElementIconRegistryEvent`, `ConfigRegistryEvent`, ...

### 2.3 Configuration

- `ConfigCollection` + typed entries (`Bool/Int/Float/Enum/String/Color/
  CampaignName`), each with a `ConfigCategory`, storage location
  (`GLOBAL` / `CAMPAIGN` / per-save), and presets (`ConfigPresetManager`).
- Entries are registry-driven: mods register config entries; the options menu is
  generated from the registry (`ControllerKeybindsConfigEntry`,
  `LanguageConfigEntry` — including **per-action keybinding editing**).
- Global config `config.dcnf` (Tag-serialized); per-campaign/per-save configs
  stored inside saves.

### 2.4 Input actions

- `InputAction` registry (`InputActionRegistry`): semantic actions
  (`INPUT_ACTION_BTTL_ATTACK`, `_BTTL_COUNTER`, `_BTTL_TRANSFORM`, `_MENU_*`,
  `_GENERAL_*`, `_FREECAM_*`, `_DEBUG_*`...), each with default bindings
  (`InputBindings.initBindings`), user-editable bindings (keycode/scancode/button/
  axis activations with mods + deadzones), and `InputCodepoints` for on-screen
  button glyphs.
- **Battle addition timing input** (verified, `AdditionOverlaysEffect44`):
  per hit, `right` = `BTTL_ATTACK` (or `BTTL_COUNTER` for counter hits),
  `wrong` = the other; success if pressed within
  `[frameSuccessLowerBound, frameSuccessUpperBound]` (derived from
  `AdditionHitProperties10.overlayHitFrameOffset_02` +
  `totalSuccessFrames_03`, scaled by `ADDITION_TIMING_WINDOW_CONFIG`);
  early/late/wrong → fail; `autoCompleteType` covers tutorial/auto modes.
  **The "which action is correct" logic is hardcoded — there is no mod hook for
  alternative input evaluators today.** Third-party mods like Playable Lloyd
  (face-button sequences) require engine-level seams the native project must add
  (see `docs/architecture/luau-api.md` §Combat).

### 2.5 Story script patching

- External `script-recompiler:0.7.11`: the game's binary story-script VM
  (`OpType` opcodes, `ScriptState` execution) is disassembled to a "fate" text
  language; `patches/` contains fate patches + CSV-driven meta patches
  (`patches/meta/{params,enums,descriptions}.csv`, per-class stat CSVs);
  `ScriptPatcher` applies them at unpack time with cache + backups.
- `patches/scripts/DRGN0|DRGN1|DRGN21/...` — patched script trees.
- This is the "fix original game bugs + tweak story" layer; native must
  reimplement the fate pipeline (or an equivalent) for modding parity.

### 2.6 i18n

- `LangManager` (external): `lod.lang` / `lod_core.lang` resource bundles
  (`src/main/resources/lod*/lang/en.lang`) + `lang/` directory overrides
  (player + mod provided); `I18n.translate(key)`; mods contribute language
  overrides.

## 3. What a Java Mod Can Do Today (capability list)

1. Register/replace content entries (characters, items, equipment, spells,
   additions, elements, stats, shops, encounters, campaign types, engine states,
   input actions, configs, icons, textures).
2. Subscribe to any engine event (frame render, battle lifecycle, state changes,
   input, inventory, saves).
3. Add/modify config entries and keybinds.
4. Register new registry kinds.
5. Patch story scripts (via the fate pipeline, at unpack time).
6. Render overlays (`RenderEvent` + render API).
7. Extend saves (`modData` ListTag via Read/WriteSaveData events; DABA Save60).
8. Provide language overrides.
9. Replace assets (unpack-time replacements in `patches/replacements`).
10. Battle content: DEFFs (monster behaviour CMB), post-battle actions,
    battle UI actions, entity stat registration.

**Not possible today (Java):** alternative addition input evaluators (e.g.
button sequences), per-addition custom combat resolution, arbitrary
script-in-game (no Luau), runtime asset replacement without re-unpack,
non-Java native extensions.

## 4. Migration to Native (proposal)

| Java concept | Native equivalent |
|---|---|
| Mod JAR + `@Mod` | Mod **package** (zip dir): `mod.toml`/`mod.json` manifest (id, version range, deps, assets, optional `.so/.dll/.dylib` + `.luau` scripts) |
| `@EventListener` methods | Engine event bus with typed events; Luau `Engine.Event:Subscribe` + native plugin subscriptions |
| Registries + `RegistryId` | Same model in C++ (`RegistryId{modId, entryId}`), exposed to Luau as tables and to native plugins as string IDs |
| `LodMod` (retail content) | Built-in **`lod` game module** (C++ or Luau): registers all retail content — same structure, first-class mod API |
| fate script patches | Reimplement fate compiler/patcher (or equivalent) in a CLI tool (Milestone 10+) |
| Keybinding config | Input actions + bindings storage (port `InputBindings` model) |
| Save `modData` | Port Tag `ListTag` mod-data slot; versioned |
| Asset replacements | Runtime override directories (priority chain) — replaces unpack-time replacements (more flexible) |
| JavaFX debugger | Qt 6 debugger tooling (separate process, shared memory/IPC or socket) |

**Key decisions for the native design** (detailed in architecture docs):
1. Registry/event model is **data-adjacent** (IDs, params), so both Luau and
   native plugins can participate without a JVM.
2. Additions get a pluggable **input evaluator + hit resolver** interface
   (seam missing in Java — a native advantage for the Playable-Lloyd class of
   mods).
3. Mod isolation: each mod's scripts run in a sandboxed Luau VM namespace;
   native plugins are dlopen'd with a versioned C ABI and a documented failure
   contract (a crashing plugin must not take down the game — use a fault
   handler/process isolation strategy, documented in native-api.md).

## 5. Open Questions

- External mod-loader/modding-repo source is not in the reference repo
  (jitpack `org.legendofdragoon` — separate GitHub org). For exact semantics
  (classloader isolation, version-range matching, event ordering guarantees)
  we should fetch those repos read-only. **Documented as a gap; not blocking
  Phase 0** (the in-repo usage defines the contract we need).
- Mod license diversity: mods are third-party; the native mod package format
  must carry per-mod license metadata (don't assume AGPL).
- Whether any released mods rely on Java-specific reflection/annotation tricks
  that have no native analogue (unknown — no mod source available locally).
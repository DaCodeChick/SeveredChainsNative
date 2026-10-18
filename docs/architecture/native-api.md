# Native Extension API — Versioned C ABI

Design for first-class native plugins (`.dll` / `.so` / `.dylib`) with optional
C++23 convenience wrappers. Requirements: stable ABI, version negotiation,
opaque handles, explicit ownership, no STL across the boundary, no EnTT
registry exposure, no-exceptions policy.

## 1. Core Principles

1. **The engine exports; plugins consume.** The game engine builds as a shared
   library (`scengine`), the player-facing executable is a thin host
   (`severed-chains`) that links the engine and loads plugins. (Requirement:
   "building the reusable engine as a shared library with a thin executable
   host.") This also lets standalone tools (Qt debugger, fate CLI, asset
   tools) link the same engine.
2. **C ABI only at the boundary.** `extern "C"`, C linkage, plain structs,
   pointers, fixed-width integers (`uint32_t`, `int64_t`, `float`, `double`),
   UTF-8 NUL-terminated strings. No `std::*`, no RTTI, no exceptions (the
   compiler flags guarantee they can't cross: `-fno-exceptions` on both sides
   for our code; third-party plugins are documented as unsafe-by-contract).
3. **One-way versioning.** Plugins declare a **minimum** engine API version
   they support; the engine provides **versioned function tables**; plugins
   fetch a table for a version ≤ what they declare. New engine versions add
   tables; they never remove or change existing ones.
4. **Explicit ownership.** Every handle API states ownership in its docs and
   in a uniform `sc_handle` contract: a handle returned by the engine is
   owned by the engine (plugin never frees it); a handle returned by a plugin
   callback is owned by the plugin until the engine calls the matching
   release fn. No implicit reference counting across the boundary — the
   engine may clone handles (refcounted internally) but plugins only ever
   see 1:1 ownership.
5. **Errors, not exceptions.** All fallible entry points return an `int32_t`
   status code or fill an out-param; a `sc_context` carries the last error
   (code + message buffer). See `error-handling.md` for the code table.
6. **Thread safety.** Default contract: **all engine API calls are
   main-thread (game thread) unless documented otherwise.** Callbacks the
   engine invokes are also main-thread. Audio-thread callbacks are the
   exception (documented per callback). No locks are exposed; plugins that
   need their own threads must only call thread-safe-marked APIs.
7. **Lifecycle is a state machine** (see §4) — the engine refuses (with a
   clear error) calls in the wrong state.

## 2. Entry Points (plugin → engine)

A plugin exports exactly one C entry point:

```c
// Called by the engine on dlopen/LoadLibrary.
// `interface` is the engine's versioned API root (see §3).
// `plugin_ctx` is a pointer to a plugin-provided context (opaque to engine).
// Returns SC_OK or an error code; non-zero aborts loading (plugin is
// reported as failed, mirroring MODS.getFailedToLoad()).
SC_API int32_t sc_plugin_init(const sc_engine_interface *interface,
                              uint32_t abi_version,
                              void *plugin_ctx);

// Called on unload (before dlclose). Plugins release all resources here.
SC_API void sc_plugin_shutdown(void *plugin_ctx);
```

Plus an optional query for the plugin's own capabilities:

```c
// Optional: engine calls to discover exported capabilities (events,
// registries, script functions, asset loaders...). Default: none.
SC_API const sc_plugin_manifest *sc_plugin_manifest(void *plugin_ctx);
```

**Plugin manifest** (static C struct, string-annotated):
```c
typedef struct sc_plugin_manifest {
    const char *id;            // reverse-DNS / scoped id: "example.dragoon_fury"
    const char *version;       // semver "1.2.0"
    const char *name;
    const char *author;
    const char *license;       // SPDX id or short text (per-mod license metadata)
    uint32_t min_api_version;  // SC_API_VERSION_MAJOR/MINOR packed
    const char **depends;      // NULL-terminated list of mod/plugin ids
    uint32_t flags;            // SC_PLUGIN_FLAG_* (e.g. SCRIPTS | ASSETS | GAMEPLAY)
} sc_plugin_manifest;
```

## 3. Engine Interface (engine → plugin)

The engine passes a root interface; plugins walk **versioned tables**:

```c
typedef struct sc_engine_interface {
    uint32_t size;                 // sizeof(*this) — forward compat
    uint32_t api_version;          // packed MAJOR.MINOR
    const char *engine_version;    // "0.1.0+commit"
    // Fetch a versioned table by id; returns NULL if the running engine
    // doesn't provide that table id/version.
    const void *(*get_table)(const char *table_id, uint32_t table_version);
    // Error facilities
    int32_t (*get_last_error)(const char **message, uint32_t *message_size);
    // Diagnostics
    void (*log)(uint32_t level, const char *tag, const char *fmt, ...);
} sc_engine_interface;
```

**Table id convention:** `"sc.<area>.<N>"` where N is the table version,
e.g. `sc.gameplay.1`, `sc.render.1`, `sc.assets.1`, `sc.events.1`,
`sc.save.1`, `sc.input.1`, `sc.audio.1`, `sc.script.1`, `sc.log.1`.
Tables are `const` structs of function pointers; plugins check availability
via `get_table` and degrade gracefully (documented per capability).

Example table (`sc.gameplay.1`):
```c
typedef struct sc_gameplay_table_1 {
    uint32_t size;
    // Entity queries (handles are uint32 scene-entity ids; NOT EnTT types)
    int32_t (*entity_count)(sc_scene scene);
    int32_t (*entity_query)(sc_scene scene, const sc_entity_query *q,
                            uint32_t *out_ids, uint32_t max, uint32_t *out_count);
    int32_t (*entity_transform_get)(sc_scene scene, uint32_t entity,
                                    sc_vec3 *out_pos, sc_quat *out_rot,
                                    sc_vec3 *out_scale);
    int32_t (*entity_transform_set)(sc_scene scene, uint32_t entity,
                                    const sc_vec3 *pos, const sc_quat *rot,
                                    const sc_vec3 *scale);
    int32_t (*entity_spawn)(sc_scene scene, const sc_spawn_desc *desc,
                            uint32_t *out_entity);
    int32_t (*entity_destroy)(sc_scene scene, uint32_t entity);
    // Game state
    int32_t (*game_state_get)(sc_game_state *out);        // enum + chapter etc.
    int32_t (*game_clock_get)(uint64_t *out_timestamp);
    // Save data (see sc.save)
    ...
} sc_gameplay_table_1;
```

**Data types at the boundary** (all POD):
```c
typedef struct { float x,y,z; } sc_vec3;
typedef struct { float x,y,z,w; } sc_quat;
typedef struct { float m[16]; } sc_mat4;          // column-major, std140-style
typedef struct { uint8_t r,g,b,a; } sc_color;
typedef int64_t sc_timestamp;
typedef uint32_t sc_entity;                        // scene entity id
typedef uint32_t sc_scene;                         // scene context id
typedef const char *sc_registry_id;                // "modid.entryid"
```

**Registry access from plugins** (string-IDs, mirrors `RegistryId`):
```c
typedef struct sc_registry_table_1 {
    uint32_t size;
    // Generic entry lookup: returns a POD descriptor for known entry kinds;
    // complex entries expose per-kind tables (characters, additions, items...)
    int32_t (*entry_get)(sc_registry_id id, sc_entry_kind kind,
                         void *out_desc, uint32_t out_size);
    int32_t (*entry_list)(sc_entry_kind kind, sc_registry_id *out,
                          uint32_t max, uint32_t *out_count);
    // Read registry-registered config
    int32_t (*config_get_bool)(sc_registry_id id, int *out);
    int32_t (*config_get_int)(sc_registry_id id, int32_t *out);
    int32_t (*config_get_float)(sc_registry_id id, float *out);
    int32_t (*config_get_string)(sc_registry_id id, char *out, uint32_t size);
} sc_registry_table_1;
```

## 4. Lifecycle

```
discover (mod dir scan: native/*.so|dll|dylib + manifest read)
  → validate (manifest schema, id uniqueness, dependency closure,
              min_api_version <= engine api_version, file signature optional)
  → load (dlopen RTLD_LAZY | LoadLibrary; capture dlerror)
  → init   (sc_plugin_init; plugin fetches tables; plugin registers
             callbacks/entries; any error → plugin marked failed, unloaded,
             reported in mod list — parity with Java's failed-mod UX)
  → ready  (plugin's registered events fire; script functions available)
  → shutdown (sc_plugin_shutdown; engine calls before dlclose; engine
             verifies no engine handles are still owned by the plugin —
             handle-leak check logged)
  → unload (dlclose)
```

- **Hot reload: not supported in v1.** Documented limitation: unloading a
  native plugin with live resources is unsafe across platforms (static
  destructors, registered callbacks, GPU resources owned by the plugin).
  Reload = restart (the Qt mod manager can orchestrate a game restart with
  the new mod set, mirroring Java's update-restart flow).
- **Crash containment:** a crashing plugin (segfault) takes down the process —
  unavoidable with in-process dlopen. Mitigations: (a) strict API contracts +
  fuzz/ABI tests; (b) optional out-of-process mode (documented future work):
  run untrusted plugins in a child process with the C ABI over a socket —
  not in v1, but the table-based design keeps it possible.
- Plugin failure at any stage → logged + surfaced in mod manager + in-game
  notification (parity: `GameOverlay.addNotification`).

## 5. Capability Areas (table map)

| Table | Capabilities |
|---|---|
| `sc.gameplay` | entity queries/spawn/destroy, transforms, game state/clock, flags |
| `sc.events` | subscribe/unsubscribe to engine events (§6), per-subscription callback |
| `sc.input` | read action state (pressed/held/repeat/axis), define new actions, bind queries |
| `sc.render` | register custom draw callbacks (render event hook with a scoped `sc_render` command handle: pipelines the plugin creates via engine-provided builder, textures, vertex/index buffers), post-shader uniform hooks (e.g. `turn_order_bounds`-style) |
| `sc.audio` | register music (sequence file refs), play SFX from soundbanks, volume control, music event queries |
| `sc.assets` | register path-override roots (asset replacement), query/decode assets via engine decoders (TMD/TIM/ANM...), upload decoded data to GPU |
| `sc.save` | read/write the plugin's `modData` slot (scoped by plugin id; engine serializes as Tag list entries — byte-compat with the Java `modData` slot layout for the same id), save-validation hooks |
| `sc.script` | register story-script functions (op → callback with param in/out buffers), query script flag state (parity with `ReadGlobalFlagsEvent` + `@ScriptDescription`) |
| `sc.combat` | battle lifecycle queries, **addition input evaluator registration** (§7), hit result overrides |
| `sc.log` | leveled logging with tags |

## 6. Events (C ABI)

```c
typedef struct sc_events_table_1 {
    uint32_t size;
    int32_t (*subscribe)(const char *event_id,   // e.g. "battle.turned", "render"
                         sc_event_callback cb,
                         void *user,
                         uint32_t *out_sub_id);
    int32_t (*unsubscribe)(uint32_t sub_id);
    // Post a custom event visible to all listeners (Luau + native):
    int32_t (*post)(const char *event_id, const sc_event_payload *payload);
} sc_events_table_1;

typedef void (*sc_event_callback)(const sc_event *e, void *user);
typedef struct sc_event {
    const char *id;
    uint64_t seq;                 // monotonic event sequence
    const sc_event_payload *payload;  // typed per event id (see docs per id)
    // Cancelable events:
    int32_t (*cancel)(const sc_event *e);
} sc_event;
```

Event ids mirror the Java event catalog (documented 1:1 table in the API
docs): `render`, `engine.state.changed`, `battle.started`, `battle.ended`,
`battle.entity.turn`, `monster.stats`, `enemy.rewards`, `input.pressed`,
`input.released`, `game.loaded`, `game.saving`, `inventory.*`, `submap.*`,
`worldmap.*`, `character.created`, `script.flags.read`, ...

## 7. Combat Extensibility (the Playable-Lloyd seam)

Java hardcodes addition input to the attack/counter actions
(`AdditionOverlaysEffect44`). The native combat engine factors this:

```c
typedef struct sc_addition_input_evaluator {
    uint32_t size;
    const char *id;
    // Called per sim frame during an addition hit window.
    // `state`: hit properties (timing bounds, counter flag), frames elapsed.
    // `out_result`: SC_ADDITION_HIT_PENDING / SUCCESS / FAIL_WRONG / FAIL_LATE / FAIL_EARLY
    int32_t (*evaluate)(void *user, const sc_addition_hit_state *state,
                        sc_addition_hit_result *out_result);
    // Optional: custom overlay draw (default = timing square)
    int32_t (*draw_overlay)(void *user, const sc_addition_hit_state *state,
                            sc_render *render);
} sc_addition_input_evaluator;
```

- Retail content registers the **timing-square evaluator** (attack/counter
  actions, window bounds from `AdditionHitProperties10`, timing-window config
  multiplier) — behavior identical to Java.
- A mod (Luau *or* native) registers an alternative evaluator (e.g.
  face-button sequences) and marks specific additions as using it. The combat
  state machine consumes `evaluate()` results only — no core rewrite
  required (requirement §11).
- Luau gets the same capability via `Combat.Addition:setInputEvaluator(...)`
  (see luau-api.md).

## 8. C++23 Wrappers

`src/engine/plugin_api/` provides header-only C++ wrappers (in-project,
exceptions-disabled): `PluginContext` RAII, `Result<T>`-returning typed
bindings, `Event<T>` typed subscriptions, string views that never escape the
call. Wrappers are a **convenience only** — the C ABI is the contract.

## 9. ABI Testing & Stability Policy

- **ABI tests:** `tests/abi/` — a plugin compiled against a given API version
  loads and passes functional tests against the current engine (CI matrix:
  oldest supported plugin version vs newest engine).
- Symbol stability: engine shared lib exports a **stable C symbol prefix
  `sc_`** (versioned export lists per platform: `.def`/`export map`/
  `-exported_symbols_list`); C++ symbols are hidden (`-fvisibility=hidden` /
  MSVC `/Gy` + export-on-demand).
- Semver: breaking ABI change = MAJOR bump + old table retained (old plugins
  keep working; new features in new tables).
- Fuzz + sanitizers on the boundary (see testing.md).

## 10. Limits & Documented Non-Goals

- No hot reload; no JIT; no plugin-provided GPU devices.
- No cross-plugin dependencies (plugin A can't call plugin B's exports; they
  coordinate via engine events/registries — keeps isolation clean).
- Plugins cannot register *new registry kinds* in v1 (Lua can; registry-kind
  extension for native is a v2 consideration — Java allowed it; assess demand).
- Audio callback timing: synth/SFX mixing stays on the audio thread; plugins
  get audio-thread-marked callbacks with the same no-heap-alloc expectation
  as Java's audio thread (documented).

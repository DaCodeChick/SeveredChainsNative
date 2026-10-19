# Severed Chains Native — Agent Guide

## Project Goals

Cross-platform native C++23 reimplementation of *The Legend of Dragoon: Severed Chains*,
preserving gameplay, narrative, combat, audiovisual presentation, and timing, plus
Severed Chains' enhancements and modding capabilities.

- **Not** a PS1 emulator; no cycle-accuracy or PS1 hardware limits unless they serve
  genuine gameplay/compatibility purposes.
- Reusable engine technology must be kept separate from Legend of Dragoon-specific code.
  The engine should be potentially reusable for other PS1 games (e.g. *Legend of Legaia*).
- Original game data must be legally obtained by the player; we never distribute game data.
- Use measurements, not assumptions, for performance work. Do not assume C++ is faster.

## Authoritative Reference Repository

- **Path:** `~/Documents/GitHub/Severed-Chains` (Java, Gradle, LWJGL/SDL3/OpenGL, JavaFX)
- **Treat as READ-ONLY.** Never modify its source, config, build files, or Git history.
- Record its commit hash in `progress.md` before starting research (done: see progress.md).
- Java source references in docs use paths relative to that repository root.
- Do not confuse it with the native C++ project (this repository).

## Approved Technology Stack

| Technology | Purpose |
|---|---|
| C++23 | Core engine and game |
| SDL3 | Windowing, input, controllers, audio, platform integration |
| SDL3 GPU | Rendering (Vulkan / Metal / D3D12 backends) |
| GLM | Mathematics |
| EnTT | Entity-component management and events (where it pays off) |
| Luau | Gameplay scripting and portable modding |
| Qt 6 | Launcher, settings, desktop UI, development tools |
| CMake + Ninja | Build system |

- SDL_shadercross and Dear ImGui are **not** part of this project.
- Extra third-party dependencies require a documented justification
  (purpose, alternatives, license, portability, maintenance) and explicit approval.
- Player-facing game UI (menus, HUDs, dialogue) renders via SDL3 GPU, **not** Qt widgets.
- Keep the core engine Qt-independent where practical.

## C++23 Coding Standards

- **MANDATORY READING: `docs/guidelines/cpp.md`** — every C++ implementation
  task follows it before and during work. Modern, type-safe C++23 is the
  default idiom: `std::format`/`std::print`-based logging with literal format
  strings (`std::format_string`), `std::expected<T, Status>` error
  propagation, RAII/value semantics, `std::span`/`std::string_view`/
  `std::filesystem`/ranges, `enum class`/`constexpr`/`[[nodiscard]]`. The
  printf family, `std::va_list`, C strings in C++ APIs, raw owning pointers,
  and unnecessary macros are **prohibited**; the documented exceptions are
  C third-party interop (SDL3) and PSX binary-format work (guidelines §9).
- RAII, clear ownership/lifetimes, value semantics, explicit error/result types
  (`std::expected` / error enums), minimal global mutable state, minimal inheritance.
- Clear interfaces between subsystems; standard containers and algorithms.

### MANDATORY: No C++ Exceptions

- No `throw`, `try`, or `catch` in any project-owned C++ code (engine, game, tools,
  Qt integration, Luau bindings, plugins, tests).
- Project targets compile with exceptions disabled:
  Clang/GCC `-fno-exceptions`, MSVC `/EHs-c-` (verify effective flags).
- CMake must enforce this consistently for all project-owned targets.
- Precompiled third-party libraries (e.g. Qt 6) may have their own exception behavior;
  document limitations, do not assume our flags change them.
- Luau runtime errors must be handled via Luau's own error-reporting facilities,
  never by letting C++ exceptions cross the boundary.
- **No RTTI across public API boundaries:** the native plugin API and engine public
  headers compile with RTTI disabled (Clang/GCC `-fno-rtti`, MSVC `/GR-`) and never
  use `dynamic_cast` (native-api.md §1.6). Engine-internal code may use RTTI only
  where it does not appear in a public header.

### MANDATORY: Doxygen-Commented Public APIs

- **All public project-owned C++ APIs carry Doxygen-compatible comments**: engine
  interfaces, the PSX library APIs (psx-library.md), and the native plugin interface
  (native-api.md) first; internal helpers where clarity benefits.
- Document: purpose, `@param` for each parameter (in/out/ownership), `@return`
  / `std::expected` semantics, `@errors` (which `Status` codes and when) for
  fallible APIs,
  **ownership/lifetime** of any handle or pointer, and **thread safety** (which
  thread may call; default is main-thread unless documented).
- Use standard Doxygen tags (`/** ... */`, `@brief`, `@param`, `@return`, `@errors`,
  `@note`, `@thread`, `@see`) so `doxygen` generates the API reference without
  per-file tweaks. Generated docs land in `docs/api/` (build target, see
  build-system.md).
- Third-party API names are preserved verbatim (no re-naming to fit our conventions).

### Naming Conventions (mandatory for project-owned code)

| Element | Convention | Example |
|---|---|---|
| Namespaces | abbreviated flatcase | `sc::render`, `psx::tmd`, `game::combat` |
| Classes, structs, functions, methods, constants, enum values | PascalCase | `RenderEngine`, `DecompressBPE`, `EntitySpawn`, `OnLoad`, `kMaxVoices`, `Status::Ok` |
| Variables and parameters | camelCase | `simFrame`, `outCount` |
| Private fields | camelCase with trailing underscore | `simFrame_`, `outCount_` |
| Macros | UPPER_SNAKE_CASE | `SC_API`, `SC_NOEXCEPT_TARGETS` |
| C-linkage discovery symbols (plugin entry only) | PascalCase with `Sc` prefix | `ScPluginCreateV1` |
| Files | snake_case | `render_engine.cpp`, `render_engine.hpp` |

- **Acronyms are fully capitalized inside PascalCase identifiers:**
  `EngineAPI`, `GameplayAPI`, `XADecoder`, `GTEView`, `SFXVoice`, `DecompressBPE` —
  never `EngineApi` / `XaDecoder` / `GteView` / `DecompressBpe`. The rule
  applies to type names, function/method names, constants, and enum values.
- The conventional suffix **`Id` / `Ids` is treated as a word** (matching
  the reference project's `RegistryId`, `modId`, `entryId`) and is **not**
  capitalized as an acronym.
- Free functions and struct/class methods alike are PascalCase
  (e.g. `DecompressBPE(bytes)`, `EngineAPI::QueryTable(...)`,
  `Plugin::OnLoad(...)`); camelCase is reserved for variables/parameters/
  private fields.
- String *ids* and table id constants (e.g. `"sc.gameplay"`, registry ids,
  event ids) are lowercase per the id grammar (mod-loader.md §4) — they are
  identifiers in a string namespace, not C++ identifiers.
- Third-party API naming is preserved (e.g. SDL_*, luau_*, Qt names stay
  as-is; Java reference class names are cited verbatim in docs, e.g.
  `GlApi`, `XaAdpcm`).
- Enum values are PascalCase; `enum class` type names are PascalCase plural or
  singular by clarity (`EntryKind`, `Priority`).

### Formatting: .clang-format (mandatory)

- A root-level `.clang-format` (LLVM baseline, 2-space indent, **Allman
  braces**, 120-column limit, grouped/sorted includes, Doxygen comments left
  intact) is the
  single source of formatting truth. Run `clang-format` on project-owned files you
  touch; do **not** mass-reformat untouched files.
- Doxygen comment blocks are preserved (no reflow that breaks `@param`/`@return`
  alignment); clang-format's `ReflowComments` stays off.
- Where hand layout aids readability (e.g. aligned struct fields), prefer
  clang-format output; document intentional deviations with a `// NOLINT` + reason.

## Architectural Boundaries

- **PSX library (reusable PlayStation 1 tech, independent):** GTE math, PS1 GPU/GS
  semantics + software reference rasterizer, SPU voice model, XA/SShd/SSSq decoding,
  ISO 9660 (PS1) reading, TMD/TIM/ANM/MRG/DEFF/BPE/Tag formats — zero third-party
  deps, no SDL/Qt/game logic (psx-library.md). **Prefix-free identity** (target
  `psx`, namespace `psx::` — no Sc/SC/sc; it may be extracted to its own repo),
  root-level subproject (build form follows CMake `BUILD_SHARED_LIBS` — the
  user's choice). Consumed by engine, game, and tools.
- **Engine (reusable):** platform/lifecycle, SDL3 GPU rendering, input, audio, assets,
  scene/ECS, animation, save serialization, scripting host, native plugin host, timing.
- **Game (LoD-specific):** characters, combat, Additions, dialogue/events, submaps,
  world map, saves content, retail registries — the equivalent of the Java `lod` mod.
- **Mogul Dabas minigame:** separate C++23 library (`src/libs/dabas`, target
  `scdabas`) for the original in-game handheld minigame; first-party content,
  not a plugin, not PSX
  hardware (dig-dabas.md §5).
- **Desktop tools (Qt 6):** launcher, mod manager, asset inspection, debuggers.
- **Native plugin API:** **C++23 interface (not a C ABI)** — C-linkage discovery
  symbol only; frozen-layout C++ interfaces, toolchain-parity rule, RAII ownership,
  no EnTT/psx exposure, versioned interfaces (native-api.md). Qt's plugin system is
  **not** used for the engine's plugin architecture.
- **Luau:** primary portable scripting; independent of native plugins; both share
  controlled engine services. Luau APIs must not be required by native extensions.

## Research Methodology

- Work in bounded subsystems; persist findings into `docs/` as you go.
- Every significant finding must cite concrete Java source files/classes/symbols
  (paths relative to `~/Documents/GitHub/Severed-Chains`).
- Distinguish **verified behavior** from **assumptions** and **open questions**.
- Do not guess: if a question can't be answered from available sources, document the gap.
- Write research documentation only into THIS repository.

## Build Requirements

- CMake + Ninja; clean-clone build procedure documented per platform
  (Windows, Linux, macOS/Apple Silicon).
- All project-owned targets built with exceptions disabled (see above); public API
  targets additionally built with RTTI disabled (see above).
- `psx` compiles standalone (no engine/SDL/Qt/Luau dependencies) — enforced by the
  build system (build-system.md). Project-owned libraries are declared without a
  library type: CMake's built-in `BUILD_SHARED_LIBS` selects the build form at
  configuration time — the architecture prescribes no default (the choice is
  the user's).
- Pinned, stable dependency versions; straightforward builds; avoid unnecessary
  Git submodules.
- Shader pipeline must produce backend-appropriate artifacts (SPIR-V / Metal / DXIL)
  without SDL_shadercross.

## Licensing Considerations

- Original Severed Chains: AGPL-3.0 (verify against reference repo `LICENSE`).
- This project is an **independent derivative implementation**; document derivative-work
  obligations (attribution, source availability, distribution terms).
- Qt 6 licensing (LGPL v3 / commercial) implications must be documented.
- Proprietary LoD assets are never distributed; players supply their own legal data.
- Do not assume mods share the project's license; document per-mod licensing uncertainty.
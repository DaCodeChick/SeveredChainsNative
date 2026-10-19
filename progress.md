# Progress Log

## Reference Repositories

- **Severed Chains (Java, authoritative behavioral reference)**
  `~/Documents/GitHub/Severed-Chains` — branch `main`, commit
  **`ab5dfe5ba6d4c07464a2f0e67985b9e77e2f7bdb`**. Read-only throughout
  Phase 0 (verified: `git status` clean, no writes).
- **Mod-Loader** `~/Documents/GitHub/Mod-Loader` — branch `main`, commit
  **`c692134907a070a8b2c7db7b2b779010bc16deb9`** (artifact
  `org.legendofdragoon:mod-loader:4.3.5`, AGPL-3.0). Read-only.
- **Script-Recompiler** `~/Documents/GitHub/Script-Recompiler` (task's
  "Script-Decompiler") — branch `main`, commit
  **`3433e6f74bb6d562aeff0490d11e8937da437511`** (artifact `org.
  legendofdragoon:script-recompiler:0.7.11`, AGPL-3.0). Read-only.
- **Dig-Dabas** `~/Documents/GitHub/Dig-Dabas` — branch `main`, commit
  **`4c1fbfebced123d43282b6e3b08286336b6b05f0`** (artifact `org.
  legendofdragoon:mogul-dabas:0.0.1`, AGPL-3.0). Read-only.
- Language/build: Java 25 (SC), Java 21 (Mod-Loader); Gradle. Key SC deps:
  LWJGL 3.4.3 (SDL3, OpenGL, OpenAL, Opus, STB, assimp, shaderc,
  SPIRV-Cross), JOML 1.10.8, JavaFX 21.0.1, log4j2, JSON/GSON, fastutil,
  JNA, discord-sdk4j, javacv/ffmpeg.

## Current Phase

**Milestone 1 — Application Foundation: build-system + project-foundation
portion COMPLETE (owner-approved scope; 2026-10-18/19).** All M1
acceptance criteria verified on macOS Apple Silicon (see the M1 section
below). Remaining M1 increments (Action-parity scheduler, Tag-based
`config.dcnf`, crash handler, input parity table) and M2+ are pending.
No Git history modified (M1 work is uncommitted, per owner instruction).

## Milestone 1 (2026-10-18/19 — build-system + project foundation)

### Scope (as approved by the owner)

CMake + Ninja project skeleton, mandatory compiler policy (no exceptions,
no RTTI on public API, probes), SDL3 init/window/input/event loop,
diagnostics (type-safe logging, version injection), `psx` target skeleton,
in-repo test framework + smoke tests, CI config, Doxygen target. No
GPU, no gameplay, no asset extraction, no mod loading.

### Owner feedback addressed (this session)

The owner flagged legacy C/C++ patterns (printf-style logging, `const
char*` APIs, `std::va_list`) in the initial implementation and required
prescriptive guidelines. Delivered:

- **`docs/guidelines/cpp.md`** — prescriptive C++23 coding guidelines:
  modern stdlib facilities table (`std::format`/`std::print` type-safe
  logging with literal format strings, `std::expected`, RAII/ownership,
  `std::span`/`string_view`/ranges, `enum class`/`constexpr`/
  `[[nodiscard]]`, strong typing), naming + Doxygen conventions, a
  prohibited-patterns table (no printf family, no `std::va_list`, no C
  strings in C++ APIs, no unnecessary macros), and documented exceptions
  for C third-party interop (SDL3/PSX binary formats/plugins/Qt).
- **`AGENTS.md`** — the guidelines are now MANDATORY READING for all C++
  work in this repo.
- **Audit + correction**: all M1 source rewritten/re-audited against the
  guidelines — zero `printf`/`va_list`/C-string API surface in project
  code (verified by grep: NONE). Test harnesses use `std::print`,
  `std::string`, `string_view::find`.
- One deliberate, documented limitation: `std::source_location` capture is
  not on the formatted template logging path — Clang reports the template
  body's line there, not the call site (verified with a minimal repro). It
  remains available on non-template logging entry points where it is
  accurate. Logged in guidelines §3.

### What was built

| Artifact | Notes |
|---|---|
| `CMakeLists.txt` (root) | C++23 mandatory, no compiler extensions, output dirs, version metadata injection (version/commit/timestamp/channel), subdirectory layout |
| `cmake/versions.cmake` | Pinned SDL3 (3.4.18 fetch fallback + sha256), project options (`SC_TESTS`, `SC_DOCS`, `SC_SANITIZERS`) |
| `cmake/ScToolchain.cmake` | `sc_noexcept`/`sc_nortti`/`sc_warnings` interface targets; `sc_add_library()` (typeless — `BUILD_SHARED_LIBS` decides the build form); configure-time compiler policy probes; sanitizer options |
| `cmake/ScDeps.cmake` | SDL3 resolution: system/Homebrew first, pinned FetchContent fallback |
| `cmake/ScDocs.cmake` | `sc_add_doxygen_target()` → `sc-api-docs` (warning-clean) |
| `cmake/probes/` | `throw_probe.cpp`, `rtti_probe.cpp` — verify the policy flags actually reject `throw`/`dynamic_cast` (FATAL_ERROR otherwise) |
| `psx/` | `psx` target (typeless, ZERO deps, prefix-free identity, self-contained CMake — builds standalone with `cmake -S psx`); `psx/core` version + error modules; **self-contained `psx-tests`** (own `psx_test.hpp` harness — psx is extractable, so its tests don't depend on the host repo's framework) |
| `src/engine/` (`scengine`) | `util` (Status/expected, Logger, Version), `platform` (SdlPlatform, Window, Input), `app` (App: SDL init → window → event loop with frame pacing → clean shutdown) |
| `src/host/` (`severed-chains`) | Thin `main`: `--version`, `--help`, `--frames`, `--headless`, `--log-level` |
| `tests/` | In-repo framework (`sc-test-framework`, bool-based, no exceptions) + engine smoke tests (status, log, input, platform round-trip, headless app run, version injection) |
| `.github/workflows/ci.yml` | 3-OS matrix (macOS/Windows/Linux): configure → probes → build → ctest → clang-format check |

### Verification results (macOS Apple Silicon, all re-run from clean slate)

| Check | Result |
|---|---|
| `cmake -G Ninja && ninja` (RelWithDebInfo) | PASS — 0 errors, 0 warnings |
| Compiler policy probes at configure time | PASS — `throw` and `dynamic_cast` rejected by the toolchain flags |
| `ctest` (integrated) | PASS — 14 engine + 2 psx tests |
| `psx` standalone build + tests | PASS — `cmake -S psx`, 2/2 tests, zero deps |
| `sc-api-docs` (Doxygen) | PASS — HTML generated, 0 warnings |
| `clang-format --dry-run --Werror` on all sources | PASS |
| Flag audit (`compile_commands.json`) | PASS — every TU has `-fno-exceptions` + `-Werror`; the 9 public-API TUs also `-fno-rtti` |
| `./severed-chains --version` | PASS — prints injected version/commit/timestamp/channel |
| Headless run (`--frames 30 --headless`) | PASS — clean shutdown, correct pacing (~15.4 ms/frame at 60) |
| Real-display run (500 frames) | PASS — window opens, event loop runs (~63 fps), clean shutdown log |
| Legacy-pattern audit | PASS — no `printf`/`va_list`/C-string APIs in project code |

### SDL3 handle ownership via unique_ptr + custom deleter (2026-10-19)

Owner request: no raw owning C pointers — use `std::unique_ptr` with an
explicit custom deleter (`make_unique` where the type is complete;
`decltype` where the deleter is a lambda). Applied to the one raw owning
pointer in the codebase, `Window::window_` (`SDL_Window*`):

- `SDLWindowDeleter` — namespace-scope lambda in `window.hpp` that calls
  `SDL_DestroyWindow` (fully capitalized acronym, per the convention);
  `SDLWindowOwner = std::unique_ptr<SDL_Window, decltype(SDLWindowDeleter)>`
  aliases the owner type; `Window` now holds an `SDLWindowOwner`.
- **`make_unique` cannot apply to SDL3 handles**: SDL3 types are *opaque*
  C types (incomplete in C++), and `make_unique` needs a complete type
  (`new T(...)`). The handle returned by `SDL_CreateWindow` is therefore
  owned via `window_.reset(SDL_CreateWindow(...))` with the custom
  deleter; `Destroy()` uses `window_.reset()` (deleter fires);
  all C calls take `window_.get()`; `Handle()` returns the non-owning
  raw handle (documented escape hatch for the M2 GPU layer).
- Guidelines codified: cpp.md §5 ("C resource ownership" — unique_ptr +
  explicit custom deleter, make_unique for complete types, opaque C types
  via reset; `SDLWindowDeleter` naming) and §9.1 (SDL3 interop).
- Verified: clean build, 16/16 tests, doxygen 0 warnings, format clean,
  headless + real-display runs (window opens, 300 frames, destroyed via
  the deleter, clean shutdown). No raw owning C pointers remain.

### Windows CI probe fix (2026-10-19)

GitHub CI (windows-latest) failed configure at the RTTI policy probe:
the `dynamic_cast` TU **compiled** under `/EHs-c- /GR-`. Root cause
(documented MSVC behavior, warning **C4541**): MSVC `/GR-` degrades an
RTTI-dependent `dynamic_cast` on a polymorphic type to a *warning*
("unpredictable behavior may result") instead of the hard error Clang/GCC
`-fno-rtti` produces — so the probe's expected compile failure never
happened. Fixed by making the probes toolchain-agnostic:

- **Preprocessor guards in both probe TUs**: hard `#error` when the
  compiler reports the feature enabled (`_CPPRTTI`/`__GXX_RTTI` for RTTI;
  `_CPPUNWIND`/`__EXCEPTIONS` for exceptions) — catches missing policy
  flags regardless of the toolchain's diagnostic behavior.
- **MSVC RTTI probe now adds `/WX`**: promotes C4541 to an error so the
  dynamic_cast use is still rejected on MSVC.
- `try_compile` calls now capture `OUTPUT_VARIABLE` and embed the probe
  build output in the FATAL_ERROR — the next cross-toolchain surprise
  shows the exact compile line + diagnostics.
- All four probe behaviors verified locally on Clang (flags in effect →
  RTTI/throw hard error; flags missing → `#error` guard fires); build-
system.md §2 documents the MSVC nuance. Windows/Linux re-run pending the
  next CI run.

### Known toolchain findings (documented for future sessions)

- CMake 4.4 `try_compile` source form: policy flags must go through
  `COMPILE_DEFINITIONS` (verified to inject raw compile-line arguments);
  `CMAKE_FLAGS "-DCMAKE_CXX_FLAGS=..."` does NOT reach the probe compile.
- Apple Clang 21: a variadic pack followed by a trailing defaulted
  `std::source_location` parameter breaks template deduction (repro
  kept in guidelines §3 note).
- SDL3 3.4.18: `SDL_GetVersion()` is argument-less returning a packed int
  (use `SDL_VERSIONNUM_MAJOR/MINOR/MICRO`); no `SDL_WaitEventTimeoutNS`
  (ms-based `SDL_WaitEventTimeout` only); resize data is
  `event.window.data1/data2` (SDL3 removed the top-level `SDL_Event.data*`).
- Doxygen 1.18: `USE_MDFILE_AS_MASTER` unsupported; no line-wrapping in
  Doxyfile (all INPUT paths on one line).

### M1 items explicitly deferred (with reason)

- **Tag-based `config.dcnf` load/save** — depends on the `psx` Tag module
  (M2 scope); window/config parameters are compile-time defaults for now.
- **Action-parity scheduler** — lands with the input-parity increment
  (reference `Action.java` 60 Hz/5 ms queue semantics need the full input
  map from `ModLoaderOptions`).
- **Crash handler / crash log** — the diagnostics increment; the logger and
  version metadata it depends on are in place.
- **Windows/Linux** — CI matrix configured but not yet executed (no
  runners on this machine); verify when CI runs.

### Next steps

1. Owner review of M1 build + guidelines (code is uncommitted).
2. M1 remainder: Action-parity scheduler, input parity table, Tag-based
   config, crash handler (needs no new dependencies).
3. G2 (SDL3_image) approval → M2 start; in parallel `psx` modules per
   psx-library.md (gte/gpu/spu first — dependency-free, golden-gated).
4. Golden-fixture generation with the user (Java reference + discs) for
   M5+.

## Naming-convention correction pass (2026-10-19, owner-directed)

Owner review of the M1 code produced corrections, applied and verified
(clean rebuild, 16/16 tests, doxygen, format, flag audit).

**Prefix split — owner clarification: the LoD rename applies to C++ code
ONLY; the build system stays Sc (Severed Chains is the project name), and
the app executable is `lodsc`:**

| Domain | Prefix | Examples |
|---|---|---|
| C++ code (namespaces, types, C++ macros, C-linkage symbols, C++ files) | `LoD`/`LOD`/`lod` | `lod::` namespace, `LOD_TEST`, `SDLPlatform`, `LoDPluginCreateV1`, `lod_test.hpp`, C++ macros `LOD_VERSION`/`LOD_CHANNEL` |
| Build system (CMake files, target names, CMake variables/options) | `Sc`/`SC`/`sc` | `ScToolchain.cmake`, `scengine`, `sc-tests`, `sc-test-framework`, `sc_noexcept`, `SC_SDL3_*`, `SC_ENABLE_ASAN` |
| Product (the app) | `lodsc` | the host executable (`LoD: Severed Chains`); window title / version strings keep the product name |

Applied: `lod::` namespace + `LOD_*` C++ macros + `SDLPlatform`/
`VersionString` naming in C++ code (kept); `LodToolchain/LodDeps/LodDocs`
reverted to `Sc*`; library/target names reverted to `scengine`, `sc-tests`,
`sc-test-framework`, `sc_noexcept`/`sc_nortti`/`sc_warnings`,
`sc_add_library`, `sc-api-docs`; options back to `SC_TESTS`/`SC_DOCS`/
`SC_ENABLE_*`; CMake metadata variables `SC_COMMIT_HASH`/
`SC_BUILD_TIMESTAMP`/`SC_CHANNEL` (injected to C++ as the `LOD_*` macros);
host target `severed-chains` → **`lodsc`**. Docs + AGENTS.md + cpp.md
synced to the split (incl. `sc-sdk`, `sc_qt_shim`, `sc_toolchain.cmake`,
`scgame`/`scdabas` targets with the `game` C++ namespace). `psx` stays
prefix-free (no Sc/SC/sc, no LoD/LOD/lod).

Other corrections from the same review:

1. **No Java-implementation mentions in code comments** — all `.java`/
   `log4j`/"reference" citations removed from code comments (SdlWindow,
   SdlPlatformManager, Config, InputBindings, Version.isRelease, Action);
   comments now state behavior directly. cpp.md §7 prohibits Java
   citations in code comments (they belong in `docs/`).
2. **cpp.md rules updated to the conventions** (owner: "apply our naming
   conventions to the cpp.md rules"): library namespaces only, acronym
   capitalization, member names must not repeat the class name,
   prefix-by-domain rule.
3. **Type/member naming**: `SdlPlatform` → `SDLPlatform`,
   `SdlVersionString` → `VersionString`; `sc::util`/`sc::platform`/
   `sc::app` merged into the single engine namespace `lod` (the `App` class
   has no namespace of its own; the test framework keeps `lod::test` as the
   separate test library's namespace).
4. **No `k` prefix on constants** (owner, 2026-10-19): convention updated
   (cpp.md §7 + AGENTS.md: PascalCase, no `k` prefix) and applied —
   `kWindowMinWidth`/`kWindowMinHeight` → `WindowMinWidth`/`WindowMinHeight`,
   `kOrigin` → `Origin`, psx `kMajorVersion`/`kMinorVersion`/`kPatchVersion`
   → `MajorVersion`/`MinorVersion`/`PatchVersion`, test-framework registrar
   `kRegistration_*` → `Registration_*`, the `kVersionInfo` instance became
   a function-local `static const VersionInfo info` (avoids the
   type-name collision), doc examples updated (`MaxVoices`,
   `T::ID`/`T::Version` in native-api.md; also fixed a leftover
   `"sc.combat"` string id → `"lod.combat"`).
5. **Acronyms fully capitalized in every position** (owner, 2026-10-19):
   `Id`/`Sdl`/`Api` → `ID`/`SDL`/`API` even inside PascalCase — the old
   "`Id`/`Ids` is a word, not an acronym" rule is **retired**. Only
   exception: a camelCase variable/field may *start* with the acronym
   meaning the acronym (`id`, `api`, `fps`). Convention updated (cpp.md §7
   + AGENTS.md); no C++ code identifiers violated it (audited — code
   already clean: `SDLPlatform`, `LogLevel`, etc.); docs examples fixed
   (`T::ID`, `EventID`, `subID`, `ID()` in native-api.md; dropped the
   bogus `Rtps` example). **The game library's namespace is `game`**
   (owner: "lodgame should simply be game"), not `lodgame`.

M1 work was committed by the owner as `aa253f2` ("Establish the C++23
application foundation") before this pass; the rename + correction changes
are uncommitted working-tree modifications.

## Phase 0 Follow-Up (prior session)

### 1. Three additional repositories researched (all read-only, all AGPL-3.0)

| Repo | What it is (verified) | Research doc |
|---|---|---|
| **Script-Recompiler** (`3433e6f`) | The *tooling* half of the story-script VM: `Disassembler` (LE u32 word array → token tree w/ register-type tracking), `Assembler` (op encoding `opcode \| paramCount<<8 \| headerParam<<16`; LodString 2-chars/word; relative pointer tables), `Translator`/`Tokenizer` (canonical text = the patch contract), `Patcher` (unified diff over stripped disassembly + `call` index↔name normalization), **fate compiler** (ANTLR grammar → opcodes); CLI `Shell` (versions/decompile/compile/genpatch/applypatch/undopatch/strip). Used **at runtime** by `GameEngine.SCRIPTS` (`legend.game.scripting.ScriptManager`) and at unpack by `ScriptPatcher`. **Game uses LOCAL meta** (`patches/meta/*.csv`); only the CLI fetches the remote `legendofdragoon.org/scmeta`. The shipped `patches/` dir (built into the app) has `scripts.csv` (DIFF/REPLACEMENT list), `scripts/*.diff`, fate `.txt` replacements (incl. `player_combat_script.txt` "Major rewrites to support modding"), `libs/` includes, `meta/`, `replacements/`. Apply = backup→disassemble(backup)→translate→diff/compile→assemble→write, with CRC-of-includes cache + restore-on-delete. | `docs/research/script-recompiler.md` |
| **Mod-Loader** (`c692134`) | Mod platform primitives (~1,390 LOC): `ModManager` (one **shared URLClassLoader** over all mod JARs — no per-mod isolation; Reflections `@Mod` scan; NPM-semver ranges; duplicate id → skip; required-mods gate; `pathToModId` mod-id tracking), `EventManager` (synchronous, **exact-class** dispatch, **priority HIGHEST→LOWEST**, stopPropagation/cancel, **critical events re-throw / non-critical → `errorCallback(modId, e)`**, weak instance-listeners), registries (`RegistryId` grammar `[a-z][a-z0-9_-]*` ≤255 chars; **lazy delegates** with use-time failure + mod attribution; `Registrar` lazy suppliers; registry lock), `LangManager` (4-level specificity **OVERRIDE>LANG_COUNTRY>LANG>FALLBACK**, `.lang` = Java properties UTF-8, mod load order = override order, key-as-fallback). | `docs/research/mod-loader.md` |
| **Dig-Dabas** (`4c1fbfe`) | **Mogul Dabas** — the original in-game *minigame* (a "digging" adventure: mushrooms/bones/nuggets/coins/hearts, caves, chests, monsters, bosses, HP, speech bubbles). Runs on a **fictional handheld virtual console**: ARMv4-labeled CPU, 32×32 1bpp display, mono-8 DAC SPU (upsampled to 44.1k), 3 free-running timers + wall-clock BCD RTC, power/clock control, 5 buttons. The ROM (`OHTA/MCX/DABAS.BIN`) comes from the **player's disc**; the Java code is a decompilation re-implementation (methods annotated `@Method(0x...)` with original-code labels), **not** an ARM emulator. Host integration (SC): `legend/game/dabas/Dabas.java` implements the 6-method `DabasInterface` (renderer hijack to 320×320 ortho, 5-button input map, audio, save via `DABAS_SAVE_DATA` config blob 0x260, shutdown); `DabasScreen` (in-game menu) + `DabasRewardsEvent` (gold/items/special rewards → inventory); crash isolation (VM crash → notification + clean shutdown, host survives). Retail provenance: retail memory addresses (`dabasData_8011d7c0`, `dabasMenuGlyphs_80114228`) + disc ROM. "Next Dig" = latest feature (2026-05-20). | `docs/research/dig-dabas.md` |

**Open questions added:** Q-MOD-1..3 (mod error-callback behavior;
built-in-mod scan scope; i18n retail-vs-mod precedence), Q-SCR-1..3
(fate compiler approach — **recommendation: hand-rolled parser, no new
dep**; multi-mod patch merging; entrypoint-count source), Q-DAB-1..4
(disruptor usage; **timer determinism — recommendation: host-driven
fixed step**; regional DABAS.BIN variants; **0x260 save grid byte
layout**).

### 2. Reusable PSX library added to the architecture

New doc `docs/architecture/psx-library.md`: an independent C++23 library
**`psx` (target `psx`, namespace `psx::`) — prefix-free (no Sc/SC/sc;
independent identity, may be extracted to its own repo), zero third-party
deps, no SDL/Qt/game logic, not a PS1 emulator**. Modules (all verified against
reference source): `psx::gte` (GTE math), `psx::gpu` (PS1 GS/GPU
semantics + the software reference rasterizer — parity oracle + legacy/
VRAM render modes), `psx::spu` (SPU voice model), `psx::xa` (XA ADPCM),
`psx::sssq` (SShd/SSSq parse), `psx::cdrom` (ISO 9660 Level 1),
`psx::tmd`/`tim`/`anm`/`archive` (MRG/DEFF)/`bpe`/`tag` (the generic
PS1-era formats). LoD-only containers (CContainer/MCQ/CMB) stay in
`game/`; the Dabas handheld is **not** PS1 hardware → `src/libs/dabas`
(target `scdabas`), not the `psx` library. Build rule: `psx` compiles
standalone (no engine/SDL/Qt/Luau). Updated: engine-design.md (module
tree + boundary rules), build-system.md (`psx`/`scdabas` targets, standalone
test target, CMake dependency audit), dependencies.md (in-repo zero-dep
libraries),
testing.md (§2.11 psx standalone golden suite), roadmap.md (psx starts
M2, completes M6, rasterizer M7).

### 3. Native plugin API corrected: **C++23, not a C ABI**

`docs/architecture/native-api.md` fully rewritten per the owner decision.
Summary:
- **C-linkage discovery entry only** (`extern "C" ScPluginCreateV<N>()` →
  `const sc::Plugin*`); all actual interfaces are C++ (abstract frozen
  classes, RAII, `std::string`, `std::expected`/`sc::Status`).
- **Binary compatibility = frozen-layout discipline** (no added virtuals/
  members on shipped classes; new capability = new interface class;
  additive-only within a major) **+ toolchain parity** (plugin built with
  the same compiler family + standard library; **toolchain hash checked at
  load** — makes the `std::`/vtable assumptions checked, not hoped).
- `EngineAPI` root (frozen) + `api.Table<T>()` typed capability discovery
  (Gameplay/Events/Input/Render/Audio/Assets/Save/Script/Combat/Log).
- No RTTI across public API boundaries (`-fno-rtti`/`/GR-` + CI probe);
  no exceptions (SDK toolchain file enforces); ownership = RAII
  (borrow/transfer/`Shared<T>`); handle-leak check at unload.
- Lifecycle state machine retained (discover→validate→load→init→ready→
  shutdown→unload); no hot reload v1; crash containment documented as a
  limitation; **Qt's plugin system explicitly NOT used**.
- ABI tests: oldest-plugin matrix, frozen-layout `static_assert` probe,
  toolchain-mismatch negative test, SDK throw/rtti compile probes.
Propagated to: AGENTS.md (boundaries), engine-design.md, luau-api.md,
error-handling.md, build-system.md (sc-sdk, probes), testing.md (§2.9),
roadmap.md (M10), licensing.md (arm's-length note).

### 4. Doxygen + naming conventions mandated

AGENTS.md now requires: **Doxygen-compatible comments on all public
project-owned C++ APIs** (engine, PSX library, plugin API — params,
returns, errors, ownership, thread safety; `doxygen` → `docs/api/` build
target) and the **naming conventions**:

| Element | Convention |
|---|---|
| Namespaces | abbreviated flatcase (`sc::render`, `psx::tmd`, `game::combat`) |
| Classes, structs, functions, methods, constants, enum values | PascalCase (acronyms fully capitalized: `EngineAPI`, `DecompressBPE`) |
| Variables and parameters | camelCase |
| Private fields | camelCase + trailing underscore |
| Macros | UPPER_SNAKE_CASE |
| C-linkage discovery symbols | PascalCase, `Sc` prefix (`ScPluginCreateV1`) |
| Files | snake_case |

Third-party API naming preserved; `Id`/`Ids` treated as a word (not an
acronym); string ids (registry/event/table ids) stay lowercase per the id
grammar.

### 5. `.clang-format` created

Root-level `.clang-format`: **LLVM baseline, 4-space indent, attached
braces, 120-column limit**, grouped/sorted includes (project-local first,
then sc/psx, SDL3, Luau, GLM, EnTT, system), `ReflowComments: false`
(preserves Doxygen blocks), `FixNamespaceComments`, `Standard: c++23`.
AGENTS.md documents the expectation (run on touched files; **no
mass-reformat** of untouched code).

### 6. Docs changed/created this session

**Created:** `docs/research/mod-loader.md`, `docs/research/script-
recompiler.md`, `docs/research/dig-dabas.md`, `docs/architecture/psx-
library.md`, `.clang-format`.
**Modified in place:** `AGENTS.md`, `progress.md`, `docs/research/modding-
analysis.md` (verified mod-loader/fate/i18n semantics; C++ API; open
questions resolved), `docs/research/licensing.md` (3 repos AGPL verified;
copyleft note updated), `docs/architecture/native-api.md` (full rewrite),
`docs/architecture/engine-design.md` (module tree: psx (root subproject),
libs/dabas, sdk; boundary rules; C++ plugin API), `docs/architecture/luau-api.md`,
`docs/architecture/error-handling.md`, `docs/architecture/build-system.md`
(targets, sc-sdk, RTTI probes, doxygen target, CI), `docs/architecture/
dependencies.md` (in-repo zero-dep libs; ANTLR4-C++ decision row),
`docs/architecture/testing.md` (§2.5 psx; §2.9 C++23; new §2.11–2.13;
renumbered), `docs/architecture/roadmap.md` (M2 psx start; M5 psx audio;
M6 decoders + **fate patch pipeline moved to M6** (verified: it runs at
unpack time); M7 psx::gte/gpu; M8 **Mogul Dabas**; M10 C++23 SDK; M12 fate
CLI).

### 7. Corrections to prior Phase 0 documents

- Fate script-patching: verified to run **at unpack time** (not a
  post-unpack tool-only path; not M12-only) — roadmap M6 now includes it;
  M12 is the CLI tool. Game uses **local** bundled meta CSVs (web fetch is
  CLI-only).
- Mod class-loading: one **shared** URLClassLoader (no per-mod isolation) —
  prior doc's "standard Java classloader per mod" corrected.
- RegistryId: grammar + ≤255-char limit + lazy-delegate semantics verified
  (prior doc had only the `modId:entryId` shape).
- Event dispatch: exact-class match + 5-tier priority + critical/non-
  critical error split verified (prior doc: "ordered listeners" only).
- "Script-Decompiler" = `Script-Recompiler` (actual directory name).

### 8. Post-follow-up revisions (owner preferences, 2026-10-18)

Owner decisions refined the follow-up deliverables (docs updated in
place; no new files):

1. **Naming — acronyms fully capitalized + PascalCase methods.** In
   PascalCase identifiers, acronyms are fully capitalized: `EngineAPI`,
   `GameplayAPI`, `XADecoder`, `GTEView`, `SFXVoice`, `DecompressBPE`
   (never `EngineApi`/`XaDecoder`). Struct/class **methods are PascalCase**
   (`OnLoad`, `QueryTable`, `Subscribe`, `EntitySpawn`), like free
   functions; camelCase stays for variables/parameters/private fields.
   C-linkage discovery symbols: PascalCase with `Sc` prefix
   (`ScPluginCreateV1`). `Id`/`Ids` treated as a word (matches the
   reference project's `RegistryId`/`modId`). Applied: AGENTS.md
   conventions table (examples + rules), native-api.md (full rename of all
   interfaces/methods), luau-api.md, roadmap.md, testing.md, psx-library.md
   (conventions section), progress.md. Third-party names (SDL_*, luau_*,
   Qt, verbatim Java citations) preserved.
2. **Build form — user's choice via CMake's built-in
   `BUILD_SHARED_LIBS`.** Project-owned libraries (`psx`, `scengine`,
   `scgame`, `scdabas`) are declared without a library type; CMake's
   built-in `BUILD_SHARED_LIBS` selects the build form at configuration
   time. The architecture prescribes no build form (no "static"/
   "dynamic" build-form terminology, no custom option — a drafted
   `SC_BUILD_STATIC` was superseded by the built-in). The sample plugin is
   the single explicitly-SHARED exception (must be loadable at runtime).
3. **PSX library is prefix-free.** The reusable PSX library must not carry
   the project prefix — it is separate from Severed Chains (and may become
   its own project): target `psx` (was `scpsx`), namespace `psx::` (unchanged),
   produced `psx.dll`/`libpsx.so`/`psx.dylib`, moved to a **root-level
   `psx/` subproject** (was `src/libs/psx`) so future extraction is
   mechanical. Its test target is `psx-tests` (was `sc-tests-psx`).
   `scdabas` keeps its prefix (LoD-specific first-party content). Updated:
   psx-library.md (identity + build form), build-system.md (layout, option,
   targets, CI, packaging), engine-design.md (tree + boundaries),
   testing.md, roadmap.md, dependencies.md, AGENTS.md, dig-dabas.md §5
   (build-form framing).

## Completed Research (Phase 0 original, unchanged)

All Phase 0 original docs remain valid: `docs/research/{source-
architecture,subsystem-inventory,rendering-analysis,shader-migration,asset-
pipeline,modding-analysis}.md`, `docs/architecture/{engine-design,
rendering-sdl3-gpu,qt-integration,native-api,luau-api,error-handling,
dependencies,build-system,testing,roadmap}.md`, `docs/research/licensing.md`
(updated). Key verified findings and architectural decisions are recorded
there and summarized in this file's history; the follow-up sections above
supersede the specific items listed in §7.

## Verified Findings (highlights — original Phase 0, still valid)

1. The Java project already runs on SDL3 for windowing/input
   (`SdlPlatformManager`); only graphics is raw LWJGL OpenGL 3.3/GLES 3.2.
2. Dynamic pacing: `Scus94491BpeSegment.gameLoop` reprograms render+input
   rate per frame: `hz = 60/max(1,vsyncMode)*speedMultiplier`; 30Hz sim in
   field areas; `timestamp_a0 += vsyncMode`.
3. Three render modes: modern shaders; legacy = per-primitive CPU GTE +
   software PS1 GS; VRAM = software GS presented. `Gpu.java` is a complete
   software GS (ordering table, top-left rule, PS1 blending,
   perspective-correct interpolation).
4. Texture decode in fragment shader (R32UI `tex15`, RGBA8 `tex24`), CLUT
   animation UBO, in-shader scissor.
5. Geometry shader (PS1 near-plane cull + per-primitive avg depth) replaced
   by CPU primitive pass + per-primitive buffer.
6. Two-pass translucency quirk preserved.
7. Addition timing: per-hit window from `AdditionHitProperties10`,
   attack/counter action check per sim frame; **not mod-pluggable in
   Java** → native `sc::CombatAPI::AdditionInputEvaluator` seam (shared
   with Luau).
8. Retail content is a mod (`LodMod`) → native game module registers
   content only through the mod API.
9. Saves: Tag binary tree, V8→V10, per-save configs, memcard import,
   crash-recovery.
10. Audio: software PS1 SPU music synth @120Hz + SFX SPU @60TPS + XA
    transcribed to Opus at unpack (native recommendation: PCM/WAV cache).
11. BPE codec + MRG/DEFF documented.
12. B+1/4F: Java modern path simplifies (B_PLUS_F); native implements
    exactly (in-shader 0.25 scale) — documented fidelity improvement.
13. SDL3 GPU verified API facts (shader formats, blending, push uniforms,
    left-handed NDC, format queries, backend requirements).
14. **Mogul Dabas** is original in-game content (disc ROM + retail memory
    addresses), integrated by SC's in-game menu; ported as `scdabas`
    (dig-dabas.md).

## Architectural Decisions (final, Phase 0 + follow-up)

- Engine library (`scengine`) + thin host; **`psx` (PSX tech — prefix-free,
  independent identity) + `scdabas` (minigame) as zero-dep libraries**; build
  form is the user's choice via CMake `BUILD_SHARED_LIBS`. Game content
  module registers everything through the mod API (LodMod pattern).
- Sim/render decoupled, rate-locked to original vsyncMode × speed.
- SDL3 GPU only; GLSL 450 → SPIR-V (glslang) → MSL (SPIRV-Cross) → DXIL
  (SPIRV-Cross+DXC); build-time only; content-hash cache.
- Geometry shader → CPU per-primitive cull/depth + primitiveParams buffer.
- EnTT for scenes/entities only; out of the plugin API.
- `std::expected<T, Status>` everywhere; `-fno-exceptions` + **`-fno-rtti`
  on public API targets** (CI probes); Qt qtbridge C-shim = documented
  exception.
- **Native plugin API = C++23** (C-linkage discovery only; frozen-layout
  interfaces; toolchain-parity rule; RAII; versioned interfaces). Qt's
  plugin system not used.
- Luau: one VM per mod, sandboxed, protected-call boundary; byte-exact Tag
  saves; registry/event model 1:1 with the verified Java catalog.
- **Doxygen on all public APIs; naming conventions + `.clang-format`
  (LLVM/4-space/attached/120) mandated.**
- Software GS port = optional fidelity mode + parity oracle (now `psx::gpu`).

## Unresolved Issues / Open Questions (consolidated)

1. **Owner confirmations:** native release license intent (AGPL-3.0,
   LICENSE in repo — confirm); intro.mp4 reuse; Discord scope (deferred to
   M11); FMVs (G5, likely no).
2. **Decisions (dependencies.md §2):** G1 shader tools (glslang/
   SPIRV-Cross/DXC); G2 SDL3_image (replaces stb_image — owner: we use
   SDL3, so the official SDL3 companion); G3 XA cache PCM/WAV (reco) vs libopus;
   G4 libcurl updater (reco yes); G5 ffmpeg (reco no); **new: fate
   compiler — ANTLR4-C++ (LGPL, runtime, exceptions) vs recommended
   hand-rolled parser (no new dep)**.
3. Q-MOD-1: mod error-callback behavior (permanent vs per-event disable).
4. Q-MOD-3: retail-vs-mod i18n precedence within one specificity.
5. Q-SCR-2: multi-mod `scripts.csv` patch merging in the reference build.
6. Q-SCR-3: per-script entrypoint count source (`ScriptFile`).
7. Q-DAB-2: Dabas timer determinism (reco: host-driven fixed step — needs
   sign-off, changes determinism character).
8. Q-DAB-4: Dabas 0x260 save blob tile-grid byte layout (32×16?).
9. Q-DAB-3: regional `DABAS.BIN` variants (US/EU/JP).
10. `legacyMode` 1 vs 2 exact difference (M7).
11. Full vsyncMode-per-state table (M8).
12. Which RNG draws affect gameplay determinism (M9).
13. EU/JP/Asia disc content differences (US = reference).
14. R32UINT/D32-FLOAT per-target support (runtime-verified M2/M3;
    fallbacks designed).
15. Qt 6.8 vs 6.9 LTS pin (M10/M11).
16. DXC-on-Linux/macOS prebuilts (M4).
17. Third-party mods' Java-specific reliance (migration backlog, M12).

## Blockers

- None for M1. M4 blocked on G1; M6 on G3 + the fate-compiler decision.
- Golden-fixture generation requires user cooperation (Java reference +
  discs + machine).

## Next Steps (superseded by the M1 section above — kept for history)

1. Confirm approval gates (G1/G2 + the fate-compiler decision at minimum).
2. Milestone 1: CMake skeleton + sc_noexcept/sc_nortti + SDL3 platform/
   window/input + config + diagnostics + shutdown; 3-OS matrix in CI.
3. In parallel: start the `psx` library (M2 scope) — dependency-free, golden-gated.
4. Golden-generation setup with the user (for M5+ fixtures).
</content>
</invoke>
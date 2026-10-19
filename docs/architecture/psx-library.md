# PSX Library — Reusable PlayStation 1 Technology

**Decision: an independent C++23 library (CMake target `psx`, namespace
`psx::`) is part of the native architecture.** It is the home for
PS1-specific technology that is reusable across PlayStation game projects
(this project, and potentially e.g. *Legend of Legaia*), kept strictly
separate from Legend of Dragoon game logic, Qt, and SDL3 GPU rendering.
It is **not** a PS1 emulator: it provides *semantics* (math, decoding,
state machines) and one software reference rasterizer — not CPU, timing,
or full-system emulation.

**Identity (owner decision):** the library is **prefix-free — no Sc/SC/sc, no LoD/LOD/lod
in any of its names** (CMake target `psx`, namespace `psx::`, source dir
`psx/` at the repository root). It is a separate project identity from
Severed Chains today and may be extracted to its own repository in the
future; the root-level placement keeps that extraction mechanical.
**Build form:** not prescribed by the architecture — the library is
declared without a library type and CMake's built-in `BUILD_SHARED_LIBS`
selects the build form at configuration time; that choice belongs to the
user (build-system.md §1).

## 1. Boundary

| Belongs in `psx/` (reusable PS1 tech) | Does NOT belong in `psx/` |
|---|---|
| GTE fixed-point math (geometry transform engine) | LoD game logic (characters, combat, additions, story, registries content) |
| PS1 GPU/GS *semantics*: primitive set, ordering table, top-left rule, depth test, blending modes (incl. B+1/4F), PS1 projection (H division), texture modes (fixed/CLUT/chroma key), per-primitive constant depth | The software GS's *render-engine integration* (batching, render targets, SDL3 GPU) — that's `lodengine::render` (rendering-sdl3-gpu.md) |
| PS1 GPU *software reference rasterizer* (port of the reference `Gpu`/`Graphics`) — the parity oracle and the "legacy"/"VRAM" render modes | The fictional **Dabas handheld** VM (not PS1 hardware — it's a game mechanic inside the game; lives in `libs/dabas`, dig-dabas.md §5) |
| PS1 SPU voice-model semantics (voice registers, FM/noise/reverb, key on/off, 4-bit PCM, pitch/ADSR) — the model, not a thread | The reference's 120 Hz music *synth* scheduling and its audio-thread design (`lodengine::audio` owns threading) |
| XA ADPCM decoding (PS1 CD audio format: 4-bit ADPCM, stereo/mono, block alignment, pre-skip) | Game-specific audio assets/sequences content |
| SShd / SSSq (PS1 SPU sound/sequence headers — generic PS1 audio formats) | The LoD-specific sequencer data layout details beyond the format |
| ISO 9660 (Level 1, PS1) reading: PVD/VDL, path + LBN/extent mapping, directory walk, sector I/O | LoD's 4-disc merge policy and `SCUS94491` region handling (`lodengine::assets` / `game`) |
| TMD/CTMD (GTE mesh), TIM/DABA (VRAM texture images), ANM (animation), MRG/DEFF (archives), BPE (byte-pair compression) — the *generic* PS1-era file formats, all with the same lineage across PS1 titles | LoD-only containers: CContainer (CLUT anims), MCQ (menu data), CMB (battle stages) — those decoders live in `game/` and emit `psx::` IR types |
| Tag serialization (the PS1-era save tag format LoD saves use) | The *save schema* (what a LoD save contains, V8→V10 migration content) — `game/` |
| CD-Rom sector I/O abstractions (read at LBN, multi-sector) | — |

**Independence rules (mandatory):**
- `psx` has **zero third-party dependencies** (C++23 standard library
  only). It defines its own fixed-point geometry types (MV/VC/SS/DV
  structs mirroring the GTE register semantics) rather than using GLM,
  so the library stays usable in any context (tools, tests, embedders).
- No SDL, no Qt, no Luau, no engine headers. `psx` compiles standalone
  (own target, own tests) — a hard build-system rule (build-system.md).
- No LoD strings, no `lod:` registry ids, no save-schema knowledge.
- Threading: `psx` is thread-*agnostic* — decoders are pure functions on
  input buffers; the rasterizer is single-context. Callers own
  synchronization (engine does).

## 2. Modules (verified against reference source)

Reference sources (paths relative to `~/Documents/GitHub/Severed-Chains`):

| `psx::` module | Reference source | Contents (verified) |
|---|---|---|
| `psx::gte` | `legend/core/gte/*` (`Gte.java`, `MV.java`, `VC.java`, `Ss.java`, `Dv.java`, `Mv.java`) | All GTE ops used by the game: `RTPS/RTD/RTT/RT1`, `NCST/NCSF`, `NCLIP` (near/far clip planes, `H`), `NCDT/NCSF`, `OP` (light), `NV`/`NS` (normal), `AVSZ/AVPT/AVOT/AVOT2` (average), `DTPT/DTLN`, `MAC3/MACH` (matrix), `NORM`, `C2F/C2D` (float conversion), `GPCI`; fixed-point conventions (S.x = 32.0 fixed in 24 bits? — port the exact shifts from `Gte.java`), flag accumulation (`flg`), the **`Gte` API used for camera matrices** (`worldToScreenMatrix` derivation, submap/battle camera) |
| `psx::gpu` | `legend/core/gpu/*` (`Gpu.java`, `Graphics.java`, `OrderingTable`, `Texture*`, `Bpp.java`) | PS1 GPU semantics + the **software reference rasterizer**: primitive set (3d flat/gouraud, 3d textured flat/gouraud, 2d lines/quads, sprites), **ordering table** (per-pixel/16x16-cell draw order, the reason the Java GS is pixel-exact), **top-left rule**, depth compare (per-primitive `z`, ZTEST on/off), **all PS1 blend modes** (B+F, B-F, B+1/4F, B+3/4F, B+F/2 (quarter variants), B+1/16F (5%? port exactly), no blend) — including the exact fixed-point color math (5-bit channels, 24-bit RGBA00 internal), chroma keying, CLUT fetch, **per-primitive constant depth** (`usePs1Depth` path), perspective-correct interpolation |
| `psx::spu` | `legend/core/audio/*` (`Spu.java`, `Voice.java`, `FmVoice`, `NoiseTable`, reverb) | PS1 SPU voice model: 24 voices, voice register layout (volume/ADSR, pitch, reverb/FM/noise flags, repeat), key on/off edge semantics, 4-bit PCM + FM + noise + reverb, sample-rate/shift handling — as a *deterministic voice processor* (input: control writes + sample data; output: PCM). The engine's audio thread (M5) drives it; the reference's 120 Hz music-synth *scheduling* (sequence playback) stays engine-side (it's LoD's music system, not PS1 hardware — though the SSSq/SShd *decoding* is `psx::sssq`) |
| `psx::xa` | `legend/core/audio/XaAdpcm*`, `XaTranscoder*` | PS1 XA 4-bit ADPCM decoder: frame headers (stereo/mono, blocks), the ADPCM coefficient filter, the reference's exact pre-skip (312 samples) and gain behavior — **PCM golden output must match the Java decoder byte/sample-exactly** (audio fixtures from the reference) |
| `psx::sssq` | `legend/core/audio/sequencer/*` + `SShd*`/`SSSq*` in unpacker | SShd (sound header: sample table) + SSSq (sequence: note events) *parsing* into IR structs; the *playback interpretation* (the music synth) is engine-side |
| `psx::cdrom` | `legend/game/unpacker/IsoReader.java` | ISO 9660 Level-1: PVD validation, volume/directory path table parse, LBN/extent resolution, sector-aligned reads, file tree build (the 4-disc *merge policy* is engine-side; the *reader* is generic) |
| `psx::tmd` | `legend/core/renderer/Tmd*`/`Ctmd*` + unpacker `CtmdTransformer` | TMD mesh format (vertex blocks, primitive blocks, lit/unlit, textured/untextured, gouraud/flat, tri/quad, material+texture pointers) + CTMD (scaled vertex/transform container, the `ctmdFlags` incl. `usePs1Depth`) → `psx::tmd::Mesh` IR (position/normal/uv/indices + per-primitive depth flag + lighting) |
| `psx::tim` | `legend/core/gpu/Bpp.java`, `Texture*`, DABA handling | TIM (tile image: tiles, CLUT) + DABA (VRAM image with absolute coords, stitching across multiple DABA files) → `psx::tim::Texture` IR (tiles/CLUT/coords + bpp) — the *decode*; the *GPU upload layout* (R32UI tex15 etc.) is engine-side (rendering-sdl3-gpu.md) |
| `psx::anm` | `legend/core/anim/*` (ANM) | ANM animation sequences (sprite sequences, per-frame CLUT offsets, timing) → IR |
| `psx::archive` | `legend/game/unpacker/MrgArchive.java`, `DeffArchive.java` | MRG (multi-record global archive: TOC, **virtual entries**, sector alignment) + DEFF (per-file container) → in-memory file map |
| `psx::bpe` | `legend/game/unpacker/Unpacker.decompress` (the `SCUS94491BPE` codec) | The adaptive byte-pair compression used by the disc (fully specified in asset-pipeline.md) — a generic codec: `compress` (for tooling/repair) + `decompress` (vectorized) |
| `psx::tag` | save system (Tag serializer in reference `saves` package) | The Tag binary tree format: node types (byte/short/int/float/string/double/…), length prefixes, string encoding, the serializer/parser — **byte-exact with the reference** (saves depend on it) |

IR types (`psx::tmd::Mesh`, `psx::tim::Texture`, ...) are plain
C++23 value types (POD-ish, `std::vector` of vertex/index/param structs) —
they are the contract between `psx/` decoders and the engine's GPU upload
paths. They do **not** cross the plugin ABI (native-api.md) and are not
seen by Luau (luau-api.md).

## 3. Why a separate library (vs. putting it in lodengine)

1. **Reuse requirement** (project goal: "Reusable engine technology must
   be kept separate... potentially reusable for other PS1 games"). A
   project with zero engine/SDL/Qt dependencies can be dropped into the
   next PS1 port unchanged — decoders, GTE math, and the GS reference
   rasterizer are exactly the pieces a *Legaia* port would share.
2. **Testing independence**: `psx` tests run with no windowing, no
   device, no game data directory — pure buffer-in/buffer-out golden
   tests (the fastest, most reliable test target in the repo).
3. **Layering**: `lodengine` (SDL3 GPU, audio threads, assets) and `game`
   (LoD) both *consume* `psx` — a shared dependency cannot live in either
   consumer.
4. **No-exceptions discipline**: a small, dependency-free surface is the
   easiest codebase to keep 100% exception-free and 100% Doxygen-documented
   (it's the reference implementation for the rest of the project).

## 4. Integration points

| Consumer | Use |
|---|---|
| `lodengine::render` | legacy/VRAM render modes run `psx::gpu`'s rasterizer (rendering-sdl3-gpu.md §3); GTE-derived camera matrices (`psx::gte`) feed the modern path's view matrices (M7); the rasterizer doubles as the **golden oracle** for the modern pipeline (testing.md) |
| `lodengine::audio` | the music synth + SFX engine drive `psx::spu` voice processors; `psx::xa` decodes the CD audio cache (M5/M6) |
| `lodengine::assets` | `psx::cdrom` (ISO read), `psx::bpe`/`psx::archive` (unpack), `psx::tmd`/`tim`/`anm` (decoders feeding the asset manager's IR) (M6) |
| `game/` | `psx::tag` (saves), `psx::sssq` (music data), LoD-only containers decoded into `psx::` IR (M6+), `psx::gte` (battle/camera math) |
| `tools/` (Qt) | asset inspector decodes via `psx::` (M12) |
| `libs/dabas/` | **none** (the Dabas handheld is not PS1 hardware — deliberate) |
| plugins / Luau | **none directly** — engine services expose *results*, never `psx::` types (ABI boundary stays clean) |

## 5. Conventions applied to psx

- C++23, no exceptions (`-fno-exceptions` target flag — build-system.md),
  `std::expected<T, psx::Error>` results for all fallible operations
  (error-handling.md taxonomy; each decoder defines specific error codes —
  malformed-class granularity per testing.md §2.11).
- **Doxygen on every public symbol** (AGENTS.md): params, returns, errors,
  ownership (IRs are value types; decoders take `std::span<const uint8_t>`
  in / owned IR out), thread safety ("free function, no shared state —
  thread-safe"; the rasterizer: "single-context, caller synchronizes").
- Naming: namespace `psx` (flatcase abbreviation), types PascalCase
  (`psx::tmd::Mesh`), free functions PascalCase with fully capitalized
  acronyms (`Rtps()`, `DecompressBPE()`), variables/parameters camelCase
  (`wordCount`), private members `camelCase_`, constants/macros
  UPPER_SNAKE_CASE. Fixed-point helpers carry their convention in the name
  (`ToF32`, `ToF0_31`) mirroring the GTE's.
- Golden-test-first: every module ships byte/sample-exact goldens vs the
  Java reference before the consuming milestone starts (testing.md §2.11,
  §2.14).

## 6. Migration mapping (from lodengine's original plan)

Phase 0's engine design assumed these pieces lived in `lodengine/ps1/`;
they move to `psx`:
- `lodengine/ps1/gte` → `psx` (gte)
- `lodengine/ps1/gpu` (software GS) → `psx` (gpu)
- `lodengine/ps1/decoders` (BPE/MRG/DEFF/TMD/TIM/ANM/XA/SShd/SSSq/ISO9660/
  Tag) → `psx` (per module above)
- `lodengine/ps1/spu` (voice model) → `psx` (spu); audio *scheduling*
  stays `lodengine::audio`
- **Stays in lodengine:** the R32UI/RGBA8 upload layouts, the 120 Hz music
  synth scheduler, the SFX voice *allocation* policy (LoD's Spu), the
  4-disc merge policy, the versioned `files/` cache, the low-memory mode.
- **Stays in game/:** CContainer/MCQ/CMB decoders, save schema +
  migrations, the Dabas integration.
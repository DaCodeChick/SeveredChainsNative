# Testing & Verification Strategy

Requirement §17: cover engine init/shutdown, exception-disabled compilation,
SDL3 GPU backends, shaders, resource lifetimes, asset decoding, gameplay
calculations, addition timing, save/load, Luau, native ABI, plugin lifecycle,
mod dependencies, cross-platform consistency, error handling. Use
measurements; propose tools only when necessity is established.

## 1. Test Framework

- **Decision: use a minimal dependency-free in-repo test runner**
  (`tests/framework/`): a ~200-line catch-like harness (test case
  registration via static initializers, `CHECK`/`REQUIRE` macros, section
  support, filter by tag, exit codes, no exceptions — checks are
  bool-based, failures abort the test case via longjmp-free control flow
  (early returns), never `throw`). Rationale: the approved stack has no test
  framework; adding Catch2/GTest is a new dependency requiring approval for
  zero gameplay-specific benefit; our needs (unit + golden + fuzz harnesses)
  fit a small in-repo harness. *If the team disagrees, the fallback is
  Catch2 (BSD) — flagged for approval in dependencies.md review.*
- One test binary per area (`sc_tests_engine`, `sc_tests_ps1`,
  `sc_tests_game`, `sc_tests_abi`, ...) so areas build/run independently;
  a `ctest`-style driver (CMake `add_test`) runs all.
- **No network, no game data in CI by default:** unit tests use synthetic
  fixtures (hand-built byte buffers for every format) + tiny generated
  assets. Data-dependent integration tests (real ISO) run locally / in a
  separate CI lane with a secret-provided sample (opt-in).

## 2. Per-Area Strategy

### 2.1 Engine init/shutdown & error handling
- Unit: each subsystem `init/deinit` round-trips; deinit-idempotency;
  partial-init failure (injected) → clean teardown + correct Status;
  **every Status code reachable** (error-path coverage table in the report).
- Exception-disabled probe (build-system.md §2): `throw` must not compile.
- Crash-path test: terminate handler writes crash log (temp dir).

### 2.2 SDL3 GPU device & backend selection
- Headless-ish smoke on CI where possible (Linux: Vulkan via CI GPU or
  `vulkan-1` softpipe? — softpipe supports Vulkan; run pipeline-creation
  test); macOS: Metal in CI runner (hardware available); Windows: D3D12 in
  CI runner.
- Tests: `SDL_CreateGPUDevice` succeeds with our shader-format mask on each
  CI platform; device name enumeration; **pipeline-creation matrix**: create
  every pipeline variant of every backend (shader-migration §6) — this is
  the "Vulkan, Metal, D3D12 rendering" verification for Milestone 3.
- Backend fallback: if one backend init fails, error reporting is correct
  (no silent fallback).

### 2.3 Shader compilation & loading
- Build-time: all shaders compile on all three targets (glslang/spirv-cross/
  dxc) — CI gates on zero warnings-as-errors (configurable).
- Cache: content-hash invalidation test (touch source → rebuild; unchanged
  → no rebuild).
- Runtime load: artifact load + pipeline create per backend; shader reload
  (dev mode) swaps pipelines without crash.

### 2.4 Resource lifetimes
- Unit: RAII wrapper null-state safety; deferred-delete ordering (resource
  used in frame N is destroyed ≥ frame N+2 — test with fence callbacks);
  cycling transfer buffer correctness (write while bound → cycles, no
  corruption — golden data check); stress: 10k create/destroy cycles.
- TSAN run of the audio thread + async loader (dedicated CI job).

### 2.5 Asset decoding (psx/ formats — scpsx, standalone)
- Runs in the standalone `sc-tests-psx` target (links scpsx only — the full
  suite, incl. GTE/GS goldens, is §2.11). **Round-trip tests per decoder**
  with synthetic + reference fixtures:
  BPE (compress→decompress identity on known data; the decompressor on a
  captured real BPE segment from a user-provided disc, opt-in lane), MRG
  (incl. virtual entries + sector-aligned), DEFF, TIM (all BPPs, CLUT/no
  CLUT), TMD (all primitive types: lit/unlit, textured/untextured,
  gouraud/flat, tri/quad, translucent), CTMD (scale application), ANM
  (sequence/sprite metrics), CContainer (frames + CLUT animations),
  MCQ, SShd/SSSq (subfile extraction), XA (decode a captured block →
  compare PCM against the Java `XaTranscoder`/`XaAdpcm` output — **golden
  PCM fixtures generated from the Java reference**, committed), Tag
  (serialize/deserialize identity + a real V10 save round-trip, opt-in
  lane), ISO9660 tree build (synthetic disc image fixture).
- Malformed input: truncation/corruption at every offset class → specific
  Status code (fuzz — see §3).

### 2.6 Gameplay calculations & combat
- **Golden-value tests**: port the damage/SP formulas (`combat/formula`
  package) as unit tests with the reference values (computed from the Java
  code paths on fixed inputs — generated during Phase 1 by running the
  Java reference in a harness; values committed as fixtures).
- **Addition timing** (the critical one): the timing-square evaluator is a
  pure state machine — property tests: (a) press at frame
  [lower..upper] → success; (b) before lower → fail-early; (c) after upper
  → fail-late; (d) wrong action → fail-wrong; (e) timing-window multiplier
  config scales bounds exactly (parity with Java `getHitProperty`);
  (f) auto modes (tutorial/auto) skip input. **Frame-exact**: run the
  battle state machine with a recorded input script and compare full
  sim-state snapshots (entity positions, turn order, HP, effects timeline)
  against Java reference snapshots (opt-in data lane; a small battle
  scenario suffices for the first cut).
- RNG: LCG sequence identity (port test: N steps from a seed == Java
  output fixture).
- Turn order, stat growth, level-up, encounter tables: unit tests from
  reference tables.

### 2.7 Save/load
- Tag V10 byte-exact: serialize(Java-deserialized data) == original bytes
  (fixtures: minimal save + a real save, opt-in lane).
- Migration chain V8→V9→V10 on fixtures; corrupt save → InvalidSaveResult;
  memcard import (synthetic DABA fixture + real `.mcr`, opt-in);
  crash-recovery save generation (injected mid-game crash → save loads);
  mod `modData` slot isolation (two mods' data survive a save/load; a mod
  removal leaves orphan data, not corruption).

### 2.8 Luau
- VM: per-mod isolation (one VM's error doesn't affect another);
  protected-call error paths (runtime error → Result with message, no
  exception); memory cap trigger; instruction budget trigger; stdlib
  reduction (no `os`/`io`/`debug` — negative test).
- API: each registered capability (character template, addition, item,
  event, config) round-trips through the registry and is visible to the
  game (integration tests with a script mod fixture); combat evaluator
  seam (a Luau button-sequence evaluator drives a mock battle hit — the
  Playable-Lloyd class scenario).
- Bytecode cache: version-keyed (Luau version bump → cache miss → recompile).
- Performance: per-frame hook overhead budget test (<50µs typical, p99).

### 2.9 Native plugin API & plugin lifecycle (C++23)
- **ABI stability tests** (the "native ABI compatibility" requirement,
  native-api.md §9.4): sample plugin compiled against SDK v1 (same
  toolchain) loads against engine vN and passes functional tests; interface
  versioning (`api.table<T>()` — unknown id/version → nullopt, known →
  works); **frozen-layout probe** — `static_assert`s on size/offset of every
  frozen SDK type, compiled against the SDK *and* the previous engine's SDK
  headers (CI catches drift); string lifetime rules (engine doesn't hold
  plugin strings past the call — ASAN detectable).
- **Toolchain-parity tests**: a plugin built with a *different* compiler
  family than the engine (toolchain hash mismatch) is **refused at load**
  with a clear diagnostic (negative test, per platform matrix); the SDK
  compile probes (throw / dynamic_cast TUs must fail to compile) run on the
  SDK headers with every CI compiler.
- **Lifecycle tests**: discover (entry symbol lookup V-max..V1) → validate
  (bad manifest, missing dep, version mismatch, toolchain mismatch, cycle)
  → load → init (`onLoad` success + failure injection) → ready → shutdown
  (handle-leak check) → unload; plugin crash isolation is *documented not
  testable in-process* (covered by the contract tests + the out-of-process
  future work).
- **Fuzzing** (dedicated `sc-fuzz` binary, libFuzzer): every plugin API
  surface exercised through a fuzz-host plugin with random args; every
  decoder with random/truncated buffers; Tag parser; config parser; unified-
  diff applier (fuzzed patch text). Run in CI (time-boxed) + ASAN/UBSAN.

### 2.10 Mod dependency resolution
- Unit: semver range matching (port Java's `^x.y.z` semantics — golden-test
  against semver4j NPM-mode fixtures: `^`, `~`, comparators, prerelease
  edge cases; mod-loader.md §2), dependency closure, cycles, missing deps
  (→ skip + report), duplicate ids, load order determinism (alphabetical +
  dependency order, golden test).

### 2.11 PSX library (scpsx) — standalone golden suite

`sc-tests-psx` links **scpsx only** (no engine/SDL/Qt) — the fastest test
surface in the repo, and the parity backbone (psx-library.md):
- **GTE math**: every op (RTPS/NCLIP/NCCS/AVSZ/MAC3/... ) against Java
  `Gte` outputs for a fixed input matrix (exact 32-bit integer equality —
  GTE is integer-fixed-point; no float tolerance needed); flag accumulation
  sequences; the camera-matrix derivations used by submap/battle cameras.
- **Software GS (psx::gpu)**: render a fixture scene → **byte-identical**
  to the Java `Gpu`/`Graphics` output (the parity oracle, rendering-
  analysis.md): ordering-table draws, top-left rule, every blend mode
  (incl. B+1/4F), per-primitive constant depth, CLUT/chroma-key paths.
- **Decoders**: BPE (compress→decompress identity + captured real segment),
  MRG (virtual entries, sector alignment), DEFF, TIM/DABA (all BPPs),
  TMD/CTMD (all primitive classes + `ctmdFlags`), ANM, XA (**sample-exact
  PCM vs the Java `XaAdpcm`/`XaTranscoder` fixtures, incl. the 312 pre-skip**),
  SShd/SSSq parse, ISO9660 tree (synthetic disc fixture), Tag
  (serialize/deserialize identity + real save bytes, opt-in lane).
- Malformed input: truncation/corruption at every offset class → the
  specific `psx::Error` code (fuzzed — §3).

### 2.12 Script tooling (psx::script + fate)
- **Round-trip**: disassemble→assemble == identity (byte-exact) on the full
  retail script set (opt-in data lane) + a synthetic-script corpus in-repo
  covering every op/param type, pointer-table overruns, LodStrings, fork/jmp
  tables.
- **Translator/Tokenizer**: text format round-trip (bytes → text → bytes),
  canonical-form stability (the patch contract — a re-run of translate on
  already-patched output is a no-op).
- **Unified diff component**: generate/apply/undo on fixture text (3 context
  lines, fuzz tolerance = none, failure → specific error); `call`-normalization
  (index↔name via meta) golden tests; the full reference patch corpus
  (`patches/scripts.csv` + all `.diff`/`.txt`) applies to the unpatched
  scripts and produces **byte-identical output to the Java unpacker's
  patched tree** (opt-in data lane — the strongest fate-pipeline proof).
- **Fate compiler** (hand-rolled, per Q-SCR-1 recommendation): compile the
  reference `patches/scripts/*.txt` fate files → byte-identical to the
  Java FateCompiler output (goldens); plus synthetic programs (control flow,
  functions, enums, includes, errors → structured diagnostics, no exceptions).

### 2.13 Mogul Dabas minigame (scdabas)
- **Console semantics**: timer/RTC/interrupt/power/DAC behavior unit tests
  (fixed-step host-driven ticks per dig-dabas.md Q-DAB-2 decision);
  32×32 1bpp framebuffer inversion parity with the Java host adapter.
- **RNG**: LCG+mix32 sequence identity (N draws from seed 1 == Java
  `dabas.core.Random` fixture — generated from the reference; distinct from
  the main-game RNG test).
- **Save byte-exactness**: `Save60` + tile grid (0x260 blob, grid layout per
  Q-DAB-4) serialize/parse identity + a Java-produced save round-trip.
- **Game logic**: a scripted input tape (5 buttons over N ticks) drives a
  fresh dig → rewards (gold/items/special) compared to a Java-session
  fixture (opt-in data); crash-isolation: an injected VM fault returns an
  error result and the host continues (parity with the Java
  try/catch-shutdown — the native error-path analog).

### 2.14 Cross-platform consistency
- **Golden output tests**: (a) BPE/MRG/TIM/TMD decode → byte-identical
  across OS/ABI (fixtures in repo); (b) GTE math → identical floats
  (32-bit arithmetic, no FMA contraction — compile with `-ffp-contract=off`
  where the reference math is GTE-quantized, documented); (c) Tag
  serialization bytes; (d) RNG sequence; (e) shader compile → identical
  SPIR-V hash across OS (tool pinned).
- CI matrix runs the golden suite on Win/Linux/macOS (arm64).

### 2.15 Rendering fidelity (measurement, not assumption)
- **Golden screenshots** (Milestone 7): fixed camera poses + fixed RNG +
  fixed clock → render N frames → compare against Java reference captures
  (per-scene: title, field (legacy + HQ projection), battle menu, addition
  overlay active, battle with PS1-depth CTMDs, translucent-heavy scene,
  CRT post on/off, 4:3 letterbox, widescreen submap). Metric: per-pixel
  diff with a tolerance (allow ±1/31 in 5-bit channel quantization, 0
  tolerance on geometry edges — reviewed visually when exceeded).
- **Timing tests**: sim tick rate measurement (vsyncMode 1 & 2, speed
  multiplier 1/2/4/16) against expected tick counts over a wall-clock
  window (±2%); addition window frame counts (deterministic sim → exact).
- **Audio measurement** (Milestone 5+): synth output PCM vs Java reference
  PCM for a fixed sequence/seed (spectral + sample diff), SFX voice
  timing (key-on latency), XA stream alignment (pre-skip 312 parity).

## 3. Fuzzing & Profiling Tools (proposed, necessity-established)

| Tool | Necessity |
|---|---|
| libFuzzer (Clang) / AFL++ (GCC) | established: 6+ external-format parsers + plugin API surface + unified-diff applier |
| ASAN/UBSAN/TSAN (compiler) | established: audio thread + async loader + plugin boundary |
| RenderDoc capture (Windows/Linux) + Xcode Metal capture (macOS) + D3D12 debug layer + Vulkan validation layers | established: SDL3 GPU debugging (per SDL docs; no new dependency — platform SDK facilities) |
| `perf`/Instruments/WTProfiler (platform profilers) | established: measurement-based performance work |
| **No** additional GUI/test profilers proposed in v1 (nothing else has demonstrated necessity yet) |

## 4. Verification Gating (milestone acceptance)

- Every milestone (roadmap.md) lists its concrete test gates; a milestone
  is not done until its gates pass on all three platforms (or the
  platform-specific gap is documented + user-acknowledged).
- Nightly: full unit + golden + fuzz (time-boxed) on the matrix.

## 5. Gaps / Open

- Java reference test harness for golden value generation: the reference
  repo's tests are excluded from its build (`test { exclude '**/*' }`) —
  generating goldens from Java requires running it (with a user's disc) in
  a dev harness; documented as a Phase 1 task (no CI dependency).
- D3D12 headless CI: limited; rely on the Windows runner's hardware.
- Audio golden fixtures: generate from the Java reference during Phase 1
  (same harness as gameplay goldens).

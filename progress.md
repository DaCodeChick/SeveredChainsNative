# Progress Log

## Reference Repository

- Path: `~/Documents/GitHub/Severed-Chains`
- Branch: `main`, commit: **`ab5dfe5ba6d4c07464a2f0e67985b9e77e2f7bdb`**
  (latest: "Merge branch 'main.update-readme'")
- **Read-only throughout Phase 0 — verified: `git status` clean, no writes.**
- Language/build: Java 25, Gradle. Key deps: LWJGL 3.4.3 (SDL3, OpenGL,
  OpenAL, Opus, STB, assimp, shaderc, SPIRV-Cross), JOML 1.10.8, JavaFX
  21.0.1, log4j2, JSON/GSON, fastutil, JNA, discord-sdk4j, javacv/ffmpeg.
- External org.legendofdragoon libraries (jitpack, separate repos, NOT in
  the reference repo): `mod-loader:4.3.5`, `script-recompiler:0.7.11`,
  `mogul-dabas:0.0.1` — usage-reconstructed from in-repo call sites;
  internals are a documented gap.

## Current Phase

**Phase 0 — Research and Architecture: COMPLETE.** Awaiting explicit
approval to begin Phase 1 (Milestone 1). No implementation code has been
written.

## Completed Research (all docs written to this repo)

### Research docs (`docs/research/`)

| Doc | Contents (all verified from source unless noted) |
|---|---|
| `source-architecture.md` | Project shape (1,326 Java files: core 244 / game 973 / lodmod 95 / turnorder 5), startup & shutdown sequence (Main→GameEngine.start, crash recovery save), main loop & threading (single-threaded loop, Action scheduler @60Hz input + render, OpenAL 120Hz thread, Async pool, JavaFX thread), per-frame gameLoop order, timing model (vsyncMode 30/60Hz, speed multiplier, frame-skip presentation), RNG/determinism, saves overview, graphics/audio/modding summaries, system relationship diagram, top-6 migration challenges |
| `subsystem-inventory.md` | Full package inventory with measured LOC, key classes, verified behavior, native mapping; porting size estimates (total ≈116k LOC; combat ≈40k = dominant); cross-cutting observations |
| `rendering-analysis.md` | Renderer architecture (RenderEngine/RenderApi/queues), frame flow (10 steps), render modes (OpenGL-HQ / legacy CPU-GTE / VRAM software-GS), PS1 GS state (Graphics/Gpu software rasterizer with ordering table + top-left rule), GTE (RTPS/RTPT/NCLIP/NCCS/AVSZ + flags), texture pipeline (Bpp, TIM, DABA, R32UI tex15 + RGBA8 tex24, CLUT animation, absolute-VRAM stitching), geometry (TMD vertex layout, CTMD, GS purpose), post shader (CRT chain), migration assessment table + risks |
| `shader-migration.md` | Full inventory of 12 shader files (6 pipelines) with uniforms/inputs; GLSL feature checklist (texelFetch R32UI, gl_FragDepth, in-shader scissor); **GS replacement decision** (option B: CPU cull + per-primitive params buffer); toolchain (glslang→SPIR-V→SPIRV-Cross→MSL/HLSL→DXC→DXIL) with approval request; binding layout for SDL3 GPU (sets/bindings, per-draw uniform buffer); pipeline variant plan incl. verified blending mapping (no QUARTER factor — B+1/4F done in-shader; Java's modern path doesn't even implement true B+1/4F — documented finding); build/caching/diagnostics; fidelity notes |
| `asset-pipeline.md` | Disc model (4 ISOs, region IDs, tree merge from disk 4), extraction pipeline (BPE codec fully documented, MRG virtual entries + sector alignment, DEFF, SMAP fixed offsets, per-file patchers incl. the Dart-hand bug fix, replacements, versioned cache), runtime loader, format table (TMD/CTMD/TIM/DABA/ANM/CContainer/MCQ/SShd/SSSq/XA/CMB/story scripts/saves/FMV), native pipeline design (source/cache/mod-override separation), save Tag format + V8-V10 migration + memcard import, risks (Opus decision, FMV licensing, region gaps) |
| `modding-analysis.md` | Mod model (JARs, @Mod/@EventListener, external mod-loader API surface, retail-as-mod `LodMod`, bundled `turnorder` mod), registries (RegistryId, Registrar, boot order), event bus (full catalog incl. battle events), config system (registry-driven, presets, keybinds), input actions, **addition timing input model verified** (attack/counter actions, window bounds from hit properties, timing-window config — and the missing mod seam), fate script-patching pipeline, i18n, capability list (what Java mods can/can't do), native migration table (package format, Luau/native parity, new combat seams), open questions |
| `licensing.md` | Reference = AGPL-3.0 verified (byte-identical LICENSE, 661 lines); native repo already carries the same file → release intent (needs owner confirmation); AGPL obligations for the native project (source availability, copyleft, plugin arm's-length analysis, no game data); dependency license table (Qt LGPL dynamic-link posture, etc.); mod licensing (per-mod SPDX metadata, no assumption of AGPL); 7 legal-review items |

### Architecture docs (`docs/architecture/`)

| Doc | Decision summary |
|---|---|
| `engine-design.md` | Module layout (engine/ vs game/ vs tools/qt vs plugins); app lifecycle + main loop (sim/render decoupled, fixed-step 30/60Hz from vsyncMode × speed); timing contract (sim frames = combat frames); RAII over SDL_GPU + upload ring + deferred destruction; event bus + registries (1:1 Java catalog); **EnTT only for scenes/entities** (component set defined; out of plugin ABI); story-VM port; save Tag byte-exact; config system; **content-as-mod** (game/ registers retail content exclusively through the mod API, LodMod parity); logging/diagnostics; key decisions table + risks |
| `rendering-sdl3-gpu.md` | Device creation (SPIRV\|MSL\|DXIL mask → SDL backend selection; vulkan/direct3d12/metal), frame structure (transfer→scene pass→post pass, ping-pong RGBA8 + D32/D24), pipeline variant plan (~20-40 pipelines, blending mapping verified incl. REVERSE_SUBTRACT and in-shader B+1/4F, depth-write-off translucent), shader interface conventions (explicit set/binding layouts, vertex attribute locations, **left-handed [0,1] NDC projection adaptation** — pure shader-side), texture system (R32UINT tex15 with CPU-decode fallback, samplers, upload, mod atlas), geometry submission (single-draw vs per-primitive primitiveParams for usePs1Depth — verified data-driven flag), 30Hz presentation choice, free-cam/debug port, verification plan |
| `qt-integration.md` | Qt 6 = desktop tools only (launcher, settings, mod manager, debugger, asset inspector, save inspector; fate CLI has no Qt); **separate processes** (clean SDL3/Qt coexistence answer); in-game UI stays SDL-rendered (parity options menu retained); engine shared-lib linking for tools; config file-lock sync; **no-exceptions × Qt-6 finding**: Qt uses exceptions internally → documented qtbridge C-shim boundary (the one flagged exception) + QML fallback plan; debugger = separate process over a debug socket (all 6 JavaFX panels mapped); launcher features; packaging |
| `native-api.md` | Engine-as-shared-library + thin host (required); single `sc_plugin_init/shutdown` entry; `sc_engine_interface` root + **versioned function tables** (`sc.gameplay.1`, `sc.events.1`, ...); POD-only data types; explicit ownership + status codes + thread-safety contract; lifecycle state machine (discover→validate→load→init→ready→shutdown→unload); **no hot reload in v1** (documented); crash containment limits (documented; out-of-process mode as future work); capability table map; **addition input evaluator C seam** (the Playable-Lloyd requirement); C++23 wrappers; ABI stability policy (export maps, semver, oldest-plugin CI test); non-goals |
| `luau-api.md` | One VM per mod (isolation); lifecycle mapped to Java mod boot; sandboxing (no os/io/debug, memory + instruction budgets, per-frame hook budget); **no-exceptions error path** (protected calls → engine Results, mod marked errored — mirrors onModError); namespace layout (Engine/Assets/Input/Render/Combat/Characters/Items/Saves/Audio/Script/Mod); **Java-capability→Luau parity table** (registries, events, configs, keybinds, i18n, asset override, modData byte-compat, fate, render, DABA); **extensible combat** (input evaluators, resolvers, battle actions, DEFFs, animation — one evaluator interface shared with the C ABI); mod package format (mod.toml, assets/, lang/, patches/, native/); JAR-mod migration path (5 tiers); native↔Luau independence; Luau pinning + bytecode cache; risks |
| `error-handling.md` | Compile enforcement (per-target flags + MSVC `/EHs-c-`, `throw`-probe CI test, terminate handler as diagnostic); **`std::expected<T, Status>` as the internal result type** (+ ABI int32); SDL3 error mapping; RAII null-state discipline + composition pattern; async task Results; audio-thread leniency (log+drop); ErrorContext + asset error taxonomy; **failure-semantics table** (13 situations → defined behavior); test hooks |
| `dependencies.md` | Approved stack pinned (SDL3 ≥3.2, GLM, EnTT, Luau exact, Qt6 LTS, CMake/Ninja); **approval requests**: glslang, SPIRV-Cross, DXC (all build-time, MIT), stb_image (PNG), + decisions: libopus vs PCM/WAV (reco: PCM/WAV v1), libcurl (reco: yes for updater), ffmpeg (reco: out of v1); **third-party exception-behavior register** (Qt documented limitation + mitigations); version pinning + supply chain; maintenance review cadence |
| `build-system.md` | Target map (scengine SHARED / scgame STATIC / host / Qt tools / fate CLI / plugin examples / tests); sc_noexcept interface + MSVC verification; dependency resolution (FetchContent pinned + find_package SDL3/Qt6 with platform managers); shader build integration (glslang→spv→msl/dxil, caching, diagnostics, install layout); configurations (Debug/RelWithDebInfo/Release + SC_RELEASE parity gating; sanitizers; Apple Silicon arm64 native); **clean-clone procedure + per-platform prereqs**; packaging/install layout (no game data); CI matrix (Win MSVC+clang-cl, Ubuntu GCC+Clang, macOS arm64); risks (D3D12 headless CI, MSVC C++23 min VS version, DXC availability) |
| `testing.md` | In-repo minimal test harness (no new dep; Catch2 fallback flagged if rejected); per-area strategy: init/shutdown + error-path coverage, GPU backend/pipeline-creation matrix, shader build/cache/load, resource lifetimes (fence-verified) + TSAN, asset decoders (round-trip + malformed + fuzz; **golden PCM from Java reference**), gameplay (formula goldens, **addition timing property suite**, battle snapshot diff, RNG identity), saves (byte-exact V10, migrations, memcard, crash recovery), Luau (isolation, budgets, parity API, bytecode cache), **native ABI (oldest-plugin matrix, lifecycle, fuzz)**, mod deps (semver closure golden), cross-platform consistency (golden bytes + `-ffp-contract=off` where GTE-quantized), **rendering fidelity (golden screenshots incl. software-GS byte-identical oracle)**, timing/audio measurement; fuzz/profiler tools (necessity-justified only); milestone gating; gaps (Java golden-generation harness needs user data) |
| `roadmap.md` | 12 milestones refined with scope/prereqs/deliverables/acceptance/verification; **dependency approval gates G1-G5** tied to milestones; M9 split into 9a/9b/9c (core/additions/transformations); M10 = mod system + Luau + native ABI live; M11 = feature parity audit + launcher + updater + Discord; M12 = fate pipeline + custom character + alt combat + Qt tool suite + total-overhaul proof; cross-milestone notes (goldens from Java reference, vertical slice at 9a) |

## Verified Findings (highlights with source refs)

1. **The Java project already runs on SDL3** for windowing/input
   (`SdlPlatformManager`, SDL3 event API) — only graphics is raw LWJGL
   OpenGL 3.3/GLES 3.2. The native port's platform layer is a 1:1 mapping.
2. **Dynamic pacing**: `Scus94491BpeSegment.gameLoop` reprograms render+input
   tick rate per frame: `hz = 60/max(1,vsyncMode)*speedMultiplier`;
   30Hz sim in field areas; timestamp += vsyncMode. (source-architecture §5)
3. **Three render modes**: default modern shaders; legacy = per-primitive
   CPU GTE + software PS1 GS; VRAM = software GS presented. `Gpu.java` is a
   complete software GS (ordering table, top-left rule, PS1 blending,
   perspective-correct interpolation) — ported as fidelity oracle.
4. **Texture decode in fragment shader**: 4/8bpp palette + 15-bit colors in
   one R32UI texture (`tex15`), 24bpp in RGBA8 (`tex24`); CLUT animation UBO
   remap; in-shader scissor (Intel iGPU workaround, Shirley fight comment).
5. **Geometry shader** (tmd.gsh): PS1 near-plane cull (`2z >= H`) +
   per-primitive average depth (`usePs1Depth`, data-driven per model).
   Replaced by CPU primitive pass + per-primitive buffer (shader-migration §3).
6. **Two-pass translucency quirk** (opaque pixels of translucent primitives
   first, unblended) preserved via `discardTranslucency` uniform.
7. **Addition timing**: per-hit window from `AdditionHitProperties10`
   (overlayHitFrameOffset + totalSuccessFrames), attack/counter action check
   per sim frame, timing-window config multiplier, auto modes. Input
   evaluator is NOT mod-pluggable in Java → native seam designed.
8. **Retail content is a mod** (`LodMod`, `@Mod(id="lod")`) — the native
   game module follows the same pattern (content registers only through
   registries/events).
9. **Saves**: Tag binary tree, V8→V10 migration, per-save configs, original
   memory-card import (DABA Save60), crash-recovery saves.
10. **Audio**: software PS1 SPU music synth @120Hz (24 voices, FM/noise/
    reverb, configurable fidelity) + SFX SPU @60TPS + XA CD audio
    **transcoded to Opus at unpack** (runtime streams it).
11. **BPE compression** (adaptive byte-pair, the `SCUS94491BPE` codec) fully
    specified from `Unpacker.decompress`; MRG (virtual entries, sector
    alignment) + DEFF archive formats documented.
12. **B+1/4F blending**: Java's modern GL path does NOT implement it
    (`GlApi.translucency` throws; RenderEngine renders B+1/4F models with
    B_PLUS_F state) — a known reference simplification; native will
    implement it exactly (in-shader 0.25 scale) — a deliberate, documented
    fidelity improvement over the modern path (software-GS path was exact).
13. **SDL3 GPU verified API facts** (SDL 3.2.0+): shader formats SPIRV/DXBC/
    DXIL/MSL; device creation takes the offered formats and selects the
    backend; blending factors (no QUARTER; REVERSE_SUBTRACT exists); 4 push
    uniform slots/stage; cycling resource model; left-handed NDC z[0,1];
    R32UINT/D32-FLOAT support must be queried; backend requirements
    (Vulkan 1.0+exts, D3D12 FL11.0+RBT2, macOS 10.14+ Metal).

## Architectural Decisions (final, Phase 0)

- Engine shared lib (`scengine`) + thin host; game content module registers
  everything through the mod API (LodMod pattern).
- Sim/render decoupled, rate-locked to original vsyncMode × speed; sim
  frames = combat timing frames.
- SDL3 GPU only (no RHI); GLSL 450 source → SPIR-V (glslang) → MSL (SPIRV-
  Cross) → DXIL (SPIRV-Cross+DXC); build-time only; content-hash cache.
- Geometry shader → CPU per-primitive cull/depth + primitiveParams buffer.
- EnTT for scenes/entities only; out of plugin ABI.
- `std::expected<T, Status>` results everywhere; `-fno-exceptions` enforced
  per target with a CI probe; Qt qtbridge C-shim = documented exception.
- Luau: one VM per mod, sandboxed, protected-call boundary; byte-exact Tag
  saves; registry/event model 1:1 with Java catalog.
- Native C ABI: versioned function tables, POD only, no hot reload v1,
  crash containment documented as a limitation.
- Qt 6: separate processes for all tools; in-game UI stays SDL-rendered.
- Software GS port = optional fidelity mode + parity oracle.
- Test harness in-repo (no new dep) unless the team prefers Catch2.

## Unresolved Issues / Open Questions

1. **Owner confirmations**: native release license intent (AGPL-3.0, LICENSE
   already in repo — confirm); intro.mp4 reuse rights; Discord feature in
   scope (deferred to M11 by default); FMVs in scope (G5, likely no).
2. **External repos** (mod-loader, script-recompiler, mogul-dabas) not in
   the reference repo — API reconstructed from usage; exact semantics
   (classloader isolation, event ordering, semver edge cases) unverified.
   Suggestion: fetch read-only for M10 prep (user approval).
3. `legacyMode` 1 vs 2 exact behavioral difference (both route through the
   software GS; 2 may skip the modern pipeline entirely) — verify if fidelity
   modes ship (M7).
4. vsyncMode table per engine state — partially verified (submap=2 typical);
   full table extracted during M8.
5. Which RNG draws affect gameplay determinism (crits? encounters?) — verify
   during M9 (RNG identity test is designed regardless).
6. EU/JP/Asia disc content differences (IDs recognized; behavior unverified) —
   documented gap, US is the reference.
7. R32UINT sampling + D32-FLOAT on every target driver — runtime-verified in
   M2/M3 (fallbacks designed: CPU unpack to RGBA8; D24).
8. Qt 6.8 vs 6.9 LTS pin — at M10/M11 implementation time.
9. DXC-on-Linux/macOS prebuilt availability — verify at M4 (fallback:
   commit DXIL artifacts from Windows CI).
10. Released third-party mods' reliance on Java-specific mechanics — unknown;
    tracked in the mod-migration backlog (M12).

## Dependencies Requiring Approval (task §2 / dependencies.md §2)

**Build-time shader tools (gate G1, needed for M4):**
- [ ] glslang (MIT) — GLSL→SPIR-V
- [ ] SPIRV-Cross (MIT) — SPIR-V→MSL/HLSL
- [ ] DXC (MIT) — →DXIL (fallback: committed DXIL artifacts)

**Runtime (trivial):**
- [ ] stb_image or SDL_stb_image (public domain/zlib) — PNG decoding (gate G2, M2)

**Decisions (no new dep if recommended options chosen):**
- [ ] G3: XA cache format — **recommendation: PCM/WAV at unpack (no libopus)**
- [ ] G4: launcher auto-update — **recommendation: approve libcurl (MIT)**
- [ ] G5: FMVs — **recommendation: out of v1 (no ffmpeg)**

**Fallback if in-repo harness rejected:**
- [ ] Catch2 (BSD) test framework (testing.md §1)

## Blockers

- None for M1. M4 blocked on G1 approval (tool downloads/builds).
- Golden-value generation (M5+) requires user cooperation: a machine with
  the Java reference + legally obtained discs to generate reference
  fixtures (snapshots, audio PCM, script traces, screenshots).

## Next Steps (Phase 1 start, upon approval)

1. Confirm approval gates (G1/G2 at minimum; G3/G4/G5 decisions).
2. Milestone 1: CMake skeleton + sc_noexcept + SDL3 platform/window/input +
   config + diagnostics + shutdown; 3-OS build matrix in CI.
3. In parallel (no code conflict): request user's Java-reference golden
   generation setup (discs + run instructions) for M5+ fixtures.
4. (Optional, user approval) read-only fetch of the three org.legendofdragoon
   external repos to close open question #2.

## Session Continuity Notes

- This log + AGENTS.md are the resume point. All findings are persisted in
  `docs/`; do not re-investigate what the docs mark verified.
- Reference commit for all citations: `ab5dfe5ba6d4c07464a2f0e67985b9e77e2f7bdb`.
- Native repo state: docs only (no source code yet); initial commit `904858d`
  (LICENSE + .gitignore) preserved.
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

## Architectural Boundaries

- **Engine (reusable):** platform/lifecycle, SDL3 GPU rendering, input, audio, assets,
  scene/ECS, animation, save serialization, scripting host, native plugin API, timing.
- **Game (LoD-specific):** characters, combat, Additions, dialogue/events, submaps,
  world map, saves content, retail registries — the equivalent of the Java `lod` mod.
- **Desktop tools (Qt 6):** launcher, mod manager, asset inspection, debuggers.
- **Native plugin ABI:** stable versioned C ABI, opaque handles, no STL across the
  boundary, no EnTT registry exposure, versioned function tables.
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
- All project-owned targets built with exceptions disabled (see above).
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
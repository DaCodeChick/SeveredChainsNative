# Build System — CMake + Ninja

Requirements: CMake + Ninja; reproducible builds on Windows / Linux / macOS
(Apple Silicon); exceptions disabled on all project-owned targets; stable
pinned dependencies; no unnecessary submodules; documented clean-clone
procedure + platform prerequisites; shader compilation integrated; shared
engine library + thin host; Debug/Release configurations; packaging.

## 1. Top-Level Layout

```
CMakeLists.txt                # root: project(), C++23, options, subdirs
cmake/
  ScToolchain.cmake           # compiler flags, exception policy, sanitizers
  ScDeps.cmake                # dependency resolution (FetchContent/vcpkg/Qt6)
  ScShader.cmake              # shader build rules (glslang/spirv-cross/dxc)
  ScPackaging.cmake           # install rules, bundle layout
  versions.cmake              # pinned versions (single source of truth)
src/engine/CMakeLists.txt     # scengine (STATIC or SHARED) + scengine-internal
src/game/CMakeLists.txt       # scgame (links scengine; the "lod" content lives here)
src/tools/qt/CMakeLists.txt   # launcher, settings, modmanager, debugger, inspector
src/tools/fate/CMakeLists.txt # fate CLI (pure C++, no Qt)
src/plugins/examples/         # sample plugin (SHARED, C ABI)
tests/                        # gtest-free? — see testing.md (use Catch2? no —
                              #   decision: use a minimal in-repo test runner to
                              #   avoid another dep; see testing.md)
```

**Targets:**
| Target | Type | Links |
|---|---|---|
| `scengine` | SHARED (dynamic symbol prefix `sc_`, hidden visibility) | SDL3, GLM (hdr), EnTT (hdr), Luau, stb_image, (libcurl/libopus optional) |
| `scgame` | STATIC (compiled into the host; the "lod" content module) | scengine |
| `severed-chains` (host) | EXECUTABLE (thin) | scgame, scengine |
| `severed-chains-launcher` / `-settings` / `-modmanager` / `-debugger` / `-inspector` | EXECUTABLE | scengine + Qt6 |
| `fate` | EXECUTABLE | scengine (script/fate parts) |
| `scplugin_example` | SHARED (plugin, C ABI) | scengine headers only (runtime-resolved) |
| `sc-tests`, `sc-fuzz` | EXECUTABLE | per testing.md |

## 2. Exception-Disabled Enforcement (the core requirement)

`ScToolchain.cmake`:
```cmake
# Applied to EVERY project-owned target via a shared INTERFACE target
add_library(sc_noexcept INTERFACE)
if(MSVC)
  target_compile_options(sc_noexcept INTERFACE /EHs-c- /w14458)
  # Verify: a probe TU with `throw` must fail to compile (CMake try_compile
  # at configure time, error if it compiles — CI regression guard)
else()
  target_compile_options(sc_noexcept INTERFACE -fno-exceptions)
endif()
target_compile_features(sc_noexcept INTERFACE cxx_std_23)
```
- Every target: `target_link_libraries(<t> PRIVATE sc_noexcept)`.
- **Qt bridge exception:** `qtbridge` sources (the narrow C-shim TUs,
  qt-integration.md §2) get a `sc_qt_shim` interface *without* the flag
  (documented exception; all other Qt tool code keeps `-fno-exceptions`).
- **Verification steps (Milestone 1 acceptance):**
  1. `try_compile` probe with `throw` fails on Clang/GCC/MSVC.
  2. Build with `-Werror` on the project sources.
  3. Link a TU that references a symbol with an EH personality → link
     failure expected (optional, toolchain-dependent).
- Third-party: SDL3/EnTT/GLM/Luau/stb compile fine with exceptions disabled
  (verified in dependencies.md §3); Qt is the documented boundary.

## 3. Dependency Resolution

- **Strategy:** `FetchContent` with pinned tags + SHA (no submodules, per
  requirement) for: GLM, EnTT (header-only), Luau (source build — its
  `prebuilt/VM` via its CMake), stb_image (single file). This makes the
  clean-clone build self-contained given network access; CI mirrors
  (cache) for reproducibility.
- **SDL3:** platform package manager preferred (avoids building SDL from
  source): vcpkg (Windows) with pinned port version; system/brew (Linux/macOS)
  via `find_package(SDL3)`. Fallback: FetchContent SDL3 source build (slower,
  for CI hermeticity). **Decision:** `find_package` first, FetchContent
  fallback — documented.
- **Qt6:** `find_package(Qt6 COMPONENTS Core Gui Widgets [Network])`;
  Windows via vcpkg or the Qt installer (vcpkg `qtbase`); macOS via brew
  `qt@6`; Linux via system (`libqt6*`) or vcpkg. Pin the major.minor in
  `versions.cmake` (e.g. `Qt6 6.8`).
- **Shader tools (glslang/SPIRV-Cross/DXC):** downloaded as pinned prebuilt
  release artifacts (or built from source in CI) into the build dir;
  recorded hashes in `versions.cmake`. They are *executables*, not linked
  libs — no runtime footprint.

## 4. Shader Build Integration (`ScShader.cmake`)

- Custom commands per stage (shader-migration.md §4):
  ```
  .vsh/.fsh (GLSL) --glslang--> .spv (Vulkan; -V --target-env vulkan1.2
      + explicit set/binding layouts in source)
  .spv --spirv-cross--> .metal (MSL 2.4, --msl-ios / msl-version as needed)
  .spv --spirv-cross--> .hlsl --dxc--> .dxil (SM 6.0+; verify min SM for used
      features at M4)
  ```
- Artifacts land in `${CMAKE_BINARY_DIR}/shader-cache/<hash>/` and are
  **installed** next to the binary (`<bindir>/../resources/shaders/...`);
  the runtime loads them (or from `--shaders-dir` override for dev reload).
- Diagnostics: tool stderr is captured; on failure CMake prints
  file:line:col (glslang/dxc native) and the failing command.
- Optional `SC_SHADER_VERBOSE` prints the tool invocations.

## 5. Configurations & Optimizations

- `CMAKE_BUILD_TYPE`: `Debug` (O0, asserts on, SDL debug on, GPU debug flag
  on, Luau JIT off) / `RelWithDebInfo` (O2, asserts off, GPU debug off —
  the "dev build" of Java's non-release channel) / `Release` (O3,
  LTO off by default, strips symbols; "isRelease" parity gates: free-cam
  off, FPS title off — port `Version.isRelease()` semantics to a build
  define `SC_RELEASE`).
- LTO: opt-in (`-flto`/`/GL`+`/LTCG`) behind `SC_ENABLE_LTO=ON` (measure
  first — no assumption).
- Sanitizers: `SC_ENABLE_ASAN/UBSAN/TSAN` options (dev-only; TSAN to
  validate the audio-thread design).
- Apple Silicon: default arch `arm64` (native) — no x86_64 slice required
  for v1 (documented; universal2 possible later via `CMAKE_OSX_ARCHITECTURES`).
- Reproducibility: `-gno-record-gcc-switches`-style flags where supported,
  deterministic timestamps off (`__DATE__` banned in code — version info
  comes from CMake defines: commit hash, timestamp at configure time),
  sorted link inputs where practical. (Full bit-reproducible builds are a
  *goal*, documented as best-effort in v1.)

## 6. Clean-Clone Build Procedure (per platform)

**Common:**
```
git clone <this repo> && cd SeveredChainsNative
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
# shaders compile during the build (tool auto-download per versions.cmake)
./build/bin/severed-chains            # game (needs isos/ per asset-pipeline.md)
```

**Prerequisites:**
| Platform | Prereqs |
|---|---|
| Windows | Visual Studio 2022 (MSVC v143+, C++23) or clang-cl; Ninja; vcpkg (SDL3, Qt6) or system SDL3; DirectX SDK headers (with VS); shader tools auto-downloaded |
| Linux | GCC 13+ (or Clang 16+ with libc++ for full C++23), Ninja, `libsdl3-dev` (≥3.2), `qt6-base-dev`, libvulkan (driver) for Vulkan; SDL3 GPU note: Metal not applicable; D3D12 not applicable |
| macOS (Apple Silicon) | Xcode 15+ (Clang, Metal SDK), Ninja, `brew install sdl3 qt@6` (or vcpkg); Metal for GPU; Vulkan via MoltenVK is *not* required (Metal is native) but optional if the user prefers Vulkan |

## 7. Packaging & Installation

- **Windows:** per-tool `windeployqt`-style bundling of Qt DLLs; game dir =
  `severed-chains.exe` + `scengine.dll` + SDL3 DLL + resources + `mods/` +
  `isos/` + `files/` (created at first run); NSIS/MSI installer optional in
  v1 (zip-first, like the Java distribution model).
- **macOS:** `.app` bundle (game + engine framework); notarization notes
  documented at packaging time; Qt tools as separate `.app`s.
- **Linux:** tarball or distro packaging later; v1 = tarball with system
  deps documented.
- **Install layout (prefix):**
  ```
  bin/               # host + tools
  lib/               # scengine(.dll/.so/.dylib), (Qt shared by system)
  resources/         # shaders (compiled), fonts, UI art (non-copyrighted)
  share/             # gamecontrollerdb.txt (SDL game controller db — shipped
                     #   like the Java project), default configs
  ```
- No game data in any package (requirement: legal user data only).

## 8. CI Plan (GitHub Actions — mirrors the reference's workflow shape)

- Matrix: Windows (MSVC + clang-cl), Ubuntu (GCC + Clang), macOS (arm64 +
  x86_64 runners if available).
- Jobs: build (all targets) → unit tests → ABI/plugin tests → shader
  golden compile → (per-platform) headless GPU smoke test (Milestone 2+) →
  pack artifacts.
- Caching: vcpkg dirs, FetchContent git caches, shader tool artifacts.

## 9. Risks

1. SDL3 3.2 GPU maturity on D3D12 (youngest backend) — mitigation:
   feature-query at runtime (`SDL_GPUTextureSupportsFormat`, pipeline
   creation tests per backend in CI headless mode where possible — D3D12
   headless is hard; covered by the Windows CI runner).
2. MSVC C++23 completeness (`std::expected` needs MSVC 19.34+ / VS 17.10+
   — pin the minimum VS version; `std::expected` available; `std::print`
   not used; ranges fine).
3. Shader tool cross-platform builds — prebuilt artifacts mitigate; DXC on
   Linux from MS release assets (verify availability at M4).
4. Qt via vcpkg on macOS can be slow/flaky — brew fallback documented.

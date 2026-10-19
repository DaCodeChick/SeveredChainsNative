// Compiler policy probe (build-system.md §2): this TU MUST FAIL to compile
// under the sc_noexcept flags. A compile error is the expected, verified
// outcome. Do not "fix" this file.
//
// Two mechanisms, so no toolchain can accept the TU:
//  1. Preprocessor guard — hard error when the compiler reports C++
//     exception handling enabled (_CPPUNWIND on MSVC; __EXCEPTIONS on
//     Clang/GCC). Catches a missing /EHs-c- / -fno-exceptions flag
//     regardless of the toolchain's diagnostic behavior.
//  2. An actual `throw` — a hard compile error under all three toolchains
//     when exception handling is disabled.
#if defined(_CPPUNWIND) || defined(__EXCEPTIONS)
#  error "C++ exceptions are enabled — the sc_noexcept policy flags are not in effect"
#endif

int main()
{
  throw 42;
  return 0;
}
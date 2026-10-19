// Compiler policy probe (build-system.md §2): this TU MUST FAIL to compile
// under the sc_nortti flags. Do not "fix" this file.
//
// Two mechanisms, so no toolchain can accept the TU:
//  1. Preprocessor guard — hard error when the compiler reports RTTI
//     enabled (_CPPRTTI on MSVC; __GXX_RTTI on Clang/GCC). Catches a
//     missing /GR- / -fno-rtti flag regardless of the toolchain's
//     diagnostic behavior.
//  2. An actual RTTI use — dynamic_cast across a polymorphic hierarchy:
//     a hard compile error under Clang/GCC -fno-rtti; on MSVC /GR- this
//     is only warning C4541 ("used on polymorphic type with /GR-;
//     unpredictable behavior may result"), so the probe build adds /WX
//     to promote it to an error (ScToolchain.cmake).
#if defined(_CPPRTTI) || defined(__GXX_RTTI)
#  error "RTTI is enabled — the sc_nortti policy flags are not in effect"
#endif

struct Base
{
  virtual ~Base() = default;
};

struct Derived : Base
{
};

int main()
{
  Base base;
  return dynamic_cast<Derived*>(&base) ? 1 : 0;
}
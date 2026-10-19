// Compiler policy probe (build-system.md §2): this TU MUST FAIL to compile
// under the sc_noexcept flags. A compile error is the expected, verified
// outcome. Do not "fix" this file.
int main()
{
  throw 42;
  return 0;
}
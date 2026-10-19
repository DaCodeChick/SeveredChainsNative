// Compiler policy probe (build-system.md §2): this TU MUST FAIL to compile
// under the lod_nortti flags (dynamic_cast across classes requires RTTI; a
// compile error is the expected, verified outcome). Do not "fix" this file.
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
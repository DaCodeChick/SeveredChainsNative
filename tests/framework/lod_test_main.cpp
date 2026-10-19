// Test binary entry point (one per test binary; the framework is shared).
#include "lod_test.hpp"

int main(int argc, char** argv)
{
  return lod::test::RunAll(argc, argv);
}
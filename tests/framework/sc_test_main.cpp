// Test binary entry point (one per test binary; the framework is shared).
#include "sc_test.hpp"

int main(int argc, char** argv)
{
  return sc::test::RunAll(argc, argv);
}
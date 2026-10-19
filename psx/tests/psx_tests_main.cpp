// psx-tests entry point (self-contained; no host-repo framework).
#include "psx_test.hpp"

#include <cstdlib>

int main(int argc, char** argv)
{
  return psx::test::RunAll(argc, argv);
}
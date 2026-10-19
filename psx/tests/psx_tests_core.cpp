// psx core smoke tests (M1: library target established; module suites land
// with their modules per testing.md §2.11).
#include "psx_test.hpp"

#include <psx/core/error.hpp>
#include <psx/core/version.hpp>

#include <string_view>

PSX_TEST(CoreVersion, "core")
{
  PSX_REQUIRE(psx::MajorVersion == 0);
  PSX_REQUIRE(psx::MinorVersion == 1);
  PSX_REQUIRE(psx::PatchVersion == 0);
  const std::string_view version = psx::VersionString();
  PSX_REQUIRE(version == "0.1.0");
  return true;
}

PSX_TEST(CoreErrorNames, "core")
{
  PSX_REQUIRE(psx::NameOf(psx::Error::Ok) == "Ok");
  PSX_REQUIRE(psx::NameOf(psx::Error::DecodeMalformed) == "DecodeMalformed");
  PSX_REQUIRE(psx::NameOf(psx::Error::DecodeTruncated) == "DecodeTruncated");
  // Distinct codes must not alias in diagnostics.
  PSX_REQUIRE(psx::NameOf(psx::Error::DecodeMalformed) != psx::NameOf(psx::Error::DecodeTruncated));
  return true;
}

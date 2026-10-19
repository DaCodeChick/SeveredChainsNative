// Status code tests (M1: error-handling.md §2 taxonomy).
#include "lod_test.hpp"

#include <string_view>
#include <util/status.hpp>

LOD_TEST(StatusOkIsZero, "status")
{
  LOD_REQUIRE(static_cast<int>(lod::Status::Ok) == 0);
  return true;
}

LOD_TEST(StatusNames, "status")
{
  LOD_REQUIRE(lod::ToString(lod::Status::Ok) == "Ok");
  LOD_REQUIRE(lod::ToString(lod::Status::NotFound) == "NotFound");
  LOD_REQUIRE(lod::ToString(lod::Status::InvalidArgument) != lod::ToString(lod::Status::NotSupported));
  LOD_REQUIRE(lod::ToString(lod::Status::Error) == "Error");
  return true;
}
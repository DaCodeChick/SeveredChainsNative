// Status code tests (M1: error-handling.md §2 taxonomy).
#include "sc_test.hpp"

#include <string_view>
#include <util/status.hpp>

SC_TEST(StatusOkIsZero, "status")
{
  SC_REQUIRE(static_cast<int>(sc::Status::Ok) == 0);
  return true;
}

SC_TEST(StatusNames, "status")
{
  SC_REQUIRE(sc::ToString(sc::Status::Ok) == "Ok");
  SC_REQUIRE(sc::ToString(sc::Status::NotFound) == "NotFound");
  SC_REQUIRE(sc::ToString(sc::Status::InvalidArgument) != sc::ToString(sc::Status::NotSupported));
  SC_REQUIRE(sc::ToString(sc::Status::Error) == "Error");
  return true;
}
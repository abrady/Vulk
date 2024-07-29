#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch.hpp>

#include "Vulk/Vulk.h"

void testAssertPasses() {
    VULK_ASSERT(true);
}

void testAssertMsgPasses() {
    VULK_ASSERT_FMT(true, "This is a test message");
}

void testAssertFails() {
    VULK_ASSERT(false);
}

void testAssertMsgFails() {
    VULK_ASSERT_FMT(false, "This is a test message");
}

TEST_CASE("VulkException tests") {
    REQUIRE_NOTHROW(testAssertPasses());
    REQUIRE_NOTHROW(testAssertMsgPasses());
    REQUIRE_THROWS(testAssertFails());
    REQUIRE_THROWS(testAssertMsgFails());
}

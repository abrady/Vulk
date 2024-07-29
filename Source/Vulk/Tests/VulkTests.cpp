#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch.hpp>

#include "Vulk/Vulk.h"

void testAssertPasses() {
    VULK_ASSERT(true);
}

void testAssertMsgPasses() {
    VULK_ASSERT(true, "This is a test message");
}

void testAssertFails() {
    VULK_ASSERT(false);
}

void testAssertMsgFails() {
    VULK_ASSERT(false, "This is a test message");
}

TEST_CASE("VulkException tests") {
    try {
        VULK_ASSERT(false);
    } catch (VulkException& e) {
        std::string s = e.what();
        CHECK(s.find("false") == 0);
    }
    try {
        VULK_ASSERT(false, "fmt check {}", 1);
    } catch (VulkException& e) {
        std::string s = e.what();
        CHECK(s.find("fmt check 1") == 0);
    }
    REQUIRE_NOTHROW(testAssertPasses());
    REQUIRE_NOTHROW(testAssertMsgPasses());
    REQUIRE_THROWS(testAssertFails());
    REQUIRE_THROWS(testAssertMsgFails());
}

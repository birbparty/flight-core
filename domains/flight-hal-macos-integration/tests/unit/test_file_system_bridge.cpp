#include <catch2/catch_test_macros.hpp>
#include "flight/hal/macos/file_system_bridge.hpp"

using namespace flight::hal::macos;

TEST_CASE("FileSystemBridge creation", "[unit][file_system_bridge]") {
    auto bridge = FileSystemBridge::create();
    REQUIRE(bridge != nullptr);
}

TEST_CASE("Basic file operations", "[unit][file_system_bridge]") {
    auto bridge = FileSystemBridge::create();
    REQUIRE(bridge != nullptr);
    
    // Test file existence check
    bool exists = bridge->exists("/tmp");
    REQUIRE(exists == true); // /tmp should always exist on macOS
}

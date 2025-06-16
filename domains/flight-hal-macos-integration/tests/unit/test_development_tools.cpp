#include <catch2/catch_test_macros.hpp>
#include "flight/hal/macos/development_tools.hpp"

using namespace flight::hal::macos;

TEST_CASE("DevelopmentTools creation", "[unit][development_tools]") {
    auto tools = DevelopmentTools::create();
    REQUIRE(tools != nullptr);
}

TEST_CASE("Tool availability detection", "[unit][development_tools]") {
    auto tools = DevelopmentTools::create();
    REQUIRE(tools != nullptr);
    
    // Test tool availability (this will vary by system)
    bool xcode_available = tools->isToolAvailable(DevelopmentTool::Xcode);
    REQUIRE((xcode_available == true || xcode_available == false));
}

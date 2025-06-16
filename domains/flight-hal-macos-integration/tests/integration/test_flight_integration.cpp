#include <catch2/catch_test_macros.hpp>
#include "flight/hal/macos/platform_coordinator.hpp"
#include "flight/hal/macos/process_manager.hpp"
#include "flight/hal/macos/file_system_bridge.hpp"
#include "flight/hal/macos/development_tools.hpp"

using namespace flight::hal::macos;

TEST_CASE("Full Flight HAL integration", "[integration][flight_hal]") {
    // Create all components
    auto coordinator = PlatformCoordinator::create();
    auto process_manager = ProcessManager::create();
    auto file_system = FileSystemBridge::create();
    auto dev_tools = DevelopmentTools::create();
    
    REQUIRE(coordinator != nullptr);
    REQUIRE(process_manager != nullptr);
    REQUIRE(file_system != nullptr);
    REQUIRE(dev_tools != nullptr);
    
    // Test basic integration
    SECTION("Component coordination") {
        auto result = coordinator->initializeAppleSiliconOptimizations();
        REQUIRE((result == CoordinationResult::Success || result == CoordinationResult::NotSupported));
    }
}

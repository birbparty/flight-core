#include <catch2/catch_test_macros.hpp>
#include <unistd.h>
#include "flight/hal/macos/process_manager.hpp"

using namespace flight::hal::macos;

TEST_CASE("ProcessManager creation", "[unit][process_manager]") {
    auto manager = ProcessManager::create();
    REQUIRE(manager != nullptr);
}

TEST_CASE("Process priority setting", "[unit][process_manager]") {
    auto manager = ProcessManager::create();
    REQUIRE(manager != nullptr);
    
    // Test priority setting (will use current process PID)
    int current_pid = getpid();
    bool result = manager->setPriority(current_pid, ProcessPriority::Normal);
    REQUIRE(result == true);
}

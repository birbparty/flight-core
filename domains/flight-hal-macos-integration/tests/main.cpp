#include <catch2/catch_test_macros.hpp>

// Test main is provided by Catch2::Catch2WithMain
// This file can contain global test setup if needed

// Global test setup
struct GlobalTestSetup {
    GlobalTestSetup() {
        // Initialize any global test resources
    }
    
    ~GlobalTestSetup() {
        // Cleanup any global test resources
    }
};

static GlobalTestSetup global_setup;

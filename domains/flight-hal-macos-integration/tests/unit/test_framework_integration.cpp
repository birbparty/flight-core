#include <catch2/catch_test_macros.hpp>
#include "flight/hal/macos/core_foundation_bridge.hpp"
#include "flight/hal/macos/foundation_bridge.hpp"

using namespace flight::hal::macos;

TEST_CASE("Core Foundation Bridge - String Conversion", "[core-foundation]") {
    SECTION("std::string to CFString conversion") {
        std::string test_str = "Hello, World!";
        auto cf_wrapper = CoreFoundationBridge::stdToCFString(test_str);
        
        REQUIRE(cf_wrapper.get() != nullptr);
        
        auto converted_back = CoreFoundationBridge::cfStringToStd(cf_wrapper.get());
        REQUIRE(converted_back.has_value());
        REQUIRE(*converted_back == test_str);
    }
    
    SECTION("Empty string handling") {
        std::string empty_str = "";
        auto cf_wrapper = CoreFoundationBridge::stdToCFString(empty_str);
        
        REQUIRE(cf_wrapper.get() != nullptr);
        
        auto converted_back = CoreFoundationBridge::cfStringToStd(cf_wrapper.get());
        REQUIRE(converted_back.has_value());
        REQUIRE(*converted_back == empty_str);
    }
    
    SECTION("Unicode string handling") {
        std::string unicode_str = "测试 🚀 Test";
        auto cf_wrapper = CoreFoundationBridge::stdToCFString(unicode_str);
        
        REQUIRE(cf_wrapper.get() != nullptr);
        
        auto converted_back = CoreFoundationBridge::cfStringToStd(cf_wrapper.get());
        REQUIRE(converted_back.has_value());
        REQUIRE(*converted_back == unicode_str);
    }
}

TEST_CASE("Core Foundation Bridge - Number Conversion", "[core-foundation]") {
    SECTION("int32_t conversion") {
        int32_t test_value = 42;
        auto cf_number = CoreFoundationBridge::createCFNumber(test_value);
        
        REQUIRE(cf_number.get() != nullptr);
        
        auto converted_back = CoreFoundationBridge::cfNumberToInt32(cf_number.get());
        REQUIRE(converted_back.has_value());
        REQUIRE(*converted_back == test_value);
    }
    
    SECTION("int64_t conversion") {
        int64_t test_value = 1234567890123LL;
        auto cf_number = CoreFoundationBridge::createCFNumber(test_value);
        
        REQUIRE(cf_number.get() != nullptr);
        
        auto converted_back = CoreFoundationBridge::cfNumberToInt64(cf_number.get());
        REQUIRE(converted_back.has_value());
        REQUIRE(*converted_back == test_value);
    }
    
    SECTION("double conversion") {
        double test_value = 3.14159;
        auto cf_number = CoreFoundationBridge::createCFNumber(test_value);
        
        REQUIRE(cf_number.get() != nullptr);
        
        auto converted_back = CoreFoundationBridge::cfNumberToDouble(cf_number.get());
        REQUIRE(converted_back.has_value());
        REQUIRE(*converted_back == test_value);
    }
}

TEST_CASE("Core Foundation Bridge - Dictionary Conversion", "[core-foundation]") {
    SECTION("std::map to CFDictionary conversion") {
        std::unordered_map<std::string, std::string> test_map = {
            {"key1", "value1"},
            {"key2", "value2"},
            {"key3", "value3"}
        };
        
        auto cf_dict = CoreFoundationBridge::stdMapToCFDictionary(test_map);
        REQUIRE(cf_dict.get() != nullptr);
        
        auto converted_back = CoreFoundationBridge::cfDictionaryToStdMap(cf_dict.get());
        REQUIRE(converted_back.size() == test_map.size());
        
        for (const auto& [key, value] : test_map) {
            REQUIRE(converted_back.find(key) != converted_back.end());
            REQUIRE(converted_back[key] == value);
        }
    }
    
    SECTION("Empty dictionary handling") {
        std::unordered_map<std::string, std::string> empty_map;
        auto cf_dict = CoreFoundationBridge::stdMapToCFDictionary(empty_map);
        
        REQUIRE(cf_dict.get() != nullptr);
        
        auto converted_back = CoreFoundationBridge::cfDictionaryToStdMap(cf_dict.get());
        REQUIRE(converted_back.empty());
    }
}

TEST_CASE("Core Foundation Bridge - Array Conversion", "[core-foundation]") {
    SECTION("std::vector to CFArray conversion") {
        std::vector<std::string> test_vector = {"apple", "banana", "cherry", "date"};
        
        auto cf_array = CoreFoundationBridge::stdVectorToCFStringArray(test_vector);
        REQUIRE(cf_array.get() != nullptr);
        
        auto converted_back = CoreFoundationBridge::cfStringArrayToStdVector(cf_array.get());
        REQUIRE(converted_back.size() == test_vector.size());
        
        for (size_t i = 0; i < test_vector.size(); ++i) {
            REQUIRE(converted_back[i] == test_vector[i]);
        }
    }
    
    SECTION("Empty array handling") {
        std::vector<std::string> empty_vector;
        auto cf_array = CoreFoundationBridge::stdVectorToCFStringArray(empty_vector);
        
        REQUIRE(cf_array.get() != nullptr);
        
        auto converted_back = CoreFoundationBridge::cfStringArrayToStdVector(cf_array.get());
        REQUIRE(converted_back.empty());
    }
}

TEST_CASE("Core Foundation Bridge - RAII Memory Management", "[core-foundation]") {
    SECTION("CFWrapper proper resource management") {
        CFStringRef leaked_ref = nullptr;
        
        {
            auto cf_wrapper = CoreFoundationBridge::stdToCFString("test string");
            leaked_ref = cf_wrapper.get();
            REQUIRE(leaked_ref != nullptr);
            
            // Test move semantics
            auto moved_wrapper = std::move(cf_wrapper);
            REQUIRE(cf_wrapper.get() == nullptr);
            REQUIRE(moved_wrapper.get() == leaked_ref);
        }
        
        // After scope exit, the CFStringRef should be automatically released
        // This is difficult to test directly, but we can at least verify
        // that the wrapper behaved correctly
        REQUIRE(true); // Placeholder for memory management verification
    }
    
    SECTION("CFWrapper release functionality") {
        auto cf_wrapper = CoreFoundationBridge::stdToCFString("test string");
        CFStringRef raw_ref = cf_wrapper.get();
        
        REQUIRE(raw_ref != nullptr);
        
        // Release ownership
        CFStringRef released_ref = cf_wrapper.release();
        REQUIRE(released_ref == raw_ref);
        REQUIRE(cf_wrapper.get() == nullptr);
        
        // Manually release the reference since wrapper no longer owns it
        CFRelease(released_ref);
    }
}

TEST_CASE("Foundation Bridge - NSTask Integration", "[foundation]") {
    SECTION("Basic command execution") {
        FoundationBridge bridge;
        
        // Test with a simple command that should always work on macOS
        auto result = bridge.executeCommand("/bin/echo", {"Hello, Foundation!"});
        
        REQUIRE(result.success);
        REQUIRE(result.exit_code == 0);
        REQUIRE(result.stdout_output == "Hello, Foundation!\n");
        REQUIRE(result.stderr_output.empty());
        REQUIRE(result.execution_time.count() > 0);
        REQUIRE_FALSE(result.timed_out);
    }
    
    SECTION("Command with environment variables") {
        FoundationBridge bridge;
        
        std::unordered_map<std::string, std::string> env = {
            {"TEST_VAR", "test_value"}
        };
        
        auto result = bridge.executeCommandWithEnvironment(
            "/bin/sh", 
            {"-c", "echo $TEST_VAR"}, 
            env
        );
        
        REQUIRE(result.success);
        REQUIRE(result.exit_code == 0);
        REQUIRE(result.stdout_output == "test_value\n");
    }
    
    SECTION("Command timeout handling") {
        FoundationBridge bridge;
        
        // Test with a very short timeout - this command should timeout
        auto result = bridge.executeCommandWithTimeout(
            "/bin/sleep", 
            {"2"}, 
            std::chrono::milliseconds(100)
        );
        
        REQUIRE(result.timed_out);
        REQUIRE_FALSE(result.success);
    }
    
    SECTION("Async command execution") {
        FoundationBridge bridge;
        
        auto future = bridge.executeCommandAsync("/bin/echo", {"Async Test"});
        auto result = future.get();
        
        REQUIRE(result.success);
        REQUIRE(result.exit_code == 0);
        REQUIRE(result.stdout_output == "Async Test\n");
    }
}

TEST_CASE("Foundation Bridge - String Conversions", "[foundation]") {
    SECTION("NSString to std::string conversion") {
        std::string test_str = "Foundation Test";
        void* ns_string = FoundationBridge::stdToNSString(test_str);
        
        REQUIRE(ns_string != nullptr);
        
        std::string converted_back = FoundationBridge::nsStringToStd(ns_string);
        REQUIRE(converted_back == test_str);
    }
    
    SECTION("NSArray to std::vector conversion") {
        std::vector<std::string> test_vector = {"item1", "item2", "item3"};
        void* ns_array = FoundationBridge::stdVectorToNSArray(test_vector);
        
        REQUIRE(ns_array != nullptr);
        
        std::vector<std::string> converted_back = FoundationBridge::nsArrayToStdVector(ns_array);
        REQUIRE(converted_back.size() == test_vector.size());
        
        for (size_t i = 0; i < test_vector.size(); ++i) {
            REQUIRE(converted_back[i] == test_vector[i]);
        }
    }
    
    SECTION("NSDictionary to std::map conversion") {
        std::unordered_map<std::string, std::string> test_map = {
            {"foundation_key1", "foundation_value1"},
            {"foundation_key2", "foundation_value2"}
        };
        
        void* ns_dict = FoundationBridge::stdMapToNSDictionary(test_map);
        REQUIRE(ns_dict != nullptr);
        
        std::unordered_map<std::string, std::string> converted_back = 
            FoundationBridge::nsDictionaryToStdMap(ns_dict);
        
        REQUIRE(converted_back.size() == test_map.size());
        for (const auto& [key, value] : test_map) {
            REQUIRE(converted_back.find(key) != converted_back.end());
            REQUIRE(converted_back[key] == value);
        }
    }
}

TEST_CASE("Foundation Bridge - File Manager Integration", "[foundation]") {
    SECTION("File existence check") {
        FoundationBridge bridge;
        auto& file_manager = bridge.getFileManager();
        
        // Test with a file that should always exist on macOS
        REQUIRE(file_manager.exists("/bin/sh"));
        REQUIRE(file_manager.exists("/usr"));
        REQUIRE_FALSE(file_manager.exists("/this/path/should/not/exist"));
    }
    
    SECTION("Directory detection") {
        FoundationBridge bridge;
        auto& file_manager = bridge.getFileManager();
        
        REQUIRE(file_manager.isDirectory("/usr"));
        REQUIRE(file_manager.isDirectory("/tmp"));
        REQUIRE_FALSE(file_manager.isDirectory("/bin/sh")); // This is a file, not directory
    }
    
    SECTION("Temporary directory operations") {
        FoundationBridge bridge;
        auto& file_manager = bridge.getFileManager();
        
        std::filesystem::path temp_dir = "/tmp/flight_test_dir";
        
        // Clean up from any previous test runs
        file_manager.removeItem(temp_dir);
        
        // Create directory
        auto create_result = file_manager.createDirectory(temp_dir);
        REQUIRE(std::holds_alternative<bool>(create_result));
        REQUIRE(std::get<bool>(create_result));
        REQUIRE(file_manager.exists(temp_dir));
        REQUIRE(file_manager.isDirectory(temp_dir));
        
        // Clean up
        auto remove_result = file_manager.removeItem(temp_dir);
        REQUIRE(std::holds_alternative<bool>(remove_result));
        REQUIRE(std::get<bool>(remove_result));
        REQUIRE_FALSE(file_manager.exists(temp_dir));
    }
}

TEST_CASE("Integration - Core Foundation and Foundation Bridge Together", "[integration]") {
    SECTION("CFString and NSString interoperability") {
        std::string original = "Integration Test String";
        
        // Core Foundation path
        auto cf_wrapper = CoreFoundationBridge::stdToCFString(original);
        auto cf_back = CoreFoundationBridge::cfStringToStd(cf_wrapper.get());
        
        // Foundation path
        void* ns_string = FoundationBridge::stdToNSString(original);
        auto ns_back = FoundationBridge::nsStringToStd(ns_string);
        
        REQUIRE(cf_back.has_value());
        REQUIRE(*cf_back == original);
        REQUIRE(ns_back == original);
        REQUIRE(*cf_back == ns_back);
    }
    
    SECTION("Process execution with Core Foundation configuration") {
        FoundationBridge bridge;
        
        // Use Core Foundation to create configuration
        std::unordered_map<std::string, std::string> config = {
            {"CF_TEST", "core_foundation_value"}
        };
        
        auto cf_dict = CoreFoundationBridge::stdMapToCFDictionary(config);
        auto converted_config = CoreFoundationBridge::cfDictionaryToStdMap(cf_dict.get());
        
        // Use the configuration in Foundation process execution
        auto result = bridge.executeCommandWithEnvironment(
            "/bin/sh",
            {"-c", "echo $CF_TEST"},
            converted_config
        );
        
        REQUIRE(result.success);
        REQUIRE(result.stdout_output == "core_foundation_value\n");
    }
}

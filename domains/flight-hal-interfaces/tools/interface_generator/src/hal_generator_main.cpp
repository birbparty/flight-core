/**
 * @file hal_generator_main.cpp
 * @brief Command-line interface for HAL Code Generator
 * 
 * Provides a command-line tool for generating HAL driver implementations,
 * tests, and configuration files from interface definitions.
 */

#include "hal_code_generator.hpp"
#include <iostream>
#include <string>
#include <map>
#include <filesystem>

using namespace flight::hal::codegen;

namespace {

/**
 * @brief Print usage information
 */
void print_usage(const char* program_name) {
    std::cout << "Flight HAL Code Generator\n";
    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -t, --type TYPE        Generation type (driver|test|config|doc)\n";
    std::cout << "  -i, --interface IFACE  Interface name (memory|graphics|audio|input|file|network|time|thread|performance)\n";
    std::cout << "  -p, --platform PLAT    Target platform (windows|linux|macos|embedded|dreamcast|generic)\n";
    std::cout << "  -c, --class CLASS      Generated class name\n";
    std::cout << "  -n, --namespace NS     Target namespace (default: flight::hal::drivers::PLATFORM)\n";
    std::cout << "  -o, --output DIR       Output directory\n";
    std::cout << "  --templates DIR        Templates directory (default: ./templates)\n";
    std::cout << "  --interfaces DIR       Interfaces directory (default: ../../include/flight/hal/interfaces)\n";
    std::cout << "  --auto-register        Include auto-registration (default: true)\n";
    std::cout << "  --examples             Include example code (default: false)\n";
    std::cout << "  --documentation        Include documentation (default: true)\n";
    std::cout << "  -h, --help             Show this help message\n";
    std::cout << "  -v, --version          Show version information\n\n";
    
    std::cout << "Examples:\n";
    std::cout << "  # Generate Windows memory driver\n";
    std::cout << "  " << program_name << " -t driver -i memory -p windows -c WindowsMemoryDriver -o ./output\n\n";
    
    std::cout << "  # Generate test skeleton for graphics interface\n";
    std::cout << "  " << program_name << " -t test -i graphics -o ./tests\n\n";
    
    std::cout << "  # Generate configuration files\n";
    std::cout << "  " << program_name << " -t config -i audio -p linux -c LinuxAudioDriver -o ./build\n\n";
}

/**
 * @brief Print version information
 */
void print_version() {
    std::cout << "Flight HAL Code Generator v1.0.0\n";
    std::cout << "Copyright (c) 2025 Flight HAL Project\n";
}

/**
 * @brief Parse generation type from string
 */
GenerationType parse_generation_type(const std::string& type_str) {
    static const std::map<std::string, GenerationType> type_map = {
        {"driver", GenerationType::DriverImplementation},
        {"test", GenerationType::TestSkeleton},
        {"config", GenerationType::Configuration},
        {"doc", GenerationType::Documentation},
        {"mock", GenerationType::MockDriver},
        {"integration", GenerationType::IntegrationTest}
    };
    
    auto it = type_map.find(type_str);
    if (it != type_map.end()) {
        return it->second;
    }
    
    throw std::invalid_argument("Invalid generation type: " + type_str);
}

/**
 * @brief Parse target platform from string
 */
TargetPlatform parse_target_platform(const std::string& platform_str) {
    static const std::map<std::string, TargetPlatform> platform_map = {
        {"windows", TargetPlatform::Windows},
        {"linux", TargetPlatform::Linux},
        {"macos", TargetPlatform::MacOS},
        {"embedded", TargetPlatform::Embedded},
        {"dreamcast", TargetPlatform::Dreamcast},
        {"generic", TargetPlatform::Generic}
    };
    
    auto it = platform_map.find(platform_str);
    if (it != platform_map.end()) {
        return it->second;
    }
    
    throw std::invalid_argument("Invalid target platform: " + platform_str);
}

/**
 * @brief Generate default class name from interface and platform
 */
std::string generate_class_name(const std::string& interface_name, TargetPlatform platform) {
    std::string platform_str = PlatformAdapter::platform_to_string(platform);
    std::string class_name = utils::to_camel_case(platform_str) + utils::to_camel_case(interface_name) + "Driver";
    return class_name;
}

/**
 * @brief Generate default namespace from platform
 */
std::string generate_namespace(TargetPlatform platform) {
    std::string platform_str = PlatformAdapter::platform_to_string(platform);
    return "flight::hal::drivers::" + platform_str;
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    try {
        // Parse command line arguments
        GenerationConfig config;
        std::string templates_dir = "./templates";
        std::string interfaces_dir = "../../include/flight/hal/interfaces";
        bool show_help = false;
        bool show_version = false;
        
        // Default values
        config.type = GenerationType::DriverImplementation;
        config.platform = TargetPlatform::Generic;
        config.include_examples = false;
        config.include_documentation = true;
        config.auto_register = true;
        
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "-h" || arg == "--help") {
                show_help = true;
            } else if (arg == "-v" || arg == "--version") {
                show_version = true;
            } else if (arg == "-t" || arg == "--type") {
                if (++i >= argc) {
                    std::cerr << "Error: Missing argument for " << arg << std::endl;
                    return 1;
                }
                config.type = parse_generation_type(argv[i]);
            } else if (arg == "-i" || arg == "--interface") {
                if (++i >= argc) {
                    std::cerr << "Error: Missing argument for " << arg << std::endl;
                    return 1;
                }
                config.interface_name = argv[i];
            } else if (arg == "-p" || arg == "--platform") {
                if (++i >= argc) {
                    std::cerr << "Error: Missing argument for " << arg << std::endl;
                    return 1;
                }
                config.platform = parse_target_platform(argv[i]);
            } else if (arg == "-c" || arg == "--class") {
                if (++i >= argc) {
                    std::cerr << "Error: Missing argument for " << arg << std::endl;
                    return 1;
                }
                config.class_name = argv[i];
            } else if (arg == "-n" || arg == "--namespace") {
                if (++i >= argc) {
                    std::cerr << "Error: Missing argument for " << arg << std::endl;
                    return 1;
                }
                config.namespace_name = argv[i];
            } else if (arg == "-o" || arg == "--output") {
                if (++i >= argc) {
                    std::cerr << "Error: Missing argument for " << arg << std::endl;
                    return 1;
                }
                config.output_directory = argv[i];
            } else if (arg == "--templates") {
                if (++i >= argc) {
                    std::cerr << "Error: Missing argument for " << arg << std::endl;
                    return 1;
                }
                templates_dir = argv[i];
            } else if (arg == "--interfaces") {
                if (++i >= argc) {
                    std::cerr << "Error: Missing argument for " << arg << std::endl;
                    return 1;
                }
                interfaces_dir = argv[i];
            } else if (arg == "--auto-register") {
                config.auto_register = true;
            } else if (arg == "--examples") {
                config.include_examples = true;
            } else if (arg == "--documentation") {
                config.include_documentation = true;
            } else {
                std::cerr << "Error: Unknown argument: " << arg << std::endl;
                return 1;
            }
        }
        
        if (show_help) {
            print_usage(argv[0]);
            return 0;
        }
        
        if (show_version) {
            print_version();
            return 0;
        }
        
        // Validate required arguments
        if (config.interface_name.empty()) {
            std::cerr << "Error: Interface name is required. Use -i or --interface." << std::endl;
            return 1;
        }
        
        if (config.output_directory.empty()) {
            std::cerr << "Error: Output directory is required. Use -o or --output." << std::endl;
            return 1;
        }
        
        // Generate defaults if not provided
        if (config.class_name.empty()) {
            config.class_name = generate_class_name(config.interface_name, config.platform);
        }
        
        if (config.namespace_name.empty()) {
            config.namespace_name = generate_namespace(config.platform);
        }
        
        // Initialize code generator
        HALCodeGenerator generator;
        generator.set_template_directory(templates_dir);
        generator.set_interfaces_directory(interfaces_dir);
        
        // Validate interface exists
        auto available_interfaces = generator.get_available_interfaces();
        bool interface_found = false;
        for (const auto& iface : available_interfaces) {
            if (iface == config.interface_name) {
                interface_found = true;
                break;
            }
        }
        
        if (!interface_found) {
            std::cerr << "Error: Interface '" << config.interface_name << "' not found." << std::endl;
            std::cerr << "Available interfaces: ";
            for (size_t i = 0; i < available_interfaces.size(); ++i) {
                if (i > 0) std::cerr << ", ";
                std::cerr << available_interfaces[i];
            }
            std::cerr << std::endl;
            return 1;
        }
        
        // Print generation info
        std::cout << "Flight HAL Code Generator\n";
        std::cout << "========================\n";
        std::cout << "Generation Type: ";
        switch (config.type) {
            case GenerationType::DriverImplementation: std::cout << "Driver Implementation"; break;
            case GenerationType::TestSkeleton: std::cout << "Test Skeleton"; break;
            case GenerationType::Configuration: std::cout << "Configuration Files"; break;
            case GenerationType::Documentation: std::cout << "Documentation"; break;
            case GenerationType::MockDriver: std::cout << "Mock Driver"; break;
            case GenerationType::IntegrationTest: std::cout << "Integration Test"; break;
        }
        std::cout << "\n";
        std::cout << "Interface: " << config.interface_name << "\n";
        std::cout << "Platform: " << PlatformAdapter::platform_to_string(config.platform) << "\n";
        std::cout << "Class Name: " << config.class_name << "\n";
        std::cout << "Namespace: " << config.namespace_name << "\n";
        std::cout << "Output Directory: " << config.output_directory << "\n";
        std::cout << "\n";
        
        // Generate code
        bool success = generator.generate(config);
        
        if (success) {
            std::cout << "Code generation completed successfully!" << std::endl;
            return 0;
        } else {
            std::cerr << "Code generation failed!" << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

/**
 * @file hal_code_generator.hpp
 * @brief Flight HAL Code Generation Framework
 * 
 * Core framework for generating HAL driver implementations, tests, and configuration
 * files from interface definitions. Supports multiple platforms and customizable
 * template-based generation.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace flight::hal::codegen {

/**
 * @brief Supported generation types
 */
enum class GenerationType {
    DriverImplementation,   ///< Complete driver implementation
    TestSkeleton,          ///< Test framework skeleton
    Configuration,         ///< Build and config files
    Documentation,         ///< API documentation
    MockDriver,           ///< Mock driver implementation
    IntegrationTest       ///< Integration test suite
};

/**
 * @brief Target platform specifications
 */
enum class TargetPlatform {
    Windows,              ///< Windows platform
    Linux,                ///< Linux platform
    MacOS,                ///< macOS platform
    Embedded,             ///< Embedded systems
    Dreamcast,            ///< Sega Dreamcast
    Generic               ///< Platform-agnostic
};

/**
 * @brief Interface method information
 */
struct MethodInfo {
    std::string name;                           ///< Method name
    std::string return_type;                    ///< Return type
    std::vector<std::string> parameters;        ///< Parameter list
    std::vector<std::string> parameter_names;   ///< Parameter names
    bool is_virtual;                           ///< Is virtual method
    bool is_pure_virtual;                      ///< Is pure virtual
    bool is_override;                          ///< Is override
    std::string documentation;                  ///< Method documentation
};

/**
 * @brief Interface class information
 */
struct InterfaceInfo {
    std::string name;                          ///< Interface class name
    std::string namespace_name;                ///< Namespace
    std::string base_class;                    ///< Base class name
    std::vector<std::string> includes;         ///< Required includes
    std::vector<MethodInfo> methods;           ///< Interface methods
    std::string documentation;                 ///< Class documentation
    std::unordered_map<std::string, std::string> metadata; ///< Additional metadata
};

/**
 * @brief Code generation parameters
 */
struct GenerationConfig {
    GenerationType type;                       ///< Type of generation
    TargetPlatform platform;                   ///< Target platform
    std::string interface_name;                ///< Interface to generate for
    std::string output_directory;              ///< Output directory
    std::string class_name;                    ///< Generated class name
    std::string namespace_name;                ///< Target namespace
    bool include_examples;                     ///< Include example code
    bool include_documentation;                ///< Include documentation
    bool auto_register;                        ///< Include auto-registration
    std::unordered_map<std::string, std::string> custom_options; ///< Custom options
};

/**
 * @brief Template variable container
 */
using TemplateVars = std::unordered_map<std::string, std::string>;

/**
 * @brief Interface parser for extracting information from header files
 */
class InterfaceParser {
public:
    /**
     * @brief Parse interface header file
     * @param header_path Path to interface header
     * @return Parsed interface information
     */
    InterfaceInfo parse_interface(const std::string& header_path);
    
    /**
     * @brief Get list of available interfaces
     * @param interfaces_dir Directory containing interface headers
     * @return Vector of interface names
     */
    std::vector<std::string> get_available_interfaces(const std::string& interfaces_dir);
    
private:
    /**
     * @brief Extract methods from interface class
     */
    std::vector<MethodInfo> extract_methods(const std::string& class_content);
    
    /**
     * @brief Parse method signature
     */
    MethodInfo parse_method(const std::string& method_line);
    
    /**
     * @brief Extract documentation comments
     */
    std::string extract_documentation(const std::string& content, size_t method_pos);
};

/**
 * @brief Template processing engine
 */
class TemplateEngine {
public:
    /**
     * @brief Load template from file
     * @param template_path Path to template file
     * @return Template content
     */
    std::string load_template(const std::string& template_path);
    
    /**
     * @brief Process template with variables
     * @param template_content Template content
     * @param variables Variable substitutions
     * @return Processed content
     */
    std::string process_template(const std::string& template_content, 
                                const TemplateVars& variables);
    
    /**
     * @brief Register custom template function
     * @param name Function name
     * @param func Function implementation
     */
    void register_function(const std::string& name, 
                          std::function<std::string(const std::vector<std::string>&)> func);

private:
    /**
     * @brief Replace template variables
     */
    std::string replace_variables(const std::string& content, const TemplateVars& variables);
    
    /**
     * @brief Process template functions
     */
    std::string process_functions(const std::string& content);
    
    std::unordered_map<std::string, std::function<std::string(const std::vector<std::string>&)>> functions_;
};

/**
 * @brief Platform-specific code generation utilities
 */
class PlatformAdapter {
public:
    /**
     * @brief Get platform-specific template variables
     * @param platform Target platform
     * @return Platform-specific variables
     */
    static TemplateVars get_platform_variables(TargetPlatform platform);
    
    /**
     * @brief Get platform-specific includes
     * @param platform Target platform
     * @return Required includes for platform
     */
    static std::vector<std::string> get_platform_includes(TargetPlatform platform);
    
    /**
     * @brief Get platform capabilities
     * @param platform Target platform
     * @return Capability flags for platform
     */
    static std::vector<std::string> get_platform_capabilities(TargetPlatform platform);
    
    /**
     * @brief Convert platform enum to string
     * @param platform Platform enum
     * @return String representation
     */
    static std::string platform_to_string(TargetPlatform platform);
};

/**
 * @brief Main code generator class
 */
class HALCodeGenerator {
public:
    HALCodeGenerator();
    ~HALCodeGenerator() = default;
    
    /**
     * @brief Generate code based on configuration
     * @param config Generation configuration
     * @return Success status
     */
    bool generate(const GenerationConfig& config);
    
    /**
     * @brief Generate driver implementation
     * @param config Generation configuration
     * @return Success status
     */
    bool generate_driver(const GenerationConfig& config);
    
    /**
     * @brief Generate test skeleton
     * @param config Generation configuration
     * @return Success status
     */
    bool generate_tests(const GenerationConfig& config);
    
    /**
     * @brief Generate configuration files
     * @param config Generation configuration
     * @return Success status
     */
    bool generate_config(const GenerationConfig& config);
    
    /**
     * @brief Generate documentation
     * @param config Generation configuration
     * @return Success status
     */
    bool generate_documentation(const GenerationConfig& config);
    
    /**
     * @brief Set template directory
     * @param template_dir Directory containing templates
     */
    void set_template_directory(const std::string& template_dir);
    
    /**
     * @brief Set interfaces directory
     * @param interfaces_dir Directory containing interface headers
     */
    void set_interfaces_directory(const std::string& interfaces_dir);
    
    /**
     * @brief Get available interfaces
     * @return Vector of interface names
     */
    std::vector<std::string> get_available_interfaces();
    
    /**
     * @brief Get available platforms
     * @return Vector of platform names
     */
    std::vector<std::string> get_available_platforms();

private:
    /**
     * @brief Build template variables for generation
     */
    TemplateVars build_template_variables(const GenerationConfig& config, 
                                         const InterfaceInfo& interface);
    
    /**
     * @brief Ensure output directory exists
     */
    bool ensure_output_directory(const std::string& path);
    
    /**
     * @brief Write generated content to file
     */
    bool write_file(const std::string& path, const std::string& content);
    
    /**
     * @brief Get template path for generation type
     */
    std::string get_template_path(GenerationType type, const std::string& template_name);
    
    std::unique_ptr<InterfaceParser> parser_;
    std::unique_ptr<TemplateEngine> template_engine_;
    std::string template_directory_;
    std::string interfaces_directory_;
};

/**
 * @brief Utility functions for code generation
 */
namespace utils {

/**
 * @brief Convert string to uppercase
 */
std::string to_upper(const std::string& str);

/**
 * @brief Convert string to lowercase
 */
std::string to_lower(const std::string& str);

/**
 * @brief Convert string to camel case
 */
std::string to_camel_case(const std::string& str);

/**
 * @brief Convert string to snake case
 */
std::string to_snake_case(const std::string& str);

/**
 * @brief Generate include guard name
 */
std::string generate_include_guard(const std::string& filename);

/**
 * @brief Extract filename from path
 */
std::string extract_filename(const std::string& path);

/**
 * @brief Create directory if it doesn't exist
 */
bool create_directory(const std::string& path);

/**
 * @brief Check if file exists
 */
bool file_exists(const std::string& path);

/**
 * @brief Read entire file content
 */
std::string read_file(const std::string& path);

/**
 * @brief Write content to file
 */
bool write_file(const std::string& path, const std::string& content);

} // namespace utils

} // namespace flight::hal::codegen

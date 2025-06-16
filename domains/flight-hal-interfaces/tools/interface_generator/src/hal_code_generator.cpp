/**
 * @file hal_code_generator.cpp
 * @brief Implementation of HAL Code Generation Framework
 */

#include "hal_code_generator.hpp"
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <chrono>

namespace flight::hal::codegen {

// === InterfaceParser Implementation ===

InterfaceInfo InterfaceParser::parse_interface(const std::string& header_path) {
    InterfaceInfo info;
    
    std::string content = utils::read_file(header_path);
    if (content.empty()) {
        return info;
    }
    
    // Extract namespace
    std::regex namespace_regex(R"(namespace\s+([^{]+)\s*\{)");
    std::smatch namespace_match;
    if (std::regex_search(content, namespace_match, namespace_regex)) {
        info.namespace_name = namespace_match[1].str();
        // Clean up namespace (remove flight::hal:: prefix if present)
        std::regex clean_regex(R"(flight::hal::(.+))");
        std::smatch clean_match;
        if (std::regex_search(info.namespace_name, clean_match, clean_regex)) {
            info.namespace_name = clean_match[1].str();
        }
    } else {
        info.namespace_name = "flight::hal";
    }
    
    // Extract interface class name and base class
    std::regex class_regex(R"(class\s+([I][A-Za-z0-9_]+Interface)\s*(?::\s*public\s+([A-Za-z0-9_:]+))?)");
    std::smatch class_match;
    if (std::regex_search(content, class_match, class_regex)) {
        info.name = class_match[1].str();
        if (class_match[2].matched) {
            info.base_class = class_match[2].str();
        }
    }
    
    // Extract includes
    std::regex include_regex(R"(#include\s+[<"]([^>"]+)[>"])");
    std::sregex_iterator include_begin(content.begin(), content.end(), include_regex);
    std::sregex_iterator include_end;
    for (std::sregex_iterator i = include_begin; i != include_end; ++i) {
        info.includes.push_back((*i)[1].str());
    }
    
    // Extract methods
    info.methods = extract_methods(content);
    
    // Extract class documentation
    size_t class_pos = content.find("class " + info.name);
    if (class_pos != std::string::npos) {
        info.documentation = extract_documentation(content, class_pos);
    }
    
    return info;
}

std::vector<std::string> InterfaceParser::get_available_interfaces(const std::string& interfaces_dir) {
    std::vector<std::string> interfaces;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(interfaces_dir)) {
            if (entry.path().extension() == ".hpp") {
                std::string filename = entry.path().stem().string();
                interfaces.push_back(filename);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error accessing interfaces directory: " << e.what() << std::endl;
    }
    
    return interfaces;
}

std::vector<MethodInfo> InterfaceParser::extract_methods(const std::string& class_content) {
    std::vector<MethodInfo> methods;
    
    // Find class definition start
    size_t class_pos = class_content.find("class ");
    if (class_pos == std::string::npos) {
        return methods;
    }
    
    // Find opening brace
    size_t brace_start = class_content.find('{', class_pos);
    if (brace_start == std::string::npos) {
        return methods;
    }
    
    // Find matching closing brace
    size_t brace_end = brace_start + 1;
    int brace_count = 1;
    while (brace_end < class_content.length() && brace_count > 0) {
        if (class_content[brace_end] == '{') {
            brace_count++;
        } else if (class_content[brace_end] == '}') {
            brace_count--;
        }
        brace_end++;
    }
    
    if (brace_count != 0) {
        return methods; // Unmatched braces
    }
    
    // Extract class body
    std::string class_body = class_content.substr(brace_start + 1, brace_end - brace_start - 2);
    
    // Extract method declarations line by line
    std::istringstream stream(class_body);
    std::string line;
    std::string accumulated_line;
    
    while (std::getline(stream, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        // Skip empty lines and comments
        if (line.empty() || line.find("//") == 0 || line.find("/*") == 0) {
            continue;
        }
        
        // Accumulate multi-line declarations
        accumulated_line += line + " ";
        
        // Check if line ends with semicolon (complete declaration)
        if (line.back() == ';') {
            MethodInfo method = parse_method(accumulated_line);
            if (!method.name.empty()) {
                methods.push_back(method);
            }
            accumulated_line.clear();
        }
    }
    
    return methods;
}

MethodInfo InterfaceParser::parse_method(const std::string& method_line) {
    MethodInfo method;
    
    // Check for virtual/override/pure virtual
    method.is_virtual = method_line.find("virtual") != std::string::npos;
    method.is_override = method_line.find("override") != std::string::npos;
    method.is_pure_virtual = method_line.find("= 0") != std::string::npos;
    
    // Parse method signature
    std::regex signature_regex(R"((?:virtual\s+)?([A-Za-z0-9_:<>,\s&*]+)\s+([A-Za-z0-9_]+)\s*\(([^)]*)\))");
    std::smatch signature_match;
    
    if (std::regex_search(method_line, signature_match, signature_regex)) {
        method.return_type = signature_match[1].str();
        method.name = signature_match[2].str();
        
        // Parse parameters
        std::string params = signature_match[3].str();
        if (!params.empty() && params != "void") {
            std::regex param_regex(R"(([^,]+)(?:,|$))");
            std::sregex_iterator param_begin(params.begin(), params.end(), param_regex);
            std::sregex_iterator param_end;
            
            for (std::sregex_iterator i = param_begin; i != param_end; ++i) {
                std::string param = (*i)[1].str();
                // Trim whitespace
                param.erase(0, param.find_first_not_of(" \t"));
                param.erase(param.find_last_not_of(" \t") + 1);
                
                method.parameters.push_back(param);
                
                // Extract parameter name
                std::regex param_name_regex(R"(\b([A-Za-z_][A-Za-z0-9_]*)\s*$)");
                std::smatch param_name_match;
                if (std::regex_search(param, param_name_match, param_name_regex)) {
                    method.parameter_names.push_back(param_name_match[1].str());
                } else {
                    method.parameter_names.push_back("param" + std::to_string(method.parameter_names.size()));
                }
            }
        }
    }
    
    return method;
}

std::string InterfaceParser::extract_documentation(const std::string& content, size_t method_pos) {
    // Look backwards for documentation comment
    size_t doc_end = content.rfind("*/", method_pos);
    if (doc_end == std::string::npos) {
        return "";
    }
    
    size_t doc_start = content.rfind("/**", doc_end);
    if (doc_start == std::string::npos) {
        return "";
    }
    
    return content.substr(doc_start, doc_end - doc_start + 2);
}

// === TemplateEngine Implementation ===

std::string TemplateEngine::load_template(const std::string& template_path) {
    return utils::read_file(template_path);
}

std::string TemplateEngine::process_template(const std::string& template_content, 
                                           const TemplateVars& variables) {
    std::string result = replace_variables(template_content, variables);
    result = process_functions(result);
    return result;
}

void TemplateEngine::register_function(const std::string& name, 
                                      std::function<std::string(const std::vector<std::string>&)> func) {
    functions_[name] = func;
}

std::string TemplateEngine::replace_variables(const std::string& content, const TemplateVars& variables) {
    std::string result = content;
    
    // Replace {{variable}} patterns
    std::regex var_regex(R"(\{\{([A-Za-z0-9_]+)\}\})");
    std::smatch match;
    
    while (std::regex_search(result, match, var_regex)) {
        std::string var_name = match[1].str();
        auto it = variables.find(var_name);
        if (it != variables.end()) {
            result.replace(match.position(), match.length(), it->second);
        } else {
            // Replace with empty string if variable not found
            result.replace(match.position(), match.length(), "");
        }
    }
    
    return result;
}

std::string TemplateEngine::process_functions(const std::string& content) {
    std::string result = content;
    
    // Process {{function(arg1, arg2)}} patterns
    std::regex func_regex(R"(\{\{([A-Za-z0-9_]+)\(([^)]*)\)\}\})");
    std::smatch match;
    
    while (std::regex_search(result, match, func_regex)) {
        std::string func_name = match[1].str();
        std::string args_str = match[2].str();
        
        // Parse arguments
        std::vector<std::string> args;
        if (!args_str.empty()) {
            std::regex arg_regex(R"([^,]+)");
            std::sregex_iterator arg_begin(args_str.begin(), args_str.end(), arg_regex);
            std::sregex_iterator arg_end;
            
            for (std::sregex_iterator i = arg_begin; i != arg_end; ++i) {
                std::string arg = (*i)[0].str();
                // Trim whitespace
                arg.erase(0, arg.find_first_not_of(" \t"));
                arg.erase(arg.find_last_not_of(" \t") + 1);
                args.push_back(arg);
            }
        }
        
        // Call function
        auto it = functions_.find(func_name);
        if (it != functions_.end()) {
            std::string func_result = it->second(args);
            result.replace(match.position(), match.length(), func_result);
        } else {
            // Replace with empty string if function not found
            result.replace(match.position(), match.length(), "");
        }
    }
    
    return result;
}

// === PlatformAdapter Implementation ===

TemplateVars PlatformAdapter::get_platform_variables(TargetPlatform platform) {
    TemplateVars vars;
    
    switch (platform) {
        case TargetPlatform::Windows:
            vars["PLATFORM"] = "windows";
            vars["PLATFORM_UPPER"] = "WINDOWS";
            vars["PLATFORM_DEFINE"] = "FLIGHT_PLATFORM_WINDOWS";
            vars["PLATFORM_SPECIFIC_INCLUDES"] = "#include <windows.h>";
            break;
            
        case TargetPlatform::Linux:
            vars["PLATFORM"] = "linux";
            vars["PLATFORM_UPPER"] = "LINUX";
            vars["PLATFORM_DEFINE"] = "FLIGHT_PLATFORM_LINUX";
            vars["PLATFORM_SPECIFIC_INCLUDES"] = "#include <unistd.h>\n#include <pthread.h>";
            break;
            
        case TargetPlatform::MacOS:
            vars["PLATFORM"] = "macos";
            vars["PLATFORM_UPPER"] = "MACOS";
            vars["PLATFORM_DEFINE"] = "FLIGHT_PLATFORM_MACOS";
            vars["PLATFORM_SPECIFIC_INCLUDES"] = "#include <unistd.h>\n#include <pthread.h>";
            break;
            
        case TargetPlatform::Embedded:
            vars["PLATFORM"] = "embedded";
            vars["PLATFORM_UPPER"] = "EMBEDDED";
            vars["PLATFORM_DEFINE"] = "FLIGHT_PLATFORM_EMBEDDED";
            vars["PLATFORM_SPECIFIC_INCLUDES"] = "// Platform-specific includes";
            break;
            
        case TargetPlatform::Dreamcast:
            vars["PLATFORM"] = "dreamcast";
            vars["PLATFORM_UPPER"] = "DREAMCAST";
            vars["PLATFORM_DEFINE"] = "FLIGHT_PLATFORM_DREAMCAST";
            vars["PLATFORM_SPECIFIC_INCLUDES"] = "#include <kos.h>";
            break;
            
        case TargetPlatform::Generic:
        default:
            vars["PLATFORM"] = "generic";
            vars["PLATFORM_UPPER"] = "GENERIC";
            vars["PLATFORM_DEFINE"] = "FLIGHT_PLATFORM_GENERIC";
            vars["PLATFORM_SPECIFIC_INCLUDES"] = "// No platform-specific includes";
            break;
    }
    
    return vars;
}

std::vector<std::string> PlatformAdapter::get_platform_includes(TargetPlatform platform) {
    switch (platform) {
        case TargetPlatform::Windows:
            return {"<windows.h>", "<d3d11.h>", "<dsound.h>"};
            
        case TargetPlatform::Linux:
            return {"<unistd.h>", "<pthread.h>", "<X11/Xlib.h>", "<GL/gl.h>", "<alsa/asoundlib.h>"};
            
        case TargetPlatform::MacOS:
            return {"<unistd.h>", "<pthread.h>", "<CoreFoundation/CoreFoundation.h>", "<OpenGL/gl.h>", "<AudioUnit/AudioUnit.h>"};
            
        case TargetPlatform::Dreamcast:
            return {"<kos.h>", "<dc/pvr.h>", "<dc/sound/sound.h>"};
            
        case TargetPlatform::Embedded:
            return {"// Platform-specific includes to be added"};
            
        case TargetPlatform::Generic:
        default:
            return {};
    }
}

std::vector<std::string> PlatformAdapter::get_platform_capabilities(TargetPlatform platform) {
    switch (platform) {
        case TargetPlatform::Windows:
            return {"HALCapability::Memory", "HALCapability::Graphics", "HALCapability::Audio", 
                   "HALCapability::Input", "HALCapability::File", "HALCapability::Network",
                   "HALCapability::Time", "HALCapability::Thread", "HALCapability::Performance"};
            
        case TargetPlatform::Linux:
            return {"HALCapability::Memory", "HALCapability::Graphics", "HALCapability::Audio", 
                   "HALCapability::Input", "HALCapability::File", "HALCapability::Network",
                   "HALCapability::Time", "HALCapability::Thread", "HALCapability::Performance"};
            
        case TargetPlatform::MacOS:
            return {"HALCapability::Memory", "HALCapability::Graphics", "HALCapability::Audio", 
                   "HALCapability::Input", "HALCapability::File", "HALCapability::Network",
                   "HALCapability::Time", "HALCapability::Thread", "HALCapability::Performance"};
            
        case TargetPlatform::Dreamcast:
            return {"HALCapability::Memory", "HALCapability::Graphics", "HALCapability::Audio", 
                   "HALCapability::Input", "HALCapability::Time"};
            
        case TargetPlatform::Embedded:
            return {"HALCapability::Memory", "HALCapability::Time"};
            
        case TargetPlatform::Generic:
        default:
            return {"HALCapability::Memory"};
    }
}

std::string PlatformAdapter::platform_to_string(TargetPlatform platform) {
    switch (platform) {
        case TargetPlatform::Windows: return "windows";
        case TargetPlatform::Linux: return "linux";
        case TargetPlatform::MacOS: return "macos";
        case TargetPlatform::Embedded: return "embedded";
        case TargetPlatform::Dreamcast: return "dreamcast";
        case TargetPlatform::Generic: return "generic";
        default: return "unknown";
    }
}

// === HALCodeGenerator Implementation ===

HALCodeGenerator::HALCodeGenerator() 
    : parser_(std::make_unique<InterfaceParser>())
    , template_engine_(std::make_unique<TemplateEngine>()) {
    
    // Register template functions
    template_engine_->register_function("upper", [](const std::vector<std::string>& args) {
        return args.empty() ? "" : utils::to_upper(args[0]);
    });
    
    template_engine_->register_function("lower", [](const std::vector<std::string>& args) {
        return args.empty() ? "" : utils::to_lower(args[0]);
    });
    
    template_engine_->register_function("camel", [](const std::vector<std::string>& args) {
        return args.empty() ? "" : utils::to_camel_case(args[0]);
    });
    
    template_engine_->register_function("snake", [](const std::vector<std::string>& args) {
        return args.empty() ? "" : utils::to_snake_case(args[0]);
    });
    
    template_engine_->register_function("include_guard", [](const std::vector<std::string>& args) {
        return args.empty() ? "" : utils::generate_include_guard(args[0]);
    });
}

bool HALCodeGenerator::generate(const GenerationConfig& config) {
    switch (config.type) {
        case GenerationType::DriverImplementation:
            return generate_driver(config);
        case GenerationType::TestSkeleton:
            return generate_tests(config);
        case GenerationType::Configuration:
            return generate_config(config);
        case GenerationType::Documentation:
            return generate_documentation(config);
        case GenerationType::MockDriver:
            return generate_driver(config); // Use same logic with different template
        case GenerationType::IntegrationTest:
            return generate_tests(config); // Use same logic with different template
        default:
            std::cerr << "Unsupported generation type" << std::endl;
            return false;
    }
}

bool HALCodeGenerator::generate_driver(const GenerationConfig& config) {
    // Parse interface
    std::string interface_path = interfaces_directory_ + "/" + config.interface_name + ".hpp";
    InterfaceInfo interface = parser_->parse_interface(interface_path);
    
    if (interface.name.empty()) {
        std::cerr << "Failed to parse interface: " << interface_path << std::endl;
        return false;
    }
    
    // Build template variables
    TemplateVars vars = build_template_variables(config, interface);
    
    // Generate header file
    std::string header_template_path = get_template_path(config.type, "driver_header.hpp.template");
    std::string header_template = template_engine_->load_template(header_template_path);
    std::string header_content = template_engine_->process_template(header_template, vars);
    
    std::string header_output_path = config.output_directory + "/" + config.class_name + ".hpp";
    if (!write_file(header_output_path, header_content)) {
        return false;
    }
    
    // Generate implementation file
    std::string impl_template_path = get_template_path(config.type, "driver_impl.cpp.template");
    std::string impl_template = template_engine_->load_template(impl_template_path);
    std::string impl_content = template_engine_->process_template(impl_template, vars);
    
    std::string impl_output_path = config.output_directory + "/" + config.class_name + ".cpp";
    if (!write_file(impl_output_path, impl_content)) {
        return false;
    }
    
    std::cout << "Generated driver implementation:" << std::endl;
    std::cout << "  Header: " << header_output_path << std::endl;
    std::cout << "  Implementation: " << impl_output_path << std::endl;
    
    return true;
}

bool HALCodeGenerator::generate_tests(const GenerationConfig& config) {
    // Parse interface
    std::string interface_path = interfaces_directory_ + "/" + config.interface_name + ".hpp";
    InterfaceInfo interface = parser_->parse_interface(interface_path);
    
    if (interface.name.empty()) {
        std::cerr << "Failed to parse interface: " << interface_path << std::endl;
        return false;
    }
    
    // Build template variables
    TemplateVars vars = build_template_variables(config, interface);
    
    // Generate test file
    std::string test_template_path = get_template_path(config.type, "test_skeleton.cpp.template");
    std::string test_template = template_engine_->load_template(test_template_path);
    std::string test_content = template_engine_->process_template(test_template, vars);
    
    std::string test_output_path = config.output_directory + "/" + config.interface_name + "_test.cpp";
    if (!write_file(test_output_path, test_content)) {
        return false;
    }
    
    std::cout << "Generated test skeleton: " << test_output_path << std::endl;
    
    return true;
}

bool HALCodeGenerator::generate_config(const GenerationConfig& config) {
    // Build template variables
    TemplateVars vars;
    vars["CLASS_NAME"] = config.class_name;
    vars["INTERFACE_NAME"] = config.interface_name;
    vars["NAMESPACE"] = config.namespace_name;
    
    // Add platform-specific variables
    TemplateVars platform_vars = PlatformAdapter::get_platform_variables(config.platform);
    vars.insert(platform_vars.begin(), platform_vars.end());
    
    // Generate CMakeLists.txt
    std::string cmake_template_path = get_template_path(config.type, "CMakeLists.txt.template");
    std::string cmake_template = template_engine_->load_template(cmake_template_path);
    std::string cmake_content = template_engine_->process_template(cmake_template, vars);
    
    std::string cmake_output_path = config.output_directory + "/CMakeLists.txt";
    if (!write_file(cmake_output_path, cmake_content)) {
        return false;
    }
    
    std::cout << "Generated configuration: " << cmake_output_path << std::endl;
    
    return true;
}

bool HALCodeGenerator::generate_documentation(const GenerationConfig& config) {
    // Parse interface
    std::string interface_path = interfaces_directory_ + "/" + config.interface_name + ".hpp";
    InterfaceInfo interface = parser_->parse_interface(interface_path);
    
    if (interface.name.empty()) {
        std::cerr << "Failed to parse interface: " << interface_path << std::endl;
        return false;
    }
    
    // Build template variables
    TemplateVars vars = build_template_variables(config, interface);
    
    // Generate documentation
    std::string doc_template_path = get_template_path(config.type, "interface_doc.md.template");
    std::string doc_template = template_engine_->load_template(doc_template_path);
    std::string doc_content = template_engine_->process_template(doc_template, vars);
    
    std::string doc_output_path = config.output_directory + "/" + config.interface_name + "_driver_guide.md";
    if (!write_file(doc_output_path, doc_content)) {
        return false;
    }
    
    std::cout << "Generated documentation: " << doc_output_path << std::endl;
    
    return true;
}

void HALCodeGenerator::set_template_directory(const std::string& template_dir) {
    template_directory_ = template_dir;
}

void HALCodeGenerator::set_interfaces_directory(const std::string& interfaces_dir) {
    interfaces_directory_ = interfaces_dir;
}

std::vector<std::string> HALCodeGenerator::get_available_interfaces() {
    return parser_->get_available_interfaces(interfaces_directory_);
}

std::vector<std::string> HALCodeGenerator::get_available_platforms() {
    return {"windows", "linux", "macos", "embedded", "dreamcast", "generic"};
}

TemplateVars HALCodeGenerator::build_template_variables(const GenerationConfig& config, 
                                                       const InterfaceInfo& interface) {
    TemplateVars vars;
    
    // Basic variables
    vars["CLASS_NAME"] = config.class_name;
    vars["INTERFACE_NAME"] = interface.name;
    vars["INTERFACE_FILENAME"] = config.interface_name;
    vars["NAMESPACE"] = config.namespace_name;
    vars["BASE_CLASS"] = interface.base_class;
    
    // Platform-specific variables
    TemplateVars platform_vars = PlatformAdapter::get_platform_variables(config.platform);
    vars.insert(platform_vars.begin(), platform_vars.end());
    
    // Method information
    std::stringstream methods_ss;
    for (const auto& method : interface.methods) {
        methods_ss << "    " << method.return_type << " " << method.name << "(";
        for (size_t i = 0; i < method.parameters.size(); ++i) {
            if (i > 0) methods_ss << ", ";
            methods_ss << method.parameters[i];
        }
        methods_ss << ")";
        if (method.is_override) methods_ss << " override";
        methods_ss << ";\n";
    }
    vars["METHODS"] = methods_ss.str();
    
    // Include guard
    vars["INCLUDE_GUARD"] = utils::generate_include_guard(config.class_name + ".hpp");
    
    // Current date
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    vars["DATE"] = std::ctime(&time_t);
    
    return vars;
}

bool HALCodeGenerator::ensure_output_directory(const std::string& path) {
    return utils::create_directory(path);
}

bool HALCodeGenerator::write_file(const std::string& path, const std::string& content) {
    // Ensure directory exists
    std::filesystem::path file_path(path);
    if (!ensure_output_directory(file_path.parent_path().string())) {
        return false;
    }
    
    return utils::write_file(path, content);
}

std::string HALCodeGenerator::get_template_path(GenerationType type, const std::string& template_name) {
    std::string subdir;
    switch (type) {
        case GenerationType::DriverImplementation:
        case GenerationType::MockDriver:
            subdir = "driver_templates";
            break;
        case GenerationType::TestSkeleton:
        case GenerationType::IntegrationTest:
            subdir = "test_templates";
            break;
        case GenerationType::Configuration:
            subdir = "config_templates";
            break;
        case GenerationType::Documentation:
            subdir = "doc_templates";
            break;
    }
    
    return template_directory_ + "/" + subdir + "/" + template_name;
}

// === Utility Functions Implementation ===

namespace utils {

std::string to_upper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

std::string to_lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string to_camel_case(const std::string& str) {
    std::string result;
    bool capitalize_next = true;
    
    for (char c : str) {
        if (c == '_' || c == '-' || c == ' ') {
            capitalize_next = true;
        } else if (capitalize_next) {
            result += std::toupper(c);
            capitalize_next = false;
        } else {
            result += std::tolower(c);
        }
    }
    
    return result;
}

std::string to_snake_case(const std::string& str) {
    std::string result;
    
    for (size_t i = 0; i < str.length(); ++i) {
        char c = str[i];
        if (std::isupper(c) && i > 0) {
            result += '_';
        }
        result += std::tolower(c);
    }
    
    return result;
}

std::string generate_include_guard(const std::string& filename) {
    std::string guard = "FLIGHT_HAL_" + to_upper(filename);
    std::replace(guard.begin(), guard.end(), '.', '_');
    std::replace(guard.begin(), guard.end(), '/', '_');
    std::replace(guard.begin(), guard.end(), '\\', '_');
    return guard + "_HPP";
}

std::string extract_filename(const std::string& path) {
    std::filesystem::path file_path(path);
    return file_path.filename().string();
}

bool create_directory(const std::string& path) {
    try {
        return std::filesystem::create_directories(path);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error creating directory " << path << ": " << e.what() << std::endl;
        return false;
    }
}

bool file_exists(const std::string& path) {
    return std::filesystem::exists(path);
}

std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << path << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool write_file(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << path << std::endl;
        return false;
    }
    
    file << content;
    return true;
}

} // namespace utils

} // namespace flight::hal::codegen

/**
 * @file network.cpp
 * @brief Flight HAL Network Interface Implementation
 * 
 * Core implementation of network interface functionality with utility functions
 * and default bandwidth configurations for cross-platform networking support.
 */

#include "../../include/flight/hal/interfaces/network.hpp"
#include "../../include/flight/hal/core/hal_logging.hpp"
#include <algorithm>
#include <sstream>
#include <regex>
#include <iomanip>

namespace flight::hal {

namespace network {

/**
 * @brief Convert network capability to string
 */
const char* to_string(NetworkCapability capability) {
    switch (capability) {
        case NetworkCapability::TCP: return "TCP";
        case NetworkCapability::UDP: return "UDP";
        case NetworkCapability::IPv4: return "IPv4";
        case NetworkCapability::IPv6: return "IPv6";
        case NetworkCapability::Dialup: return "Dialup";
        case NetworkCapability::Ethernet: return "Ethernet";
        case NetworkCapability::WiFi: return "WiFi";
        case NetworkCapability::Broadband: return "Broadband";
        case NetworkCapability::HTTP: return "HTTP";
        case NetworkCapability::HTTPS: return "HTTPS";
        case NetworkCapability::WebSocket: return "WebSocket";
        case NetworkCapability::DNS: return "DNS";
        case NetworkCapability::ConnectionPooling: return "ConnectionPooling";
        case NetworkCapability::Compression: return "Compression";
        case NetworkCapability::AsyncOperations: return "AsyncOperations";
        case NetworkCapability::MultiThreaded: return "MultiThreaded";
        case NetworkCapability::AdHocMode: return "AdHocMode";
        case NetworkCapability::InfrastructureMode: return "InfrastructureMode";
        case NetworkCapability::BrowserAPI: return "BrowserAPI";
        case NetworkCapability::CORS: return "CORS";
        case NetworkCapability::BandwidthThrottling: return "BandwidthThrottling";
        case NetworkCapability::AdaptiveTimeout: return "AdaptiveTimeout";
        case NetworkCapability::QualityOfService: return "QualityOfService";
        case NetworkCapability::TrafficShaping: return "TrafficShaping";
        case NetworkCapability::TLS: return "TLS";
        case NetworkCapability::CertificateValidation: return "CertificateValidation";
        case NetworkCapability::SecureRandom: return "SecureRandom";
        case NetworkCapability::Statistics: return "Statistics";
        case NetworkCapability::Diagnostics: return "Diagnostics";
        case NetworkCapability::Profiling: return "Profiling";
        default: return "Unknown";
    }
}

/**
 * @brief Convert network protocol to string
 */
const char* to_string(NetworkProtocol protocol) {
    switch (protocol) {
        case NetworkProtocol::TCP: return "TCP";
        case NetworkProtocol::UDP: return "UDP";
        case NetworkProtocol::HTTP: return "HTTP";
        case NetworkProtocol::HTTPS: return "HTTPS";
        case NetworkProtocol::WebSocket: return "WebSocket";
        case NetworkProtocol::Custom: return "Custom";
        default: return "Unknown";
    }
}

/**
 * @brief Convert socket type to string
 */
const char* to_string(SocketType type) {
    switch (type) {
        case SocketType::Stream: return "Stream";
        case SocketType::Datagram: return "Datagram";
        case SocketType::Raw: return "Raw";
        case SocketType::WebSocket: return "WebSocket";
        default: return "Unknown";
    }
}

/**
 * @brief Convert connection state to string
 */
const char* to_string(ConnectionState state) {
    switch (state) {
        case ConnectionState::Disconnected: return "Disconnected";
        case ConnectionState::Connecting: return "Connecting";
        case ConnectionState::Connected: return "Connected";
        case ConnectionState::Disconnecting: return "Disconnecting";
        case ConnectionState::Error: return "Error";
        case ConnectionState::Timeout: return "Timeout";
        default: return "Unknown";
    }
}

/**
 * @brief Convert HTTP method to string
 */
const char* to_string(HTTPMethod method) {
    switch (method) {
        case HTTPMethod::GET: return "GET";
        case HTTPMethod::POST: return "POST";
        case HTTPMethod::PUT: return "PUT";
        case HTTPMethod::DELETE: return "DELETE";
        case HTTPMethod::HEAD: return "HEAD";
        case HTTPMethod::OPTIONS: return "OPTIONS";
        case HTTPMethod::PATCH: return "PATCH";
        case HTTPMethod::TRACE: return "TRACE";
        case HTTPMethod::CONNECT: return "CONNECT";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Convert HTTP status code to string
 */
const char* to_string(HTTPStatusCode status_code) {
    switch (status_code) {
        case HTTPStatusCode::OK: return "OK";
        case HTTPStatusCode::Created: return "Created";
        case HTTPStatusCode::NoContent: return "No Content";
        case HTTPStatusCode::MovedPermanently: return "Moved Permanently";
        case HTTPStatusCode::Found: return "Found";
        case HTTPStatusCode::NotModified: return "Not Modified";
        case HTTPStatusCode::BadRequest: return "Bad Request";
        case HTTPStatusCode::Unauthorized: return "Unauthorized";
        case HTTPStatusCode::Forbidden: return "Forbidden";
        case HTTPStatusCode::NotFound: return "Not Found";
        case HTTPStatusCode::MethodNotAllowed: return "Method Not Allowed";
        case HTTPStatusCode::RequestTimeout: return "Request Timeout";
        case HTTPStatusCode::InternalServerError: return "Internal Server Error";
        case HTTPStatusCode::NotImplemented: return "Not Implemented";
        case HTTPStatusCode::BadGateway: return "Bad Gateway";
        case HTTPStatusCode::ServiceUnavailable: return "Service Unavailable";
        case HTTPStatusCode::GatewayTimeout: return "Gateway Timeout";
        default: return "Unknown Status";
    }
}

/**
 * @brief Convert bandwidth profile to string
 */
const char* to_string(BandwidthProfile profile) {
    switch (profile) {
        case BandwidthProfile::Dialup: return "Dialup";
        case BandwidthProfile::Narrowband: return "Narrowband";
        case BandwidthProfile::Broadband: return "Broadband";
        case BandwidthProfile::HighSpeed: return "HighSpeed";
        case BandwidthProfile::Unlimited: return "Unlimited";
        case BandwidthProfile::Custom: return "Custom";
        default: return "Unknown";
    }
}

/**
 * @brief Parse URL into components
 */
bool parse_url(const std::string& url, std::string& protocol, std::string& host, 
               uint16_t& port, std::string& path) {
    // Regular expression to parse URL components
    // Matches: protocol://host:port/path or protocol://host/path
    std::regex url_regex(R"(^([a-zA-Z][a-zA-Z0-9+.-]*):\/\/([^:\/\s]+)(?::(\d+))?(\/.*)?$)");
    std::smatch matches;
    
    if (!std::regex_match(url, matches, url_regex)) {
        return false;
    }
    
    protocol = matches[1].str();
    host = matches[2].str();
    
    // Parse port
    if (matches[3].matched) {
        try {
            int port_num = std::stoi(matches[3].str());
            if (port_num < 1 || port_num > 65535) {
                return false;
            }
            port = static_cast<uint16_t>(port_num);
        } catch (const std::exception&) {
            return false;
        }
    } else {
        // Use default port for protocol
        NetworkProtocol proto;
        if (protocol == "http") {
            proto = NetworkProtocol::HTTP;
        } else if (protocol == "https") {
            proto = NetworkProtocol::HTTPS;
        } else if (protocol == "ws") {
            proto = NetworkProtocol::WebSocket;
        } else if (protocol == "wss") {
            proto = NetworkProtocol::WebSocket;  // Secure WebSocket
        } else {
            return false;  // Unknown protocol
        }
        port = get_default_port(proto);
    }
    
    // Parse path
    path = matches[4].matched ? matches[4].str() : "/";
    
    return true;
}

/**
 * @brief Build URL from components
 */
std::string build_url(const std::string& protocol, const std::string& host,
                     uint16_t port, const std::string& path) {
    std::stringstream url;
    url << protocol << "://" << host;
    
    // Only include port if it's not the default for the protocol
    uint16_t default_port = 80;  // Default fallback
    if (protocol == "http") {
        default_port = 80;
    } else if (protocol == "https") {
        default_port = 443;
    } else if (protocol == "ws") {
        default_port = 80;
    } else if (protocol == "wss") {
        default_port = 443;
    }
    
    if (port != default_port) {
        url << ":" << port;
    }
    
    url << path;
    return url.str();
}

/**
 * @brief URL encode string
 */
std::string url_encode(const std::string& input) {
    std::stringstream encoded;
    encoded << std::hex << std::uppercase;
    
    for (unsigned char c : input) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << c;
        } else {
            encoded << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(c);
        }
    }
    
    return encoded.str();
}

/**
 * @brief URL decode string
 */
std::string url_decode(const std::string& input) {
    std::stringstream decoded;
    
    for (size_t i = 0; i < input.length(); ++i) {
        if (input[i] == '%' && i + 2 < input.length()) {
            try {
                int value = std::stoi(input.substr(i + 1, 2), nullptr, 16);
                decoded << static_cast<char>(value);
                i += 2;
            } catch (const std::exception&) {
                decoded << input[i];  // Keep original character if decode fails
            }
        } else if (input[i] == '+') {
            decoded << ' ';  // Convert + to space
        } else {
            decoded << input[i];
        }
    }
    
    return decoded.str();
}

/**
 * @brief Create default connection parameters
 */
ConnectionParams make_connection_params(const NetworkAddress& address,
                                       uint32_t timeout_ms) {
    ConnectionParams params;
    params.address = address;
    params.timeout_ms = timeout_ms;
    params.keep_alive_interval_ms = 30000;  // 30 seconds
    params.max_retries = 3;
    params.bandwidth_profile = BandwidthProfile::Broadband;
    params.enable_compression = false;
    params.enable_keep_alive = true;
    params.verify_certificates = true;
    params.user_agent = "FlightHAL/1.0";
    params.debug_name = "";
    
    return params;
}

/**
 * @brief Create connection parameters for low bandwidth
 */
ConnectionParams make_low_bandwidth_params(const NetworkAddress& address,
                                          BandwidthProfile bandwidth_profile) {
    ConnectionParams params = make_connection_params(address, 60000);  // Longer timeout
    params.bandwidth_profile = bandwidth_profile;
    params.enable_compression = true;  // Enable compression for low bandwidth
    params.keep_alive_interval_ms = 60000;  // Longer keep-alive interval
    params.max_retries = 5;  // More retries for unreliable connections
    
    return params;
}

/**
 * @brief Create HTTP request
 */
HTTPRequest make_http_request(HTTPMethod method, const std::string& url,
                             const std::string& body) {
    HTTPRequest request;
    request.method = method;
    request.url = url;
    request.body = body;
    request.timeout_ms = 30000;  // 30 seconds
    request.follow_redirects = true;
    request.max_redirects = 5;
    request.bandwidth_profile = BandwidthProfile::Broadband;
    request.debug_name = "";
    
    return request;
}

/**
 * @brief Create HTTP GET request
 */
HTTPRequest make_get_request(const std::string& url, const HTTPHeaders& headers) {
    HTTPRequest request = make_http_request(HTTPMethod::GET, url);
    request.headers = headers;
    return request;
}

/**
 * @brief Create HTTP POST request
 */
HTTPRequest make_post_request(const std::string& url, const std::string& body,
                             const std::string& content_type) {
    HTTPRequest request = make_http_request(HTTPMethod::POST, url, body);
    request.headers.set("Content-Type", content_type);
    request.headers.set("Content-Length", std::to_string(body.length()));
    return request;
}

/**
 * @brief Create default bandwidth configuration
 */
BandwidthConfig make_bandwidth_config(BandwidthProfile profile) {
    BandwidthConfig config;
    
    switch (profile) {
        case BandwidthProfile::Dialup:
            config.max_bandwidth_bps = 56000;      // 56K modem
            config.burst_size = 1024;              // 1KB burst
            config.min_timeout_ms = 10000;         // 10 seconds minimum
            config.max_timeout_ms = 120000;        // 2 minutes maximum
            config.retry_delay_ms = 5000;          // 5 second retry delay
            config.timeout_multiplier = 3.0;       // 3x timeout multiplier
            config.enable_compression = true;      // Always compress for dialup
            config.enable_keep_alive = false;      // Disable keep-alive to save bandwidth
            break;
            
        case BandwidthProfile::Narrowband:
            config.max_bandwidth_bps = 1000000;    // 1 Mbps
            config.burst_size = 4096;              // 4KB burst
            config.min_timeout_ms = 5000;          // 5 seconds minimum
            config.max_timeout_ms = 60000;         // 1 minute maximum
            config.retry_delay_ms = 2000;          // 2 second retry delay
            config.timeout_multiplier = 2.0;       // 2x timeout multiplier
            config.enable_compression = true;      // Compress for narrowband
            config.enable_keep_alive = true;       // Keep-alive okay
            break;
            
        case BandwidthProfile::Broadband:
            config.max_bandwidth_bps = 10000000;   // 10 Mbps
            config.burst_size = 16384;             // 16KB burst
            config.min_timeout_ms = 3000;          // 3 seconds minimum
            config.max_timeout_ms = 30000;         // 30 seconds maximum
            config.retry_delay_ms = 1000;          // 1 second retry delay
            config.timeout_multiplier = 1.5;       // 1.5x timeout multiplier
            config.enable_compression = false;     // Compression optional
            config.enable_keep_alive = true;       // Keep-alive recommended
            break;
            
        case BandwidthProfile::HighSpeed:
            config.max_bandwidth_bps = 100000000;  // 100 Mbps
            config.burst_size = 65536;             // 64KB burst
            config.min_timeout_ms = 1000;          // 1 second minimum
            config.max_timeout_ms = 15000;         // 15 seconds maximum
            config.retry_delay_ms = 500;           // 500ms retry delay
            config.timeout_multiplier = 1.2;       // 1.2x timeout multiplier
            config.enable_compression = false;     // No compression needed
            config.enable_keep_alive = true;       // Keep-alive recommended
            break;
            
        case BandwidthProfile::Unlimited:
            config.max_bandwidth_bps = UINT32_MAX; // No limit
            config.burst_size = 131072;            // 128KB burst
            config.min_timeout_ms = 500;           // 500ms minimum
            config.max_timeout_ms = 10000;         // 10 seconds maximum
            config.retry_delay_ms = 200;           // 200ms retry delay
            config.timeout_multiplier = 1.0;       // No timeout multiplier
            config.enable_compression = false;     // No compression needed
            config.enable_keep_alive = true;       // Keep-alive recommended
            break;
            
        case BandwidthProfile::Custom:
        default:
            // Default to broadband settings
            config = make_bandwidth_config(BandwidthProfile::Broadband);
            break;
    }
    
    return config;
}

/**
 * @brief Create ad-hoc parameters for PSP
 */
AdHocParams make_adhoc_params(const std::string& ssid, uint8_t channel,
                             uint32_t max_players) {
    AdHocParams params;
    params.ssid = ssid;
    params.channel = (channel == 1 || channel == 6 || channel == 11) ? channel : 1;
    params.group_name = "FlightHAL";
    params.max_players = std::min(max_players, 16u);  // PSP max is typically 16
    params.auto_accept_connections = true;
    params.game_id = "FLHT00001";  // Default game ID
    
    return params;
}

/**
 * @brief Check if IP address is valid
 */
bool is_valid_ip_address(const std::string& ip_address, bool is_ipv6) {
    if (is_ipv6) {
        // IPv6 validation (simplified)
        std::regex ipv6_regex(R"(^([0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$|^::1$|^::$)");
        return std::regex_match(ip_address, ipv6_regex);
    } else {
        // IPv4 validation
        std::regex ipv4_regex(R"(^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$)");
        std::smatch matches;
        
        if (!std::regex_match(ip_address, matches, ipv4_regex)) {
            return false;
        }
        
        // Check each octet is 0-255
        for (int i = 1; i <= 4; ++i) {
            int octet = std::stoi(matches[i].str());
            if (octet < 0 || octet > 255) {
                return false;
            }
        }
        
        return true;
    }
}

/**
 * @brief Check if hostname is valid
 */
bool is_valid_hostname(const std::string& hostname) {
    if (hostname.empty() || hostname.length() > 253) {
        return false;
    }
    
    // Check for valid hostname format
    std::regex hostname_regex(R"(^[a-zA-Z0-9]([a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?(\.[a-zA-Z0-9]([a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?)*$)");
    return std::regex_match(hostname, hostname_regex);
}

/**
 * @brief Check if port number is valid
 */
bool is_valid_port(uint16_t port) {
    return port >= 1 && port <= 65535;
}

/**
 * @brief Get default port for protocol
 */
uint16_t get_default_port(NetworkProtocol protocol) {
    switch (protocol) {
        case NetworkProtocol::HTTP: return 80;
        case NetworkProtocol::HTTPS: return 443;
        case NetworkProtocol::WebSocket: return 80;  // ws:// uses port 80
        case NetworkProtocol::TCP: return 0;  // No default for raw TCP
        case NetworkProtocol::UDP: return 0;  // No default for raw UDP
        case NetworkProtocol::Custom: return 0;  // No default for custom
        default: return 80;  // Fallback to HTTP
    }
}

/**
 * @brief Calculate timeout based on bandwidth profile
 */
uint32_t calculate_adaptive_timeout(uint32_t base_timeout_ms, BandwidthProfile profile) {
    BandwidthConfig config = make_bandwidth_config(profile);
    uint32_t adaptive_timeout = static_cast<uint32_t>(base_timeout_ms * config.timeout_multiplier);
    
    // Clamp to min/max values
    adaptive_timeout = std::max(adaptive_timeout, config.min_timeout_ms);
    adaptive_timeout = std::min(adaptive_timeout, config.max_timeout_ms);
    
    return adaptive_timeout;
}

/**
 * @brief Estimate transfer time
 */
uint32_t estimate_transfer_time(size_t data_size, uint64_t bandwidth_bps, 
                               double overhead_factor) {
    if (bandwidth_bps == 0) {
        return UINT32_MAX;  // Infinite time if no bandwidth
    }
    
    // Convert to bits and apply overhead
    uint64_t data_bits = data_size * 8;
    uint64_t adjusted_bits = static_cast<uint64_t>(data_bits * overhead_factor);
    
    // Calculate time in milliseconds
    uint64_t time_ms = (adjusted_bits * 1000) / bandwidth_bps;
    
    // Clamp to reasonable range
    return static_cast<uint32_t>(std::min(time_ms, static_cast<uint64_t>(UINT32_MAX)));
}

} // namespace network

} // namespace flight::hal

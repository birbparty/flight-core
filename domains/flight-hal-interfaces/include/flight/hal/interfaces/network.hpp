/**
 * @file network.hpp
 * @brief Flight HAL Elite Network Interface
 * 
 * Unified networking interface supporting TCP/UDP sockets, HTTP clients, and
 * platform-specific networking from Dreamcast modem/Ethernet to modern high-speed
 * connections. Designed for cross-platform game development spanning 25+ years
 * of hardware evolution with bandwidth optimization and connection management.
 */

#pragma once

#include "../core/driver_registry.hpp"
#include "../core/hal_result.hpp"
#include "../core/hal_capabilities.hpp"
#include "memory.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>
#include <string>
#include <string_view>
#include <memory>
#include <chrono>
#include <array>

namespace flight::hal {

/**
 * @brief Network-specific capabilities
 */
enum class NetworkCapability : uint32_t {
    // Basic socket capabilities
    TCP = 1 << 0,                   ///< TCP socket support
    UDP = 1 << 1,                   ///< UDP socket support
    IPv4 = 1 << 2,                  ///< IPv4 protocol support
    IPv6 = 1 << 3,                  ///< IPv6 protocol support
    
    // Connection types
    Dialup = 1 << 4,                ///< Dialup modem connection (Dreamcast)
    Ethernet = 1 << 5,              ///< Ethernet connection
    WiFi = 1 << 6,                  ///< WiFi connection (PSP, modern)
    Broadband = 1 << 7,             ///< High-speed broadband
    
    // Protocol support
    HTTP = 1 << 8,                  ///< HTTP/1.1 client support
    HTTPS = 1 << 9,                 ///< HTTPS/TLS support
    WebSocket = 1 << 10,            ///< WebSocket protocol support
    DNS = 1 << 11,                  ///< DNS resolution support
    
    // Advanced features
    ConnectionPooling = 1 << 12,    ///< HTTP connection pooling
    Compression = 1 << 13,          ///< Data compression support
    AsyncOperations = 1 << 14,      ///< Asynchronous network operations
    MultiThreaded = 1 << 15,        ///< Multi-threaded networking
    
    // Platform-specific features
    AdHocMode = 1 << 16,            ///< PSP ad-hoc networking
    InfrastructureMode = 1 << 17,   ///< PSP infrastructure mode
    BrowserAPI = 1 << 18,           ///< Browser networking APIs
    CORS = 1 << 19,                 ///< Cross-origin resource sharing
    
    // Bandwidth management
    BandwidthThrottling = 1 << 20,  ///< Bandwidth throttling support
    AdaptiveTimeout = 1 << 21,      ///< Adaptive timeout handling
    QualityOfService = 1 << 22,     ///< QoS support
    TrafficShaping = 1 << 23,       ///< Traffic shaping capabilities
    
    // Security features
    TLS = 1 << 24,                  ///< TLS/SSL encryption
    CertificateValidation = 1 << 25, ///< Certificate validation
    SecureRandom = 1 << 26,         ///< Cryptographically secure random
    
    // Monitoring and diagnostics
    Statistics = 1 << 27,           ///< Network statistics collection
    Diagnostics = 1 << 28,          ///< Network diagnostics tools
    Profiling = 1 << 29             ///< Performance profiling
};

/**
 * @brief Network protocol types
 */
enum class NetworkProtocol : uint8_t {
    TCP = 0,            ///< Transmission Control Protocol
    UDP,                ///< User Datagram Protocol
    HTTP,               ///< Hypertext Transfer Protocol
    HTTPS,              ///< HTTP over TLS/SSL
    WebSocket,          ///< WebSocket protocol
    Custom              ///< Custom protocol implementation
};

/**
 * @brief Socket types
 */
enum class SocketType : uint8_t {
    Stream = 0,         ///< Stream socket (TCP)
    Datagram,           ///< Datagram socket (UDP)
    Raw,                ///< Raw socket (if supported)
    WebSocket           ///< WebSocket connection
};

/**
 * @brief Connection states
 */
enum class ConnectionState : uint8_t {
    Disconnected = 0,   ///< Not connected
    Connecting,         ///< Connection in progress
    Connected,          ///< Successfully connected
    Disconnecting,      ///< Disconnection in progress
    Error,              ///< Connection error state
    Timeout             ///< Connection timed out
};

/**
 * @brief HTTP methods
 */
enum class HTTPMethod : uint8_t {
    GET = 0,            ///< GET request
    POST,               ///< POST request
    PUT,                ///< PUT request
    DELETE,             ///< DELETE request
    HEAD,               ///< HEAD request
    OPTIONS,            ///< OPTIONS request
    PATCH,              ///< PATCH request
    TRACE,              ///< TRACE request
    CONNECT             ///< CONNECT request
};

/**
 * @brief HTTP status codes (common ones)
 */
enum class HTTPStatusCode : uint16_t {
    OK = 200,           ///< Request successful
    Created = 201,      ///< Resource created
    NoContent = 204,    ///< No content to return
    MovedPermanently = 301, ///< Resource moved permanently
    Found = 302,        ///< Resource found at different location
    NotModified = 304,  ///< Resource not modified
    BadRequest = 400,   ///< Bad request syntax
    Unauthorized = 401, ///< Authentication required
    Forbidden = 403,    ///< Access forbidden
    NotFound = 404,     ///< Resource not found
    MethodNotAllowed = 405, ///< HTTP method not allowed
    RequestTimeout = 408, ///< Request timeout
    InternalServerError = 500, ///< Internal server error
    NotImplemented = 501, ///< Feature not implemented
    BadGateway = 502,   ///< Bad gateway
    ServiceUnavailable = 503, ///< Service temporarily unavailable
    GatewayTimeout = 504  ///< Gateway timeout
};

/**
 * @brief Bandwidth profiles for different connection types
 */
enum class BandwidthProfile : uint8_t {
    Dialup = 0,         ///< 56K modem (Dreamcast)
    Narrowband,         ///< Low-speed broadband (<1 Mbps)
    Broadband,          ///< Standard broadband (1-10 Mbps)
    HighSpeed,          ///< High-speed connection (>10 Mbps)
    Unlimited,          ///< No bandwidth constraints
    Custom              ///< Custom bandwidth settings
};

// Forward declarations
class INetworkSocket;
class IHTTPClient;
class IWebSocket;
class INetworkDiagnostics;

/**
 * @brief Network address structure
 */
struct NetworkAddress {
    std::string host;               ///< Hostname or IP address
    uint16_t port;                  ///< Port number
    NetworkProtocol protocol;       ///< Network protocol
    bool is_ipv6;                   ///< IPv6 address flag
    
    NetworkAddress() : port(0), protocol(NetworkProtocol::TCP), is_ipv6(false) {}
    NetworkAddress(const std::string& h, uint16_t p, NetworkProtocol proto = NetworkProtocol::TCP)
        : host(h), port(p), protocol(proto), is_ipv6(false) {}
    
    std::string to_string() const {
        return host + ":" + std::to_string(port);
    }
};

/**
 * @brief Socket handle for lightweight socket references
 */
struct SocketHandle {
    uint32_t id;                    ///< Unique socket identifier
    SocketType type;                ///< Socket type
    uint32_t generation;            ///< Generation counter for validation
    
    bool is_valid() const { return id != 0; }
    void invalidate() { id = 0; generation = 0; }
    
    bool operator==(const SocketHandle& other) const {
        return id == other.id && generation == other.generation;
    }
    
    bool operator!=(const SocketHandle& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Connection parameters
 */
struct ConnectionParams {
    NetworkAddress address;         ///< Target address
    uint32_t timeout_ms;            ///< Connection timeout in milliseconds
    uint32_t keep_alive_interval_ms; ///< Keep-alive interval (0 to disable)
    uint32_t max_retries;           ///< Maximum connection retries
    BandwidthProfile bandwidth_profile; ///< Bandwidth optimization profile
    bool enable_compression;        ///< Enable data compression
    bool enable_keep_alive;         ///< Enable keep-alive
    bool verify_certificates;       ///< Verify SSL certificates (HTTPS)
    std::string_view user_agent;    ///< User agent string
    std::string_view debug_name;    ///< Debug name for profiling
};

/**
 * @brief HTTP headers container
 */
struct HTTPHeaders {
    std::vector<std::pair<std::string, std::string>> headers;
    
    void add(const std::string& name, const std::string& value) {
        headers.emplace_back(name, value);
    }
    
    void set(const std::string& name, const std::string& value) {
        for (auto& header : headers) {
            if (header.first == name) {
                header.second = value;
                return;
            }
        }
        add(name, value);
    }
    
    std::string get(const std::string& name) const {
        for (const auto& header : headers) {
            if (header.first == name) {
                return header.second;
            }
        }
        return "";
    }
    
    bool has(const std::string& name) const {
        return !get(name).empty();
    }
    
    void clear() {
        headers.clear();
    }
    
    size_t size() const {
        return headers.size();
    }
};

/**
 * @brief HTTP request structure
 */
struct HTTPRequest {
    HTTPMethod method;              ///< HTTP method
    std::string url;                ///< Request URL
    HTTPHeaders headers;            ///< Request headers
    std::string body;               ///< Request body (for POST, PUT, etc.)
    uint32_t timeout_ms;            ///< Request timeout in milliseconds
    bool follow_redirects;          ///< Follow HTTP redirects
    uint32_t max_redirects;         ///< Maximum number of redirects to follow
    BandwidthProfile bandwidth_profile; ///< Bandwidth optimization profile
    std::string_view debug_name;    ///< Debug name for profiling
};

/**
 * @brief HTTP response structure
 */
struct HTTPResponse {
    HTTPStatusCode status_code;     ///< HTTP status code
    std::string status_message;     ///< HTTP status message
    HTTPHeaders headers;            ///< Response headers
    std::string body;               ///< Response body
    size_t content_length;          ///< Content length
    std::chrono::milliseconds response_time; ///< Response time
    std::string final_url;          ///< Final URL after redirects
    uint32_t redirect_count;        ///< Number of redirects followed
};

/**
 * @brief WebSocket frame types
 */
enum class WebSocketFrameType : uint8_t {
    Text = 0,           ///< Text frame
    Binary,             ///< Binary frame
    Close,              ///< Close frame
    Ping,               ///< Ping frame
    Pong                ///< Pong frame
};

/**
 * @brief WebSocket message structure
 */
struct WebSocketMessage {
    WebSocketFrameType type;        ///< Frame type
    std::vector<uint8_t> data;      ///< Message data
    bool is_final;                  ///< Final frame in message
    std::chrono::system_clock::time_point timestamp; ///< Message timestamp
};

/**
 * @brief Network statistics
 */
struct NetworkStats {
    uint64_t bytes_sent;            ///< Total bytes sent
    uint64_t bytes_received;        ///< Total bytes received
    uint64_t packets_sent;          ///< Total packets sent
    uint64_t packets_received;      ///< Total packets received
    uint64_t connections_opened;    ///< Total connections opened
    uint64_t connections_closed;    ///< Total connections closed
    uint64_t connection_errors;     ///< Total connection errors
    uint64_t timeouts;              ///< Total timeouts
    uint64_t retries;               ///< Total retry attempts
    
    // Performance metrics
    double average_response_time_ms; ///< Average response time
    double average_throughput_bps;  ///< Average throughput in bits per second
    double packet_loss_rate;        ///< Packet loss rate (0-1)
    double connection_success_rate; ///< Connection success rate (0-1)
    
    // Current state
    uint32_t active_connections;    ///< Currently active connections
    uint32_t pending_operations;    ///< Pending network operations
    size_t buffer_usage;            ///< Current buffer usage
    std::chrono::system_clock::time_point last_activity; ///< Last network activity
};

/**
 * @brief Bandwidth configuration
 */
struct BandwidthConfig {
    uint32_t max_bandwidth_bps;     ///< Maximum bandwidth in bits per second
    uint32_t burst_size;            ///< Burst size for traffic shaping
    uint32_t min_timeout_ms;        ///< Minimum timeout in milliseconds
    uint32_t max_timeout_ms;        ///< Maximum timeout in milliseconds
    uint32_t retry_delay_ms;        ///< Delay between retries
    double timeout_multiplier;      ///< Timeout multiplier for slow connections
    bool enable_compression;        ///< Enable compression for this profile
    bool enable_keep_alive;         ///< Enable keep-alive connections
};

/**
 * @brief PSP ad-hoc networking parameters
 */
struct AdHocParams {
    std::string ssid;               ///< Network SSID
    uint8_t channel;                ///< WiFi channel (1, 6, or 11)
    std::string group_name;         ///< Ad-hoc group name
    uint32_t max_players;           ///< Maximum number of players
    bool auto_accept_connections;   ///< Automatically accept connections
    std::string_view game_id;       ///< Game identifier for compatibility
};

// Callback function types
using SocketDataCallback = std::function<void(SocketHandle socket, const void* data, size_t size)>;
using SocketStateCallback = std::function<void(SocketHandle socket, ConnectionState state)>;
using HTTPResponseCallback = std::function<void(HALResult<HTTPResponse> response)>;
using WebSocketMessageCallback = std::function<void(const WebSocketMessage& message)>;
using WebSocketStateCallback = std::function<void(ConnectionState state)>;
using NetworkEventCallback = std::function<void(const std::string& event, const void* data, size_t size)>;

/**
 * @brief Network socket interface
 * 
 * Abstract interface for network socket operations.
 */
class INetworkSocket {
public:
    virtual ~INetworkSocket() = default;
    
    /**
     * @brief Get socket handle
     * @return Socket handle
     */
    virtual SocketHandle get_handle() const = 0;
    
    /**
     * @brief Get socket type
     * @return Socket type
     */
    virtual SocketType get_type() const = 0;
    
    /**
     * @brief Get connection state
     * @return Current connection state
     */
    virtual ConnectionState get_state() const = 0;
    
    /**
     * @brief Get remote address
     * @return Remote endpoint address
     */
    virtual NetworkAddress get_remote_address() const = 0;
    
    /**
     * @brief Get local address
     * @return Local endpoint address
     */
    virtual NetworkAddress get_local_address() const = 0;
    
    /**
     * @brief Connect to remote endpoint
     * @param params Connection parameters
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> connect(const ConnectionParams& params) = 0;
    
    /**
     * @brief Send data
     * @param data Data to send
     * @param size Size of data in bytes
     * @return HALResult containing bytes sent on success
     */
    virtual HALResult<size_t> send(const void* data, size_t size) = 0;
    
    /**
     * @brief Receive data
     * @param buffer Buffer to receive data
     * @param size Buffer size
     * @return HALResult containing bytes received on success
     */
    virtual HALResult<size_t> receive(void* buffer, size_t size) = 0;
    
    /**
     * @brief Close socket
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> close() = 0;
    
    /**
     * @brief Set socket option
     * @param option Option name
     * @param value Option value
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_option(const std::string& option, const std::string& value) = 0;
    
    /**
     * @brief Get socket option
     * @param option Option name
     * @return HALResult containing option value on success
     */
    virtual HALResult<std::string> get_option(const std::string& option) = 0;
    
    /**
     * @brief Check if socket is connected
     * @return true if socket is connected
     */
    virtual bool is_connected() const = 0;
    
    /**
     * @brief Get bytes available for reading
     * @return Number of bytes available
     */
    virtual size_t bytes_available() const = 0;
    
    /**
     * @brief Set data callback
     * @param callback Callback for received data
     */
    virtual void set_data_callback(SocketDataCallback callback) = 0;
    
    /**
     * @brief Set state change callback
     * @param callback Callback for state changes
     */
    virtual void set_state_callback(SocketStateCallback callback) = 0;
};

/**
 * @brief HTTP client interface
 * 
 * High-level HTTP client with automatic connection management.
 */
class IHTTPClient {
public:
    virtual ~IHTTPClient() = default;
    
    /**
     * @brief Send HTTP request
     * @param request HTTP request to send
     * @return HALResult containing HTTP response on success
     */
    virtual HALResult<HTTPResponse> send_request(const HTTPRequest& request) = 0;
    
    /**
     * @brief Send HTTP request asynchronously
     * @param request HTTP request to send
     * @param callback Callback for response
     * @return HALResult containing operation ID on success
     */
    virtual HALResult<uint32_t> send_request_async(const HTTPRequest& request,
                                                   HTTPResponseCallback callback) = 0;
    
    /**
     * @brief Send GET request
     * @param url Target URL
     * @param headers Optional headers
     * @return HALResult containing HTTP response on success
     */
    virtual HALResult<HTTPResponse> get(const std::string& url, 
                                       const HTTPHeaders& headers = {}) = 0;
    
    /**
     * @brief Send POST request
     * @param url Target URL
     * @param body Request body
     * @param headers Optional headers
     * @return HALResult containing HTTP response on success
     */
    virtual HALResult<HTTPResponse> post(const std::string& url,
                                        const std::string& body,
                                        const HTTPHeaders& headers = {}) = 0;
    
    /**
     * @brief Send PUT request
     * @param url Target URL
     * @param body Request body
     * @param headers Optional headers
     * @return HALResult containing HTTP response on success
     */
    virtual HALResult<HTTPResponse> put(const std::string& url,
                                       const std::string& body,
                                       const HTTPHeaders& headers = {}) = 0;
    
    /**
     * @brief Send DELETE request
     * @param url Target URL
     * @param headers Optional headers
     * @return HALResult containing HTTP response on success
     */
    virtual HALResult<HTTPResponse> delete_request(const std::string& url,
                                                  const HTTPHeaders& headers = {}) = 0;
    
    /**
     * @brief Cancel async operation
     * @param operation_id Operation ID from send_request_async
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> cancel_request(uint32_t operation_id) = 0;
    
    /**
     * @brief Set default headers for all requests
     * @param headers Default headers
     */
    virtual void set_default_headers(const HTTPHeaders& headers) = 0;
    
    /**
     * @brief Set default timeout for all requests
     * @param timeout_ms Timeout in milliseconds
     */
    virtual void set_default_timeout(uint32_t timeout_ms) = 0;
    
    /**
     * @brief Set bandwidth profile
     * @param profile Bandwidth profile to use
     */
    virtual void set_bandwidth_profile(BandwidthProfile profile) = 0;
    
    /**
     * @brief Clear connection pool
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> clear_connection_pool() = 0;
};

/**
 * @brief WebSocket interface
 * 
 * WebSocket client for real-time communication.
 */
class IWebSocket {
public:
    virtual ~IWebSocket() = default;
    
    /**
     * @brief Connect to WebSocket server
     * @param url WebSocket URL (ws:// or wss://)
     * @param protocols Supported protocols
     * @param headers Additional headers
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> connect(const std::string& url,
                                   const std::vector<std::string>& protocols = {},
                                   const HTTPHeaders& headers = {}) = 0;
    
    /**
     * @brief Send text message
     * @param message Text message to send
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> send_text(const std::string& message) = 0;
    
    /**
     * @brief Send binary message
     * @param data Binary data to send
     * @param size Size of data
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> send_binary(const void* data, size_t size) = 0;
    
    /**
     * @brief Send ping frame
     * @param data Optional ping data
     * @param size Size of ping data
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> ping(const void* data = nullptr, size_t size = 0) = 0;
    
    /**
     * @brief Send pong frame
     * @param data Pong data (usually echoes ping data)
     * @param size Size of pong data
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> pong(const void* data = nullptr, size_t size = 0) = 0;
    
    /**
     * @brief Close WebSocket connection
     * @param code Close code
     * @param reason Close reason
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> close(uint16_t code = 1000, const std::string& reason = "") = 0;
    
    /**
     * @brief Get connection state
     * @return Current connection state
     */
    virtual ConnectionState get_state() const = 0;
    
    /**
     * @brief Check if connected
     * @return true if connected
     */
    virtual bool is_connected() const = 0;
    
    /**
     * @brief Set message callback
     * @param callback Callback for received messages
     */
    virtual void set_message_callback(WebSocketMessageCallback callback) = 0;
    
    /**
     * @brief Set state change callback
     * @param callback Callback for state changes
     */
    virtual void set_state_callback(WebSocketStateCallback callback) = 0;
    
    /**
     * @brief Get selected protocol
     * @return Selected WebSocket protocol
     */
    virtual std::string get_selected_protocol() const = 0;
    
    /**
     * @brief Get connection URL
     * @return WebSocket connection URL
     */
    virtual std::string get_url() const = 0;
};

/**
 * @brief Network diagnostics interface
 * 
 * Network diagnostics and monitoring tools.
 */
class INetworkDiagnostics {
public:
    virtual ~INetworkDiagnostics() = default;
    
    /**
     * @brief Ping host
     * @param host Host to ping
     * @param timeout_ms Ping timeout
     * @return HALResult containing ping time in milliseconds
     */
    virtual HALResult<uint32_t> ping(const std::string& host, uint32_t timeout_ms = 5000) = 0;
    
    /**
     * @brief Trace route to host
     * @param host Target host
     * @param max_hops Maximum number of hops
     * @return HALResult containing list of hop addresses
     */
    virtual HALResult<std::vector<std::string>> traceroute(const std::string& host, 
                                                          uint32_t max_hops = 30) = 0;
    
    /**
     * @brief Test connection bandwidth
     * @param url Test server URL
     * @param duration_ms Test duration in milliseconds
     * @return HALResult containing bandwidth in bits per second
     */
    virtual HALResult<uint64_t> test_bandwidth(const std::string& url, 
                                              uint32_t duration_ms = 10000) = 0;
    
    /**
     * @brief Get network interface information
     * @return HALResult containing list of network interfaces
     */
    virtual HALResult<std::vector<std::string>> get_network_interfaces() = 0;
    
    /**
     * @brief Test DNS resolution
     * @param hostname Hostname to resolve
     * @return HALResult containing resolved IP addresses
     */
    virtual HALResult<std::vector<std::string>> resolve_hostname(const std::string& hostname) = 0;
    
    /**
     * @brief Check port connectivity
     * @param host Target host
     * @param port Target port
     * @param timeout_ms Connection timeout
     * @return HALResult indicating if port is accessible
     */
    virtual HALResult<bool> check_port(const std::string& host, uint16_t port, 
                                      uint32_t timeout_ms = 5000) = 0;
};

/**
 * @brief Enhanced network interface
 * 
 * Comprehensive networking system supporting TCP/UDP sockets, HTTP clients,
 * WebSocket connections, and platform-specific optimizations for gaming
 * applications across diverse hardware platforms.
 */
class INetworkInterface : public IHALInterface, public ICapabilityProvider {
public:
    virtual ~INetworkInterface() = default;
    
    // === Initialization and Configuration ===
    
    /**
     * @brief Initialize network subsystem
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> initialize() = 0;
    
    /**
     * @brief Shutdown network subsystem
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> shutdown() = 0;
    
    /**
     * @brief Check if network system is initialized
     * @return true if initialized and ready
     */
    virtual bool is_initialized() const = 0;
    
    /**
     * @brief Check network connectivity
     * @return true if network is available
     */
    virtual bool is_network_available() const = 0;
    
    /**
     * @brief Set bandwidth profile
     * @param profile Bandwidth profile to use
     * @param config Custom bandwidth configuration (optional)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_bandwidth_profile(BandwidthProfile profile,
                                                  const BandwidthConfig* config = nullptr) = 0;
    
    /**
     * @brief Get current bandwidth profile
     * @return Current bandwidth profile
     */
    virtual BandwidthProfile get_bandwidth_profile() const = 0;
    
    // === Socket Operations ===
    
    /**
     * @brief Create socket
     * @param type Socket type
     * @param protocol Network protocol
     * @return HALResult containing socket interface on success
     */
    virtual HALResult<std::unique_ptr<INetworkSocket>> create_socket(SocketType type,
                                                                    NetworkProtocol protocol) = 0;
    
    /**
     * @brief Create TCP socket
     * @return HALResult containing TCP socket on success
     */
    virtual HALResult<std::unique_ptr<INetworkSocket>> create_tcp_socket() = 0;
    
    /**
     * @brief Create UDP socket
     * @return HALResult containing UDP socket on success
     */
    virtual HALResult<std::unique_ptr<INetworkSocket>> create_udp_socket() = 0;
    
    /**
     * @brief Destroy socket
     * @param socket_handle Socket handle to destroy
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> destroy_socket(SocketHandle socket_handle) = 0;
    
    /**
     * @brief Get socket by handle
     * @param socket_handle Socket handle
     * @return HALResult containing socket interface on success
     */
    virtual HALResult<INetworkSocket*> get_socket(SocketHandle socket_handle) = 0;
    
    // === HTTP Client ===
    
    /**
     * @brief Get HTTP client
     * @return HALResult containing HTTP client interface on success
     */
    virtual HALResult<IHTTPClient*> get_http_client() = 0;
    
    /**
     * @brief Create dedicated HTTP client
     * @param config Custom configuration (optional)
     * @return HALResult containing HTTP client interface on success
     */
    virtual HALResult<std::unique_ptr<IHTTPClient>> create_http_client(const BandwidthConfig* config = nullptr) = 0;
    
    // === WebSocket Operations ===
    
    /**
     * @brief Create WebSocket connection
     * @return HALResult containing WebSocket interface on success
     */
    virtual HALResult<std::unique_ptr<IWebSocket>> create_websocket() = 0;
    
    // === DNS Operations ===
    
    /**
     * @brief Resolve hostname to IP addresses
     * @param hostname Hostname to resolve
     * @param timeout_ms Resolution timeout in milliseconds
     * @return HALResult containing list of IP addresses on success
     */
    virtual HALResult<std::vector<std::string>> resolve_hostname(const std::string& hostname,
                                                                uint32_t timeout_ms = 5000) = 0;
    
    /**
     * @brief Resolve hostname asynchronously
     * @param hostname Hostname to resolve
     * @param callback Callback for result
     * @param timeout_ms Resolution timeout in milliseconds
     * @return HALResult containing operation ID on success
     */
    virtual HALResult<uint32_t> resolve_hostname_async(const std::string& hostname,
                                                      std::function<void(HALResult<std::vector<std::string>>)> callback,
                                                      uint32_t timeout_ms = 5000) = 0;
    
    // === Platform-Specific Extensions ===
    
    /**
     * @brief Initialize PSP ad-hoc networking
     * @param params Ad-hoc parameters
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> initialize_adhoc_mode(const AdHocParams& params) = 0;
    
    /**
     * @brief Shutdown PSP ad-hoc networking
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> shutdown_adhoc_mode() = 0;
    
    /**
     * @brief Scan for ad-hoc networks
     * @param timeout_ms Scan timeout in milliseconds
     * @return HALResult containing list of available networks
     */
    virtual HALResult<std::vector<std::string>> scan_adhoc_networks(uint32_t timeout_ms = 10000) = 0;
    
    /**
     * @brief Join ad-hoc network
     * @param network_name Name of network to join
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> join_adhoc_network(const std::string& network_name) = 0;
    
    /**
     * @brief Leave ad-hoc network
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> leave_adhoc_network() = 0;
    
    /**
     * @brief Get browser networking interface (Web platform)
     * @return Pointer to browser networking interface (nullptr if not supported)
     */
    virtual void* get_browser_interface() = 0;
    
    // === Connection Management ===
    
    /**
     * @brief Get active connections
     * @return List of active connection handles
     */
    virtual std::vector<SocketHandle> get_active_connections() const = 0;
    
    /**
     * @brief Close all connections
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> close_all_connections() = 0;
    
    /**
     * @brief Set global connection timeout
     * @param timeout_ms Timeout in milliseconds
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_global_timeout(uint32_t timeout_ms) = 0;
    
    /**
     * @brief Get global connection timeout
     * @return Current global timeout in milliseconds
     */
    virtual uint32_t get_global_timeout() const = 0;
    
    // === Bandwidth Management ===
    
    /**
     * @brief Enable bandwidth throttling
     * @param max_bps Maximum bandwidth in bits per second
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> enable_bandwidth_throttling(uint64_t max_bps) = 0;
    
    /**
     * @brief Disable bandwidth throttling
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> disable_bandwidth_throttling() = 0;
    
    /**
     * @brief Get current bandwidth usage
     * @return Current bandwidth usage in bits per second
     */
    virtual uint64_t get_current_bandwidth_usage() const = 0;
    
    /**
     * @brief Set traffic shaping parameters
     * @param burst_size Maximum burst size in bytes
     * @param token_rate Token refill rate in bytes per second
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_traffic_shaping(uint32_t burst_size, uint32_t token_rate) = 0;
    
    // === Statistics and Monitoring ===
    
    /**
     * @brief Get network statistics
     * @return Network statistics structure
     */
    virtual NetworkStats get_stats() const = 0;
    
    /**
     * @brief Reset network statistics
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> reset_stats() = 0;
    
    /**
     * @brief Enable network event logging
     * @param callback Event callback
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> enable_event_logging(NetworkEventCallback callback) = 0;
    
    /**
     * @brief Disable network event logging
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> disable_event_logging() = 0;
    
    /**
     * @brief Get network diagnostics interface
     * @return HALResult containing diagnostics interface on success
     */
    virtual HALResult<INetworkDiagnostics*> get_diagnostics() = 0;
    
    // === Security ===
    
    /**
     * @brief Add trusted certificate
     * @param certificate_data Certificate data in PEM format
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> add_trusted_certificate(const std::string& certificate_data) = 0;
    
    /**
     * @brief Remove trusted certificate
     * @param certificate_fingerprint Certificate fingerprint
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> remove_trusted_certificate(const std::string& certificate_fingerprint) = 0;
    
    /**
     * @brief Clear all trusted certificates
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> clear_trusted_certificates() = 0;
    
    /**
     * @brief Set TLS configuration
     * @param min_version Minimum TLS version
     * @param max_version Maximum TLS version
     * @param cipher_suites Allowed cipher suites
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_tls_config(const std::string& min_version,
                                          const std::string& max_version,
                                          const std::vector<std::string>& cipher_suites) = 0;
    
    // === Capability Queries ===
    
    /**
     * @brief Check if network capability is supported
     * @param capability Network capability to check
     * @return true if capability is supported
     */
    virtual bool supports_network_capability(NetworkCapability capability) const = 0;
    
    /**
     * @brief Check if protocol is supported
     * @param protocol Network protocol to check
     * @return true if protocol is supported
     */
    virtual bool supports_protocol(NetworkProtocol protocol) const = 0;
    
    /**
     * @brief Get supported protocols
     * @return Vector of supported protocols
     */
    virtual std::vector<NetworkProtocol> get_supported_protocols() const = 0;
    
    /**
     * @brief Get maximum concurrent connections
     * @return Maximum number of concurrent connections
     */
    virtual uint32_t get_max_concurrent_connections() const = 0;
    
    /**
     * @brief Get maximum message size
     * @return Maximum message size in bytes
     */
    virtual size_t get_max_message_size() const = 0;
    
    // === Platform-Specific Extensions ===
    
    /**
     * @brief Get platform-specific extension interface
     * @param extension_name Name of extension to query
     * @return Pointer to extension interface (nullptr if not supported)
     */
    virtual void* get_extension_interface(std::string_view extension_name) = 0;
    
    /**
     * @brief Dreamcast specific: Get modem interface
     * @return Pointer to modem interface (Dreamcast only)
     */
    virtual void* get_modem_interface() = 0;
    
    /**
     * @brief PSP specific: Get WiFi interface
     * @return Pointer to WiFi interface (PSP only)
     */
    virtual void* get_wifi_interface() = 0;
    
    // === IHALInterface implementation ===
    std::string_view get_interface_name() const override {
        return "network";
    }
};

// === Utility Functions ===

/**
 * @brief Network utility functions
 */
namespace network {

/**
 * @brief Convert network capability to string
 * @param capability Network capability to convert
 * @return String representation
 */
const char* to_string(NetworkCapability capability);

/**
 * @brief Convert network protocol to string
 * @param protocol Network protocol to convert
 * @return String representation
 */
const char* to_string(NetworkProtocol protocol);

/**
 * @brief Convert socket type to string
 * @param type Socket type to convert
 * @return String representation
 */
const char* to_string(SocketType type);

/**
 * @brief Convert connection state to string
 * @param state Connection state to convert
 * @return String representation
 */
const char* to_string(ConnectionState state);

/**
 * @brief Convert HTTP method to string
 * @param method HTTP method to convert
 * @return String representation
 */
const char* to_string(HTTPMethod method);

/**
 * @brief Convert HTTP status code to string
 * @param status_code HTTP status code to convert
 * @return String representation
 */
const char* to_string(HTTPStatusCode status_code);

/**
 * @brief Convert bandwidth profile to string
 * @param profile Bandwidth profile to convert
 * @return String representation
 */
const char* to_string(BandwidthProfile profile);

/**
 * @brief Parse URL into components
 * @param url URL to parse
 * @param protocol Output protocol
 * @param host Output host
 * @param port Output port
 * @param path Output path
 * @return true if URL was parsed successfully
 */
bool parse_url(const std::string& url, std::string& protocol, std::string& host, 
               uint16_t& port, std::string& path);

/**
 * @brief Build URL from components
 * @param protocol Protocol (http, https, ws, wss)
 * @param host Host name or IP address
 * @param port Port number
 * @param path Path component
 * @return Complete URL string
 */
std::string build_url(const std::string& protocol, const std::string& host,
                     uint16_t port, const std::string& path = "/");

/**
 * @brief URL encode string
 * @param input String to encode
 * @return URL encoded string
 */
std::string url_encode(const std::string& input);

/**
 * @brief URL decode string
 * @param input URL encoded string
 * @return Decoded string
 */
std::string url_decode(const std::string& input);

/**
 * @brief Create default connection parameters
 * @param address Target address
 * @param timeout_ms Connection timeout
 * @return Configured ConnectionParams
 */
ConnectionParams make_connection_params(const NetworkAddress& address,
                                       uint32_t timeout_ms = 30000);

/**
 * @brief Create connection parameters for low bandwidth
 * @param address Target address
 * @param bandwidth_profile Bandwidth profile
 * @return Configured ConnectionParams optimized for bandwidth
 */
ConnectionParams make_low_bandwidth_params(const NetworkAddress& address,
                                          BandwidthProfile bandwidth_profile = BandwidthProfile::Dialup);

/**
 * @brief Create HTTP request
 * @param method HTTP method
 * @param url Target URL
 * @param body Request body (optional)
 * @return Configured HTTPRequest
 */
HTTPRequest make_http_request(HTTPMethod method, const std::string& url,
                             const std::string& body = "");

/**
 * @brief Create HTTP GET request
 * @param url Target URL
 * @param headers Optional headers
 * @return Configured HTTPRequest for GET
 */
HTTPRequest make_get_request(const std::string& url, const HTTPHeaders& headers = {});

/**
 * @brief Create HTTP POST request
 * @param url Target URL
 * @param body Request body
 * @param content_type Content type (default: application/json)
 * @return Configured HTTPRequest for POST
 */
HTTPRequest make_post_request(const std::string& url, const std::string& body,
                             const std::string& content_type = "application/json");

/**
 * @brief Create default bandwidth configuration
 * @param profile Bandwidth profile
 * @return Configured BandwidthConfig
 */
BandwidthConfig make_bandwidth_config(BandwidthProfile profile);

/**
 * @brief Create ad-hoc parameters for PSP
 * @param ssid Network SSID
 * @param channel WiFi channel (1, 6, or 11)
 * @param max_players Maximum players
 * @return Configured AdHocParams
 */
AdHocParams make_adhoc_params(const std::string& ssid, uint8_t channel = 1,
                             uint32_t max_players = 4);

/**
 * @brief Check if IP address is valid
 * @param ip_address IP address string
 * @param is_ipv6 Check as IPv6 (default: auto-detect)
 * @return true if IP address is valid
 */
bool is_valid_ip_address(const std::string& ip_address, bool is_ipv6 = false);

/**
 * @brief Check if hostname is valid
 * @param hostname Hostname string
 * @return true if hostname is valid
 */
bool is_valid_hostname(const std::string& hostname);

/**
 * @brief Check if port number is valid
 * @param port Port number
 * @return true if port is valid (1-65535)
 */
bool is_valid_port(uint16_t port);

/**
 * @brief Get default port for protocol
 * @param protocol Network protocol
 * @return Default port number
 */
uint16_t get_default_port(NetworkProtocol protocol);

/**
 * @brief Calculate timeout based on bandwidth profile
 * @param base_timeout_ms Base timeout in milliseconds
 * @param profile Bandwidth profile
 * @return Adjusted timeout in milliseconds
 */
uint32_t calculate_adaptive_timeout(uint32_t base_timeout_ms, BandwidthProfile profile);

/**
 * @brief Estimate transfer time
 * @param data_size Data size in bytes
 * @param bandwidth_bps Bandwidth in bits per second
 * @param overhead_factor Overhead factor (default: 1.2)
 * @return Estimated transfer time in milliseconds
 */
uint32_t estimate_transfer_time(size_t data_size, uint64_t bandwidth_bps, 
                               double overhead_factor = 1.2);

} // namespace network

} // namespace flight::hal

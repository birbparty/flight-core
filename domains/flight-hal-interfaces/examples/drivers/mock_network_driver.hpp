/**
 * @file mock_network_driver.hpp
 * @brief Mock Network Driver for Testing and Examples
 * 
 * Mock implementation of the Flight HAL Network Interface for testing,
 * examples, and development purposes. Provides simulated networking
 * capabilities with configurable behavior and platform-specific features.
 */

#pragma once

#include "../../include/flight/hal/interfaces/network.hpp"
#include "../../include/flight/hal/core/hal_logging.hpp"
#include "../../include/flight/hal/core/hal_capabilities.hpp"
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <random>

namespace flight::hal::examples {

/**
 * @brief Mock Network Socket Implementation
 * 
 * Simulates socket behavior with configurable delays and error conditions.
 */
class MockNetworkSocket : public INetworkSocket {
public:
    MockNetworkSocket(SocketType type, NetworkProtocol protocol, uint32_t socket_id);
    virtual ~MockNetworkSocket();
    
    // INetworkSocket implementation
    SocketHandle get_handle() const override;
    SocketType get_type() const override;
    ConnectionState get_state() const override;
    NetworkAddress get_remote_address() const override;
    NetworkAddress get_local_address() const override;
    
    HALResult<void> connect(const ConnectionParams& params) override;
    HALResult<size_t> send(const void* data, size_t size) override;
    HALResult<size_t> receive(void* buffer, size_t size) override;
    HALResult<void> close() override;
    
    HALResult<void> set_option(const std::string& option, const std::string& value) override;
    HALResult<std::string> get_option(const std::string& option) override;
    
    bool is_connected() const override;
    size_t bytes_available() const override;
    
    void set_data_callback(SocketDataCallback callback) override;
    void set_state_callback(SocketStateCallback callback) override;
    
    // Mock-specific methods
    void simulate_data_received(const void* data, size_t size);
    void simulate_connection_state_change(ConnectionState new_state);
    void set_simulated_latency(uint32_t latency_ms);
    void set_error_rate(double error_rate);

private:
    SocketHandle handle_;
    SocketType type_;
    NetworkProtocol protocol_;
    std::atomic<ConnectionState> state_;
    NetworkAddress remote_address_;
    NetworkAddress local_address_;
    
    SocketDataCallback data_callback_;
    SocketStateCallback state_callback_;
    
    std::unordered_map<std::string, std::string> options_;
    std::queue<uint8_t> receive_buffer_;
    mutable std::mutex mutex_;
    
    // Mock simulation parameters
    uint32_t simulated_latency_ms_;
    double error_rate_;
    std::mt19937 random_generator_;
    
    void notify_state_change(ConnectionState new_state);
};

/**
 * @brief Mock HTTP Client Implementation
 * 
 * Simulates HTTP client behavior with configurable responses and delays.
 */
class MockHTTPClient : public IHTTPClient {
public:
    MockHTTPClient();
    virtual ~MockHTTPClient();
    
    // IHTTPClient implementation
    HALResult<HTTPResponse> send_request(const HTTPRequest& request) override;
    HALResult<uint32_t> send_request_async(const HTTPRequest& request,
                                          HTTPResponseCallback callback) override;
    
    HALResult<HTTPResponse> get(const std::string& url, 
                               const HTTPHeaders& headers = {}) override;
    HALResult<HTTPResponse> post(const std::string& url,
                                const std::string& body,
                                const HTTPHeaders& headers = {}) override;
    HALResult<HTTPResponse> put(const std::string& url,
                               const std::string& body,
                               const HTTPHeaders& headers = {}) override;
    HALResult<HTTPResponse> delete_request(const std::string& url,
                                          const HTTPHeaders& headers = {}) override;
    
    HALResult<void> cancel_request(uint32_t operation_id) override;
    
    void set_default_headers(const HTTPHeaders& headers) override;
    void set_default_timeout(uint32_t timeout_ms) override;
    void set_bandwidth_profile(BandwidthProfile profile) override;
    
    HALResult<void> clear_connection_pool() override;
    
    // Mock-specific methods
    void set_mock_response(const std::string& url, const HTTPResponse& response);
    void set_simulated_latency(uint32_t latency_ms);
    void set_error_rate(double error_rate);

private:
    HTTPHeaders default_headers_;
    uint32_t default_timeout_ms_;
    BandwidthProfile bandwidth_profile_;
    
    std::unordered_map<std::string, HTTPResponse> mock_responses_;
    std::atomic<uint32_t> next_operation_id_;
    
    // Mock simulation parameters
    uint32_t simulated_latency_ms_;
    double error_rate_;
    std::mt19937 random_generator_;
    
    mutable std::mutex mutex_;
    
    HTTPResponse create_default_response(HTTPStatusCode status_code, 
                                        const std::string& body = "");
    HTTPResponse simulate_request(const HTTPRequest& request);
};

/**
 * @brief Mock WebSocket Implementation
 * 
 * Simulates WebSocket behavior with message queuing and state management.
 */
class MockWebSocket : public IWebSocket {
public:
    MockWebSocket();
    virtual ~MockWebSocket();
    
    // IWebSocket implementation
    HALResult<void> connect(const std::string& url,
                           const std::vector<std::string>& protocols = {},
                           const HTTPHeaders& headers = {}) override;
    
    HALResult<void> send_text(const std::string& message) override;
    HALResult<void> send_binary(const void* data, size_t size) override;
    HALResult<void> ping(const void* data = nullptr, size_t size = 0) override;
    HALResult<void> pong(const void* data = nullptr, size_t size = 0) override;
    HALResult<void> close(uint16_t code = 1000, const std::string& reason = "") override;
    
    ConnectionState get_state() const override;
    bool is_connected() const override;
    
    void set_message_callback(WebSocketMessageCallback callback) override;
    void set_state_callback(WebSocketStateCallback callback) override;
    
    std::string get_selected_protocol() const override;
    std::string get_url() const override;
    
    // Mock-specific methods
    void simulate_message_received(const WebSocketMessage& message);
    void simulate_connection_state_change(ConnectionState new_state);

private:
    std::atomic<ConnectionState> state_;
    std::string url_;
    std::string selected_protocol_;
    
    WebSocketMessageCallback message_callback_;
    WebSocketStateCallback state_callback_;
    
    mutable std::mutex mutex_;
};

/**
 * @brief Mock Network Diagnostics Implementation
 * 
 * Simulates network diagnostic tools with configurable results.
 */
class MockNetworkDiagnostics : public INetworkDiagnostics {
public:
    MockNetworkDiagnostics();
    virtual ~MockNetworkDiagnostics();
    
    // INetworkDiagnostics implementation
    HALResult<uint32_t> ping(const std::string& host, uint32_t timeout_ms = 5000) override;
    HALResult<std::vector<std::string>> traceroute(const std::string& host, 
                                                  uint32_t max_hops = 30) override;
    HALResult<uint64_t> test_bandwidth(const std::string& url, 
                                      uint32_t duration_ms = 10000) override;
    HALResult<std::vector<std::string>> get_network_interfaces() override;
    HALResult<std::vector<std::string>> resolve_hostname(const std::string& hostname) override;
    HALResult<bool> check_port(const std::string& host, uint16_t port, 
                              uint32_t timeout_ms = 5000) override;
    
    // Mock-specific methods
    void set_ping_time(const std::string& host, uint32_t ping_time_ms);
    void set_bandwidth(uint64_t bandwidth_bps);

private:
    std::unordered_map<std::string, uint32_t> ping_times_;
    uint64_t simulated_bandwidth_;
    mutable std::mutex mutex_;
};

/**
 * @brief Mock Network Interface Implementation
 * 
 * Complete mock implementation of the Flight HAL Network Interface
 * supporting all platforms and features with configurable behavior.
 */
class MockNetworkInterface : public INetworkInterface {
public:
    MockNetworkInterface();
    virtual ~MockNetworkInterface();
    
    // === Initialization and Configuration ===
    HALResult<void> initialize() override;
    HALResult<void> shutdown() override;
    bool is_initialized() const override;
    bool is_network_available() const override;
    
    HALResult<void> set_bandwidth_profile(BandwidthProfile profile,
                                         const BandwidthConfig* config = nullptr) override;
    BandwidthProfile get_bandwidth_profile() const override;
    
    // === Socket Operations ===
    HALResult<std::unique_ptr<INetworkSocket>> create_socket(SocketType type,
                                                            NetworkProtocol protocol) override;
    HALResult<std::unique_ptr<INetworkSocket>> create_tcp_socket() override;
    HALResult<std::unique_ptr<INetworkSocket>> create_udp_socket() override;
    HALResult<void> destroy_socket(SocketHandle socket_handle) override;
    HALResult<INetworkSocket*> get_socket(SocketHandle socket_handle) override;
    
    // === HTTP Client ===
    HALResult<IHTTPClient*> get_http_client() override;
    HALResult<std::unique_ptr<IHTTPClient>> create_http_client(const BandwidthConfig* config = nullptr) override;
    
    // === WebSocket Operations ===
    HALResult<std::unique_ptr<IWebSocket>> create_websocket() override;
    
    // === DNS Operations ===
    HALResult<std::vector<std::string>> resolve_hostname(const std::string& hostname,
                                                        uint32_t timeout_ms = 5000) override;
    HALResult<uint32_t> resolve_hostname_async(const std::string& hostname,
                                              std::function<void(HALResult<std::vector<std::string>>)> callback,
                                              uint32_t timeout_ms = 5000) override;
    
    // === Platform-Specific Extensions ===
    HALResult<void> initialize_adhoc_mode(const AdHocParams& params) override;
    HALResult<void> shutdown_adhoc_mode() override;
    HALResult<std::vector<std::string>> scan_adhoc_networks(uint32_t timeout_ms = 10000) override;
    HALResult<void> join_adhoc_network(const std::string& network_name) override;
    HALResult<void> leave_adhoc_network() override;
    void* get_browser_interface() override;
    
    // === Connection Management ===
    std::vector<SocketHandle> get_active_connections() const override;
    HALResult<void> close_all_connections() override;
    HALResult<void> set_global_timeout(uint32_t timeout_ms) override;
    uint32_t get_global_timeout() const override;
    
    // === Bandwidth Management ===
    HALResult<void> enable_bandwidth_throttling(uint64_t max_bps) override;
    HALResult<void> disable_bandwidth_throttling() override;
    uint64_t get_current_bandwidth_usage() const override;
    HALResult<void> set_traffic_shaping(uint32_t burst_size, uint32_t token_rate) override;
    
    // === Statistics and Monitoring ===
    NetworkStats get_stats() const override;
    HALResult<void> reset_stats() override;
    HALResult<void> enable_event_logging(NetworkEventCallback callback) override;
    HALResult<void> disable_event_logging() override;
    HALResult<INetworkDiagnostics*> get_diagnostics() override;
    
    // === Security ===
    HALResult<void> add_trusted_certificate(const std::string& certificate_data) override;
    HALResult<void> remove_trusted_certificate(const std::string& certificate_fingerprint) override;
    HALResult<void> clear_trusted_certificates() override;
    HALResult<void> set_tls_config(const std::string& min_version,
                                  const std::string& max_version,
                                  const std::vector<std::string>& cipher_suites) override;
    
    // === Capability Queries ===
    bool supports_network_capability(NetworkCapability capability) const override;
    bool supports_protocol(NetworkProtocol protocol) const override;
    std::vector<NetworkProtocol> get_supported_protocols() const override;
    uint32_t get_max_concurrent_connections() const override;
    size_t get_max_message_size() const override;
    
    // === Platform-Specific Extensions ===
    void* get_extension_interface(std::string_view extension_name) override;
    void* get_modem_interface() override;
    void* get_wifi_interface() override;
    
    // === ICapabilityProvider implementation ===
    bool supports_capability(HALCapability capability) const override;
    uint32_t get_capability_mask() const override;
    std::vector<HALCapability> get_capabilities() const override;
    PerformanceTier get_performance_tier() const override;
    const PlatformInfo& get_platform_info() const override;
    bool has_fallback(HALCapability capability) const override;
    
    // === Mock-specific methods ===
    void set_network_available(bool available);
    void set_platform_capabilities(uint32_t capabilities);
    void simulate_connection_error();
    void simulate_bandwidth_limitation(uint64_t max_bps);

private:
    std::atomic<bool> initialized_;
    std::atomic<bool> network_available_;
    BandwidthProfile bandwidth_profile_;
    BandwidthConfig bandwidth_config_;
    uint32_t global_timeout_ms_;
    uint32_t platform_capabilities_;
    
    std::unique_ptr<MockHTTPClient> http_client_;
    std::unique_ptr<MockNetworkDiagnostics> diagnostics_;
    
    std::unordered_map<uint32_t, std::unique_ptr<MockNetworkSocket>> sockets_;
    std::atomic<uint32_t> next_socket_id_;
    
    NetworkStats stats_;
    NetworkEventCallback event_callback_;
    
    mutable std::mutex mutex_;
    
    void update_stats_connection_opened();
    void update_stats_connection_closed();
    void update_stats_data_sent(size_t bytes);
    void update_stats_data_received(size_t bytes);
};

} // namespace flight::hal::examples

/**
 * @file comprehensive_network_example.cpp
 * @brief Comprehensive Network Interface Usage Example
 * 
 * Demonstrates all major features of the Flight HAL Network Interface
 * including socket operations, HTTP clients, WebSocket connections,
 * bandwidth management, and platform-specific networking features.
 */

#include "../../include/flight/hal/interfaces/network.hpp"
#include "../drivers/mock_network_driver.hpp"
#include "../../include/flight/hal/core/hal_logging.hpp"
#include <iostream>
#include <chrono>
#include <thread>

using namespace flight::hal;
using namespace flight::hal::examples;
using namespace flight::hal::network;

/**
 * @brief Demonstrate basic TCP socket operations
 */
void demonstrate_tcp_socket(INetworkInterface* network) {
    std::cout << "\n=== TCP Socket Example ===\n";
    
    // Create TCP socket
    auto socket_result = network->create_tcp_socket();
    if (!socket_result) {
        std::cout << "Failed to create TCP socket: " << socket_result.error().message() << "\n";
        return;
    }
    
    auto socket = std::move(socket_result.value());
    std::cout << "Created TCP socket with handle ID: " << socket->get_handle().id << "\n";
    
    // Set up connection parameters for a web server
    NetworkAddress address("www.example.com", 80, NetworkProtocol::TCP);
    ConnectionParams params = make_connection_params(address);
    params.bandwidth_profile = BandwidthProfile::Broadband;
    params.enable_compression = true;
    
    // Attempt to connect
    auto connect_result = socket->connect(params);
    if (connect_result) {
        std::cout << "Successfully connected to " << address.to_string() << "\n";
        
        // Send HTTP request
        std::string request = "GET / HTTP/1.1\r\nHost: www.example.com\r\n\r\n";
        auto send_result = socket->send(request.c_str(), request.length());
        if (send_result) {
            std::cout << "Sent " << send_result.value() << " bytes\n";
        }
        
        // Try to receive response
        char buffer[1024];
        auto recv_result = socket->receive(buffer, sizeof(buffer) - 1);
        if (recv_result) {
            buffer[recv_result.value()] = '\0';
            std::cout << "Received " << recv_result.value() << " bytes:\n";
            std::cout << buffer << "\n";
        }
        
        socket->close();
    } else {
        std::cout << "Failed to connect: " << connect_result.error().message() << "\n";
    }
}

/**
 * @brief Demonstrate UDP socket operations
 */
void demonstrate_udp_socket(INetworkInterface* network) {
    std::cout << "\n=== UDP Socket Example ===\n";
    
    auto socket_result = network->create_udp_socket();
    if (!socket_result) {
        std::cout << "Failed to create UDP socket: " << socket_result.error().message() << "\n";
        return;
    }
    
    auto socket = std::move(socket_result.value());
    std::cout << "Created UDP socket with handle ID: " << socket->get_handle().id << "\n";
    
    // UDP doesn't require connection, but we can set up remote address
    NetworkAddress address("8.8.8.8", 53, NetworkProtocol::UDP); // DNS server
    ConnectionParams params = make_connection_params(address);
    
    // For demonstration, we'll just show the socket is ready
    std::cout << "UDP socket ready for communication with " << address.to_string() << "\n";
    
    socket->close();
}

/**
 * @brief Demonstrate HTTP client usage
 */
void demonstrate_http_client(INetworkInterface* network) {
    std::cout << "\n=== HTTP Client Example ===\n";
    
    auto client_result = network->get_http_client();
    if (!client_result) {
        std::cout << "Failed to get HTTP client: " << client_result.error().message() << "\n";
        return;
    }
    
    auto client = client_result.value();
    
    // Set up default headers
    HTTPHeaders headers;
    headers.add("User-Agent", "FlightHAL/1.0 Network Example");
    headers.add("Accept", "application/json, text/plain");
    client->set_default_headers(headers);
    
    // Set bandwidth profile for slower connections
    client->set_bandwidth_profile(BandwidthProfile::Broadband);
    
    // Perform GET request
    std::cout << "Performing GET request...\n";
    auto get_response = client->get("https://httpbin.org/get");
    if (get_response) {
        const auto& response = get_response.value();
        std::cout << "GET Response - Status: " << static_cast<int>(response.status_code) 
                  << " " << to_string(response.status_code) << "\n";
        std::cout << "Content-Length: " << response.content_length << "\n";
        std::cout << "Response Time: " << response.response_time.count() << "ms\n";
        std::cout << "Body preview: " << response.body.substr(0, 200) << "...\n";
    } else {
        std::cout << "GET request failed: " << get_response.error().message() << "\n";
    }
    
    // Perform POST request
    std::cout << "\nPerforming POST request...\n";
    std::string json_data = R"({"message": "Hello from FlightHAL", "timestamp": "2025-01-01"})";
    auto post_response = client->post("https://httpbin.org/post", json_data);
    if (post_response) {
        const auto& response = post_response.value();
        std::cout << "POST Response - Status: " << static_cast<int>(response.status_code) 
                  << " " << to_string(response.status_code) << "\n";
        std::cout << "Response Time: " << response.response_time.count() << "ms\n";
    } else {
        std::cout << "POST request failed: " << post_response.error().message() << "\n";
    }
}

/**
 * @brief Demonstrate WebSocket operations
 */
void demonstrate_websocket(INetworkInterface* network) {
    std::cout << "\n=== WebSocket Example ===\n";
    
    auto websocket_result = network->create_websocket();
    if (!websocket_result) {
        std::cout << "Failed to create WebSocket: " << websocket_result.error().message() << "\n";
        return;
    }
    
    auto websocket = std::move(websocket_result.value());
    
    // Set up message callback
    websocket->set_message_callback([](const WebSocketMessage& message) {
        std::cout << "Received WebSocket message of type: " << static_cast<int>(message.type) << "\n";
        if (message.type == WebSocketFrameType::Text) {
            std::string text(message.data.begin(), message.data.end());
            std::cout << "Message content: " << text << "\n";
        }
    });
    
    // Set up state callback
    websocket->set_state_callback([](ConnectionState state) {
        std::cout << "WebSocket state changed to: " << to_string(state) << "\n";
    });
    
    // Connect to echo service
    std::cout << "Connecting to WebSocket echo service...\n";
    auto connect_result = websocket->connect("wss://echo.websocket.org/");
    if (connect_result) {
        std::cout << "Connected to: " << websocket->get_url() << "\n";
        
        // Send test message
        websocket->send_text("Hello from FlightHAL Network Interface!");
        
        // Send ping
        websocket->ping();
        
        // Simulate some time for responses
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        websocket->close();
    } else {
        std::cout << "Failed to connect: " << connect_result.error().message() << "\n";
    }
}

/**
 * @brief Demonstrate DNS resolution
 */
void demonstrate_dns_resolution(INetworkInterface* network) {
    std::cout << "\n=== DNS Resolution Example ===\n";
    
    std::vector<std::string> hostnames = {
        "www.google.com",
        "www.github.com",
        "www.stackoverflow.com"
    };
    
    for (const auto& hostname : hostnames) {
        std::cout << "Resolving " << hostname << "...\n";
        auto resolve_result = network->resolve_hostname(hostname, 5000);
        if (resolve_result) {
            const auto& addresses = resolve_result.value();
            std::cout << "  Found " << addresses.size() << " address(es):\n";
            for (const auto& addr : addresses) {
                std::cout << "    " << addr << "\n";
            }
        } else {
            std::cout << "  Resolution failed: " << resolve_result.error().message() << "\n";
        }
    }
}

/**
 * @brief Demonstrate bandwidth management
 */
void demonstrate_bandwidth_management(INetworkInterface* network) {
    std::cout << "\n=== Bandwidth Management Example ===\n";
    
    // Test different bandwidth profiles
    std::vector<BandwidthProfile> profiles = {
        BandwidthProfile::Dialup,
        BandwidthProfile::Narrowband,
        BandwidthProfile::Broadband,
        BandwidthProfile::HighSpeed
    };
    
    for (auto profile : profiles) {
        std::cout << "Testing " << to_string(profile) << " profile:\n";
        
        auto result = network->set_bandwidth_profile(profile);
        if (result) {
            auto config = make_bandwidth_config(profile);
            std::cout << "  Max bandwidth: " << config.max_bandwidth_bps << " bps\n";
            std::cout << "  Timeout range: " << config.min_timeout_ms << "-" 
                      << config.max_timeout_ms << " ms\n";
            std::cout << "  Compression: " << (config.enable_compression ? "enabled" : "disabled") << "\n";
            
            // Test adaptive timeout calculation
            uint32_t base_timeout = 5000;
            uint32_t adaptive_timeout = calculate_adaptive_timeout(base_timeout, profile);
            std::cout << "  Adaptive timeout: " << adaptive_timeout << "ms (base: " << base_timeout << "ms)\n";
        }
        std::cout << "\n";
    }
    
    // Enable bandwidth throttling
    std::cout << "Enabling bandwidth throttling to 1 Mbps...\n";
    auto throttle_result = network->enable_bandwidth_throttling(1000000); // 1 Mbps
    if (throttle_result) {
        std::cout << "Bandwidth throttling enabled\n";
        std::cout << "Current usage: " << network->get_current_bandwidth_usage() << " bps\n";
        
        network->disable_bandwidth_throttling();
        std::cout << "Bandwidth throttling disabled\n";
    }
}

/**
 * @brief Demonstrate PSP ad-hoc networking (platform-specific)
 */
void demonstrate_adhoc_networking(INetworkInterface* network) {
    std::cout << "\n=== PSP Ad-Hoc Networking Example ===\n";
    
    // Check if ad-hoc is supported
    if (!network->supports_network_capability(NetworkCapability::AdHocMode)) {
        std::cout << "Ad-hoc networking not supported on this platform\n";
        return;
    }
    
    // Set up ad-hoc parameters
    AdHocParams params = make_adhoc_params("FlightHAL_Game", 6, 4);
    std::cout << "Initializing ad-hoc mode:\n";
    std::cout << "  SSID: " << params.ssid << "\n";
    std::cout << "  Channel: " << static_cast<int>(params.channel) << "\n";
    std::cout << "  Max players: " << params.max_players << "\n";
    
    auto init_result = network->initialize_adhoc_mode(params);
    if (init_result) {
        std::cout << "Ad-hoc mode initialized successfully\n";
        
        // Scan for other ad-hoc networks
        std::cout << "Scanning for ad-hoc networks...\n";
        auto scan_result = network->scan_adhoc_networks(5000);
        if (scan_result) {
            const auto& networks = scan_result.value();
            std::cout << "Found " << networks.size() << " ad-hoc network(s):\n";
            for (const auto& net : networks) {
                std::cout << "  " << net << "\n";
            }
        }
        
        network->shutdown_adhoc_mode();
        std::cout << "Ad-hoc mode shut down\n";
    } else {
        std::cout << "Failed to initialize ad-hoc mode: " << init_result.error().message() << "\n";
    }
}

/**
 * @brief Demonstrate network diagnostics
 */
void demonstrate_network_diagnostics(INetworkInterface* network) {
    std::cout << "\n=== Network Diagnostics Example ===\n";
    
    auto diag_result = network->get_diagnostics();
    if (!diag_result) {
        std::cout << "Network diagnostics not available\n";
        return;
    }
    
    auto diagnostics = diag_result.value();
    
    // Test ping
    std::cout << "Pinging google.com...\n";
    auto ping_result = diagnostics->ping("google.com", 5000);
    if (ping_result) {
        std::cout << "Ping time: " << ping_result.value() << "ms\n";
    } else {
        std::cout << "Ping failed\n";
    }
    
    // Test bandwidth
    std::cout << "Testing bandwidth...\n";
    auto bandwidth_result = diagnostics->test_bandwidth("http://speedtest.org", 5000);
    if (bandwidth_result) {
        uint64_t bps = bandwidth_result.value();
        std::cout << "Estimated bandwidth: " << (bps / 1000000) << " Mbps\n";
    } else {
        std::cout << "Bandwidth test failed\n";
    }
    
    // Get network interfaces
    std::cout << "Available network interfaces:\n";
    auto interfaces_result = diagnostics->get_network_interfaces();
    if (interfaces_result) {
        for (const auto& iface : interfaces_result.value()) {
            std::cout << "  " << iface << "\n";
        }
    }
}

/**
 * @brief Demonstrate network statistics monitoring
 */
void demonstrate_network_statistics(INetworkInterface* network) {
    std::cout << "\n=== Network Statistics Example ===\n";
    
    // Reset stats for clean slate
    network->reset_stats();
    
    // Perform some network operations to generate stats
    auto client_result = network->get_http_client();
    if (client_result) {
        auto client = client_result.value();
        client->get("https://httpbin.org/json");
    }
    
    // Get and display statistics
    auto stats = network->get_stats();
    std::cout << "Network Statistics:\n";
    std::cout << "  Bytes sent: " << stats.bytes_sent << "\n";
    std::cout << "  Bytes received: " << stats.bytes_received << "\n";
    std::cout << "  Connections opened: " << stats.connections_opened << "\n";
    std::cout << "  Connections closed: " << stats.connections_closed << "\n";
    std::cout << "  Active connections: " << stats.active_connections << "\n";
    std::cout << "  Connection errors: " << stats.connection_errors << "\n";
    std::cout << "  Average response time: " << stats.average_response_time_ms << "ms\n";
    std::cout << "  Average throughput: " << (stats.average_throughput_bps / 1000) << " Kbps\n";
}

/**
 * @brief Demonstrate capability detection
 */
void demonstrate_capability_detection(INetworkInterface* network) {
    std::cout << "\n=== Network Capability Detection ===\n";
    
    std::vector<NetworkCapability> capabilities_to_check = {
        NetworkCapability::TCP,
        NetworkCapability::UDP,
        NetworkCapability::HTTP,
        NetworkCapability::HTTPS,
        NetworkCapability::WebSocket,
        NetworkCapability::AdHocMode,
        NetworkCapability::BrowserAPI,
        NetworkCapability::Compression,
        NetworkCapability::AsyncOperations
    };
    
    std::cout << "Supported network capabilities:\n";
    for (auto cap : capabilities_to_check) {
        bool supported = network->supports_network_capability(cap);
        std::cout << "  " << to_string(cap) << ": " << (supported ? "YES" : "NO") << "\n";
    }
    
    std::cout << "\nSupported protocols:\n";
    auto protocols = network->get_supported_protocols();
    for (auto proto : protocols) {
        std::cout << "  " << to_string(proto) << "\n";
    }
    
    std::cout << "\nLimits:\n";
    std::cout << "  Max concurrent connections: " << network->get_max_concurrent_connections() << "\n";
    std::cout << "  Max message size: " << network->get_max_message_size() << " bytes\n";
    
    // Platform capability detection
    std::cout << "\nPlatform capabilities:\n";
    // Note: These methods may not be available in all network interface implementations
    // For a complete example, you would access these through a platform info service
    std::cout << "  Performance tier: Available through platform detection service\n";
    std::cout << "  Platform info: Available through platform detection service\n";
    std::cout << "  Use the platform detection interface for detailed hardware information\n";
}

/**
 * @brief Main demonstration function
 */
int main() {
    std::cout << "Flight HAL Network Interface - Comprehensive Example\n";
    std::cout << "===================================================\n";
    
    // Create and initialize mock network interface
    auto network = std::make_unique<MockNetworkInterface>();
    
    // Initialize the network subsystem
    auto init_result = network->initialize();
    if (!init_result) {
        std::cout << "Failed to initialize network interface: " << init_result.error().message() << "\n";
        return 1;
    }
    
    std::cout << "Network interface initialized successfully\n";
    std::cout << "Network available: " << (network->is_network_available() ? "YES" : "NO") << "\n";
    
    try {
        // Run all demonstrations
        demonstrate_capability_detection(network.get());
        demonstrate_tcp_socket(network.get());
        demonstrate_udp_socket(network.get());
        demonstrate_http_client(network.get());
        demonstrate_websocket(network.get());
        demonstrate_dns_resolution(network.get());
        demonstrate_bandwidth_management(network.get());
        demonstrate_adhoc_networking(network.get());
        demonstrate_network_diagnostics(network.get());
        demonstrate_network_statistics(network.get());
        
    } catch (const std::exception& e) {
        std::cout << "Exception during demonstration: " << e.what() << "\n";
    }
    
    // Clean shutdown
    network->shutdown();
    std::cout << "\nNetwork interface shut down successfully\n";
    
    std::cout << "\n=== Network Example Complete ===\n";
    return 0;
}

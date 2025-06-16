/**
 * @file cross_driver_messenger.cpp
 * @brief Flight HAL Cross-Driver Messaging System Implementation
 */

#include "../include/flight/hal/coordination/cross_driver_messenger.hpp"
#include "../include/flight/hal/core/hal_error.hpp"
#include <cstring>
#include <algorithm>

namespace flight::hal::coordination {

// Message implementation
Message::Message(const MessageHeader& header, std::unique_ptr<IMessagePayload> payload)
    : header_(header), payload_(std::move(payload)) {
    if (payload_) {
        header_.payload_size = static_cast<uint32_t>(payload_->serialize().size());
    }
}

Message::Message(const Message& other) 
    : header_(other.header_) {
    if (other.payload_) {
        payload_ = other.payload_->clone();
    }
}

Message& Message::operator=(const Message& other) {
    if (this != &other) {
        header_ = other.header_;
        if (other.payload_) {
            payload_ = other.payload_->clone();
        } else {
            payload_.reset();
        }
    }
    return *this;
}

void Message::set_payload(std::unique_ptr<IMessagePayload> payload) {
    payload_ = std::move(payload);
    if (payload_) {
        header_.payload_size = static_cast<uint32_t>(payload_->serialize().size());
    } else {
        header_.payload_size = 0;
    }
}

bool Message::is_expired() const noexcept {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - header_.timestamp);
    return elapsed >= header_.timeout;
}

std::chrono::milliseconds Message::get_age() const noexcept {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - header_.timestamp);
}

// ResourceRequestPayload implementation
ResourceRequestPayload::ResourceRequestPayload(Operation op, const ResourceHandle& handle)
    : operation(op), resource_handle(handle) {}

std::vector<uint8_t> ResourceRequestPayload::serialize() const {
    std::vector<uint8_t> data;
    
    // Serialize operation
    data.push_back(static_cast<uint8_t>(operation));
    
    // Serialize resource handle ID
    uint64_t id = resource_handle.id();
    data.resize(data.size() + sizeof(uint64_t));
    std::memcpy(data.data() + data.size() - sizeof(uint64_t), &id, sizeof(uint64_t));
    
    // Serialize resource handle name
    const std::string& name = resource_handle.name();
    uint32_t name_size = static_cast<uint32_t>(name.size());
    data.resize(data.size() + sizeof(uint32_t));
    std::memcpy(data.data() + data.size() - sizeof(uint32_t), &name_size, sizeof(uint32_t));
    
    size_t old_size = data.size();
    data.resize(old_size + name_size);
    std::memcpy(data.data() + old_size, name.data(), name_size);
    
    return data;
}

HALResult<void> ResourceRequestPayload::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(uint8_t) + sizeof(uint64_t) + sizeof(uint32_t)) {
        return HALResult<void>::error(HALError(HALErrorCategory::Validation, 1, 
            "Invalid ResourceRequestPayload data size", nullptr));
    }
    
    size_t offset = 0;
    
    // Deserialize operation
    operation = static_cast<Operation>(data[offset]);
    offset += sizeof(uint8_t);
    
    // Deserialize resource handle ID
    uint64_t id;
    std::memcpy(&id, data.data() + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    
    // Deserialize resource handle name
    uint32_t name_size;
    std::memcpy(&name_size, data.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    
    if (offset + name_size > data.size()) {
        return HALResult<void>::error(HALError(HALErrorCategory::Validation, 2, 
            "Invalid ResourceRequestPayload name size", nullptr));
    }
    
    std::string name(reinterpret_cast<const char*>(data.data() + offset), name_size);
    
    // Reconstruct resource handle (this is a simplified version)
    // In a real implementation, you'd need to look up the handle from the registry
    ResourceMetadata dummy_metadata;
    resource_handle = ResourceHandle(name, dummy_metadata);
    
    return HALResult<void>::success();
}

std::unique_ptr<IMessagePayload> ResourceRequestPayload::clone() const {
    auto clone = std::make_unique<ResourceRequestPayload>();
    clone->operation = operation;
    clone->resource_handle = resource_handle;
    clone->metadata = metadata;
    return std::move(clone);
}

// ResourceResponsePayload implementation
ResourceResponsePayload::ResourceResponsePayload(bool success, const std::string& message)
    : success(success), message(message) {}

std::vector<uint8_t> ResourceResponsePayload::serialize() const {
    std::vector<uint8_t> data;
    
    // Serialize success flag
    data.push_back(success ? 1 : 0);
    
    // Serialize message
    uint32_t message_size = static_cast<uint32_t>(message.size());
    data.resize(data.size() + sizeof(uint32_t));
    std::memcpy(data.data() + data.size() - sizeof(uint32_t), &message_size, sizeof(uint32_t));
    
    size_t old_size = data.size();
    data.resize(old_size + message_size);
    std::memcpy(data.data() + old_size, message.data(), message_size);
    
    return data;
}

HALResult<void> ResourceResponsePayload::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(uint8_t) + sizeof(uint32_t)) {
        return HALResult<void>::error(HALError(HALErrorCategory::Validation, 3, 
            "Invalid ResourceResponsePayload data size", nullptr));
    }
    
    size_t offset = 0;
    
    // Deserialize success flag
    success = (data[offset] != 0);
    offset += sizeof(uint8_t);
    
    // Deserialize message
    uint32_t message_size;
    std::memcpy(&message_size, data.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    
    if (offset + message_size > data.size()) {
        return HALResult<void>::error(HALError(HALErrorCategory::Validation, 4, 
            "Invalid ResourceResponsePayload message size", nullptr));
    }
    
    message = std::string(reinterpret_cast<const char*>(data.data() + offset), message_size);
    
    return HALResult<void>::success();
}

std::unique_ptr<IMessagePayload> ResourceResponsePayload::clone() const {
    auto clone = std::make_unique<ResourceResponsePayload>();
    clone->success = success;
    clone->message = message;
    clone->resource_handle = resource_handle;
    clone->metadata = metadata;
    return std::move(clone);
}

// PerformancePayload implementation
std::vector<uint8_t> PerformancePayload::serialize() const {
    std::vector<uint8_t> data;
    
    // Serialize driver_id
    uint32_t driver_id_size = static_cast<uint32_t>(driver_id.size());
    data.resize(sizeof(uint32_t));
    std::memcpy(data.data(), &driver_id_size, sizeof(uint32_t));
    
    size_t old_size = data.size();
    data.resize(old_size + driver_id_size);
    std::memcpy(data.data() + old_size, driver_id.data(), driver_id_size);
    
    // Serialize metrics count
    uint32_t metrics_count = static_cast<uint32_t>(metrics.size());
    old_size = data.size();
    data.resize(old_size + sizeof(uint32_t));
    std::memcpy(data.data() + old_size, &metrics_count, sizeof(uint32_t));
    
    // Serialize each metric
    for (const auto& metric : metrics) {
        // Serialize metric name
        uint32_t name_size = static_cast<uint32_t>(metric.name.size());
        old_size = data.size();
        data.resize(old_size + sizeof(uint32_t));
        std::memcpy(data.data() + old_size, &name_size, sizeof(uint32_t));
        
        old_size = data.size();
        data.resize(old_size + name_size);
        std::memcpy(data.data() + old_size, metric.name.data(), name_size);
        
        // Serialize metric value
        old_size = data.size();
        data.resize(old_size + sizeof(double));
        std::memcpy(data.data() + old_size, &metric.value, sizeof(double));
        
        // Serialize metric unit
        uint32_t unit_size = static_cast<uint32_t>(metric.unit.size());
        old_size = data.size();
        data.resize(old_size + sizeof(uint32_t));
        std::memcpy(data.data() + old_size, &unit_size, sizeof(uint32_t));
        
        old_size = data.size();
        data.resize(old_size + unit_size);
        std::memcpy(data.data() + old_size, metric.unit.data(), unit_size);
    }
    
    return data;
}

HALResult<void> PerformancePayload::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(uint32_t) * 2) {
        return HALResult<void>::error(HALError(HALErrorCategory::Validation, 5, 
            "Invalid PerformancePayload data size", nullptr));
    }
    
    size_t offset = 0;
    
    // Deserialize driver_id
    uint32_t driver_id_size;
    std::memcpy(&driver_id_size, data.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    
    if (offset + driver_id_size > data.size()) {
        return HALResult<void>::error(HALError(HALErrorCategory::Validation, 6, 
            "Invalid PerformancePayload driver_id size", nullptr));
    }
    
    driver_id = std::string(reinterpret_cast<const char*>(data.data() + offset), driver_id_size);
    offset += driver_id_size;
    
    // Deserialize metrics count
    uint32_t metrics_count;
    if (offset + sizeof(uint32_t) > data.size()) {
        return HALResult<void>::error(HALError(HALErrorCategory::Validation, 7, 
            "Invalid PerformancePayload metrics count", nullptr));
    }
    
    std::memcpy(&metrics_count, data.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    
    // Deserialize each metric
    metrics.clear();
    metrics.reserve(metrics_count);
    
    for (uint32_t i = 0; i < metrics_count; ++i) {
        Metric metric;
        
        // Deserialize metric name
        if (offset + sizeof(uint32_t) > data.size()) {
            return HALResult<void>::error(HALError(HALErrorCategory::Validation, 8, 
                "Invalid metric name size", nullptr));
        }
        
        uint32_t name_size;
        std::memcpy(&name_size, data.data() + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        
        if (offset + name_size > data.size()) {
            return HALResult<void>::error(HALError(HALErrorCategory::Validation, 9, 
                "Invalid metric name data", nullptr));
        }
        
        metric.name = std::string(reinterpret_cast<const char*>(data.data() + offset), name_size);
        offset += name_size;
        
        // Deserialize metric value
        if (offset + sizeof(double) > data.size()) {
            return HALResult<void>::error(HALError(HALErrorCategory::Validation, 10, 
                "Invalid metric value", nullptr));
        }
        
        std::memcpy(&metric.value, data.data() + offset, sizeof(double));
        offset += sizeof(double);
        
        // Deserialize metric unit
        if (offset + sizeof(uint32_t) > data.size()) {
            return HALResult<void>::error(HALError(HALErrorCategory::Validation, 11, 
                "Invalid metric unit size", nullptr));
        }
        
        uint32_t unit_size;
        std::memcpy(&unit_size, data.data() + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        
        if (offset + unit_size > data.size()) {
            return HALResult<void>::error(HALError(HALErrorCategory::Validation, 12, 
                "Invalid metric unit data", nullptr));
        }
        
        metric.unit = std::string(reinterpret_cast<const char*>(data.data() + offset), unit_size);
        offset += unit_size;
        
        metric.timestamp = std::chrono::steady_clock::now();
        metrics.push_back(std::move(metric));
    }
    
    return HALResult<void>::success();
}

std::unique_ptr<IMessagePayload> PerformancePayload::clone() const {
    auto clone = std::make_unique<PerformancePayload>();
    clone->metrics = metrics;
    clone->driver_id = driver_id;
    return std::move(clone);
}

// CrossDriverMessenger implementation
CrossDriverMessenger& CrossDriverMessenger::instance() {
    static CrossDriverMessenger instance;
    return instance;
}

HALResult<void> CrossDriverMessenger::initialize() {
    if (initialized_.load()) {
        return HALResult<void>::success();
    }
    
    shutdown_requested_.store(false);
    
    try {
        processing_thread_ = std::thread(&CrossDriverMessenger::message_processing_thread, this);
        initialized_.store(true);
        
        return HALResult<void>::success();
    } catch (const std::exception& e) {
        return HALResult<void>::error(HALError(HALErrorCategory::Internal, 1, 
            "Failed to initialize messenger", e.what()));
    }
}

HALResult<void> CrossDriverMessenger::shutdown() {
    if (!initialized_.load()) {
        return HALResult<void>::success();
    }
    
    shutdown_requested_.store(true);
    
    if (processing_thread_.joinable()) {
        processing_thread_.join();
    }
    
    // Clear handlers and pending requests
    {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        handlers_.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(pending_requests_mutex_);
        pending_requests_.clear();
    }
    
    initialized_.store(false);
    
    return HALResult<void>::success();
}

HALResult<void> CrossDriverMessenger::register_handler(const std::string& handler_id,
                                                      std::shared_ptr<IMessageHandler> handler) {
    if (!handler) {
        return HALResult<void>::error(HALError(HALErrorCategory::Configuration, 1, 
            "Handler cannot be null", handler_id.c_str()));
    }
    
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    
    if (handlers_.find(handler_id) != handlers_.end()) {
        return HALResult<void>::error(HALError(HALErrorCategory::Configuration, 2, 
            "Handler already registered", handler_id.c_str()));
    }
    
    handlers_[handler_id] = handler;
    
    return HALResult<void>::success();
}

HALResult<void> CrossDriverMessenger::unregister_handler(const std::string& handler_id) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    
    auto it = handlers_.find(handler_id);
    if (it == handlers_.end()) {
        return HALResult<void>::error(HALError(HALErrorCategory::Configuration, 3, 
            "Handler not found", handler_id.c_str()));
    }
    
    handlers_.erase(it);
    
    return HALResult<void>::success();
}

HALResult<void> CrossDriverMessenger::send_message(Message&& message) {
    if (!initialized_.load()) {
        return HALResult<void>::error(HALError(HALErrorCategory::Internal, 2, 
            "Messenger not initialized", nullptr));
    }
    
    // Generate message ID if not set
    if (message.header().id == 0) {
        message.header().id = generate_message_id();
    }
    
    // Try to enqueue message
    if (!message_queue_.try_enqueue(std::move(message))) {
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.messages_dropped++;
        }
        
        return HALResult<void>::error(HALError(HALErrorCategory::Resource, 1, 
            "Message queue full", nullptr));
    }
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.messages_sent++;
    }
    
    return HALResult<void>::success();
}

HALResult<Message> CrossDriverMessenger::send_request(Message&& request,
                                                     std::chrono::milliseconds timeout) {
    if (!initialized_.load()) {
        return HALResult<Message>::error(HALError(HALErrorCategory::Internal, 3, 
            "Messenger not initialized", nullptr));
    }
    
    // Set up request message
    request.header().type = MessageType::Request;
    request.header().id = generate_message_id();
    request.header().correlation_id = request.header().id;
    request.header().timeout = timeout;
    
    // Create promise for response
    std::promise<Message> response_promise;
    auto response_future = response_promise.get_future();
    
    {
        std::lock_guard<std::mutex> lock(pending_requests_mutex_);
        pending_requests_[request.header().id] = std::move(response_promise);
    }
    
    // Send request
    auto send_result = send_message(std::move(request));
    if (!send_result) {
        // Remove from pending requests on failure
        std::lock_guard<std::mutex> lock(pending_requests_mutex_);
        pending_requests_.erase(request.header().id);
        return HALResult<Message>::error(send_result.error());
    }
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.requests_sent++;
    }
    
    // Wait for response
    auto status = response_future.wait_for(timeout);
    
    if (status == std::future_status::timeout) {
        // Remove from pending requests on timeout
        std::lock_guard<std::mutex> lock(pending_requests_mutex_);
        pending_requests_.erase(request.header().id);
        
        {
            std::lock_guard<std::mutex> stats_lock(stats_mutex_);
            stats_.requests_timeout++;
        }
        
        return HALResult<Message>::error(HALError(HALErrorCategory::Resource, 2, 
            "Request timeout", nullptr));
    }
    
    try {
        Message response = response_future.get();
        return HALResult<Message>::success(std::move(response));
    } catch (const std::exception& e) {
        return HALResult<Message>::error(HALError(HALErrorCategory::Internal, 4, 
            "Failed to get response", e.what()));
    }
}

HALResult<void> CrossDriverMessenger::send_notification(const std::string& sender_id,
                                                       const std::string& recipient_id,
                                                       std::unique_ptr<IMessagePayload> payload,
                                                       MessagePriority priority) {
    MessageHeader header;
    header.type = MessageType::Notification;
    header.priority = priority;
    header.sender_id = sender_id;
    header.recipient_id = recipient_id;
    
    Message message(header, std::move(payload));
    return send_message(std::move(message));
}

HALResult<void> CrossDriverMessenger::broadcast_event(const std::string& sender_id,
                                                     std::unique_ptr<IMessagePayload> payload) {
    MessageHeader header;
    header.type = MessageType::Event;
    header.sender_id = sender_id;
    header.recipient_id = "*"; // Broadcast indicator
    
    Message message(header, std::move(payload));
    return send_message(std::move(message));
}

CrossDriverMessenger::MessageStats CrossDriverMessenger::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void CrossDriverMessenger::clear_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = MessageStats{};
}

void CrossDriverMessenger::message_processing_thread() {
    Message message;
    
    while (!shutdown_requested_.load()) {
        if (message_queue_.try_dequeue(message)) {
            process_message(message);
        } else {
            // Sleep briefly if no messages to process
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
}

void CrossDriverMessenger::process_message(const Message& message) {
    // Check if message has expired
    if (message.is_expired()) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.messages_expired++;
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.messages_received++;
    }
    
    // Handle response messages
    if (message.header().type == MessageType::Response) {
        std::lock_guard<std::mutex> lock(pending_requests_mutex_);
        auto it = pending_requests_.find(message.header().correlation_id);
        if (it != pending_requests_.end()) {
            it->second.set_value(message);
            pending_requests_.erase(it);
        }
        return;
    }
    
    // Route message to appropriate handler(s)
    std::vector<std::shared_ptr<IMessageHandler>> target_handlers;
    
    {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        
        if (message.header().recipient_id == "*") {
            // Broadcast message - send to all handlers that can handle this type
            for (const auto& [id, handler] : handlers_) {
                if (handler->can_handle(message.header().type)) {
                    target_handlers.push_back(handler);
                }
            }
        } else {
            // Direct message - send to specific handler
            auto it = handlers_.find(message.header().recipient_id);
            if (it != handlers_.end() && it->second->can_handle(message.header().type)) {
                target_handlers.push_back(it->second);
            }
        }
    }
    
    // Process message with each target handler
    for (auto& handler : target_handlers) {
        try {
            auto response_result = handler->handle_message(message);
            
            // If handler returned a response, send it back
            if (response_result && response_result.value()) {
                auto response = std::move(response_result.value());
                response->header().type = MessageType::Response;
                response->header().correlation_id = message.header().id;
                response->header().recipient_id = message.header().sender_id;
                response->header().sender_id = handler->get_handler_id();
                
                send_message(std::move(*response));
            }
        } catch (const std::exception&) {
            // Handler threw exception - ignore and continue
        }
    }
}

} // namespace flight::hal::coordination

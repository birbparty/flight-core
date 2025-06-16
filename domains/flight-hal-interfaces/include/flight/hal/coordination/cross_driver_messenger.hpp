/**
 * @file cross_driver_messenger.hpp
 * @brief Flight HAL Cross-Driver Messaging System
 * 
 * Provides lock-free message passing, event notifications, and request-response
 * protocols for safe cross-driver communication.
 */

#pragma once

#include "../core/hal_result.hpp"
#include "resource_handle.hpp"
#include <cstdint>
#include <string>
#include <memory>
#include <atomic>
#include <functional>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <unordered_map>
#include <thread>
#include <future>

namespace flight::hal::coordination {

/**
 * @brief Message types for cross-driver communication
 */
enum class MessageType : uint8_t {
    Request,        ///< Request message expecting a response
    Response,       ///< Response to a previous request
    Notification,   ///< One-way notification message
    Event,          ///< System event notification
    Performance,    ///< Performance telemetry data
    Resource        ///< Resource-related message
};

/**
 * @brief Message priority levels
 */
enum class MessagePriority : uint8_t {
    Low = 0,        ///< Background messages
    Normal = 1,     ///< Standard messages
    High = 2,       ///< Time-critical messages
    Critical = 3    ///< System-critical messages
};

/**
 * @brief Message header with routing and metadata
 */
struct MessageHeader {
    uint64_t id;                    ///< Unique message identifier
    uint64_t correlation_id;        ///< For request-response correlation
    MessageType type;               ///< Message type
    MessagePriority priority;       ///< Message priority
    std::string sender_id;          ///< Sender driver identifier
    std::string recipient_id;       ///< Recipient driver identifier
    std::chrono::steady_clock::time_point timestamp; ///< Message timestamp
    std::chrono::milliseconds timeout; ///< Message timeout
    uint32_t payload_size;          ///< Size of payload data
    
    MessageHeader()
        : id(0)
        , correlation_id(0)
        , type(MessageType::Notification)
        , priority(MessagePriority::Normal)
        , timestamp(std::chrono::steady_clock::now())
        , timeout(std::chrono::milliseconds(5000))
        , payload_size(0)
    {}
};

/**
 * @brief Generic message payload interface
 */
class IMessagePayload {
public:
    virtual ~IMessagePayload() = default;
    
    /**
     * @brief Get payload type identifier
     * @return String identifying the payload type
     */
    virtual std::string get_type() const = 0;
    
    /**
     * @brief Serialize payload to bytes
     * @return Vector of bytes representing the payload
     */
    virtual std::vector<uint8_t> serialize() const = 0;
    
    /**
     * @brief Deserialize payload from bytes
     * @param data Byte data to deserialize
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> deserialize(const std::vector<uint8_t>& data) = 0;
    
    /**
     * @brief Clone the payload
     * @return Unique pointer to cloned payload
     */
    virtual std::unique_ptr<IMessagePayload> clone() const = 0;
};

/**
 * @brief Complete message with header and payload
 */
class Message {
public:
    /**
     * @brief Create empty message
     */
    Message() = default;
    
    /**
     * @brief Create message with header and payload
     * @param header Message header
     * @param payload Message payload
     */
    Message(const MessageHeader& header, std::unique_ptr<IMessagePayload> payload);
    
    /**
     * @brief Copy constructor
     */
    Message(const Message& other);
    
    /**
     * @brief Move constructor
     */
    Message(Message&& other) noexcept = default;
    
    /**
     * @brief Assignment operator
     */
    Message& operator=(const Message& other);
    
    /**
     * @brief Move assignment operator
     */
    Message& operator=(Message&& other) noexcept = default;
    
    /**
     * @brief Get message header
     * @return Const reference to message header
     */
    const MessageHeader& header() const noexcept { return header_; }
    
    /**
     * @brief Get mutable message header
     * @return Reference to message header
     */
    MessageHeader& header() noexcept { return header_; }
    
    /**
     * @brief Get message payload
     * @return Const pointer to payload (may be null)
     */
    const IMessagePayload* payload() const noexcept { return payload_.get(); }
    
    /**
     * @brief Get typed payload
     * @tparam PayloadType Type to cast payload to
     * @return Pointer to typed payload or nullptr if wrong type
     */
    template<typename PayloadType>
    const PayloadType* payload_as() const noexcept;
    
    /**
     * @brief Set message payload
     * @param payload New payload
     */
    void set_payload(std::unique_ptr<IMessagePayload> payload);
    
    /**
     * @brief Check if message has expired based on timeout
     * @return true if message has expired
     */
    bool is_expired() const noexcept;
    
    /**
     * @brief Get message age
     * @return Duration since message was created
     */
    std::chrono::milliseconds get_age() const noexcept;

private:
    MessageHeader header_;
    std::unique_ptr<IMessagePayload> payload_;
};

/**
 * @brief Resource operation request payload
 */
class ResourceRequestPayload : public IMessagePayload {
public:
    enum class Operation {
        Acquire,    ///< Request resource acquisition
        Release,    ///< Request resource release
        Query,      ///< Query resource status
        Update      ///< Update resource metadata
    };
    
    ResourceRequestPayload() = default;
    ResourceRequestPayload(Operation op, const ResourceHandle& handle);
    
    // IMessagePayload implementation
    std::string get_type() const override { return "ResourceRequest"; }
    std::vector<uint8_t> serialize() const override;
    HALResult<void> deserialize(const std::vector<uint8_t>& data) override;
    std::unique_ptr<IMessagePayload> clone() const override;
    
    Operation operation = Operation::Query;
    ResourceHandle resource_handle;
    ResourceMetadata metadata; // For update operations
};

/**
 * @brief Resource operation response payload
 */
class ResourceResponsePayload : public IMessagePayload {
public:
    ResourceResponsePayload() = default;
    ResourceResponsePayload(bool success, const std::string& message = "");
    
    // IMessagePayload implementation
    std::string get_type() const override { return "ResourceResponse"; }
    std::vector<uint8_t> serialize() const override;
    HALResult<void> deserialize(const std::vector<uint8_t>& data) override;
    std::unique_ptr<IMessagePayload> clone() const override;
    
    bool success = true;
    std::string message;
    ResourceHandle resource_handle;
    ResourceMetadata metadata;
};

/**
 * @brief Performance telemetry payload
 */
class PerformancePayload : public IMessagePayload {
public:
    struct Metric {
        std::string name;
        double value;
        std::string unit;
        std::chrono::steady_clock::time_point timestamp;
    };
    
    PerformancePayload() = default;
    
    // IMessagePayload implementation
    std::string get_type() const override { return "Performance"; }
    std::vector<uint8_t> serialize() const override;
    HALResult<void> deserialize(const std::vector<uint8_t>& data) override;
    std::unique_ptr<IMessagePayload> clone() const override;
    
    std::vector<Metric> metrics;
    std::string driver_id;
};

/**
 * @brief Message handler interface
 */
class IMessageHandler {
public:
    virtual ~IMessageHandler() = default;
    
    /**
     * @brief Handle incoming message
     * @param message The message to handle
     * @return HALResult containing optional response message
     */
    virtual HALResult<std::unique_ptr<Message>> handle_message(const Message& message) = 0;
    
    /**
     * @brief Get handler identifier
     * @return Handler ID string
     */
    virtual std::string get_handler_id() const = 0;
    
    /**
     * @brief Check if handler can process message type
     * @param type Message type to check
     * @return true if handler can process this type
     */
    virtual bool can_handle(MessageType type) const = 0;
};

/**
 * @brief Lock-free message queue for single producer, single consumer
 */
template<size_t Capacity>
class SPSCMessageQueue {
public:
    SPSCMessageQueue() : head_(0), tail_(0) {}
    
    /**
     * @brief Try to enqueue a message (non-blocking)
     * @param message Message to enqueue
     * @return true if message was enqueued successfully
     */
    bool try_enqueue(Message&& message) noexcept;
    
    /**
     * @brief Try to dequeue a message (non-blocking)
     * @param message Output parameter for dequeued message
     * @return true if message was dequeued successfully
     */
    bool try_dequeue(Message& message) noexcept;
    
    /**
     * @brief Check if queue is empty
     * @return true if queue is empty
     */
    bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Get approximate queue size
     * @return Number of messages in queue (approximate)
     */
    size_t size() const noexcept {
        auto head = head_.load(std::memory_order_acquire);
        auto tail = tail_.load(std::memory_order_acquire);
        return (tail >= head) ? (tail - head) : (Capacity - head + tail);
    }

private:
    alignas(64) std::atomic<size_t> head_;  // Consumer index
    alignas(64) std::atomic<size_t> tail_;  // Producer index
    
    struct alignas(64) Slot {
        std::atomic<bool> valid{false};
        Message message;
    };
    
    Slot slots_[Capacity];
};

/**
 * @brief Cross-driver messaging system
 */
class CrossDriverMessenger {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to global messenger
     */
    static CrossDriverMessenger& instance();
    
    /**
     * @brief Initialize messenger system
     * @return HALResult indicating success or failure
     */
    HALResult<void> initialize();
    
    /**
     * @brief Shutdown messenger system
     * @return HALResult indicating success or failure
     */
    HALResult<void> shutdown();
    
    /**
     * @brief Register a message handler
     * @param handler_id Unique handler identifier
     * @param handler Message handler implementation
     * @return HALResult indicating success or failure
     */
    HALResult<void> register_handler(const std::string& handler_id,
                                    std::shared_ptr<IMessageHandler> handler);
    
    /**
     * @brief Unregister a message handler
     * @param handler_id Handler identifier to remove
     * @return HALResult indicating success or failure
     */
    HALResult<void> unregister_handler(const std::string& handler_id);
    
    /**
     * @brief Send a message asynchronously
     * @param message Message to send
     * @return HALResult indicating success or failure
     */
    HALResult<void> send_message(Message&& message);
    
    /**
     * @brief Send a request and wait for response
     * @param request Request message to send
     * @param timeout Maximum time to wait for response
     * @return HALResult containing response message or error
     */
    HALResult<Message> send_request(Message&& request,
                                   std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));
    
    /**
     * @brief Send a notification (fire-and-forget)
     * @param sender_id Sender identifier
     * @param recipient_id Recipient identifier
     * @param payload Message payload
     * @param priority Message priority
     * @return HALResult indicating success or failure
     */
    HALResult<void> send_notification(const std::string& sender_id,
                                     const std::string& recipient_id,
                                     std::unique_ptr<IMessagePayload> payload,
                                     MessagePriority priority = MessagePriority::Normal);
    
    /**
     * @brief Broadcast event to all registered handlers
     * @param sender_id Sender identifier
     * @param payload Event payload
     * @return HALResult indicating success or failure
     */
    HALResult<void> broadcast_event(const std::string& sender_id,
                                   std::unique_ptr<IMessagePayload> payload);
    
    /**
     * @brief Get message statistics
     * @return Structure containing messaging statistics
     */
    struct MessageStats {
        uint64_t messages_sent;
        uint64_t messages_received;
        uint64_t messages_dropped;
        uint64_t messages_expired;
        uint64_t requests_sent;
        uint64_t requests_timeout;
        double average_response_time_ms;
    };
    
    MessageStats get_stats() const;
    
    /**
     * @brief Clear message statistics
     */
    void clear_stats();

private:
    CrossDriverMessenger() = default;
    ~CrossDriverMessenger() = default;
    CrossDriverMessenger(const CrossDriverMessenger&) = delete;
    CrossDriverMessenger& operator=(const CrossDriverMessenger&) = delete;
    
    /**
     * @brief Message processing thread function
     */
    void message_processing_thread();
    
    /**
     * @brief Process a single message
     * @param message Message to process
     */
    void process_message(const Message& message);
    
    /**
     * @brief Generate unique message ID
     * @return Unique message identifier
     */
    uint64_t generate_message_id() noexcept {
        return next_message_id_.fetch_add(1, std::memory_order_relaxed);
    }
    
    // Threading and synchronization
    std::atomic<bool> initialized_{false};
    std::atomic<bool> shutdown_requested_{false};
    std::thread processing_thread_;
    
    // Message handling
    mutable std::mutex handlers_mutex_;
    std::unordered_map<std::string, std::shared_ptr<IMessageHandler>> handlers_;
    
    // Message queues
    static constexpr size_t QUEUE_CAPACITY = 1024;
    SPSCMessageQueue<QUEUE_CAPACITY> message_queue_;
    
    // Pending requests for request-response pattern
    mutable std::mutex pending_requests_mutex_;
    std::unordered_map<uint64_t, std::promise<Message>> pending_requests_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    MessageStats stats_{};
    
    // ID generation
    std::atomic<uint64_t> next_message_id_{1};
};

// Template implementation
template<typename PayloadType>
const PayloadType* Message::payload_as() const noexcept {
    if (!payload_) return nullptr;
    return dynamic_cast<const PayloadType*>(payload_.get());
}

template<size_t Capacity>
bool SPSCMessageQueue<Capacity>::try_enqueue(Message&& message) noexcept {
    auto tail = tail_.load(std::memory_order_relaxed);
    auto next_tail = (tail + 1) % Capacity;
    
    if (next_tail == head_.load(std::memory_order_acquire)) {
        return false; // Queue is full
    }
    
    auto& slot = slots_[tail];
    slot.message = std::move(message);
    slot.valid.store(true, std::memory_order_release);
    tail_.store(next_tail, std::memory_order_release);
    
    return true;
}

template<size_t Capacity>
bool SPSCMessageQueue<Capacity>::try_dequeue(Message& message) noexcept {
    auto head = head_.load(std::memory_order_relaxed);
    
    if (head == tail_.load(std::memory_order_acquire)) {
        return false; // Queue is empty
    }
    
    auto& slot = slots_[head];
    if (!slot.valid.load(std::memory_order_acquire)) {
        return false; // Slot not ready
    }
    
    message = std::move(slot.message);
    slot.valid.store(false, std::memory_order_release);
    head_.store((head + 1) % Capacity, std::memory_order_release);
    
    return true;
}

} // namespace flight::hal::coordination

/**
 * @file memory.cpp
 * @brief Flight HAL Memory Interface Implementation
 * 
 * Implementation of memory management utilities and string conversion functions.
 */

#include "../../include/flight/hal/interfaces/memory.hpp"
#include <stdexcept>

namespace flight::hal::memory {

const char* to_string(MemoryType type) {
    switch (type) {
        case MemoryType::System:    return "System";
        case MemoryType::Video:     return "Video";
        case MemoryType::Audio:     return "Audio";
        case MemoryType::Network:   return "Network";
        case MemoryType::Shared:    return "Shared";
        case MemoryType::Temporary: return "Temporary";
        case MemoryType::Stack:     return "Stack";
        case MemoryType::Pool:      return "Pool";
        default:                    return "Unknown";
    }
}

const char* to_string(AllocatorType type) {
    switch (type) {
        case AllocatorType::Linear:   return "Linear";
        case AllocatorType::Stack:    return "Stack";
        case AllocatorType::Pool:     return "Pool";
        case AllocatorType::Buddy:    return "Buddy";
        case AllocatorType::FreeList: return "FreeList";
        case AllocatorType::System:   return "System";
        default:                      return "Unknown";
    }
}

const char* to_string(MemoryPressureLevel level) {
    switch (level) {
        case MemoryPressureLevel::None:     return "None";
        case MemoryPressureLevel::Low:      return "Low";
        case MemoryPressureLevel::Medium:   return "Medium";
        case MemoryPressureLevel::High:     return "High";
        case MemoryPressureLevel::Critical: return "Critical";
        default:                            return "Unknown";
    }
}

AllocationRequest make_allocation_request(size_t size, 
                                         MemoryType type,
                                         MemoryAlignment alignment,
                                         MemoryFlags flags) {
    AllocationRequest request{};
    request.size = size;
    request.alignment = alignment;
    request.flags = flags;
    request.preferred_type = type;
    request.preferred_allocator = AllocatorType::System;
    request.timeout_ms = 0; // No timeout by default
    request.allow_fallback = true;
    return request;
}

} // namespace flight::hal::memory

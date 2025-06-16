/**
 * @file hal_capabilities.cpp
 * @brief Flight HAL Capability Detection Implementation
 */

#include "flight/hal/core/hal_capabilities.hpp"
#include <algorithm>

namespace flight::hal::capabilities {

const char* to_string(HALCapability capability) {
    switch (capability) {
        case HALCapability::Threading: return "Threading";
        case HALCapability::AtomicOperations: return "AtomicOperations";
        case HALCapability::DMA: return "DMA";
        case HALCapability::VirtualMemory: return "VirtualMemory";
        case HALCapability::MemoryProtection: return "MemoryProtection";
        case HALCapability::Hardware3D: return "Hardware3D";
        case HALCapability::Hardware2D: return "Hardware2D";
        case HALCapability::VertexShaders: return "VertexShaders";
        case HALCapability::FragmentShaders: return "FragmentShaders";
        case HALCapability::ComputeShaders: return "ComputeShaders";
        case HALCapability::HardwareAudio: return "HardwareAudio";
        case HALCapability::MultiChannelAudio: return "MultiChannelAudio";
        case HALCapability::AudioEffects: return "AudioEffects";
        case HALCapability::MultiTouch: return "MultiTouch";
        case HALCapability::Accelerometer: return "Accelerometer";
        case HALCapability::Gamepad: return "Gamepad";
        case HALCapability::Networking: return "Networking";
        case HALCapability::WiFi: return "WiFi";
        case HALCapability::Bluetooth: return "Bluetooth";
        case HALCapability::HighPrecisionTimer: return "HighPrecisionTimer";
        case HALCapability::RealTimeClock: return "RealTimeClock";
        case HALCapability::PersistentStorage: return "PersistentStorage";
        case HALCapability::AsyncIO: return "AsyncIO";
        case HALCapability::PowerManagement: return "PowerManagement";
        case HALCapability::SandboxRestrictions: return "SandboxRestrictions";
        default: return "Unknown";
    }
}

const char* to_string(PerformanceTier tier) {
    switch (tier) {
        case PerformanceTier::Minimal: return "Minimal";
        case PerformanceTier::Limited: return "Limited";
        case PerformanceTier::Standard: return "Standard";
        case PerformanceTier::High: return "High";
        default: return "Unknown";
    }
}

uint32_t make_capability_mask(const std::vector<HALCapability>& caps) {
    uint32_t mask = 0;
    for (auto cap : caps) {
        mask |= static_cast<uint32_t>(cap);
    }
    return mask;
}

std::vector<HALCapability> extract_capabilities(uint32_t mask) {
    std::vector<HALCapability> capabilities;
    
    // Check each possible capability bit
    for (uint32_t bit = 0; bit < 32; ++bit) {
        uint32_t capability_value = 1u << bit;
        if ((mask & capability_value) != 0) {
            capabilities.push_back(static_cast<HALCapability>(capability_value));
        }
    }
    
    return capabilities;
}

} // namespace flight::hal::capabilities

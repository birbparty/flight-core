#ifndef FLIGHT_HAL_DRIVER_HPP
#define FLIGHT_HAL_DRIVER_HPP

#include "result.hpp"
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>
#include <string>

namespace flight
{
    namespace hal
    {

        // Driver types in the system
        enum class DriverType : uint32_t
        {
            Video,
            Audio,
            Input,
            File,
            Thread,
            Time,
            Memory,
            Network,
            Storage
        };

        // Convert driver type to string for debugging
        inline const char *driver_type_to_string(DriverType type) noexcept
        {
            switch (type)
            {
            case DriverType::Video:
                return "Video";
            case DriverType::Audio:
                return "Audio";
            case DriverType::Input:
                return "Input";
            case DriverType::File:
                return "File";
            case DriverType::Thread:
                return "Thread";
            case DriverType::Time:
                return "Time";
            case DriverType::Memory:
                return "Memory";
            case DriverType::Network:
                return "Network";
            case DriverType::Storage:
                return "Storage";
            default:
                return "Unknown";
            }
        }

        // Base driver interface (RetroArch pattern)
        class Driver
        {
        public:
            virtual ~Driver() = default;

            // Driver identification
            virtual const char *name() const = 0;
            virtual const char *description() const = 0;
            virtual DriverType type() const = 0;

            // Driver lifecycle
            virtual Result<void> initialize() = 0;
            virtual void shutdown() = 0;
            virtual bool is_initialized() const = 0;

            // Driver capabilities
            virtual uint32_t version() const { return 1; }
            virtual bool is_hardware_accelerated() const { return false; }
            virtual bool is_thread_safe() const { return false; }
        };

        // Driver factory function type
        using DriverFactory = std::function<std::unique_ptr<Driver>()>;

        // Driver info for registration
        struct DriverInfo
        {
            const char *name;
            const char *description;
            DriverType type;
            DriverFactory factory;
            uint32_t priority; // Higher priority drivers are preferred
        };

        // Driver registry (singleton pattern like RetroArch)
        class DriverRegistry
        {
        private:
            // Map from driver type to list of registered drivers
            std::unordered_map<DriverType, std::vector<DriverInfo>> drivers_;

            // Private constructor for singleton
            DriverRegistry() = default;

        public:
            // Get singleton instance
            static DriverRegistry &instance()
            {
                static DriverRegistry registry;
                return registry;
            }

            // Delete copy/move
            DriverRegistry(const DriverRegistry &) = delete;
            DriverRegistry &operator=(const DriverRegistry &) = delete;
            DriverRegistry(DriverRegistry &&) = delete;
            DriverRegistry &operator=(DriverRegistry &&) = delete;

            // Register a driver
            void register_driver(const DriverInfo &info)
            {
                drivers_[info.type].push_back(info);
                // Sort by priority (highest first)
                auto &vec = drivers_[info.type];
                std::sort(vec.begin(), vec.end(),
                          [](const DriverInfo &a, const DriverInfo &b)
                          {
                              return a.priority > b.priority;
                          });
            }

            // Get available drivers for a type
            std::vector<const char *> get_available_drivers(DriverType type) const
            {
                std::vector<const char *> names;
                auto it = drivers_.find(type);
                if (it != drivers_.end())
                {
                    for (const auto &info : it->second)
                    {
                        names.push_back(info.name);
                    }
                }
                return names;
            }

            // Create a specific driver
            std::unique_ptr<Driver> create_driver(DriverType type, const char *name)
            {
                auto it = drivers_.find(type);
                if (it != drivers_.end())
                {
                    for (const auto &info : it->second)
                    {
                        if (std::strcmp(info.name, name) == 0)
                        {
                            return info.factory();
                        }
                    }
                }
                return nullptr;
            }

            // Create the best available driver for a type
            std::unique_ptr<Driver> create_default_driver(DriverType type)
            {
                auto it = drivers_.find(type);
                if (it != drivers_.end() && !it->second.empty())
                {
                    // Return highest priority driver
                    return it->second.front().factory();
                }
                return nullptr;
            }

            // Get driver info
            const DriverInfo *get_driver_info(DriverType type, const char *name) const
            {
                auto it = drivers_.find(type);
                if (it != drivers_.end())
                {
                    for (const auto &info : it->second)
                    {
                        if (std::strcmp(info.name, name) == 0)
                        {
                            return &info;
                        }
                    }
                }
                return nullptr;
            }

            // Clear all registrations (mainly for testing)
            void clear()
            {
                drivers_.clear();
            }
        };

        // Helper macro for driver registration (inspired by RetroArch)
#define REGISTER_DRIVER(type, name, desc, factory_func, priority) \
    namespace                                                     \
    {                                                             \
        struct DriverRegistrar_##name                             \
        {                                                         \
            DriverRegistrar_##name()                              \
            {                                                     \
                DriverRegistry::instance().register_driver(       \
                    {#name, desc, type, factory_func, priority}); \
            }                                                     \
        } driver_registrar_##name##_instance;                     \
    }

        // Driver capability flags (for querying what a driver supports)
        namespace driver_caps
        {
            constexpr uint32_t NONE = 0;
            constexpr uint32_t HARDWARE_ACCELERATED = (1 << 0);
            constexpr uint32_t THREAD_SAFE = (1 << 1);
            constexpr uint32_t HOT_PLUGGABLE = (1 << 2);
            constexpr uint32_t LOW_LATENCY = (1 << 3);
            constexpr uint32_t HIGH_PRECISION = (1 << 4);
        }

    } // namespace hal
} // namespace flight

#endif // FLIGHT_HAL_DRIVER_HPP

/**
 * @file capability_cache.hpp
 * @brief Capability Caching and Storage System
 * 
 * Provides efficient caching mechanisms for capability detection results
 * with TTL-based invalidation, persistent storage, and background updates
 * to optimize capability queries and reduce detection overhead.
 */

#pragma once

#include "hal_capabilities.hpp"
#include "hal_result.hpp"
#include "hardware_detection.hpp"
#include "performance_benchmarking.hpp"
#include <chrono>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>

namespace flight::hal {

/**
 * @brief Cache entry metadata
 */
struct CacheEntryMetadata {
    std::chrono::steady_clock::time_point created_at;  ///< When entry was created
    std::chrono::steady_clock::time_point last_accessed; ///< Last access timestamp
    std::chrono::steady_clock::time_point expires_at;    ///< Expiration timestamp
    uint32_t access_count;                              ///< Number of times accessed
    bool is_dirty;                                      ///< Whether entry needs refresh
    std::string source;                                 ///< Detection source identifier
    uint64_t hash;                                      ///< Content hash for validation
};

/**
 * @brief Cached capability information
 */
struct CachedCapabilityInfo {
    uint32_t capability_mask;           ///< Cached capability bitmask
    PerformanceTier performance_tier;   ///< Cached performance tier
    PlatformInfo platform_info;        ///< Cached platform information
    CacheEntryMetadata metadata;       ///< Cache entry metadata
};

/**
 * @brief Cached hardware features
 */
struct CachedHardwareFeatures {
    CPUFeatures cpu_features;          ///< Cached CPU features
    GPUFeatures gpu_features;          ///< Cached GPU features
    MemoryFeatures memory_features;    ///< Cached memory features
    IOFeatures io_features;            ///< Cached I/O features
    CacheEntryMetadata metadata;       ///< Cache entry metadata
};

/**
 * @brief Cached benchmark results
 */
struct CachedBenchmarkResults {
    SystemBenchmarkResults results;    ///< Cached benchmark results
    CacheEntryMetadata metadata;       ///< Cache entry metadata
};

/**
 * @brief Cache configuration
 */
struct CacheConfig {
    std::chrono::minutes default_ttl;         ///< Default time-to-live
    std::chrono::minutes max_ttl;             ///< Maximum time-to-live
    std::chrono::minutes min_ttl;             ///< Minimum time-to-live
    size_t max_entries;                       ///< Maximum cache entries
    size_t max_memory_usage;                  ///< Maximum memory usage in bytes
    bool enable_persistent_storage;           ///< Enable persistent storage
    bool enable_background_refresh;           ///< Enable background refresh
    std::chrono::minutes background_interval; ///< Background refresh interval
    double cache_hit_ratio_threshold;         ///< Minimum acceptable hit ratio
    bool enable_compression;                  ///< Enable cache compression
};

/**
 * @brief Cache statistics
 */
struct CacheStatistics {
    uint64_t total_requests;           ///< Total cache requests
    uint64_t cache_hits;               ///< Cache hits
    uint64_t cache_misses;             ///< Cache misses
    uint64_t evictions;                ///< Cache evictions
    uint64_t background_refreshes;     ///< Background refreshes performed
    size_t current_entries;            ///< Current number of entries
    size_t memory_usage_bytes;         ///< Current memory usage
    double hit_ratio;                  ///< Cache hit ratio
    std::chrono::milliseconds avg_access_time; ///< Average access time
    std::chrono::steady_clock::time_point last_reset; ///< Last statistics reset
};

/**
 * @brief Cache invalidation reason
 */
enum class InvalidationReason {
    Expired,              ///< Entry expired based on TTL
    Manual,               ///< Manually invalidated
    HardwareChange,       ///< Hardware configuration changed
    DriverUpdate,         ///< Driver was updated
    ConfigChange,         ///< Configuration changed
    MemoryPressure,       ///< Evicted due to memory pressure
    BackgroundRefresh     ///< Refreshed by background process
};

/**
 * @brief Capability cache interface
 */
class ICapabilityCache {
public:
    virtual ~ICapabilityCache() = default;
    
    /**
     * @brief Get cached capability information
     * @param platform_id Platform identifier
     * @return Cached capability info or cache miss
     */
    virtual HALResult<CachedCapabilityInfo> get_capability_info(const std::string& platform_id) = 0;
    
    /**
     * @brief Store capability information in cache
     * @param platform_id Platform identifier
     * @param info Capability information to cache
     * @param ttl Time-to-live for this entry
     * @return Success result
     */
    virtual HALResult<void> store_capability_info(
        const std::string& platform_id,
        const CachedCapabilityInfo& info,
        std::chrono::minutes ttl
    ) = 0;
    
    /**
     * @brief Get cached hardware features
     * @param hardware_id Hardware identifier
     * @return Cached hardware features or cache miss
     */
    virtual HALResult<CachedHardwareFeatures> get_hardware_features(const std::string& hardware_id) = 0;
    
    /**
     * @brief Store hardware features in cache
     * @param hardware_id Hardware identifier
     * @param features Hardware features to cache
     * @param ttl Time-to-live for this entry
     * @return Success result
     */
    virtual HALResult<void> store_hardware_features(
        const std::string& hardware_id,
        const CachedHardwareFeatures& features,
        std::chrono::minutes ttl
    ) = 0;
    
    /**
     * @brief Get cached benchmark results
     * @param benchmark_id Benchmark identifier
     * @return Cached benchmark results or cache miss
     */
    virtual HALResult<CachedBenchmarkResults> get_benchmark_results(const std::string& benchmark_id) = 0;
    
    /**
     * @brief Store benchmark results in cache
     * @param benchmark_id Benchmark identifier
     * @param results Benchmark results to cache
     * @param ttl Time-to-live for this entry
     * @return Success result
     */
    virtual HALResult<void> store_benchmark_results(
        const std::string& benchmark_id,
        const CachedBenchmarkResults& results,
        std::chrono::minutes ttl
    ) = 0;
    
    /**
     * @brief Invalidate cache entry
     * @param key Cache key to invalidate
     * @param reason Reason for invalidation
     * @return Success result
     */
    virtual HALResult<void> invalidate_entry(const std::string& key, InvalidationReason reason) = 0;
    
    /**
     * @brief Invalidate all cache entries
     * @param reason Reason for invalidation
     * @return Success result
     */
    virtual HALResult<void> invalidate_all(InvalidationReason reason) = 0;
    
    /**
     * @brief Perform cache maintenance
     * @return Success result
     */
    virtual HALResult<void> perform_maintenance() = 0;
    
    /**
     * @brief Get cache statistics
     * @return Current cache statistics
     */
    virtual CacheStatistics get_statistics() const = 0;
    
    /**
     * @brief Reset cache statistics
     * @return Success result
     */
    virtual HALResult<void> reset_statistics() = 0;
    
    /**
     * @brief Configure cache settings
     * @param config New cache configuration
     * @return Success result
     */
    virtual HALResult<void> configure(const CacheConfig& config) = 0;
    
    /**
     * @brief Get current cache configuration
     * @return Current cache configuration
     */
    virtual CacheConfig get_configuration() const = 0;
};

/**
 * @brief Persistent cache storage interface
 */
class IPersistentCacheStorage {
public:
    virtual ~IPersistentCacheStorage() = default;
    
    /**
     * @brief Load cache from persistent storage
     * @return Success result
     */
    virtual HALResult<void> load_cache() = 0;
    
    /**
     * @brief Save cache to persistent storage
     * @return Success result
     */
    virtual HALResult<void> save_cache() = 0;
    
    /**
     * @brief Check if persistent storage is available
     * @return true if storage is available
     */
    virtual bool is_storage_available() const = 0;
    
    /**
     * @brief Get storage file path
     * @return Path to cache storage file
     */
    virtual std::string get_storage_path() const = 0;
    
    /**
     * @brief Set storage file path
     * @param path New storage path
     * @return Success result
     */
    virtual HALResult<void> set_storage_path(const std::string& path) = 0;
    
    /**
     * @brief Clear persistent storage
     * @return Success result
     */
    virtual HALResult<void> clear_storage() = 0;
    
    /**
     * @brief Get storage size in bytes
     * @return Storage size
     */
    virtual size_t get_storage_size() const = 0;
};

/**
 * @brief Background cache updater interface
 */
class IBackgroundCacheUpdater {
public:
    virtual ~IBackgroundCacheUpdater() = default;
    
    /**
     * @brief Start background cache updates
     * @param interval Update interval
     * @return Success result
     */
    virtual HALResult<void> start_background_updates(std::chrono::minutes interval) = 0;
    
    /**
     * @brief Stop background cache updates
     * @return Success result
     */
    virtual HALResult<void> stop_background_updates() = 0;
    
    /**
     * @brief Check if background updates are running
     * @return true if background updates are active
     */
    virtual bool is_updating() const = 0;
    
    /**
     * @brief Trigger immediate background update
     * @return Success result
     */
    virtual HALResult<void> trigger_immediate_update() = 0;
    
    /**
     * @brief Register update completion callback
     * @param callback Function to call when update completes
     * @return Success result
     */
    virtual HALResult<void> register_update_callback(
        std::function<void(const std::vector<std::string>&)> callback
    ) = 0;
    
    /**
     * @brief Set update priorities
     * @param priorities Map of cache keys to update priorities
     * @return Success result
     */
    virtual HALResult<void> set_update_priorities(
        const std::unordered_map<std::string, uint32_t>& priorities
    ) = 0;
    
    /**
     * @brief Get update progress
     * @return Progress percentage (0.0 to 1.0)
     */
    virtual double get_update_progress() const = 0;
};

/**
 * @brief Cache event listener interface
 */
class ICacheEventListener {
public:
    virtual ~ICacheEventListener() = default;
    
    /**
     * @brief Called when cache entry is accessed
     * @param key Cache key
     * @param hit Whether it was a cache hit
     */
    virtual void on_cache_access(const std::string& key, bool hit) = 0;
    
    /**
     * @brief Called when cache entry is invalidated
     * @param key Cache key
     * @param reason Invalidation reason
     */
    virtual void on_cache_invalidation(const std::string& key, InvalidationReason reason) = 0;
    
    /**
     * @brief Called when cache maintenance is performed
     * @param entries_cleaned Number of entries cleaned
     * @param memory_freed Memory freed in bytes
     */
    virtual void on_cache_maintenance(size_t entries_cleaned, size_t memory_freed) = 0;
    
    /**
     * @brief Called when cache configuration changes
     * @param old_config Previous configuration
     * @param new_config New configuration
     */
    virtual void on_cache_reconfiguration(const CacheConfig& old_config, const CacheConfig& new_config) = 0;
};

/**
 * @brief Create memory-based capability cache
 * @param config Cache configuration
 * @return Memory cache instance
 */
std::unique_ptr<ICapabilityCache> create_memory_cache(const CacheConfig& config);

/**
 * @brief Create persistent capability cache
 * @param config Cache configuration
 * @param storage_path Path for persistent storage
 * @return Persistent cache instance
 */
std::unique_ptr<ICapabilityCache> create_persistent_cache(
    const CacheConfig& config, 
    const std::string& storage_path
);

/**
 * @brief Create hybrid capability cache (memory + persistent)
 * @param config Cache configuration
 * @param storage_path Path for persistent storage
 * @return Hybrid cache instance
 */
std::unique_ptr<ICapabilityCache> create_hybrid_cache(
    const CacheConfig& config,
    const std::string& storage_path
);

/**
 * @brief Create background cache updater
 * @param cache Cache to update
 * @param detector Hardware detector for updates
 * @param benchmark Benchmark system for updates
 * @return Background updater instance
 */
std::unique_ptr<IBackgroundCacheUpdater> create_background_cache_updater(
    std::shared_ptr<ICapabilityCache> cache,
    std::shared_ptr<IHardwareDetector> detector,
    std::shared_ptr<IPerformanceBenchmark> benchmark
);

/**
 * @brief Create default cache configuration
 * @param cache_type Cache type ("memory", "persistent", "hybrid")
 * @return Default cache configuration
 */
CacheConfig create_default_cache_config(const std::string& cache_type = "hybrid");

/**
 * @brief Create cache configuration for specific use case
 * @param use_case Use case ("gaming", "embedded", "server", "development")
 * @return Optimized cache configuration
 */
CacheConfig create_cache_config_for_use_case(const std::string& use_case);

/**
 * @brief Generate cache key for platform capabilities
 * @param platform_info Platform information
 * @return Generated cache key
 */
std::string generate_platform_cache_key(const PlatformInfo& platform_info);

/**
 * @brief Generate cache key for hardware features
 * @param cpu_features CPU features
 * @param gpu_features GPU features
 * @param memory_features Memory features
 * @param io_features I/O features
 * @return Generated cache key
 */
std::string generate_hardware_cache_key(
    const CPUFeatures& cpu_features,
    const GPUFeatures& gpu_features,
    const MemoryFeatures& memory_features,
    const IOFeatures& io_features
);

/**
 * @brief Generate cache key for benchmark results
 * @param benchmark_config Benchmark configuration
 * @param hardware_id Hardware identifier
 * @return Generated cache key
 */
std::string generate_benchmark_cache_key(
    const BenchmarkConfig& benchmark_config,
    const std::string& hardware_id
);

} // namespace flight::hal

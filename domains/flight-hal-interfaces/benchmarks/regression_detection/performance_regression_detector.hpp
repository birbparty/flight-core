/**
 * @file performance_regression_detector.hpp
 * @brief Advanced Performance Regression Detection System
 * 
 * Statistical analysis and automated regression detection for HAL performance
 * benchmarks with CI/CD integration and alerting capabilities.
 */

#pragma once

#include <vector>
#include <string>
#include <chrono>
#include <memory>
#include <functional>
#include <unordered_map>
#include <cstdint>

namespace flight::hal::benchmarking {

/**
 * @brief Performance measurement data point
 */
struct PerformanceMeasurement {
    std::string benchmark_name;         ///< Name of the benchmark
    std::string platform;               ///< Platform identifier  
    std::string configuration;          ///< Build configuration (Debug/Release)
    std::string git_commit;             ///< Git commit hash
    std::chrono::system_clock::time_point timestamp; ///< Measurement timestamp
    
    // Performance metrics
    double mean_time_ns;                ///< Mean execution time in nanoseconds
    double median_time_ns;              ///< Median execution time
    double min_time_ns;                 ///< Minimum execution time
    double max_time_ns;                 ///< Maximum execution time
    double std_dev_ns;                  ///< Standard deviation
    double cpu_time_ns;                 ///< CPU time
    double real_time_ns;                ///< Real (wall clock) time
    
    // Throughput metrics
    double items_per_second;            ///< Items processed per second
    double bytes_per_second;            ///< Bytes processed per second
    
    // Additional metrics
    size_t iterations;                  ///< Number of benchmark iterations
    size_t memory_usage_bytes;          ///< Peak memory usage
    double cpu_utilization;             ///< CPU utilization percentage
    
    // Quality metrics
    double coefficient_of_variation;    ///< CV = std_dev / mean
    bool is_reliable;                   ///< Whether measurement is reliable
};

/**
 * @brief Regression detection result
 */
struct RegressionResult {
    enum class Type {
        NoRegression,       ///< No significant performance change
        Improvement,        ///< Performance improved significantly
        Regression,         ///< Performance regressed significantly
        Inconclusive        ///< Results are inconclusive
    };
    
    Type type;                          ///< Type of result
    std::string benchmark_name;         ///< Affected benchmark
    double confidence_level;            ///< Statistical confidence (0.0-1.0)
    double effect_size;                 ///< Magnitude of change (Cohen's d)
    double percentage_change;           ///< Percentage change from baseline
    
    // Statistical test results
    double p_value;                     ///< Statistical significance p-value
    double t_statistic;                 ///< T-test statistic
    
    // Baseline comparison
    PerformanceMeasurement baseline;    ///< Baseline measurement
    PerformanceMeasurement current;     ///< Current measurement
    
    std::string description;            ///< Human-readable description
    std::string recommendation;         ///< Recommended action
};

/**
 * @brief Regression detection configuration
 */
struct RegressionDetectionConfig {
    // Statistical thresholds
    double significance_level = 0.05;          ///< Alpha for statistical tests
    double regression_threshold = 0.05;        ///< 5% performance degradation threshold
    double improvement_threshold = 0.05;       ///< 5% performance improvement threshold
    double effect_size_threshold = 0.5;        ///< Cohen's d threshold for meaningful effect
    
    // Reliability filters
    double max_coefficient_variation = 0.15;   ///< Maximum acceptable CV
    size_t min_iterations = 10;                ///< Minimum benchmark iterations
    size_t min_baseline_samples = 5;           ///< Minimum baseline samples needed
    
    // Trend analysis
    size_t trend_window_size = 10;             ///< Number of measurements for trend analysis
    bool enable_trend_detection = true;        ///< Enable trend-based regression detection
    
    // Platform-specific settings
    std::unordered_map<std::string, double> platform_variance_tolerance; ///< Per-platform variance tolerance
    
    // Alert settings
    bool enable_email_alerts = false;          ///< Send email alerts
    bool enable_slack_notifications = false;   ///< Send Slack notifications
    std::vector<std::string> alert_recipients; ///< Alert recipient list
};

/**
 * @brief Performance baseline storage interface
 */
class IPerformanceBaseline {
public:
    virtual ~IPerformanceBaseline() = default;
    
    /**
     * @brief Store performance measurement
     * @param measurement Performance measurement to store
     * @return Success indicator
     */
    virtual bool store_measurement(const PerformanceMeasurement& measurement) = 0;
    
    /**
     * @brief Get baseline measurements for a benchmark
     * @param benchmark_name Name of the benchmark
     * @param platform Platform identifier
     * @param max_samples Maximum number of samples to return
     * @return Vector of baseline measurements
     */
    virtual std::vector<PerformanceMeasurement> get_baseline_measurements(
        const std::string& benchmark_name,
        const std::string& platform,
        size_t max_samples = 50
    ) = 0;
    
    /**
     * @brief Get latest measurement for a benchmark
     * @param benchmark_name Name of the benchmark
     * @param platform Platform identifier
     * @return Latest measurement if available
     */
    virtual std::optional<PerformanceMeasurement> get_latest_measurement(
        const std::string& benchmark_name,
        const std::string& platform
    ) = 0;
    
    /**
     * @brief Clean old measurements beyond retention period
     * @param retention_days Number of days to retain
     * @return Number of measurements cleaned
     */
    virtual size_t cleanup_old_measurements(int retention_days = 90) = 0;
};

/**
 * @brief Statistical analysis utilities
 */
class StatisticalAnalyzer {
public:
    /**
     * @brief Perform Welch's t-test for unequal variances
     * @param sample1 First sample data
     * @param sample2 Second sample data
     * @return P-value and t-statistic
     */
    static std::pair<double, double> welch_t_test(
        const std::vector<double>& sample1,
        const std::vector<double>& sample2
    );
    
    /**
     * @brief Calculate Cohen's d effect size
     * @param mean1 Mean of first sample
     * @param mean2 Mean of second sample
     * @param std1 Standard deviation of first sample
     * @param std2 Standard deviation of second sample
     * @param n1 Size of first sample
     * @param n2 Size of second sample
     * @return Cohen's d value
     */
    static double cohens_d(double mean1, double mean2, double std1, double std2, size_t n1, size_t n2);
    
    /**
     * @brief Perform Mann-Whitney U test (non-parametric)
     * @param sample1 First sample data
     * @param sample2 Second sample data
     * @return P-value
     */
    static double mann_whitney_u_test(
        const std::vector<double>& sample1,
        const std::vector<double>& sample2
    );
    
    /**
     * @brief Detect trend using linear regression
     * @param measurements Time series of measurements
     * @return Slope and R-squared values
     */
    static std::pair<double, double> detect_trend(
        const std::vector<PerformanceMeasurement>& measurements
    );
    
    /**
     * @brief Calculate percentage change with confidence interval
     * @param baseline_mean Baseline mean value
     * @param current_mean Current mean value
     * @param baseline_std Baseline standard deviation
     * @param current_std Current standard deviation
     * @param baseline_n Baseline sample size
     * @param current_n Current sample size
     * @param confidence_level Confidence level (default 0.95)
     * @return Percentage change and confidence interval
     */
    static std::tuple<double, double, double> percentage_change_with_ci(
        double baseline_mean, double current_mean,
        double baseline_std, double current_std,
        size_t baseline_n, size_t current_n,
        double confidence_level = 0.95
    );
};

/**
 * @brief Main performance regression detector
 */
class PerformanceRegressionDetector {
public:
    /**
     * @brief Constructor
     * @param baseline_storage Baseline storage implementation
     * @param config Detection configuration
     */
    PerformanceRegressionDetector(
        std::unique_ptr<IPerformanceBaseline> baseline_storage,
        const RegressionDetectionConfig& config = RegressionDetectionConfig{}
    );
    
    /**
     * @brief Add new performance measurement
     * @param measurement New measurement to analyze
     * @return Regression analysis result
     */
    RegressionResult analyze_measurement(const PerformanceMeasurement& measurement);
    
    /**
     * @brief Analyze multiple measurements (batch mode)
     * @param measurements Vector of measurements to analyze
     * @return Vector of regression results
     */
    std::vector<RegressionResult> analyze_measurements(
        const std::vector<PerformanceMeasurement>& measurements
    );
    
    /**
     * @brief Update detection configuration
     * @param config New configuration
     */
    void update_config(const RegressionDetectionConfig& config);
    
    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    const RegressionDetectionConfig& get_config() const;
    
    /**
     * @brief Generate performance report
     * @param platform Platform to report on
     * @param days_back Number of days to include in report
     * @return Performance report in JSON format
     */
    std::string generate_performance_report(
        const std::string& platform,
        int days_back = 7
    );
    
    /**
     * @brief Set alert callback for regression notifications
     * @param callback Function to call when regression is detected
     */
    void set_alert_callback(std::function<void(const RegressionResult&)> callback);

private:
    std::unique_ptr<IPerformanceBaseline> baseline_storage_;
    RegressionDetectionConfig config_;
    std::function<void(const RegressionResult&)> alert_callback_;
    
    /**
     * @brief Perform statistical comparison between current and baseline
     * @param current Current measurement
     * @param baseline_measurements Historical baseline measurements
     * @return Regression result
     */
    RegressionResult compare_with_baseline(
        const PerformanceMeasurement& current,
        const std::vector<PerformanceMeasurement>& baseline_measurements
    );
    
    /**
     * @brief Check measurement reliability
     * @param measurement Measurement to check
     * @return true if measurement is reliable
     */
    bool is_measurement_reliable(const PerformanceMeasurement& measurement) const;
    
    /**
     * @brief Extract performance values for statistical analysis
     * @param measurements Vector of measurements
     * @return Vector of performance values (mean times)
     */
    std::vector<double> extract_performance_values(
        const std::vector<PerformanceMeasurement>& measurements
    ) const;
    
    /**
     * @brief Generate human-readable description of regression
     * @param result Regression result to describe
     * @return Description string
     */
    std::string generate_description(const RegressionResult& result) const;
    
    /**
     * @brief Generate recommendation based on regression result
     * @param result Regression result
     * @return Recommendation string
     */
    std::string generate_recommendation(const RegressionResult& result) const;
};

/**
 * @brief File-based baseline storage implementation
 */
class FileBasedBaseline : public IPerformanceBaseline {
public:
    /**
     * @brief Constructor
     * @param storage_directory Directory to store baseline files
     */
    explicit FileBasedBaseline(const std::string& storage_directory);
    
    bool store_measurement(const PerformanceMeasurement& measurement) override;
    
    std::vector<PerformanceMeasurement> get_baseline_measurements(
        const std::string& benchmark_name,
        const std::string& platform,
        size_t max_samples = 50
    ) override;
    
    std::optional<PerformanceMeasurement> get_latest_measurement(
        const std::string& benchmark_name,
        const std::string& platform
    ) override;
    
    size_t cleanup_old_measurements(int retention_days = 90) override;

private:
    std::string storage_directory_;
    
    /**
     * @brief Get filename for benchmark measurements
     * @param benchmark_name Name of benchmark
     * @param platform Platform identifier
     * @return Filename for storage
     */
    std::string get_filename(const std::string& benchmark_name, const std::string& platform) const;
};

/**
 * @brief Utility functions for regression detection
 */
namespace regression_utils {

/**
 * @brief Parse Google Benchmark JSON output
 * @param json_content JSON content from benchmark
 * @param platform Platform identifier
 * @param git_commit Git commit hash
 * @return Vector of performance measurements
 */
std::vector<PerformanceMeasurement> parse_benchmark_json(
    const std::string& json_content,
    const std::string& platform,
    const std::string& git_commit = ""
);

/**
 * @brief Create default regression detection config for platform
 * @param platform Platform identifier
 * @return Platform-optimized configuration
 */
RegressionDetectionConfig create_platform_config(const std::string& platform);

/**
 * @brief Format regression result for CI/CD output
 * @param result Regression result to format
 * @return Formatted string for CI logs
 */
std::string format_for_ci(const RegressionResult& result);

/**
 * @brief Send Slack notification for regression
 * @param webhook_url Slack webhook URL
 * @param result Regression result to notify about
 * @return Success indicator
 */
bool send_slack_notification(const std::string& webhook_url, const RegressionResult& result);

/**
 * @brief Generate performance badge for README
 * @param measurements Recent measurements
 * @param platform Platform identifier
 * @return Badge URL or markdown
 */
std::string generate_performance_badge(
    const std::vector<PerformanceMeasurement>& measurements,
    const std::string& platform
);

} // namespace regression_utils

} // namespace flight::hal::benchmarking

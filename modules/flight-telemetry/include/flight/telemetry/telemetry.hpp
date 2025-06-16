#ifndef FLIGHT_TELEMETRY_TELEMETRY_HPP
#define FLIGHT_TELEMETRY_TELEMETRY_HPP

#include <string>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <variant>

namespace flight
{
    namespace telemetry
    {

        // Forward declarations
        class Span;
        class Meter;
        class Tracer;
        class Exporter;

        // Span attributes
        using AttributeValue = std::variant<
            bool,
            int64_t,
            double,
            std::string,
            std::vector<bool>,
            std::vector<int64_t>,
            std::vector<double>,
            std::vector<std::string>>;
        using Attributes = std::unordered_map<std::string, AttributeValue>;

        // Span status
        enum class StatusCode
        {
            Unset,
            Ok,
            Error
        };

        // Span interface
        class Span
        {
        public:
            virtual ~Span() = default;

            virtual void set_attribute(const std::string &key, const AttributeValue &value) = 0;
            virtual void set_status(StatusCode code, const std::string &description = "") = 0;
            virtual void add_event(const std::string &name, const Attributes &attributes = {}) = 0;
            virtual void end() = 0;
        };

        // Metric types
        enum class MetricKind
        {
            Counter,
            Histogram,
            Gauge
        };

        // Tracer interface
        class Tracer
        {
        public:
            virtual ~Tracer() = default;

            virtual std::unique_ptr<Span> start_span(
                const std::string &name,
                const Attributes &attributes = {}) = 0;
        };

        // Meter interface
        class Meter
        {
        public:
            virtual ~Meter() = default;

            virtual void record_counter(
                const std::string &name,
                int64_t value,
                const Attributes &attributes = {}) = 0;

            virtual void record_histogram(
                const std::string &name,
                double value,
                const Attributes &attributes = {}) = 0;

            virtual void record_gauge(
                const std::string &name,
                double value,
                const Attributes &attributes = {}) = 0;
        };

        // Telemetry provider
        class TelemetryProvider
        {
        public:
            static TelemetryProvider &instance();

            std::shared_ptr<Tracer> get_tracer(const std::string &name);
            std::shared_ptr<Meter> get_meter(const std::string &name);

            void add_exporter(std::unique_ptr<Exporter> exporter);
            void shutdown();

        private:
            TelemetryProvider() = default;
        };

        // This module provides:
        // - OpenTelemetry integration
        // - Spans for distributed tracing
        // - Metrics (counters, histograms, gauges)
        // - Export interfaces for various backends
        // - Minimal overhead when disabled

    } // namespace telemetry
} // namespace flight

#endif // FLIGHT_TELEMETRY_TELEMETRY_HPP

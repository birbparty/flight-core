# Flight Telemetry Module

## Purpose and Scope

The `flight-telemetry` module provides observability for Flight through OpenTelemetry-compatible instrumentation. It enables distributed tracing, metrics collection, and performance monitoring with minimal overhead, allowing developers to understand runtime behavior in production environments.

## Public API Overview

### Tracing
- `Tracer`: Create and manage distributed traces
- `Span`: Individual units of work within a trace
- `StatusCode`: Span completion status (Unset, Ok, Error)

### Metrics
- `Meter`: Record application metrics
- `MetricKind`: Counter, Histogram, and Gauge metrics
- Automatic performance metrics for WASM execution

### Telemetry Management
- `TelemetryProvider`: Singleton for telemetry configuration
- `Exporter`: Interface for telemetry data export
- `Attributes`: Key-value pairs for contextual information

## Usage Examples

```cpp
#include <flight/telemetry/telemetry.hpp>

using namespace flight::telemetry;

// Get tracer instance
auto& provider = TelemetryProvider::instance();
auto tracer = provider.get_tracer("flight");

// Create a span
auto span = tracer->start_span("wasm_execution", {
    {"module", "example.wasm"},
    {"function", "main"}
});

// Record metrics
auto meter = provider.get_meter("flight");
meter->record_counter("wasm_calls", 1, {{"status", "success"}});
meter->record_histogram("execution_time_ms", 42.5);

// Set span status and end
span->set_status(StatusCode::Ok);
span->end();
```

## Dependencies

This module can instrument any other Flight module but has no required dependencies. It integrates with standard OpenTelemetry exporters.

## Build Instructions

This module is built as part of the Flight project:

```bash
# From project root
cmake -B build
cmake --build build --target flight-telemetry
```

To run tests:
```bash
cmake --build build --target flight-telemetry-tests
ctest -R flight-telemetry
```

## Architecture Notes

- Zero-cost when disabled at compile time
- Lock-free metrics collection using atomics
- Efficient span creation with object pooling
- Pluggable exporters for various backends
- Automatic context propagation

## Exporter Support

- **Console**: Debug output to stdout/stderr
- **OTLP**: OpenTelemetry Protocol (gRPC/HTTP)
- **Jaeger**: Direct Jaeger protocol support
- **Prometheus**: Metrics exposition endpoint
- **Custom**: Implement the Exporter interface

## Performance Considerations

- Compile-time flag to completely disable telemetry
- Sampling strategies to reduce overhead
- Batch export to minimize I/O impact
- Pre-allocated span and attribute pools
- Minimal overhead: <5% in instrumented code paths

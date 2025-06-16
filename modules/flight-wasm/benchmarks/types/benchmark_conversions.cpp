// =============================================================================
// Flight WASM Benchmarks - Type Conversion System Performance
// =============================================================================

#include <benchmark/benchmark.h>
#include <flight/wasm/types/conversions.hpp>
#include <random>

using namespace flight::wasm;
using namespace flight::wasm::conversions;

// =============================================================================
// Integer Conversion Benchmarks
// =============================================================================

static void BM_I32_Wrap_I64(benchmark::State& state) {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<int64_t> dis;
    
    std::vector<Value> test_values;
    test_values.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        test_values.push_back(Value::from_i64(dis(gen)));
    }
    
    size_t index = 0;
    for (auto _ : state) {
        auto result = TypeConverter::i32_wrap_i64(test_values[index % test_values.size()]);
        benchmark::DoNotOptimize(result);
        ++index;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_I32_Wrap_I64);

static void BM_I64_Extend_I32_S(benchmark::State& state) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int32_t> dis;
    
    std::vector<Value> test_values;
    test_values.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        test_values.push_back(Value::from_i32(dis(gen)));
    }
    
    size_t index = 0;
    for (auto _ : state) {
        auto result = TypeConverter::i64_extend_i32_s(test_values[index % test_values.size()]);
        benchmark::DoNotOptimize(result);
        ++index;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_I64_Extend_I32_S);

static void BM_I64_Extend_I32_U(benchmark::State& state) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int32_t> dis;
    
    std::vector<Value> test_values;
    test_values.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        test_values.push_back(Value::from_i32(dis(gen)));
    }
    
    size_t index = 0;
    for (auto _ : state) {
        auto result = TypeConverter::i64_extend_i32_u(test_values[index % test_values.size()]);
        benchmark::DoNotOptimize(result);
        ++index;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_I64_Extend_I32_U);

// =============================================================================
// Floating-Point Conversion Benchmarks
// =============================================================================

static void BM_F32_Demote_F64(benchmark::State& state) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(-1e6, 1e6);
    
    std::vector<Value> test_values;
    test_values.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        test_values.push_back(Value::from_f64(dis(gen)));
    }
    
    size_t index = 0;
    for (auto _ : state) {
        auto result = TypeConverter::f32_demote_f64(test_values[index % test_values.size()]);
        benchmark::DoNotOptimize(result);
        ++index;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_F32_Demote_F64);

static void BM_F64_Promote_F32(benchmark::State& state) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-1e6f, 1e6f);
    
    std::vector<Value> test_values;
    test_values.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        test_values.push_back(Value::from_f32(dis(gen)));
    }
    
    size_t index = 0;
    for (auto _ : state) {
        auto result = TypeConverter::f64_promote_f32(test_values[index % test_values.size()]);
        benchmark::DoNotOptimize(result);
        ++index;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_F64_Promote_F32);

// =============================================================================
// Truncation Operation Benchmarks
// =============================================================================

static void BM_I32_Trunc_F32_S(benchmark::State& state) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-1e6f, 1e6f);
    
    std::vector<Value> test_values;
    test_values.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        test_values.push_back(Value::from_f32(dis(gen)));
    }
    
    size_t index = 0;
    for (auto _ : state) {
        auto result = TypeConverter::i32_trunc_f32_s(test_values[index % test_values.size()]);
        benchmark::DoNotOptimize(result);
        ++index;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_I32_Trunc_F32_S);

static void BM_I32_Trunc_F64_S(benchmark::State& state) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(-1e6, 1e6);
    
    std::vector<Value> test_values;
    test_values.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        test_values.push_back(Value::from_f64(dis(gen)));
    }
    
    size_t index = 0;
    for (auto _ : state) {
        auto result = TypeConverter::i32_trunc_f64_s(test_values[index % test_values.size()]);
        benchmark::DoNotOptimize(result);
        ++index;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_I32_Trunc_F64_S);

static void BM_I64_Trunc_F64_S(benchmark::State& state) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(-1e15, 1e15);
    
    std::vector<Value> test_values;
    test_values.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        test_values.push_back(Value::from_f64(dis(gen)));
    }
    
    size_t index = 0;
    for (auto _ : state) {
        auto result = TypeConverter::i64_trunc_f64_s(test_values[index % test_values.size()]);
        benchmark::DoNotOptimize(result);
        ++index;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_I64_Trunc_F64_S);

// =============================================================================
// Integer to Float Conversion Benchmarks
// =============================================================================

static void BM_F32_Convert_I32_S(benchmark::State& state) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int32_t> dis;
    
    std::vector<Value> test_values;
    test_values.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        test_values.push_back(Value::from_i32(dis(gen)));
    }
    
    size_t index = 0;
    for (auto _ : state) {
        auto result = TypeConverter::f32_convert_i32_s(test_values[index % test_values.size()]);
        benchmark::DoNotOptimize(result);
        ++index;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_F32_Convert_I32_S);

static void BM_F64_Convert_I64_S(benchmark::State& state) {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<int64_t> dis;
    
    std::vector<Value> test_values;
    test_values.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        test_values.push_back(Value::from_i64(dis(gen)));
    }
    
    size_t index = 0;
    for (auto _ : state) {
        auto result = TypeConverter::f64_convert_i64_s(test_values[index % test_values.size()]);
        benchmark::DoNotOptimize(result);
        ++index;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_F64_Convert_I64_S);

// =============================================================================
// Reinterpretation Operation Benchmarks
// =============================================================================

static void BM_I32_Reinterpret_F32(benchmark::State& state) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-1e6f, 1e6f);
    
    std::vector<Value> test_values;
    test_values.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        test_values.push_back(Value::from_f32(dis(gen)));
    }
    
    size_t index = 0;
    for (auto _ : state) {
        auto result = TypeConverter::i32_reinterpret_f32(test_values[index % test_values.size()]);
        benchmark::DoNotOptimize(result);
        ++index;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_I32_Reinterpret_F32);

static void BM_F32_Reinterpret_I32(benchmark::State& state) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int32_t> dis;
    
    std::vector<Value> test_values;
    test_values.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        test_values.push_back(Value::from_i32(dis(gen)));
    }
    
    size_t index = 0;
    for (auto _ : state) {
        auto result = TypeConverter::f32_reinterpret_i32(test_values[index % test_values.size()]);
        benchmark::DoNotOptimize(result);
        ++index;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_F32_Reinterpret_I32);

static void BM_I64_Reinterpret_F64(benchmark::State& state) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(-1e15, 1e15);
    
    std::vector<Value> test_values;
    test_values.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        test_values.push_back(Value::from_f64(dis(gen)));
    }
    
    size_t index = 0;
    for (auto _ : state) {
        auto result = TypeConverter::i64_reinterpret_f64(test_values[index % test_values.size()]);
        benchmark::DoNotOptimize(result);
        ++index;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_I64_Reinterpret_F64);

// =============================================================================
// Generic Conversion Interface Benchmarks
// =============================================================================

static void BM_Generic_Convert_I32_To_I64(benchmark::State& state) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int32_t> dis;
    
    std::vector<Value> test_values;
    test_values.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        test_values.push_back(Value::from_i32(dis(gen)));
    }
    
    size_t index = 0;
    for (auto _ : state) {
        auto result = TypeConverter::convert(test_values[index % test_values.size()], ValueType::I64);
        benchmark::DoNotOptimize(result);
        ++index;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Generic_Convert_I32_To_I64);

static void BM_Generic_Convert_F32_To_F64(benchmark::State& state) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-1e6f, 1e6f);
    
    std::vector<Value> test_values;
    test_values.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        test_values.push_back(Value::from_f32(dis(gen)));
    }
    
    size_t index = 0;
    for (auto _ : state) {
        auto result = TypeConverter::convert(test_values[index % test_values.size()], ValueType::F64);
        benchmark::DoNotOptimize(result);
        ++index;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Generic_Convert_F32_To_F64);

// =============================================================================
// Template-Based Zero-Overhead Conversion Benchmarks
// =============================================================================

static void BM_Template_Convert_I32_To_I64(benchmark::State& state) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int32_t> dis;
    
    std::vector<Value> test_values;
    test_values.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        test_values.push_back(Value::from_i32(dis(gen)));
    }
    
    size_t index = 0;
    for (auto _ : state) {
        auto result = conversions::optimized::convert_to<ValueType::I64>(test_values[index % test_values.size()]);
        benchmark::DoNotOptimize(result);
        ++index;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Template_Convert_I32_To_I64);

// =============================================================================
// Conversion Validation Benchmarks
// =============================================================================

static void BM_Is_Conversion_Valid(benchmark::State& state) {
    const ValueType from_types[] = {ValueType::I32, ValueType::I64, ValueType::F32, ValueType::F64};
    const ValueType to_types[] = {ValueType::I32, ValueType::I64, ValueType::F32, ValueType::F64};
    
    size_t index = 0;
    for (auto _ : state) {
        auto from = from_types[index % 4];
        auto to = to_types[(index + 1) % 4];
        auto result = TypeConverter::is_conversion_valid(from, to);
        benchmark::DoNotOptimize(result);
        ++index;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Is_Conversion_Valid);

static void BM_Is_Conversion_Lossy(benchmark::State& state) {
    const ValueType from_types[] = {ValueType::I32, ValueType::I64, ValueType::F32, ValueType::F64};
    const ValueType to_types[] = {ValueType::I32, ValueType::I64, ValueType::F32, ValueType::F64};
    
    size_t index = 0;
    for (auto _ : state) {
        auto from = from_types[index % 4];
        auto to = to_types[(index + 1) % 4];
        auto result = TypeConverter::is_conversion_lossy(from, to);
        benchmark::DoNotOptimize(result);
        ++index;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Is_Conversion_Lossy);

// =============================================================================
// Benchmark Main
// =============================================================================

BENCHMARK_MAIN();

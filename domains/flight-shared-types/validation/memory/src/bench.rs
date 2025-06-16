//! Performance benchmarks for memory validation component

use crate::*;
use std::time::Instant;

#[cfg(test)]
mod benchmarks {
    use super::*;
    
    #[test]
    fn benchmark_allocation_validation() {
        let mut guard = get_validator();
        let validator = guard.as_mut().unwrap();
        
        // Initialize test session
        let _ = validator.initialize_session("test-session".to_string(), "v6r-medium".to_string());
        
        let session_id = "test-session".to_string();
        let size = MemorySize {
            bytes: 1024 * 1024, // 1MB
            human_readable: "1MB".to_string(),
        };
        
        // Benchmark validation performance
        let start = Instant::now();
        let iterations = 10000;
        
        for _ in 0..iterations {
            let _ = validator.validate_allocation_request(
                &session_id,
                &size,
                MemoryPurpose::VmHeap,
            );
        }
        
        let duration = start.elapsed();
        let per_operation = duration.as_nanos() / iterations;
        
        println!("Allocation validation performance: {} ns per operation", per_operation);
        
        // Ensure validation is fast enough (<1000ns target from task)
        assert!(per_operation < 1000, "Validation too slow: {} ns", per_operation);
    }
    
    #[test]
    fn benchmark_memory_pressure_calculation() {
        let guard = get_validator();
        let validator = guard.as_ref().unwrap();
        
        let mut session = SessionMemoryState {
            session_id: "test".to_string(),
            platform: "dreamcast".to_string(),
            total_memory: 16 * 1024 * 1024, // 16MB
            used_memory: 8 * 1024 * 1024,   // 8MB used
            allocations: HashMap::new(),
            limits: MemoryLimits {
                heap_max: MemorySize { bytes: 8 * 1024 * 1024, human_readable: "8MB".to_string() },
                stack_max: MemorySize { bytes: 1024 * 1024, human_readable: "1MB".to_string() },
                cache_max: MemorySize { bytes: 2 * 1024 * 1024, human_readable: "2MB".to_string() },
                soft_limit: MemorySize { bytes: 12 * 1024 * 1024, human_readable: "12MB".to_string() },
                hard_limit: MemorySize { bytes: 15 * 1024 * 1024, human_readable: "15MB".to_string() },
            },
            created_at: 0,
            last_activity: 0,
            snapshots: Vec::new(),
        };
        
        // Benchmark memory pressure calculation
        let start = Instant::now();
        let iterations = 100000;
        
        for _ in 0..iterations {
            let _ = validator.calculate_memory_pressure(&session);
        }
        
        let duration = start.elapsed();
        let per_operation = duration.as_nanos() / iterations;
        
        println!("Memory pressure calculation: {} ns per operation", per_operation);
        
        // Should be very fast
        assert!(per_operation < 100, "Pressure calculation too slow: {} ns", per_operation);
    }
    
    #[test]
    fn benchmark_fragmentation_calculation() {
        let guard = get_validator();
        let validator = guard.as_ref().unwrap();
        
        let mut session = SessionMemoryState {
            session_id: "test".to_string(),
            platform: "dreamcast".to_string(),
            total_memory: 16 * 1024 * 1024, // 16MB
            used_memory: 8 * 1024 * 1024,   // 8MB used
            allocations: HashMap::new(),
            limits: MemoryLimits {
                heap_max: MemorySize { bytes: 8 * 1024 * 1024, human_readable: "8MB".to_string() },
                stack_max: MemorySize { bytes: 1024 * 1024, human_readable: "1MB".to_string() },
                cache_max: MemorySize { bytes: 2 * 1024 * 1024, human_readable: "2MB".to_string() },
                soft_limit: MemorySize { bytes: 12 * 1024 * 1024, human_readable: "12MB".to_string() },
                hard_limit: MemorySize { bytes: 15 * 1024 * 1024, human_readable: "15MB".to_string() },
            },
            created_at: 0,
            last_activity: 0,
            snapshots: Vec::new(),
        };
        
        // Add some allocations for realistic fragmentation calculation
        for i in 0..50 {
            session.allocations.insert(
                format!("alloc_{}", i),
                MemoryAllocation {
                    id: format!("alloc_{}", i),
                    session_id: "test".to_string(),
                    size: MemorySize { bytes: 1024, human_readable: "1KB".to_string() },
                    purpose: MemoryPurpose::VmHeap,
                    allocated_at: 0,
                    freed_at: None,
                }
            );
        }
        
        // Benchmark fragmentation calculation
        let start = Instant::now();
        let iterations = 100000;
        
        for _ in 0..iterations {
            let _ = validator.calculate_fragmentation(&session);
        }
        
        let duration = start.elapsed();
        let per_operation = duration.as_nanos() / iterations;
        
        println!("Fragmentation calculation: {} ns per operation", per_operation);
        
        // Should be very fast
        assert!(per_operation < 50, "Fragmentation calculation too slow: {} ns", per_operation);
    }
    
    #[test]
    fn test_memory_pressure_scenarios() {
        let guard = get_validator();
        let validator = guard.as_ref().unwrap();
        
        // Test different memory pressure scenarios across platforms
        for platform in &["dreamcast", "psp", "vita", "v6r-small", "v6r-medium", "v6r-large"] {
            let constraints = validator.platform_constraints.get(*platform).unwrap();
            
            let mut session = SessionMemoryState {
                session_id: format!("test-{}", platform),
                platform: platform.to_string(),
                total_memory: constraints.memory_limit,
                used_memory: 0,
                allocations: HashMap::new(),
                limits: MemoryLimits {
                    heap_max: MemorySize { bytes: constraints.memory_limit / 2, human_readable: "Half".to_string() },
                    stack_max: MemorySize { bytes: constraints.memory_limit / 8, human_readable: "Eighth".to_string() },
                    cache_max: MemorySize { bytes: constraints.memory_limit / 4, human_readable: "Quarter".to_string() },
                    soft_limit: MemorySize { bytes: (constraints.memory_limit as f32 * constraints.pressure_thresholds.medium) as u64, human_readable: "Soft".to_string() },
                    hard_limit: MemorySize { bytes: constraints.memory_limit, human_readable: "Hard".to_string() },
                },
                created_at: 0,
                last_activity: 0,
                snapshots: Vec::new(),
            };
            
            // Low pressure (30% usage)
            session.used_memory = (session.total_memory as f32 * 0.3) as u64;
            assert!(matches!(validator.calculate_memory_pressure(&session), MemoryPressure::Low), 
                "Platform {} should show Low pressure at 30% usage", platform);
            
            // Medium pressure (70% usage)
            session.used_memory = (session.total_memory as f32 * 0.7) as u64;
            assert!(matches!(validator.calculate_memory_pressure(&session), MemoryPressure::Medium), 
                "Platform {} should show Medium pressure at 70% usage", platform);
            
            // High pressure (85% usage)
            session.used_memory = (session.total_memory as f32 * 0.85) as u64;
            assert!(matches!(validator.calculate_memory_pressure(&session), MemoryPressure::High), 
                "Platform {} should show High pressure at 85% usage", platform);
            
            // Critical pressure (96% usage)
            session.used_memory = (session.total_memory as f32 * 0.96) as u64;
            assert!(matches!(validator.calculate_memory_pressure(&session), MemoryPressure::Critical), 
                "Platform {} should show Critical pressure at 96% usage", platform);
            
            println!("✅ Platform {} memory pressure scenarios validated", platform);
        }
    }
    
    #[test]
    fn benchmark_full_allocation_lifecycle() {
        let mut guard = get_validator();
        let validator = guard.as_mut().unwrap();
        
        // Initialize test session
        let _ = validator.initialize_session("lifecycle-test".to_string(), "v6r-medium".to_string());
        
        let session_id = "lifecycle-test".to_string();
        let size = MemorySize {
            bytes: 64 * 1024, // 64KB
            human_readable: "64KB".to_string(),
        };
        
        // Benchmark full allocation lifecycle (validate -> allocate -> free)
        let start = Instant::now();
        let iterations = 1000;
        
        for i in 0..iterations {
            // Validate
            let _ = validator.validate_allocation_request(&session_id, &size, MemoryPurpose::VmHeap);
            
            // Simulate allocation (create tracking entry)
            let allocation_id = format!("bench_alloc_{}", i);
            if let Some(session) = validator.sessions.get_mut(&session_id) {
                session.allocations.insert(allocation_id.clone(), MemoryAllocation {
                    id: allocation_id.clone(),
                    session_id: session_id.clone(),
                    size: size.clone(),
                    purpose: MemoryPurpose::VmHeap,
                    allocated_at: MemoryValidator::current_timestamp(),
                    freed_at: None,
                });
                session.used_memory += size.bytes;
            }
            
            // Free allocation
            if let Some(session) = validator.sessions.get_mut(&session_id) {
                if let Some(_) = session.allocations.remove(&allocation_id) {
                    session.used_memory = session.used_memory.saturating_sub(size.bytes);
                }
            }
        }
        
        let duration = start.elapsed();
        let per_cycle = duration.as_nanos() / iterations;
        
        println!("Full allocation lifecycle: {} ns per cycle", per_cycle);
        
        // Should be fast enough for real-time usage
        assert!(per_cycle < 10000, "Allocation lifecycle too slow: {} ns", per_cycle);
    }
    
    #[test]
    fn benchmark_memory_snapshot_generation() {
        let guard = get_validator();
        let validator = guard.as_ref().unwrap();
        
        let session = SessionMemoryState {
            session_id: "snapshot-test".to_string(),
            platform: "v6r-medium".to_string(),
            total_memory: 1024 * 1024 * 1024, // 1GB
            used_memory: 512 * 1024 * 1024,   // 512MB used
            allocations: HashMap::new(),
            limits: MemoryLimits {
                heap_max: MemorySize { bytes: 512 * 1024 * 1024, human_readable: "512MB".to_string() },
                stack_max: MemorySize { bytes: 64 * 1024 * 1024, human_readable: "64MB".to_string() },
                cache_max: MemorySize { bytes: 256 * 1024 * 1024, human_readable: "256MB".to_string() },
                soft_limit: MemorySize { bytes: 768 * 1024 * 1024, human_readable: "768MB".to_string() },
                hard_limit: MemorySize { bytes: 1024 * 1024 * 1024, human_readable: "1GB".to_string() },
            },
            created_at: 0,
            last_activity: 0,
            snapshots: Vec::new(),
        };
        
        // Benchmark memory snapshot generation
        let start = Instant::now();
        let iterations = 10000;
        
        for _ in 0..iterations {
            let _snapshot = MemoryUsageSnapshot {
                timestamp: MemoryValidator::current_timestamp(),
                session_id: session.session_id.clone(),
                platform: session.platform.clone(),
                total: MemorySize {
                    bytes: session.total_memory,
                    human_readable: format_bytes(session.total_memory),
                },
                used: MemorySize {
                    bytes: session.used_memory,
                    human_readable: format_bytes(session.used_memory),
                },
                available: MemorySize {
                    bytes: session.total_memory - session.used_memory,
                    human_readable: format_bytes(session.total_memory - session.used_memory),
                },
                fragmentation_ratio: validator.calculate_fragmentation(&session),
            };
        }
        
        let duration = start.elapsed();
        let per_operation = duration.as_nanos() / iterations;
        
        println!("Memory snapshot generation: {} ns per operation", per_operation);
        
        // Should be fast for real-time monitoring
        assert!(per_operation < 2000, "Snapshot generation too slow: {} ns", per_operation);
    }
}

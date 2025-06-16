// Enhanced Flight Memory Types - Generic Implementation
// Production-ready types with enhanced type safety and utilities
export var MemoryPurpose;
(function (MemoryPurpose) {
    MemoryPurpose["VmHeap"] = "vm-heap";
    MemoryPurpose["ComponentStack"] = "component-stack";
    MemoryPurpose["AssetCache"] = "asset-cache";
    MemoryPurpose["JitCodeCache"] = "jit-code-cache";
    MemoryPurpose["SystemReserved"] = "system-reserved";
    MemoryPurpose["WasmLinear"] = "wasm-linear";
    MemoryPurpose["NetworkBuffers"] = "network-buffers";
    MemoryPurpose["Temporary"] = "temporary";
})(MemoryPurpose || (MemoryPurpose = {}));
export var MemoryPressure;
(function (MemoryPressure) {
    MemoryPressure["Low"] = "low";
    MemoryPressure["Medium"] = "medium";
    MemoryPressure["High"] = "high";
    MemoryPressure["Critical"] = "critical";
})(MemoryPressure || (MemoryPressure = {}));
export var MemoryErrorCode;
(function (MemoryErrorCode) {
    MemoryErrorCode["InsufficientMemory"] = "insufficient-memory";
    MemoryErrorCode["LimitExceeded"] = "limit-exceeded";
    MemoryErrorCode["InvalidSize"] = "invalid-size";
    MemoryErrorCode["AllocationFailed"] = "allocation-failed";
    MemoryErrorCode["AlreadyFreed"] = "already-freed";
    MemoryErrorCode["InvalidAllocation"] = "invalid-allocation";
    MemoryErrorCode["UnsupportedPlatform"] = "unsupported-platform";
    MemoryErrorCode["FragmentationError"] = "fragmentation-error";
})(MemoryErrorCode || (MemoryErrorCode = {}));
export var TrendDirection;
(function (TrendDirection) {
    TrendDirection["Increasing"] = "increasing";
    TrendDirection["Decreasing"] = "decreasing";
    TrendDirection["Stable"] = "stable";
    TrendDirection["Volatile"] = "volatile";
})(TrendDirection || (TrendDirection = {}));
//# sourceMappingURL=types.js.map
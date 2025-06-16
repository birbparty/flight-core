// 3D graphics rendering benchmark
uint64_t benchmark_3d_rendering() {
    // Initialize graphics system
    sceGuInit();
    sceGuStart(GU_DIRECT, display_list);
    
    // Create test mesh (1000 triangles)
    TestVertex vertices[3000];
    for (int i = 0; i < 3000; i++) {
        vertices[i].x = (float)(rand() % 200 - 100);
        vertices[i].y = (float)(rand() % 200 - 100);
        vertices[i].z = (float)(rand() % 200 - 100);
        vertices[i].u = (float)(rand() % 256);
        vertices[i].v = (float)(rand() % 256);
    }
    
    // Load test texture
    uint32_t test_texture = pwge_create_test_texture(256, 256);
    
    uint64_t start_time;
    sceRtcGetCurrentTick(&start_time);
    
    // Render 100 frames
    for (int frame = 0; frame < 100; frame++) {
        sceGuStart(GU_DIRECT, display_list);
        
        // Set up matrices
        sceGumMatrixMode(GU_PROJECTION);
        sceGumLoadIdentity();
        sceGumPerspective(75.0f, 16.0f/9.0f, 1.0f, 1000.0f);
        
        sceGumMatrixMode(GU_VIEW);
        sceGumLoadIdentity();
        
        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();
        sceGumRotateX(frame * 0.1f);
        sceGumRotateY(frame * 0.05f);
        
        // Bind texture and render
        sceGuTexMode(GU_PSM_5650, 0, 0, 0);
        sceGuTexImage(0, 256, 256, 256, test_texture);
        sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
        
        sceGumDrawArray(GU_TRIANGLES, 
                       GU_TEXTURE_16BIT | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                       3000, 0, vertices);
        
        sceGuFinish();
        sceGuSync(0, 0);
        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }
    
    uint64_t end_time;
    sceRtcGetCurrentTick(&end_time);
    
    return end_time - start_time;
}

// Network performance benchmark
uint64_t benchmark_network_throughput() {
    if (!pwge_network_init() || !pwge_wifi_connect_test()) {
        return UINT64_MAX;  // Skip if no network
    }
    
    // Create test socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return UINT64_MAX;
    
    // Test data
    uint8_t test_data[1024];
    for (int i = 0; i < 1024; i++) {
        test_data[i] = i & 0xFF;
    }
    
    struct sockaddr_in test_addr;
    test_addr.sin_family = AF_INET;
    test_addr.sin_port = htons(12345);
    inet_aton("127.0.0.1", &test_addr.sin_addr);  // Loopback test
    
    uint64_t start_time;
    sceRtcGetCurrentTick(&start_time);
    
    // Send 1000 packets
    for (int i = 0; i < 1000; i++) {
        sendto(sock, test_data, 1024, 0, (struct sockaddr*)&test_addr, sizeof(test_addr));
        
        // Small delay to avoid overwhelming
        sceKernelDelayThread(1000);  // 1ms
    }
    
    uint64_t end_time;
    sceRtcGetCurrentTick(&end_time);
    
    close(sock);
    return end_time - start_time;
}

// Memory allocation performance
uint64_t benchmark_memory_allocation() {
    pwge_memory_init();
    
    uint64_t start_time;
    sceRtcGetCurrentTick(&start_time);
    
    // Allocate and free various sized blocks
    void* ptrs[1000];
    for (int i = 0; i < 1000; i++) {
        size_t size = (i % 10 + 1) * 1024;  // 1KB to 10KB blocks
        ptrs[i] = pwge_memory_alloc(0, size);
    }
    
    for (int i = 0; i < 1000; i++) {
        pwge_memory_free(0, ptrs[i]);
    }
    
    uint64_t end_time;
    sceRtcGetCurrentTick(&end_time);
    
    return end_time - start_time;
}

// Component scheduling benchmark
uint64_t benchmark_component_scheduling() {
    ComponentScheduler scheduler;
    pwge_scheduler_init(&scheduler);
    
    // Create 8 test components with different priorities
    for (int i = 0; i < 8; i++) {
        PWGEComponent comp;
        comp.component_id = i;
        comp.priority = i % 4;  // Mix of priorities
        comp.cycles_budget = 10000 + i * 5000;
        comp.state = COMPONENT_ACTIVE;
        pwge_scheduler_add_component(&scheduler, &comp);
    }
    
    uint64_t start_time;
    sceRtcGetCurrentTick(&start_time);
    
    // Run 1000 scheduling cycles
    for (int i = 0; i < 1000; i++) {
        pwge_scheduler_update(&scheduler);
    }
    
    uint64_t end_time;
    sceRtcGetCurrentTick(&end_time);
    
    return end_time - start_time;
}

// Audio mixing benchmark
uint64_t benchmark_audio_mixing() {
    pwge_audio_init();
    
    // Create 16 test audio voices
    for (int i = 0; i < 16; i++) {
        PWGEAudioVoice* voice = &audio_engine.voices[i];
        voice->sample_data = pwge_generate_test_tone(440 + i * 55);  // Different frequencies
        voice->sample_count = 44100;  // 1 second
        voice->volume = 0.5f;
        voice->pitch = 1.0f;
        voice->active = true;
        voice->current_position = 0;
    }
    
    uint64_t start_time;
    sceRtcGetCurrentTick(&start_time);
    
    // Mix 1000 audio buffers (simulating real-time mixing)
    for (int i = 0; i < 1000; i++) {
        int16_t mix_buffer[2048];  // Stereo buffer
        pwge_audio_mix_callback(mix_buffer, 1024, NULL);
    }
    
    uint64_t end_time;
    sceRtcGetCurrentTick(&end_time);
    
    return end_time - start_time;
}

// File I/O benchmark (Memory Stick)
uint64_t benchmark_file_io() {
    const char* test_filename = "ms0:/test_data.bin";
    uint8_t test_data[64*1024];  // 64KB test data
    
    // Fill with test pattern
    for (int i = 0; i < sizeof(test_data); i++) {
        test_data[i] = i & 0xFF;
    }
    
    uint64_t start_time;
    sceRtcGetCurrentTick(&start_time);
    
    // Write and read test (simulating save/load operations)
    for (int i = 0; i < 100; i++) {
        // Write
        FILE* f = fopen(test_filename, "wb");
        if (f) {
            fwrite(test_data, 1, sizeof(test_data), f);
            fclose(f);
        }
        
        // Read
        f = fopen(test_filename, "rb");
        if (f) {
            uint8_t read_buffer[64*1024];
            fread(read_buffer, 1, sizeof(read_buffer), f);
            fclose(f);
        }
    }
    
    uint64_t end_time;
    sceRtcGetCurrentTick(&end_time);
    
    // Cleanup
    remove(test_filename);
    
    return end_time - start_time;
}

static BenchmarkTest benchmarks[] = {
    {"JIT Compilation", benchmark_jit_compilation, 50000, 0, false, 
     "Time to compile 100 WASM functions to MIPS code"},
    
    {"WASM Execution", benchmark_wasm_execution, 100000, 0, false,
     "Execution speed of JIT-compiled WASM arithmetic"},
     
    {"3D Rendering", benchmark_3d_rendering, 1666000, 0, false,  // 16.67ms * 100 frames
     "Hardware-accelerated 3D rendering (1000 triangles/frame)"},
     
    {"Network Throughput", benchmark_network_throughput, 1000000, 0, false,
     "UDP packet transmission over WiFi"},
     
    {"Memory Allocation", benchmark_memory_allocation, 10000, 0, false,
     "Dynamic memory allocation and deallocation"},
     
    {"Component Scheduling", benchmark_component_scheduling, 5000, 0, false,
     "Multi-component execution scheduling overhead"},
     
    {"Audio Mixing", benchmark_audio_mixing, 50000, 0, false,
     "Real-time audio mixing (16 voices, 44.1kHz)"},
     
    {"File I/O", benchmark_file_io, 2000000, 0, false,
     "Memory Stick read/write performance (64KB blocks)"},
};

void pwge_run_benchmarks() {
    pspDebugScreenInit();
    pspDebugScreenPrintf("PWGE Performance Benchmarks\n");
    pspDebugScreenPrintf("============================\n\n");
    
    int passed = 0;
    int total = sizeof(benchmarks) / sizeof(BenchmarkTest);
    
    for (int i = 0; i < total; i++) {
        pspDebugScreenPrintf("Running %s...\n", benchmarks[i].name);
        pspDebugScreenPrintf("  %s\n", benchmarks[i].description);
        
        // Run benchmark 3 times and take best result
        uint64_t best_time = UINT64_MAX;
        for (int run = 0; run < 3; run++) {
            uint64_t time = benchmarks[i].benchmark_func();
            if (time < best_time) {
                best_time = time;
            }
        }
        
        benchmarks[i].actual_time_us = best_time;
        benchmarks[i].passed = (best_time <= benchmarks[i].target_time_us);
        
        if (benchmarks[i].passed) {
            pspDebugScreenPrintf("  PASS (%llu/%llu us)\n\n", 
                               best_time, benchmarks[i].target_time_us);
            passed++;
        } else {
            pspDebugScreenPrintf("  FAIL (%llu/%llu us) - %.1fx over target\n\n",
                               best_time, benchmarks[i].target_time_us,
                               (float)best_time / benchmarks[i].target_time_us);
        }
    }
    
    pspDebugScreenPrintf("Benchmark Results: %d/%d passed (%.1f%%)\n", 
                        passed, total, (float)passed / total * 100.0f);
    
    if (passed == total) {
        pspDebugScreenPrintf("All benchmarks passed! PWGE performance targets met.\n");
    } else {
        pspDebugScreenPrintf("Some benchmarks failed. Optimization needed.\n");
    }
    
    pspDebugScreenPrintf("\nPress X to continue...\n");
    
    // Wait for input
    SceCtrlData pad;
    do {
        sceCtrlReadBufferPositive(&pad, 1);
        sceKernelDelayThread(10000);
    } while (!(pad.Buttons & PSP_CTRL_CROSS));
}

// Performance monitoring during gameplay
typedef struct {
    uint64_t frame_start_time;
    uint64_t jit_time;
    uint64_t component_time;
    uint64_t graphics_time;
    uint64_t audio_time;
    uint64_t network_time;
    uint32_t triangles_rendered;
    uint32_t draw_calls;
    uint32_t texture_switches;
    uint32_t memory_allocations;
    uint32_t network_packets;
} FrameProfileData;

static FrameProfileData profile_data;
static bool profiling_enabled = false;

void pwge_profiler_start_frame() {
    if (!profiling_enabled) return;
    
    sceRtcGetCurrentTick(&profile_data.frame_start_time);
    profile_data.jit_time = 0;
    profile_data.component_time = 0;
    profile_data.graphics_time = 0;
    profile_data.audio_time = 0;
    profile_data.network_time = 0;
    profile_data.triangles_rendered = 0;
    profile_data.draw_calls = 0;
    profile_data.texture_switches = 0;
    profile_data.memory_allocations = 0;
    profile_data.network_packets = 0;
}

void pwge_profiler_end_frame() {
    if (!profiling_enabled) return;
    
    uint64_t frame_end_time;
    sceRtcGetCurrentTick(&frame_end_time);
    
    uint64_t total_frame_time = frame_end_time - profile_data.frame_start_time;
    
    // Log performance data every 60 frames (1 second at 60fps)
    static int frame_counter = 0;
    frame_counter++;
    
    if (frame_counter >= 60) {
        pspDebugScreenPrintf("Frame: %llu us (%.1f fps)\n", 
                           total_frame_time, 1000000.0f / total_frame_time);
        pspDebugScreenPrintf("  JIT: %llu us\n", profile_data.jit_time);
        pspDebugScreenPrintf("  Components: %llu us\n", profile_data.component_time);
        pspDebugScreenPrintf("  Graphics: %llu us (%d triangles, %d draws)\n", 
                           profile_data.graphics_time, profile_data.triangles_rendered, profile_data.draw_calls);
        pspDebugScreenPrintf("  Audio: %llu us\n", profile_data.audio_time);
        pspDebugScreenPrintf("  Network: %llu us (%d packets)\n", 
                           profile_data.network_time, profile_data.network_packets);
        
        frame_counter = 0;
    }
}

void pwge_profiler_enable(bool enabled) {
    profiling_enabled = enabled;
}
```

---

## Hot Reload and Development Tools

### Advanced Hot Reload System

```python
# tools/hot_deploy.py

import os
import sys
import socket
import struct
import hashlib
import time
import threading
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

class PSPHotReloadClient:
    def __init__(self, psp_ip, port=31338):
        self.psp_ip = psp_ip
        self.port = port
        self.socket = None
        self.connected = False
        
    def connect(self):
        """Connect to PSP hot reload server"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(5.0)
            self.socket.connect((self.psp_ip, self.port))
            
            # Send handshake
            handshake = b"PWGE_HOT_RELOAD_V1"
            self.socket.send(struct.pack('<I', len(handshake)))
            self.socket.send(handshake)
            
            # Wait for acknowledgment
            response = self.socket.recv(4)
            if struct.unpack('<I', response)[0] == 0x12345678:
                self.connected = True
                print(f"Connected to PSP at {self.psp_ip}:{self.port}")
                return True
            else:
                print("Invalid handshake response")
                return False
                
        except Exception as e:
            print(f"Failed to connect to PSP: {e}")
            return False
    
    def deploy_component(self, component_path, component_id):
        """Deploy a single WASM component to PSP"""
        if not self.connected:
            print("Not connected to PSP")
            return False
        
        try:
            with open(component_path, 'rb') as f:
                wasm_data = f.read()
            
            # Calculate checksum
            checksum = hashlib.md5(wasm_data).hexdigest()
            
            # Send deployment message
            message = {
                'type': 'deploy_component',
                'component_id': component_id,
                'size': len(wasm_data),
                'checksum': checksum
            }
            
            # Send header
            header = f"{message['type']}:{component_id}:{len(wasm_data)}:{checksum}".encode('utf-8')
            self.socket.send(struct.pack('<I', len(header)))
            self.socket.send(header)
            
            # Send WASM data in chunks
            chunk_size = 8192
            for i in range(0, len(wasm_data), chunk_size):
                chunk = wasm_data[i:i + chunk_size]
                self.socket.send(chunk)
            
            # Wait for response
            response_len = struct.unpack('<I', self.socket.recv(4))[0]
            response = self.socket.recv(response_len).decode('utf-8')
            
            if response.startswith('SUCCESS'):
                print(f"✓ Component {component_id} deployed successfully")
                return True
            else:
                print(f"✗ Deployment failed: {response}")
                return False
                
        except Exception as e:
            print(f"Deployment error: {e}")
            return False
    
    def reload_component(self, component_id):
        """Trigger component reload on PSP"""
        if not self.connected:
            return False
        
        try:
            message = f"reload_component:{component_id}".encode('utf-8')
            self.socket.send(struct.pack('<I', len(message)))
            self.socket.send(message)
            
            response_len = struct.unpack('<I', self.socket.recv(4))[0]
            response = self.socket.recv(response_len).decode('utf-8')
            
            if response == 'RELOADED':
                print(f"✓ Component {component_id} reloaded")
                return True
            else:
                print(f"✗ Reload failed: {response}")
                return False
                
        except Exception as e:
            print(f"Reload error: {e}")
            return False
    
    def get_performance_stats(self):
        """Get real-time performance statistics from PSP"""
        if not self.connected:
            return None
        
        try:
            message = b"get_stats"
            self.socket.send(struct.pack('<I', len(message)))
            self.socket.send(message)
            
            response_len = struct.unpack('<I', self.socket.recv(4))[0]
            response = self.socket.recv(response_len).decode('utf-8')
            
            # Parse JSON response
            import json
            return json.loads(response)
            
        except Exception as e:
            print(f"Stats error: {e}")
            return None
    
    def disconnect(self):
        """Disconnect from PSP"""
        if self.socket:
            try:
                message = b"disconnect"
                self.socket.send(struct.pack('<I', len(message)))
                self.socket.send(message)
            except:
                pass
            
            self.socket.close()
            self.connected = False

class ComponentWatcher(FileSystemEventHandler):
    def __init__(self, hot_reload_client, component_map):
        self.client = hot_reload_client
        self.component_map = component_map  # path -> component_id mapping
        self.last_modified = {}
        
    def on_modified(self, event):
        if event.is_directory:
            return
        
        if event.src_path.endswith('.wasm'):
            # Debounce file changes (avoid multiple rapid deployments)
            current_time = time.time()
            if event.src_path in self.last_modified:
                if current_time - self.last_modified[event.src_path] < 1.0:
                    return
            
            self.last_modified[event.src_path] = current_time
            
            component_id = self.component_map.get(event.src_path)
            if component_id:
                print(f"📁 Detected change in {event.src_path}")
                if self.client.deploy_component(event.src_path, component_id):
                    self.client.reload_component(component_id)

def main():
    if len(sys.argv) < 2:
        print("Usage: python hot_deploy.py <psp_ip> [component_files...]")
        print("Example: python hot_deploy.py 192.168.1.100 build/*.wasm")
        sys.exit(1)
    
    psp_ip = sys.argv[1]
    component_files = sys.argv[2:] if len(sys.argv) > 2 else []
    
    # Create hot reload client
    client = PSPHotReloadClient(psp_ip)
    
    if not client.connect():
        print("Failed to connect to PSP")
        sys.exit(1)
    
    # Deploy initial components
    component_map = {}
    for i, component_file in enumerate(component_files):
        if os.path.exists(component_file):
            component_id = os.path.basename(component_file).replace('.wasm', '')
            component_map[os.path.abspath(component_file)] = component_id
            client.deploy_component(component_file, component_id)
    
    if component_files:
        # Set up file watching for hot reload
        event_handler = ComponentWatcher(client, component_map)
        observer = Observer()
        
        # Watch build directory and component directories
        watch_dirs = set()
        for component_file in component_files:
            watch_dirs.add(os.path.dirname(os.path.abspath(component_file)))
        
        for watch_dir in watch_dirs:
            observer.schedule(event_handler, watch_dir, recursive=True)
        
        observer.start()
        
        print("🔥 Hot reload active! Watching for changes...")
        print("📊 Press 's' for performance stats, 'q' to quit")
        
        # Interactive command loop
        try:
            while True:
                cmd = input().strip().lower()
                
                if cmd == 'q':
                    break
                elif cmd == 's':
                    stats = client.get_performance_stats()
                    if stats:
                        print("\n📊 Performance Stats:")
                        print(f"  Frame Time: {stats.get('frame_time_ms', 0):.2f}ms")
                        print(f"  FPS: {stats.get('fps', 0):.1f}")
                        print(f"  Memory Used: {stats.get('memory_used_mb', 0):.1f}MB")
                        print(f"  JIT Cache: {stats.get('jit_cache_mb', 0):.1f}MB")
                        print(f"  Network: {stats.get('network_packets_per_sec', 0)} pps")
                        print()
                elif cmd.startswith('reload '):
                    component_id = cmd.split(' ', 1)[1]
                    client.reload_component(component_id)
                elif cmd == 'help':
                    print("Commands:")
                    print("  s - Show performance stats")
                    print("  reload <component> - Reload specific component")
                    print("  q - Quit")
                    
        except KeyboardInterrupt:
            pass
        
        observer.stop()
        observer.join()
    
    client.disconnect()
    print("Disconnected from PSP")

if __name__ == "__main__":
    main()
```

### PSP-Side Hot Reload Server

```c
// pwge-tools/hot_reload_server.c

#include <pspkernel.h>
#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspthreadman.h>
#include <string.h>

typedef struct {
    int server_socket;
    int client_socket;
    bool server_running;
    SceUID thread_id;
    ComponentManager* component_manager;
    JITCompiler* jit_compiler;
} HotReloadServer;

static HotReloadServer reload_server;

// Handle deployment of new component
static int handle_component_deployment(const char* header, int data_size) {
    char component_id[64];
    char checksum[33];
    
    // Parse header: "deploy_component:component_id:size:checksum"
    if (sscanf(header, "deploy_component:%63[^:]:%*d:%32s", component_id, checksum) != 2) {
        return -1;
    }
    
    // Allocate buffer for WASM data
    uint8_t* wasm_data = malloc(data_size);
    if (!wasm_data) {
        return -1;
    }
    
    // Receive WASM data
    int bytes_received = 0;
    while (bytes_received < data_size) {
        int chunk = recv(reload_server.client_socket, 
                        wasm_data + bytes_received, 
                        data_size - bytes_received, 0);
        if (chunk <= 0) {
            free(wasm_data);
            return -1;
        }
        bytes_received += chunk;
    }
    
    // Verify checksum
    char calculated_checksum[33];
    if (!verify_md5_checksum(wasm_data, data_size, checksum, calculated_checksum)) {
        free(wasm_data);
        send_response("ERROR:Checksum mismatch");
        return -1;
    }
    
    // Find existing component or create new slot
    PWGEComponent* component = pwge_find_component_by_id(reload_server.component_manager, component_id);
    if (!component) {
        component = pwge_create_component_slot(reload_server.component_manager, component_id);
        if (!component) {
            free(wasm_data);
            send_response("ERROR:No component slots available");
            return -1;
        }
    }
    
    // Suspend component if running
    if (component->state == COMPONENT_ACTIVE) {
        pwge_suspend_component(component);
    }
    
    // Compile new WASM module
    WasmModule* new_module = pwge_load_wasm_module(wasm_data, data_size);
    if (!new_module) {
        free(wasm_data);
        send_response("ERROR:Invalid WASM module");
        return -1;
    }
    
    // JIT compile functions
    JITCompiledFunction* compiled_functions = pwge_jit_compile_module(
        reload_server.jit_compiler, new_module);
    if (!compiled_functions) {
        pwge_free_wasm_module(new_module);
        free(wasm_data);
        send_response("ERROR:JIT compilation failed");
        return -1;
    }
    
    // Atomic swap of component implementation
    WasmModule* old_module = component->module;
    JITCompiledFunction* old_functions = component->compiled_functions;
    
    component->module = new_module;
    component->compiled_functions = compiled_functions;
    component->hot_reload_version++;
    
    // Clean up old implementation
    if (old_module) {
        pwge_jit_free_functions(old_functions);
        pwge_free_wasm_module(old_module);
    }
    
    free(wasm_data);
    send_response("SUCCESS:Component deployed");
    
    return 0;
}

// Handle component reload request
static int handle_component_reload(const char* header) {
    char component_id[64];
    
    // Parse header: "reload_component:component_id"
    if (sscanf(header, "reload_component:%63s", component_id) != 1) {
        return -1;
    }
    
    PWGEComponent* component = pwge_find_component_by_id(reload_server.component_manager, component_id);
    if (!component) {
        send_response("ERROR:Component not found");
        return -1;
    }
    
    // Restart component with new implementation
    if (component->state == COMPONENT_SUSPENDED) {
        ComponentContext ctx = {
            .component_id = component->component_id,
            .delta_time = 0.0f,
            .frame_count = 0,
            .memory_available = component->memory_size,
            .cycles_remaining = component->cycles_budget,
            .psp_clock_speed = sceKernelGetSystemTimeLow(),
            .battery_level = scePowerGetBatteryLifePercent(),
            .wifi_connected = pwge_wifi_is_connected()
        };
        
        // Call component init function
        if (pwge_call_wasm_function(component, "init", &ctx) == 0) {
            component->state = COMPONENT_ACTIVE;
            send_response("RELOADED");
        } else {
            send_response("ERROR:Component init failed");
        }
    } else {
        send_response("ERROR:Component not suspended");
    }
    
    return 0;
}

// Send performance statistics
static int handle_stats_request() {
    char stats_json[1024];
    
    PerformanceCounters counters;
    pwge_get_performance_counters(&counters);
    
    snprintf(stats_json, sizeof(stats_json),
        "{"
        "\"frame_time_ms\":%.2f,"
        "\"fps\":%.1f,"
        "\"memory_used_mb\":%.1f,"
        "\"jit_cache_mb\":%.1f,"
        "\"network_packets_per_sec\":%d,"
        "\"components_active\":%d,"
        "[[component]]
id = "track_renderer"
source = "track_renderer/target/wasm32-unknown-unknown/release/track_renderer.wasm"
priority = "high"
max_memory = "8MB"
max_cycles_per_frame = 150000

[component.capabilities]
graphics = ["mesh_renderer", "texture_streaming", "level_of_detail"]
storage = ["track_cache"]

[[component]]
id = "ai_opponents"
source = "ai_opponents/target/wasm32-unknown-unknown/release/ai_opponents.wasm"
priority = "medium"
max_memory = "2MB"
max_cycles_per_frame = 50000

[component.capabilities]
ai = ["pathfinding", "behavior_trees"]
physics = ["collision_detection"]

[[component]]
id = "hud_system"
source = "hud_system/target/wasm32-unknown-unknown/release/hud_system.wasm"
priority = "low"
max_memory = "1MB"
max_cycles_per_frame = 20000

[component.capabilities]
graphics = ["2d_renderer", "text_rendering"]
input = ["menu_navigation"]

[[component]]
id = "network_manager"
source = "network_manager/target/wasm32-unknown-unknown/release/network_manager.wasm"
priority = "critical"
max_memory = "2MB"
max_cycles_per_frame = 40000

[component.capabilities]
network = ["udp_sockets", "lobby_system", "lag_compensation"]

[graphics]
resolution = "480x272"
color_depth = 32
render_target = "back_buffer"
vsync = true
anti_aliasing = "2x"
texture_filtering = "bilinear"
max_triangles_per_frame = 50000
max_textures = 64

[audio]
sample_rate = 44100
channels = 2
buffer_size = 2048
max_voices = 16
reverb = true
3d_audio = true

[networking]
protocol = "udp"
max_players = 8
lobby_system = true
dedicated_server = false
peer_to_peer = true
lag_compensation = true
rollback_frames = 6

[performance]
target_fps = 60
max_frame_time_ms = 16.67
cpu_governor = "performance"
gpu_clock = "166mhz"
memory_clock = "166mhz"

[input]
analog_deadzone = 0.1
button_repeat_delay = 150
gesture_recognition = false

[storage]
save_format = "binary_compressed"
max_save_size = "1MB"
autosave_interval = 30
backup_saves = true
```

---

## Development Toolchain

### Advanced Build System

```makefile
# Makefile for PWGE projects

# PSP toolchain paths
PSP_DEV = $(shell psp-config --pspsdk-path)
PSP_CC = psp-gcc
PSP_CXX = psp-g++
PSP_AS = psp-as
PSP_LD = psp-ld
PSP_OBJCOPY = psp-objcopy
PSP_STRIP = psp-strip

# WASM toolchain
WASM_CC = clang
WASM_RUST = cargo
WASM_TARGET = --target=wasm32-unknown-unknown
WASM_FLAGS = -O3 -flto -nostdlib -Wl,--no-entry -Wl,--allow-undefined

# Project structure
SRC_DIR = src
WASM_DIR = wasm-components
BUILD_DIR = build
DIST_DIR = dist
ASSETS_DIR = assets

# PWGE runtime sources
PWGE_SOURCES = \
    $(SRC_DIR)/pwge-runtime/jit_compiler.c \
    $(SRC_DIR)/pwge-runtime/component_manager.c \
    $(SRC_DIR)/pwge-runtime/memory_manager.c \
    $(SRC_DIR)/pwge-graphics/gu_interface.c \
    $(SRC_DIR)/pwge-audio/audio_engine.c \
    $(SRC_DIR)/pwge-network/wifi_manager.c \
    $(SRC_DIR)/pwge-core/scheduler.c

# WASM component sources
WASM_COMPONENTS = \
    $(WASM_DIR)/car_physics \
    $(WASM_DIR)/track_renderer \
    $(WASM_DIR)/ai_opponents \
    $(WASM_DIR)/hud_system \
    $(WASM_DIR)/network_manager

# PSP-specific flags
PSP_CFLAGS = -G0 -Wall -O3 -ffast-math -fno-strict-aliasing
PSP_CXXFLAGS = $(PSP_CFLAGS) -fno-exceptions -fno-rtti
PSP_LIBS = -lpspaudiolib -lpspaudio -lpspgu -lpspgum -lpsprtc -lpsppower \
           -lpspnet -lpspnet_inet -lpspnet_apctl -lpspwlan

# Build targets
.PHONY: all clean wasm-components psp-binary eboot assets

all: eboot

# Build WASM components
wasm-components:
	@echo "Building WASM components..."
	@for component in $(WASM_COMPONENTS); do \
		echo "Building $component..."; \
		cd $component && \
		if [ -f Cargo.toml ]; then \
			$(WASM_RUST) build --release --target wasm32-unknown-unknown; \
			cp target/wasm32-unknown-unknown/release/*.wasm ../../$(BUILD_DIR)/; \
		else \
			$(WASM_CC) $(WASM_TARGET) $(WASM_FLAGS) \
				-I../../include/pwge-api \
				src/*.c \
				-o ../../$(BUILD_DIR)/$(basename $component).wasm; \
		fi; \
		cd ../..; \
	done

# Process assets
assets:
	@echo "Processing game assets..."
	@mkdir -p $(BUILD_DIR)/assets
	@python3 tools/asset_processor.py $(ASSETS_DIR) $(BUILD_DIR)/assets
	@echo "Assets processed successfully"

# Build PSP binary
psp-binary: wasm-components assets
	@echo "Building PSP binary..."
	@mkdir -p $(BUILD_DIR)
	$(PSP_CC) $(PSP_CFLAGS) \
		-I$(PSP_DEV)/include \
		-I$(PSP_DEV)/include/libc \
		-Iinclude \
		$(PWGE_SOURCES) \
		src/main.c \
		-L$(PSP_DEV)/lib \
		$(PSP_LIBS) \
		-o $(BUILD_DIR)/pwge_game.elf

# Create EBOOT.PBP
eboot: psp-binary
	@echo "Creating EBOOT.PBP..."
	@mkdir -p $(DIST_DIR)
	
	# Strip debug symbols for smaller size
	$(PSP_STRIP) $(BUILD_DIR)/pwge_game.elf
	
	# Create EBOOT.PBP with game assets
	pack-pbp $(DIST_DIR)/EBOOT.PBP \
		PARAM.SFO \
		NULL \
		assets/icon0.png \
		NULL \
		assets/pic1.png \
		NULL \
		$(BUILD_DIR)/pwge_game.elf \
		NULL

# Create PARAM.SFO
param-sfo:
	@echo "Creating PARAM.SFO..."
	mksfoex -d MEMSIZE=1 "PSP WASM Game Engine" PARAM.SFO

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR) PARAM.SFO

# Development helpers
dev-server:
	@echo "Starting development server with hot-reload..."
	python3 tools/dev_server.py --psp-ip=$(PSP_IP) --port=8080

hot-reload:
	@echo "Deploying hot-reload update..."
	@if [ -z "$(PSP_IP)" ]; then \
		echo "Error: PSP_IP not set"; \
		exit 1; \
	fi
	@$(MAKE) wasm-components
	@tools/hot_deploy.py $(PSP_IP) $(BUILD_DIR)/*.wasm

test-emulator:
	@echo "Testing in PPSSPP emulator..."
	ppsspp $(DIST_DIR)/EBOOT.PBP

deploy-psp:
	@echo "Deploying to PSP via USB..."
	@if [ -z "$(PSP_MOUNT)" ]; then \
		echo "Error: PSP_MOUNT not set (e.g., /media/PSP/PSP/GAME/PWGE/)"; \
		exit 1; \
	fi
	@mkdir -p $(PSP_MOUNT)
	@cp $(DIST_DIR)/EBOOT.PBP $(PSP_MOUNT)/
	@cp -r $(BUILD_DIR)/assets $(PSP_MOUNT)/
	@sync
	@echo "Deployment complete"

# Performance profiling
profile:
	@echo "Building with profiling enabled..."
	@$(MAKE) PSP_CFLAGS="$(PSP_CFLAGS) -DPWGE_PROFILING_ENABLED" psp-binary

# Debug build
debug:
	@echo "Building debug version..."
	@$(MAKE) PSP_CFLAGS="$(PSP_CFLAGS) -g -DDEBUG -O0" psp-binary

# Release build with optimizations
release:
	@echo "Building release version..."
	@$(MAKE) PSP_CFLAGS="$(PSP_CFLAGS) -DNDEBUG -O3 -fomit-frame-pointer" eboot

# Component development
new-component:
	@read -p "Component name: " name; \
	tools/create_component.sh $name rust

# Documentation
docs:
	@echo "Generating documentation..."
	@cd docs && mdbook build
	@echo "Documentation available at docs/book/index.html"

# Performance benchmarks
benchmark:
	@echo "Running performance benchmarks..."
	@$(MAKE) profile
	@tools/run_benchmarks.py $(DIST_DIR)/EBOOT.PBP
```

### Advanced Asset Processing Pipeline

```python
# tools/asset_processor.py

import os
import sys
import struct
import json
import subprocess
from PIL import Image
import wave
import gzip

class PSPAssetProcessor:
    def __init__(self, input_dir, output_dir):
        self.input_dir = input_dir
        self.output_dir = output_dir
        self.texture_formats = {
            'RGB565': 0,
            'RGBA4444': 1, 
            'RGBA8888': 2,
            'COMPRESSED': 3
        }
        
    def process_all_assets(self):
        """Process all assets for PSP deployment"""
        os.makedirs(self.output_dir, exist_ok=True)
        
        print("Processing PSP assets...")
        
        # Process different asset types
        self.process_textures(os.path.join(self.input_dir, "textures"))
        self.process_models(os.path.join(self.input_dir, "models"))
        self.process_audio(os.path.join(self.input_dir, "audio"))
        self.process_fonts(os.path.join(self.input_dir, "fonts"))
        self.process_shaders(os.path.join(self.input_dir, "shaders"))
        
        # Generate asset manifest
        self.generate_manifest()
        
        print("Asset processing complete!")
        
    def process_textures(self, texture_dir):
        """Convert textures to PSP-optimized formats"""
        if not os.path.exists(texture_dir):
            return
            
        print("Processing textures...")
        
        textures = []
        
        for filename in os.listdir(texture_dir):
            if filename.lower().endswith(('.png', '.jpg', '.jpeg', '.tga')):
                img_path = os.path.join(texture_dir, filename)
                texture = self.optimize_texture(img_path, filename)
                if texture:
                    textures.append(texture)
        
        # Create texture atlas for efficiency
        if textures:
            self.create_texture_atlas(textures)
    
    def optimize_texture(self, img_path, filename):
        """Optimize single texture for PSP"""
        img = Image.open(img_path)
        
        # Convert to RGB if necessary
        if img.mode not in ['RGB', 'RGBA']:
            img = img.convert('RGBA' if 'transparency' in img.info else 'RGB')
        
        # Get optimal size (power of 2, max 512x512 for PSP)
        max_size = 512
        width, height = img.size
        
        # Scale down if too large
        if width > max_size or height > max_size:
            img.thumbnail((max_size, max_size), Image.Resampling.LANCZOS)
            width, height = img.size
        
        # Ensure power of 2 dimensions
        new_width = self.next_power_of_2(width)
        new_height = self.next_power_of_2(height)
        
        if new_width != width or new_height != height:
            # Resize to power of 2
            img = img.resize((new_width, new_height), Image.Resampling.LANCZOS)
        
        # Choose optimal format based on content
        has_alpha = img.mode == 'RGBA' and any(pixel[3] < 255 for pixel in img.getdata())
        
        if has_alpha:
            # Use RGBA4444 for alpha textures (smaller)
            format_type = 'RGBA4444'
            texture_data = self.convert_to_rgba4444(img)
        else:
            # Use RGB565 for opaque textures
            format_type = 'RGB565'
            img = img.convert('RGB')
            texture_data = self.convert_to_rgb565(img)
        
        # Save processed texture
        texture_filename = os.path.splitext(filename)[0] + '.ptex'
        texture_path = os.path.join(self.output_dir, texture_filename)
        
        with open(texture_path, 'wb') as f:
            # PTEX header
            f.write(b'PTEX')  # Magic
            f.write(struct.pack('<I', 1))  # Version
            f.write(struct.pack('<H', new_width))
            f.write(struct.pack('<H', new_height))
            f.write(struct.pack('<B', self.texture_formats[format_type]))
            f.write(struct.pack('<B', 1 if self.should_swizzle(new_width, new_height) else 0))
            f.write(struct.pack('<H', 0))  # Padding
            
            # Swizzle texture data for PSP cache efficiency
            if self.should_swizzle(new_width, new_height):
                texture_data = self.swizzle_texture(texture_data, new_width, new_height, format_type)
            
            f.write(texture_data)
        
        return {
            'name': os.path.splitext(filename)[0],
            'filename': texture_filename,
            'width': new_width,
            'height': new_height,
            'format': format_type,
            'swizzled': self.should_swizzle(new_width, new_height)
        }
    
    def convert_to_rgb565(self, img):
        """Convert PIL image to RGB565 format"""
        rgb_data = img.tobytes()
        rgb565_data = bytearray()
        
        for i in range(0, len(rgb_data), 3):
            r = rgb_data[i] >> 3      # 5 bits
            g = rgb_data[i+1] >> 2    # 6 bits
            b = rgb_data[i+2] >> 3    # 5 bits
            
            rgb565 = (r << 11) | (g << 5) | b
            rgb565_data.extend(struct.pack('<H', rgb565))
        
        return bytes(rgb565_data)
    
    def convert_to_rgba4444(self, img):
        """Convert PIL image to RGBA4444 format"""
        rgba_data = img.tobytes()
        rgba4444_data = bytearray()
        
        for i in range(0, len(rgba_data), 4):
            r = rgba_data[i] >> 4     # 4 bits
            g = rgba_data[i+1] >> 4   # 4 bits
            b = rgba_data[i+2] >> 4   # 4 bits
            a = rgba_data[i+3] >> 4   # 4 bits
            
            rgba4444 = (a << 12) | (r << 8) | (g << 4) | b
            rgba4444_data.extend(struct.pack('<H', rgba4444))
        
        return bytes(rgba4444_data)
    
    def should_swizzle(self, width, height):
        """Determine if texture should be swizzled for PSP cache efficiency"""
        return width >= 16 and height >= 16 and width <= 512 and height <= 512
    
    def swizzle_texture(self, data, width, height, format_type):
        """Apply PSP texture swizzling for cache optimization"""
        bytes_per_pixel = 2  # Both RGB565 and RGBA4444 are 16-bit
        
        swizzled = bytearray(len(data))
        
        for y in range(height):
            for x in range(width):
                src_offset = (y * width + x) * bytes_per_pixel
                dst_offset = self.swizzle_coordinate(x, y, width) * bytes_per_pixel
                
                swizzled[dst_offset:dst_offset+bytes_per_pixel] = data[src_offset:src_offset+bytes_per_pixel]
        
        return bytes(swizzled)
    
    def swizzle_coordinate(self, x, y, width):
        """Calculate swizzled coordinate for PSP texture cache"""
        # PSP swizzling algorithm for optimal cache usage
        return ((y & 0xF) << 4) | (x & 0xF) | ((y & 0x30) << 6) | ((x & 0x30) << 2) | ((y & 0xC0) << 10) | ((x & 0xC0) << 6)
    
    def process_models(self, model_dir):
        """Process 3D models for PSP"""
        if not os.path.exists(model_dir):
            return
            
        print("Processing 3D models...")
        
        for filename in os.listdir(model_dir):
            if filename.lower().endswith(('.obj', '.fbx', '.dae')):
                model_path = os.path.join(model_dir, filename)
                self.convert_model(model_path, filename)
    
    def convert_model(self, model_path, filename):
        """Convert 3D model to PSP format"""
        # Use Assimp to load model
        try:
            import pyassimp
        except ImportError:
            print("Warning: pyassimp not available, skipping 3D models")
            return
        
        scene = pyassimp.load(model_path)
        
        for mesh_idx, mesh in enumerate(scene.meshes):
            mesh_filename = f"{os.path.splitext(filename)[0]}_{mesh_idx}.pmesh"
            mesh_path = os.path.join(self.output_dir, mesh_filename)
            
            with open(mesh_path, 'wb') as f:
                # PMESH header
                f.write(b'PMESH')  # Magic
                f.write(struct.pack('<I', 1))  # Version
                f.write(struct.pack('<I', len(mesh.vertices)))  # Vertex count
                f.write(struct.pack('<I', len(mesh.faces)))     # Face count
                
                # Vertex data (position, normal, texcoord)
                for vertex in mesh.vertices:
                    f.write(struct.pack('<fff', vertex[0], vertex[1], vertex[2]))
                
                for normal in mesh.normals:
                    f.write(struct.pack('<fff', normal[0], normal[1], normal[2]))
                
                if mesh.texturecoords[0] is not None:
                    for texcoord in mesh.texturecoords[0]:
                        f.write(struct.pack('<ff', texcoord[0], texcoord[1]))
                else:
                    # Default texture coordinates
                    for _ in range(len(mesh.vertices)):
                        f.write(struct.pack('<ff', 0.0, 0.0))
                
                # Index data
                for face in mesh.faces:
                    if len(face.indices) == 3:  # Triangle
                        f.write(struct.pack('<HHH', face.indices[0], face.indices[1], face.indices[2]))
                    else:
                        print(f"Warning: Non-triangle face in {filename}, skipping")
        
        pyassimp.release(scene)
    
    def process_audio(self, audio_dir):
        """Process audio files for PSP"""
        if not os.path.exists(audio_dir):
            return
            
        print("Processing audio...")
        
        for filename in os.listdir(audio_dir):
            if filename.lower().endswith(('.wav', '.mp3', '.ogg')):
                audio_path = os.path.join(audio_dir, filename)
                self.convert_audio(audio_path, filename)
    
    def convert_audio(self, audio_path, filename):
        """Convert audio to PSP-optimized format"""
        # Convert to WAV first if needed
        if not filename.lower().endswith('.wav'):
            wav_path = '/tmp/temp_audio.wav'
            subprocess.run(['ffmpeg', '-i', audio_path, '-ar', '44100', '-ac', '2', '-y', wav_path], 
                         check=True, capture_output=True)
            audio_path = wav_path
        
        with wave.open(audio_path, 'rb') as wav:
            frames = wav.readframes(wav.getnframes())
            sample_rate = wav.getframerate()
            channels = wav.getnchannels()
            sample_width = wav.getsampwidth()
        
        # Ensure 16-bit stereo 44.1kHz
        if sample_rate != 44100 or channels != 2 or sample_width != 2:
            # Convert using ffmpeg
            converted_path = '/tmp/converted_audio.wav'
            subprocess.run(['ffmpeg', '-i', audio_path, '-ar', '44100', '-ac', '2', '-sample_fmt', 's16', '-y', converted_path],
                         check=True, capture_output=True)
            
            with wave.open(converted_path, 'rb') as wav:
                frames = wav.readframes(wav.getnframes())
        
        # Compress audio data
        compressed_frames = gzip.compress(frames)
        
        # Save as PSP audio format
        audio_filename = os.path.splitext(filename)[0] + '.paudio'
        audio_path = os.path.join(self.output_dir, audio_filename)
        
        with open(audio_path, 'wb') as f:
            f.write(b'PAUDIO')  # Magic
            f.write(struct.pack('<I', 1))  # Version
            f.write(struct.pack('<I', 44100))  # Sample rate
            f.write(struct.pack('<H', 2))  # Channels
            f.write(struct.pack('<H', 16))  # Bits per sample
            f.write(struct.pack('<I', len(frames)))  # Uncompressed size
            f.write(struct.pack('<I', len(compressed_frames)))  # Compressed size
            f.write(compressed_frames)
    
    def next_power_of_2(self, n):
        """Find next power of 2 greater than or equal to n"""
        power = 1
        while power < n:
            power *= 2
        return power
    
    def generate_manifest(self):
        """Generate asset manifest for runtime loading"""
        manifest = {
            'version': 1,
            'textures': [],
            'models': [],
            'audio': [],
            'total_size': 0
        }
        
        # Scan generated assets
        for filename in os.listdir(self.output_dir):
            file_path = os.path.join(self.output_dir, filename)
            file_size = os.path.getsize(file_path)
            manifest['total_size'] += file_size
            
            if filename.endswith('.ptex'):
                manifest['textures'].append({
                    'filename': filename,
                    'size': file_size
                })
            elif filename.endswith('.pmesh'):
                manifest['models'].append({
                    'filename': filename,
                    'size': file_size
                })
            elif filename.endswith('.paudio'):
                manifest['audio'].append({
                    'filename': filename,
                    'size': file_size
                })
        
        manifest_path = os.path.join(self.output_dir, 'manifest.json')
        with open(manifest_path, 'w') as f:
            json.dump(manifest, f, indent=2)
        
        print(f"Asset manifest generated: {len(manifest['textures'])} textures, "
              f"{len(manifest['models'])} models, {len(manifest['audio'])} audio files")
        print(f"Total asset size: {manifest['total_size'] / 1024 / 1024:.1f} MB")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python asset_processor.py <input_dir> <output_dir>")
        sys.exit(1)
    
    processor = PSPAssetProcessor(sys.argv[1], sys.argv[2])
    processor.process_all_assets()
```

---

## Performance Benchmarks and Targets

### PSP Performance Characteristics

```
╭─────────────────────────────────────────────────────────╮
│                PWGE Performance Targets                 │
├─────────────────────────────────────────────────────────┤
│ Frame Rate: 60 FPS (16.67ms per frame)                 │
│ WASM JIT Execution: 8ms budget per frame               │
│ Graphics Rendering: 6ms budget per frame               │
│ Audio Processing: 1.5ms budget per frame               │
│ Network/System: 1.17ms budget per frame                │
├─────────────────────────────────────────────────────────┤
│ Memory Usage:                                           │
│   PWGE Runtime: 2MB                                    │
│   JIT Code Cache: 4MB                                  │
│   WASM Components: 16MB total                          │
│   Asset Cache: 4MB                                     │
│   Audio Buffers: 2MB                                   │
│   Video Memory: 2MB (textures + framebuffers)          │
├─────────────────────────────────────────────────────────┤
│ WASM Performance:                                       │
│   JIT Instructions/sec: ~50M (native speed)            │
│   Interpreter fallback: ~15M instructions/sec          │
│   Function calls/frame: 50,000                         │
│   Memory allocations/frame: 500                        │
│   Component switches/frame: 16                         │
├─────────────────────────────────────────────────────────┤
│ Graphics Performance:                                   │
│   Triangles/sec: 2M (hardware accelerated)             │
│   Textured triangles/sec: 1M                           │
│   2D Sprites/frame: 5,000                              │
│   Texture memory bandwidth: 200 MB/s                   │
├─────────────────────────────────────────────────────────┤
│ Network Performance:                                    │
│   WiFi throughput: 1-2 Mbps (802.11b)                  │
│   Latency: 50-200ms (typical)                          │
│   Packets/sec: 100-500                                 │
│   Concurrent connections: 8 players max                │
╰─────────────────────────────────────────────────────────╯
```

### Comprehensive Benchmark Suite

```c
// tools/benchmark/pwge_benchmarks.c

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <psprtc.h>
#include <string.h>

typedef struct {
    const char* name;
    uint64_t (*benchmark_func)(void);
    uint64_t target_time_us;
    uint64_t actual_time_us;
    bool passed;
    const char* description;
} BenchmarkTest;

// WASM JIT compilation benchmark
uint64_t benchmark_jit_compilation() {
    // Test compilation of various WASM instruction patterns
    uint8_t test_wasm[] = {
        // Complex arithmetic and control flow
        0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,  // WASM header
        // ... test bytecode with loops, function calls, etc.
    };
    
    JITCompiler jit;
    pwge_jit_init(&jit);
    
    uint64_t start_time;
    sceRtcGetCurrentTick(&start_time);
    
    // Compile test module
    for (int i = 0; i < 100; i++) {
        JITCompiledFunction* func = pwge_jit_compile_function(&jit, test_wasm, sizeof(test_wasm));
        pwge_jit_free_function(func);
    }
    
    uint64_t end_time;
    sceRtcGetCurrentTick(&end_time);
    
    pwge_jit_cleanup(&jit);
    return end_time - start_time;
}

// WASM execution performance benchmark
uint64_t benchmark_wasm_execution() {
    // Test execution of compiled WASM code
    JITCompiler jit;
    pwge_jit_init(&jit);
    
    // Compile arithmetic-heavy function
    uint8_t arithmetic_wasm[] = {
        // Function that does 1000 iterations of: result = (a * b + c) / d
        // Tests JIT quality for common game math operations
    };
    
    JITCompiledFunction* func = pwge_jit_compile_function(&jit, arithmetic_wasm, sizeof(arithmetic_wasm));
    
    uint64_t start_time;
    sceRtcGetCurrentTick(&start_time);
    
    // Execute 10,000 times
    for (int i = 0; i < 10000; i++) {
        pwge_jit_execute_function(func, NULL, 0);
    }
    
    uint64_t end_time;
    sceRtcGetCurrentTick(&end_time);
    
    pwge_jit_free_function(func);
    pwge_jit_cleanup(&jit);
    
    return end_time - start_time;
}

// 3D graphics rendering benchmark
uint# PSP WASM Game Engine (PWGE)
## Flight-Inspired WebAssembly Runtime for Sony PlayStation Portable

**Project Codename:** "Flight Portable"  
**Target Platform:** Sony PSP (MIPS R4000, 32MB RAM, GPU)  
**Design Philosophy:** High-performance component architecture meets portable gaming power  
**Created:** May 31, 2025

---

## Executive Summary

The PSP WASM Game Engine (PWGE) revolutionizes portable game development by bringing modern WebAssembly-based component architecture to the Sony PlayStation Portable. Inspired by the Flight application host, PWGE provides a sophisticated runtime where game components are written in modern languages (Rust, C++, AssemblyScript) and compiled to WebAssembly, then executed with near-native performance on PSP hardware.

### Key Innovation
- **First production WASM runtime for handheld gaming devices**
- **Component hot-swapping during gameplay**
- **Modern development workflow targeting PSP homebrew**
- **JIT compilation for MIPS architecture**
- **Advanced networking capabilities via WiFi**

---

## Technical Architecture

### System Overview

```
┌───────────────────────────────────────────────────────────────┐
│                    Game Application Layer                     │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────────┐  │
│  │   Player    │ │  Network    │ │     Rendering           │  │
│  │ Controller  │ │   System    │ │     Engine              │  │
│  │ (WASM)      │ │  (WASM)     │ │     (WASM)              │  │
│  └─────────────┘ └─────────────┘ └─────────────────────────┘  │
├───────────────────────────────────────────────────────────────┤
│                    PWGE Runtime Core                          │
│  ┌───────────────┐ ┌─────────────┐ ┌───────────────────────┐  │
│  │ WASM JIT      │ │ Component   │ │    Memory Manager     │  │
│  │ Compiler      │ │ Scheduler   │ │    (32MB pool)        │  │
│  │ (MIPS)        │ │             │ │                       │  │
│  └───────────────┘ └─────────────┘ └───────────────────────┘  │
├───────────────────────────────────────────────────────────────┤
│                    PSP Hardware Layer                         │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ │
│  │  MIPS   │ │   GPU   │ │  Audio  │ │  WiFi   │ │ Memory  │ │
│  │ R4000   │ │ (3D/2D) │ │  DSP    │ │ 802.11b │ │ Stick   │ │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └─────────┘ │
└───────────────────────────────────────────────────────────────┘
```

### Memory Architecture

**Total Available:** 32MB Main RAM + 2MB VRAM  
**Allocation Strategy:**
```
┌─────────────────────────────────────────────────────────┐
│                  32MB Main RAM                          │
├─────────────────────────────────────────────────────────┤
│  PSP System/Kernel (4MB)                               │
├─────────────────────────────────────────────────────────┤
│  PWGE Runtime (2MB)                                    │
├─────────────────────────────────────────────────────────┤
│  JIT Code Cache (4MB)                                  │
├─────────────────────────────────────────────────────────┤
│  WASM Linear Memory Pool (16MB)                        │
│  ├─ Component A Memory (4MB max)                       │
│  ├─ Component B Memory (4MB max)                       │
│  ├─ Component C Memory (4MB max)                       │
│  └─ Shared Memory Pool (4MB)                           │
├─────────────────────────────────────────────────────────┤
│  Audio Buffers (2MB)                                   │
├─────────────────────────────────────────────────────────┤
│  Asset Cache (4MB)                                     │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│                   2MB Video RAM                         │
├─────────────────────────────────────────────────────────┤
│  Front Buffer 480x272x32 (512KB)                       │
├─────────────────────────────────────────────────────────┤
│  Back Buffer 480x272x32 (512KB)                        │
├─────────────────────────────────────────────────────────┤
│  Texture Memory (1MB)                                  │
│  ├─ Dynamic Textures (512KB)                           │
│  └─ Font/UI Textures (512KB)                           │
└─────────────────────────────────────────────────────────┘
```

---

## WASM Runtime Implementation

### JIT Compiler Architecture

```c
// pwge-jit/include/pwge_jit.h

typedef struct {
    uint32_t* mips_code;        // Generated MIPS assembly
    uint32_t code_size;         // Size in bytes
    uint32_t* branch_table;     // Jump targets for WASM branches
    uint32_t optimization_level; // 0-3 optimization
    bool hot_code;              // Frequently executed code
} JITCompiledFunction;

typedef struct {
    WasmModule* module;
    JITCompiledFunction* functions;
    uint32_t function_count;
    uint8_t* code_cache;        // 4MB MIPS code cache
    uint32_t cache_used;
    uint32_t cache_size;
    bool compilation_enabled;
} JITCompiler;

// MIPS-specific optimizations
typedef struct {
    uint32_t instruction_count;
    uint32_t cycles_estimated;
    uint32_t memory_accesses;
    uint32_t branch_count;
    bool uses_float;
    bool uses_multiply;
} CodeAnalysis;
```

### High-Performance JIT Implementation

```c
// pwge-jit/src/mips_codegen.c

// WASM i32.add -> MIPS addu optimization
static void emit_i32_add(JITCompiler* jit, uint32_t** code_ptr) {
    uint32_t* code = *code_ptr;
    
    // Pop two values from WASM stack (in MIPS registers)
    // Assume: $t0 = second operand, $t1 = first operand
    
    // MIPS: addu $t2, $t1, $t0  (unsigned add, no overflow trap)
    *code++ = 0x012a5021;  // addu $t2, $t1, $t0
    
    // Push result back to WASM stack
    // Store $t2 as new top of stack
    
    *code_ptr = code;
}

// Optimized WASM memory access with bounds checking
static void emit_i32_load(JITCompiler* jit, uint32_t** code_ptr, 
                          uint32_t offset, uint32_t align) {
    uint32_t* code = *code_ptr;
    
    // $t0 contains memory address from WASM stack
    // Add immediate offset
    if (offset > 0) {
        // addiu $t0, $t0, offset
        *code++ = 0x25080000 | (offset & 0xFFFF);
    }
    
    // Bounds check (compare with memory size)
    // sltu $t1, $t0, $t3  ($t3 contains memory size)
    *code++ = 0x010b482b;
    
    // Branch if out of bounds (trap)
    // beq $t1, $zero, trap_handler
    *code++ = 0x11200000 | ((uint32_t)trap_handler & 0xFFFF);
    
    // Load word from memory
    // lw $t2, 0($t0)
    *code++ = 0x8d0a0000;
    
    *code_ptr = code;
}

// MIPS branch optimization with delay slots
static void emit_br_if(JITCompiler* jit, uint32_t** code_ptr, uint32_t target) {
    uint32_t* code = *code_ptr;
    
    // Pop condition from stack ($t0)
    // Branch if not equal to zero
    // bne $t0, $zero, target
    *code++ = 0x15000000 | ((target >> 2) & 0xFFFF);
    
    // Delay slot - can optimize with useful instruction
    *code++ = 0x00000000;  // nop (could be optimized)
    
    *code_ptr = code;
}

// Register allocation for MIPS
typedef struct {
    uint8_t gpr_usage[32];      // General purpose registers
    uint8_t fpr_usage[32];      // Floating point registers
    uint32_t stack_slots[64];   // Virtual WASM stack mapping
    uint8_t current_stack_depth;
} RegisterAllocator;

static uint8_t allocate_gpr(RegisterAllocator* alloc) {
    // Find free general purpose register
    // Skip $zero, $at, $k0, $k1, $sp, $gp, $fp, $ra
    for (int i = 8; i <= 25; i++) {  // $t0-$t9, $s0-$s7
        if (alloc->gpr_usage[i] == 0) {
            alloc->gpr_usage[i] = 1;
            return i;
        }
    }
    
    // Spill to memory if no registers available
    return spill_register(alloc);
}

// Advanced optimization passes
void optimize_mips_code(JITCompiledFunction* func) {
    // Peephole optimizations
    eliminate_redundant_moves(func);
    combine_memory_operations(func);
    optimize_branch_patterns(func);
    
    // MIPS-specific optimizations
    utilize_delay_slots(func);
    optimize_load_store_sequences(func);
    reduce_pipeline_stalls(func);
}
```

### Component System for PSP

```c
// pwge-core/component_system.h

typedef struct {
    uint32_t component_id;
    char name[64];
    WasmModule* module;
    JITCompiledFunction* compiled_functions;
    uint8_t* linear_memory;
    uint32_t memory_size;
    uint32_t memory_max;
    ComponentPriority priority;
    ComponentState state;
    uint64_t cycles_used;
    uint64_t cycles_budget;
    uint32_t hot_reload_version;
    bool network_enabled;
    bool graphics_enabled;
    bool audio_enabled;
} PWGEComponent;

typedef enum {
    PRIORITY_CRITICAL = 0,  // Network, input
    PRIORITY_HIGH = 1,      // Game logic, physics
    PRIORITY_MEDIUM = 2,    // Audio, effects
    PRIORITY_LOW = 3,       // Background tasks, UI
    PRIORITY_IDLE = 4       // Cleanup, profiling
} ComponentPriority;

typedef struct {
    PWGEComponent components[16];
    uint32_t component_count;
    uint32_t active_components;
    
    // Scheduling
    uint64_t frame_start_time;
    uint64_t frame_target_time;  // 16.67ms for 60fps or 33.33ms for 30fps
    uint32_t total_cycles_budget;
    
    // Performance monitoring
    uint32_t frame_drops;
    uint32_t average_frame_time;
    uint32_t peak_memory_usage;
    
    // Hot reload support
    bool hot_reload_enabled;
    char* hot_reload_path;
    uint32_t file_watch_fd;
} ComponentScheduler;
```

### PSP-Specific Graphics Integration

```c
// pwge-graphics/gu_interface.c

#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>

typedef struct {
    void* display_list;
    uint32_t* frame_buffer;
    uint32_t* z_buffer;
    uint32_t texture_cache_size;
    bool vsync_enabled;
    uint32_t render_width;
    uint32_t render_height;
    float aspect_ratio;
} PWGEGraphicsContext;

// Optimized sprite rendering using GU (Graphics Unit)
void pwge_gu_draw_sprite(uint32_t texture_id, float x, float y, float scale, float rotation) {
    PWGETexture* tex = &texture_cache[texture_id];
    
    // Calculate sprite vertices
    float half_width = (tex->width * scale) * 0.5f;
    float half_height = (tex->height * scale) * 0.5f;
    
    // Create vertex array (PSP uses triangle strips)
    SpriteVertex vertices[4] = {
        // Top-left
        {.u = 0, .v = 0, .x = x - half_width, .y = y - half_height, .z = 0},
        // Top-right  
        {.u = tex->width, .v = 0, .x = x + half_width, .y = y - half_height, .z = 0},
        // Bottom-left
        {.u = 0, .v = tex->height, .x = x - half_width, .y = y + half_height, .z = 0},
        // Bottom-right
        {.u = tex->width, .v = tex->height, .x = x + half_width, .y = y + half_height, .z = 0}
    };
    
    // Apply rotation if needed
    if (rotation != 0.0f) {
        apply_rotation_to_vertices(vertices, 4, x, y, rotation);
    }
    
    // Submit to GPU
    sceGuTexMode(GU_PSM_5650, 0, 0, 0);  // RGB565 texture format
    sceGuTexImage(0, tex->width, tex->height, tex->width, tex->data);
    sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
    sceGuTexFilter(GU_LINEAR, GU_LINEAR);
    
    sceGumDrawArray(GU_TRIANGLE_STRIP, 
                    GU_TEXTURE_16BIT | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
                    4, 0, vertices);
}

// Advanced 3D rendering support
void pwge_gu_draw_mesh(uint32_t mesh_id, Matrix4x4* transform) {
    PWGEMesh* mesh = &mesh_cache[mesh_id];
    
    // Set up transformation matrices
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadMatrix(transform);
    
    // Configure materials and lighting
    if (mesh->material.texture_id != INVALID_TEXTURE) {
        sceGuEnable(GU_TEXTURE_2D);
        pwge_bind_texture(mesh->material.texture_id);
    }
    
    if (mesh->material.lighting_enabled) {
        sceGuEnable(GU_LIGHTING);
        sceGuLightMode(GU_SINGLE_COLOR);
    }
    
    // Render mesh
    sceGumDrawArray(mesh->primitive_type,
                    mesh->vertex_format,
                    mesh->vertex_count,
                    mesh->index_buffer,
                    mesh->vertex_buffer);
}

// Efficient texture loading with PSP formats
uint32_t pwge_gu_load_texture(const uint8_t* data, uint32_t len, TextureFormat format) {
    uint32_t texture_id = pwge_allocate_texture_id();
    PWGETexture* tex = &texture_cache[texture_id];
    
    // Parse texture header
    tex->width = *(uint32_t*)data;
    tex->height = *(uint32_t*)(data + 4);
    const uint8_t* pixel_data = data + 8;
    
    // Ensure power-of-2 dimensions for optimal performance
    uint32_t aligned_width = next_power_of_2(tex->width);
    uint32_t aligned_height = next_power_of_2(tex->height);
    
    // Allocate VRAM if possible, otherwise main RAM
    uint32_t texture_size = aligned_width * aligned_height * 2;  // 16-bit
    tex->data = sceGuGetMemory(texture_size);
    
    if (!tex->data) {
        // Fall back to main RAM
        tex->data = malloc(texture_size);
        tex->in_vram = false;
    } else {
        tex->in_vram = true;
    }
    
    // Convert and copy pixel data
    convert_to_psp_format(pixel_data, tex->data, tex->width, tex->height, format);
    
    // Apply swizzling for better cache performance
    if (aligned_width >= 16 && aligned_height >= 16) {
        swizzle_texture(tex->data, aligned_width, aligned_height);
        tex->swizzled = true;
    }
    
    return texture_id;
}

// PSP-specific texture swizzling for cache optimization
void swizzle_texture(void* data, uint32_t width, uint32_t height) {
    // PSP cache works best with swizzled textures
    // Complex bit manipulation to rearrange texture data
    // for optimal memory access patterns
    
    uint16_t* src = (uint16_t*)data;
    uint16_t* temp = malloc(width * height * 2);
    
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t swizzled_offset = swizzle_coordinate(x, y, width);
            temp[swizzled_offset] = src[y * width + x];
        }
    }
    
    memcpy(data, temp, width * height * 2);
    free(temp);
}
```

### Advanced Audio System

```c
// pwge-audio/audio_engine.c

#include <pspaudio.h>
#include <pspaudiolib.h>

typedef struct {
    int16_t* sample_data;
    uint32_t sample_count;
    uint32_t sample_rate;
    uint8_t channels;
    bool looping;
    float volume;
    float pitch;
    uint32_t current_position;
    bool active;
} PWGEAudioVoice;

typedef struct {
    PWGEAudioVoice voices[32];
    int16_t* mix_buffer;
    uint32_t buffer_size;
    uint32_t sample_rate;
    uint8_t master_volume;
    bool reverb_enabled;
    bool surround_enabled;
} PWGEAudioEngine;

static PWGEAudioEngine audio_engine;

// High-quality audio mixing with interpolation
void pwge_audio_mix_callback(void* buf, unsigned long reqn, void* pdata) {
    int16_t* output = (int16_t*)buf;
    int16_t* mix_buffer = audio_engine.mix_buffer;
    
    // Clear mix buffer
    memset(mix_buffer, 0, reqn * 2 * sizeof(int16_t));
    
    // Mix all active voices
    for (int i = 0; i < 32; i++) {
        PWGEAudioVoice* voice = &audio_engine.voices[i];
        
        if (!voice->active || !voice->sample_data) continue;
        
        // High-quality resampling using linear interpolation
        for (uint32_t j = 0; j < reqn; j++) {
            if (voice->current_position >= voice->sample_count) {
                if (voice->looping) {
                    voice->current_position = 0;
                } else {
                    voice->active = false;
                    break;
                }
            }
            
            // Linear interpolation for pitch shifting
            float sample_pos = voice->current_position;
            uint32_t sample_index = (uint32_t)sample_pos;
            float fraction = sample_pos - sample_index;
            
            int16_t sample1 = voice->sample_data[sample_index];
            int16_t sample2 = (sample_index + 1 < voice->sample_count) 
                ? voice->sample_data[sample_index + 1] : 0;
            
            int16_t interpolated = (int16_t)(sample1 + fraction * (sample2 - sample1));
            
            // Apply volume and mix
            int32_t mixed_sample = (int32_t)(interpolated * voice->volume);
            
            // Stereo mixing (simple pan for now)
            mix_buffer[j * 2] += (int16_t)(mixed_sample);      // Left
            mix_buffer[j * 2 + 1] += (int16_t)(mixed_sample);  // Right
            
            voice->current_position += voice->pitch;
        }
    }
    
    // Apply master volume and copy to output
    for (uint32_t j = 0; j < reqn * 2; j++) {
        int32_t sample = (mix_buffer[j] * audio_engine.master_volume) / 255;
        
        // Clamp to prevent distortion
        if (sample > 32767) sample = 32767;
        if (sample < -32768) sample = -32768;
        
        output[j] = (int16_t)sample;
    }
}

// 3D positional audio
void pwge_audio_play_3d(uint32_t sound_id, Vector3* position, Vector3* listener_pos, 
                        Vector3* listener_forward) {
    PWGEAudioVoice* voice = pwge_find_free_voice();
    if (!voice) return;
    
    // Calculate distance and direction
    Vector3 diff = {
        position->x - listener_pos->x,
        position->y - listener_pos->y,
        position->z - listener_pos->z
    };
    
    float distance = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    
    // Distance attenuation
    float volume = 1.0f / (1.0f + distance * 0.1f);
    
    // Simple stereo panning based on left/right position
    float dot_product = diff.x * listener_forward->x + diff.z * listener_forward->z;
    float pan = dot_product / (distance + 0.001f);  // Avoid division by zero
    
    // Set up voice
    voice->sample_data = audio_clips[sound_id].data;
    voice->sample_count = audio_clips[sound_id].length;
    voice->volume = volume;
    voice->pitch = 1.0f;
    voice->current_position = 0;
    voice->active = true;
    
    // Apply 3D positioning (simplified)
    voice->pan = pan;
}

// Real-time audio effects
void apply_reverb_effect(int16_t* buffer, uint32_t length) {
    static int16_t delay_buffer[8192];
    static uint32_t delay_index = 0;
    
    for (uint32_t i = 0; i < length; i++) {
        // Simple reverb using delay line
        int16_t delayed = delay_buffer[delay_index];
        int32_t output = buffer[i] + (delayed * 3) / 10;  // 30% feedback
        
        if (output > 32767) output = 32767;
        if (output < -32768) output = -32768;
        
        delay_buffer[delay_index] = buffer[i];
        buffer[i] = (int16_t)output;
        
        delay_index = (delay_index + 1) % 8192;
    }
}
```

---

## Networking and Multiplayer

### WiFi Integration

```c
// pwge-network/wifi_manager.c

#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <pspwlan.h>

typedef struct {
    char ssid[64];
    uint8_t security_type;
    uint8_t signal_strength;
    bool connected;
} WiFiNetwork;

typedef struct {
    bool wifi_enabled;
    bool connected_to_ap;
    char current_ssid[64];
    uint32_t local_ip;
    uint16_t listen_port;
    
    // Game networking
    int game_socket;
    struct sockaddr_in server_addr;
    bool hosting_game;
    uint8_t player_count;
    uint32_t peer_ips[4];
    
    // Message queues
    NetworkMessage incoming_messages[256];
    NetworkMessage outgoing_messages[256];
    uint32_t incoming_head, incoming_tail;
    uint32_t outgoing_head, outgoing_tail;
} NetworkManager;

static NetworkManager net_manager;

// Initialize PSP networking
int pwge_network_init() {
    // Load network modules
    if (sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON) < 0) return -1;
    if (sceUtilityLoadNetModule(PSP_NET_MODULE_INET) < 0) return -1;
    
    // Initialize network
    if (sceNetInit(128*1024, 42, 4*1024, 42, 4*1024) < 0) return -1;
    if (sceNetInetInit() < 0) return -1;
    if (sceNetApctlInit(0x8000, 42) < 0) return -1;
    
    net_manager.wifi_enabled = true;
    net_manager.connected_to_ap = false;
    net_manager.listen_port = 31337;
    
    return 0;
}

// Connect to WiFi access point
int pwge_wifi_connect(const char* ssid, const char* password) {
    // Configure connection
    sceNetApctlConnect(1);  // Use connection config 1
    
    // Wait for connection
    int state = 0;
    while (state != 4) {  // PSP_NET_APCTL_STATE_GOT_IP
        sceNetApctlGetState(&state);
        sceKernelDelayThread(100000);  // 100ms
        
        if (state == 0) {  // Disconnected
            return -1;
        }
    }
    
    // Get IP address
    union SceNetApctlInfo info;
    sceNetApctlGetInfo(8, &info);  // PSP_NET_APCTL_INFO_IP
    net_manager.local_ip = inet_addr(info.ip);
    net_manager.connected_to_ap = true;
    
    strncpy(net_manager.current_ssid, ssid, 63);
    
    return 0;
}

// High-performance UDP networking for games
int pwge_network_start_host(uint16_t port) {
    if (!net_manager.connected_to_ap) return -1;
    
    // Create UDP socket
    net_manager.game_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (net_manager.game_socket < 0) return -1;
    
    // Set non-blocking
    int flags = 1;
    setsockopt(net_manager.game_socket, SOL_SOCKET, SO_NONBLOCK, &flags, sizeof(flags));
    
    // Bind to port
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(net_manager.game_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(net_manager.game_socket);
        return -1;
    }
    
    net_manager.hosting_game = true;
    net_manager.listen_port = port;
    
    return 0;
}

// Connect to game host
int pwge_network_connect_to_host(const char* host_ip, uint16_t port) {
    if (!net_manager.connected_to_ap) return -1;
    
    // Create UDP socket
    net_manager.game_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (net_manager.game_socket < 0) return -1;
    
    // Set non-blocking
    int flags = 1;
    setsockopt(net_manager.game_socket, SOL_SOCKET, SO_NONBLOCK, &flags, sizeof(flags));
    
    // Set server address
    net_manager.server_addr.sin_family = AF_INET;
    net_manager.server_addr.sin_port = htons(port);
    inet_aton(host_ip, &net_manager.server_addr.sin_addr);
    
    net_manager.hosting_game = false;
    
    // Send connection request
    NetworkMessage msg;
    msg.type = MSG_CONNECT_REQUEST;
    msg.player_id = 0;  // Will be assigned by host
    pwge_network_send_message(&msg, &net_manager.server_addr);
    
    return 0;
}

// Process network messages (called each frame)
void pwge_network_update() {
    if (!net_manager.connected_to_ap || net_manager.game_socket < 0) return;
    
    // Receive messages
    struct sockaddr_in from_addr;
    socklen_t addr_len = sizeof(from_addr);
    NetworkMessage msg;
    
    while (true) {
        int bytes = recvfrom(net_manager.game_socket, &msg, sizeof(msg), 0,
                            (struct sockaddr*)&from_addr, &addr_len);
        
        if (bytes <= 0) break;  // No more messages
        
        if (bytes == sizeof(NetworkMessage)) {
            // Add to incoming queue
            uint32_t next_head = (net_manager.incoming_head + 1) % 256;
            if (next_head != net_manager.incoming_tail) {
                net_manager.incoming_messages[net_manager.incoming_head] = msg;
                net_manager.incoming_messages[net_manager.incoming_head].sender_ip = from_addr.sin_addr.s_addr;
                net_manager.incoming_head = next_head;
            }
        }
    }
    
    // Send outgoing messages
    while (net_manager.outgoing_head != net_manager.outgoing_tail) {
        NetworkMessage* msg = &net_manager.outgoing_messages[net_manager.outgoing_tail];
        
        if (net_manager.hosting_game) {
            // Broadcast to all connected peers
            for (int i = 0; i < net_manager.player_count; i++) {
                if (net_manager.peer_ips[i] != 0) {
                    struct sockaddr_in peer_addr;
                    peer_addr.sin_family = AF_INET;
                    peer_addr.sin_port = htons(net_manager.listen_port);
                    peer_addr.sin_addr.s_addr = net_manager.peer_ips[i];
                    
                    sendto(net_manager.game_socket, msg, sizeof(*msg), 0,
                           (struct sockaddr*)&peer_addr, sizeof(peer_addr));
                }
            }
        } else {
            // Send to host
            sendto(net_manager.game_socket, msg, sizeof(*msg), 0,
                   (struct sockaddr*)&net_manager.server_addr, sizeof(net_manager.server_addr));
        }
        
        net_manager.outgoing_tail = (net_manager.outgoing_tail + 1) % 256;
    }
}

// Game-specific networking messages
typedef enum {
    MSG_CONNECT_REQUEST = 1,
    MSG_CONNECT_RESPONSE = 2,
    MSG_PLAYER_INPUT = 3,
    MSG_GAME_STATE = 4,
    MSG_PLAYER_DISCONNECT = 5,
    MSG_GAME_EVENT = 6
} NetworkMessageType;

typedef struct {
    NetworkMessageType type;
    uint8_t player_id;
    uint32_t sequence;
    uint32_t timestamp;
    uint32_t sender_ip;
    
    union {
        struct {
            char player_name[32];
            uint8_t version;
        } connect_request;
        
        struct {
            uint8_t assigned_id;
            uint8_t player_count;
            uint32_t game_seed;
        } connect_response;
        
        struct {
            uint32_t buttons;
            int8_t analog_x, analog_y;
            uint32_t frame_number;
        } player_input;
        
        struct {
            uint8_t data[200];  // Compressed game state
            uint32_t checksum;
        } game_state;
        
        struct {
            uint32_t event_type;
            uint8_t data[64];
        } game_event;
    };
} NetworkMessage;

// Lag compensation and prediction
typedef struct {
    uint32_t frame_number;
    uint32_t timestamp;
    PlayerInput inputs[4];  // All players
    uint32_t checksum;
} InputFrame;

typedef struct {
    InputFrame frames[120];  // 2 seconds at 60fps
    uint32_t current_frame;
    uint32_t confirmed_frame;
    uint32_t prediction_frames;
} InputHistory;

static InputHistory input_history;

// Rollback networking for fighting games/precise timing
void pwge_network_rollback_to_frame(uint32_t frame) {
    if (frame >= input_history.current_frame) return;
    
    // Restore game state to this frame
    pwge_restore_game_state(frame);
    
    // Re-simulate forward with confirmed inputs
    for (uint32_t f = frame; f < input_history.current_frame; f++) {
        InputFrame* input_frame = &input_history.frames[f % 120];
        pwge_simulate_game_frame(input_frame);
    }
}
```

---

## Component Development API

### Rust API for PSP Components

```rust
// pwge-api/src/lib.rs

#[repr(C)]
pub struct ComponentContext {
    pub component_id: u32,
    pub delta_time: f32,
    pub frame_count: u64,
    pub memory_available: u32,
    pub cycles_remaining: u64,
    pub psp_clock_speed: u32,
    pub battery_level: u8,
    pub wifi_connected: bool,
}

#[repr(C)]
pub struct InputState {
    pub buttons: u32,
    pub analog_x: i8,
    pub analog_y: i8,
    pub trigger_l: bool,
    pub trigger_r: bool,
    pub volume_up: bool,
    pub volume_down: bool,
    pub brightness_up: bool,
    pub brightness_down: bool,
    pub wifi_switch: bool,
    pub hold_switch: bool,
}

#[repr(C)]
pub struct Vector3 {
    pub x: f32,
    pub y: f32,
    pub z: f32,
}

#[repr(C)]
pub struct Matrix4x4 {
    pub m: [[f32; 4]; 4],
}

#[repr(C)]
pub struct NetworkMessage {
    pub msg_type: u32,
    pub player_id: u8,
    pub data_len: u32,
    pub data: [u8; 256],
}

// Component lifecycle traits
pub trait PWGEComponent {
    fn init(&mut self, ctx: &ComponentContext) -> Result<(), ComponentError>;
    fn update(&mut self, ctx: &ComponentContext, input: &InputState) -> Result<(), ComponentError>;
    fn render(&mut self, ctx: &ComponentContext) -> Result<(), ComponentError>;
    fn network_message(&mut self, ctx: &ComponentContext, msg: &NetworkMessage) -> Result<(), ComponentError>;
    fn suspend(&mut self, ctx: &ComponentContext) -> Result<(), ComponentError>;
    fn resume(&mut self, ctx: &ComponentContext) -> Result<(), ComponentError>;
    fn cleanup(&mut self, ctx: &ComponentContext);
}

// Host function imports - provided by PWGE runtime
extern "C" {
    // Graphics API
    fn pwge_draw_sprite(texture_id: u32, x: f32, y: f32, scale: f32, rotation: f32) -> i32;
    fn pwge_draw_sprite_3d(texture_id: u32, transform: *const Matrix4x4) -> i32;
    fn pwge_draw_mesh(mesh_id: u32, transform: *const Matrix4x4) -> i32;
    fn pwge_load_texture(data: *const u8, len: u32) -> u32;
    fn pwge_load_mesh(data: *const u8, len: u32) -> u32;
    fn pwge_set_camera(position: *const Vector3, target: *const Vector3, up: *const Vector3) -> i32;
    fn pwge_set_projection(fov: f32, aspect: f32, near: f32, far: f32) -> i32;
    fn pwge_enable_lighting(enabled: bool) -> i32;
    fn pwge_set_light(light_id: u32, position: *const Vector3, color: u32) -> i32;
    
    // Audio API
    fn pwge_play_sound(sound_id: u32, volume: f32, pitch: f32) -> i32;
    fn pwge_play_sound_3d(sound_id: u32, position: *const Vector3) -> i32;
    fn pwge_play_music(music_id: u32, loop_mode: bool, fade_in_ms: u32) -> i32;
    fn pwge_stop_music(fade_out_ms: u32) -> i32;
    fn pwge_load_sound(data: *const u8, len: u32) -> u32;
    fn pwge_load_music(data: *const u8, len: u32) -> u32;
    fn pwge_set_master_volume(volume: f32) -> i32;
    fn pwge_enable_reverb(enabled: bool) -> i32;
    
    // Network API
    fn pwge_network_send_message(msg: *const NetworkMessage) -> i32;
    fn pwge_network_broadcast_message(msg: *const NetworkMessage) -> i32;
    fn pwge_network_get_player_count() -> u32;
    fn pwge_network_get_local_player_id() -> u32;
    fn pwge_network_is_host() -> bool;
    fn pwge_network_get_ping(player_id: u32) -> u32;
    
    // Storage API (Memory Stick)
    fn pwge_save_data(filename: *const u8, data: *const u8, len: u32) -> i32;
    fn pwge_load_data(filename: *const u8, buffer: *mut u8, max_len: u32) -> i32;
    fn pwge_delete_save(filename: *const u8) -> i32;
    fn pwge_list_saves(buffer: *mut u8, max_len: u32) -> u32;
    
    // System API
    fn pwge_get_battery_level() -> u8;
    fn pwge_get_cpu_clock() -> u32;
    fn pwge_set_cpu_clock(mhz: u32) -> i32;
    fn pwge_get_free_memory() -> u32;
    fn pwge_vibrate(duration_ms: u32) -> i32;
    fn pwge_screenshot(filename: *const u8) -> i32;
    
    // Debug API
    fn pwge_log(level: u32, message: *const u8, len: u32);
    fn pwge_get_performance_counters(counters: *mut PerformanceCounters);
    fn pwge_start_profiling(name: *const u8) -> u32;
    fn pwge_end_profiling(handle: u32) -> u64;
}

// Error types
#[repr(C)]
#[derive(Debug)]
pub enum ComponentError {
    Success = 0,
    InvalidParameter = 1,
    OutOfMemory = 2,
    ResourceNotFound = 3,
    NetworkError = 4,
    StorageError = 5,
    NotInitialized = 6,
    AlreadyInitialized = 7,
    Suspended = 8,
}

// Performance monitoring
#[repr(C)]
pub struct PerformanceCounters {
    pub frame_time_us: u64,
    pub cpu_usage_percent: f32,
    pub memory_used_bytes: u32,
    pub texture_memory_used_bytes: u32,
    pub audio_voices_active: u32,
    pub network_packets_sent: u32,
    pub network_packets_received: u32,
    pub jit_compilation_time_us: u64,
    pub gc_time_us: u64,
}

// Utility functions
pub fn create_transform_matrix(position: Vector3, rotation: Vector3, scale: Vector3) -> Matrix4x4 {
    // Create transformation matrix from position, rotation (Euler angles), and scale
    let mut matrix = Matrix4x4 { m: [[0.0; 4]; 4] };
    
    // Initialize to identity
    for i in 0..4 {
        matrix.m[i][i] = 1.0;
    }
    
    // Apply transformations (simplified - would use proper matrix math)
    matrix.m[0][3] = position.x;
    matrix.m[1][3] = position.y;
    matrix.m[2][3] = position.z;
    
    matrix
}

pub fn vector3_length(v: &Vector3) -> f32 {
    (v.x * v.x + v.y * v.y + v.z * v.z).sqrt()
}

pub fn vector3_normalize(v: &Vector3) -> Vector3 {
    let len = vector3_length(v);
    if len > 0.0001 {
        Vector3 {
            x: v.x / len,
            y: v.y / len,
            z: v.z / len,
        }
    } else {
        Vector3 { x: 0.0, y: 0.0, z: 0.0 }
    }
}

// PSP-specific constants
pub const PSP_BUTTON_TRIANGLE: u32 = 0x1000;
pub const PSP_BUTTON_SQUARE: u32 = 0x8000;
pub const PSP_BUTTON_CROSS: u32 = 0x4000;
pub const PSP_BUTTON_CIRCLE: u32 = 0x2000;
pub const PSP_BUTTON_L: u32 = 0x100;
pub const PSP_BUTTON_R: u32 = 0x200;
pub const PSP_BUTTON_DOWN: u32 = 0x40;
pub const PSP_BUTTON_LEFT: u32 = 0x80;
pub const PSP_BUTTON_UP: u32 = 0x10;
pub const PSP_BUTTON_RIGHT: u32 = 0x20;
pub const PSP_BUTTON_SELECT: u32 = 0x1;
pub const PSP_BUTTON_START: u32 = 0x8;
pub const PSP_BUTTON_HOME: u32 = 0x1000000;
pub const PSP_BUTTON_HOLD: u32 = 0x2000000;

// Screen dimensions
pub const PSP_SCREEN_WIDTH: u32 = 480;
pub const PSP_SCREEN_HEIGHT: u32 = 272;
pub const PSP_SCREEN_CENTER_X: f32 = 240.0;
pub const PSP_SCREEN_CENTER_Y: f32 = 136.0;
```

---

## Example Game Implementation

### 3D Racing Game Component

```rust
// examples/racing/car_physics/src/lib.rs

use pwge_api::*;

const GRAVITY: f32 = -9.81;
const AIR_DENSITY: f32 = 1.225;
const DRAG_COEFFICIENT: f32 = 0.3;

pub struct CarPhysicsComponent {
    // Car state
    position: Vector3,
    velocity: Vector3,
    angular_velocity: Vector3,
    orientation: Vector3,  // Euler angles
    
    // Car properties
    mass: f32,
    wheel_base: f32,
    front_axle_distance: f32,
    rear_axle_distance: f32,
    
    // Engine
    engine_power: f32,
    max_rpm: f32,
    current_rpm: f32,
    gear: i32,
    gear_ratios: [f32; 6],
    
    // Wheels
    wheel_positions: [Vector3; 4],  // FL, FR, RL, RR
    wheel_contacts: [bool; 4],
    wheel_grip: [f32; 4],
    steering_angle: f32,
    
    // Input
    throttle: f32,
    brake: f32,
    steering: f32,
    
    // Physics timestep
    physics_accumulator: f32,
    physics_timestep: f32,
    
    // 3D model
    car_mesh_id: u32,
    wheel_mesh_id: u32,
    
    // Audio
    engine_sound_id: u32,
    tire_sound_id: u32,
    brake_sound_id: u32,
}

impl CarPhysicsComponent {
    pub fn new() -> Self {
        Self {
            position: Vector3 { x: 0.0, y: 0.0, z: 0.0 },
            velocity: Vector3 { x: 0.0, y: 0.0, z: 0.0 },
            angular_velocity: Vector3 { x: 0.0, y: 0.0, z: 0.0 },
            orientation: Vector3 { x: 0.0, y: 0.0, z: 0.0 },
            
            mass: 1200.0,  // kg
            wheel_base: 2.5,  // meters
            front_axle_distance: 1.2,
            rear_axle_distance: 1.3,
            
            engine_power: 150000.0,  // watts
            max_rpm: 7000.0,
            current_rpm: 800.0,
            gear: 1,
            gear_ratios: [0.0, 3.5, 2.1, 1.4, 1.0, 0.8, 0.6],
            
            wheel_positions: [
                Vector3 { x: -0.6, y: 0.0, z: 1.2 },   // Front left
                Vector3 { x: 0.6, y: 0.0, z: 1.2 },    // Front right
                Vector3 { x: -0.6, y: 0.0, z: -1.3 },  // Rear left
                Vector3 { x: 0.6, y: 0.0, z: -1.3 },   // Rear right
            ],
            wheel_contacts: [false; 4],
            wheel_grip: [1.0; 4],
            steering_angle: 0.0,
            
            throttle: 0.0,
            brake: 0.0,
            steering: 0.0,
            
            physics_accumulator: 0.0,
            physics_timestep: 1.0 / 120.0,  // 120Hz physics
            
            car_mesh_id: 0,
            wheel_mesh_id: 0,
            
            engine_sound_id: 0,
            tire_sound_id: 0,
            brake_sound_id: 0,
        }
    }
    
    fn handle_input(&mut self, input: &InputState) {
        // Map PSP controls to car controls
        self.throttle = if (input.buttons & PSP_BUTTON_CROSS) != 0 { 1.0 } else { 0.0 };
        self.brake = if (input.buttons & PSP_BUTTON_SQUARE) != 0 { 1.0 } else { 0.0 };
        
        // Analog steering
        self.steering = input.analog_x as f32 / 128.0;
        self.steering_angle = self.steering * 30.0 * std::f32::consts::PI / 180.0;  // Max 30 degrees
        
        // Gear shifting
        if (input.buttons & PSP_BUTTON_R) != 0 && self.gear < 6 {
            self.gear += 1;
        }
        if (input.buttons & PSP_BUTTON_L) != 0 && self.gear > 1 {
            self.gear -= 1;
        }
    }
    
    fn update_physics(&mut self, delta_time: f32) {
        self.physics_accumulator += delta_time;
        
        while self.physics_accumulator >= self.physics_timestep {
            self.simulate_physics_step(self.physics_timestep);
            self.physics_accumulator -= self.physics_timestep;
        }
    }
    
    fn simulate_physics_step(&mut self, dt: f32) {
        // Update wheel contacts (simple ground detection)
        for i in 0..4 {
            let wheel_world_pos = self.transform_point(&self.wheel_positions[i]);
            self.wheel_contacts[i] = wheel_world_pos.y <= 0.1;  // Ground at y=0
        }
        
        // Engine simulation
        let gear_ratio = if self.gear > 0 { self.gear_ratios[self.gear as usize] } else { 0.0 };
        let engine_force = if gear_ratio > 0.0 {
            (self.engine_power * self.throttle / self.current_rpm.max(1.0)) * gear_ratio
        } else {
            0.0
        };
        
        // Apply forces
        let mut total_force = Vector3 { x: 0.0, y: 0.0, z: 0.0 };
        
        // Engine force (forward direction)
        let forward = self.get_forward_vector();
        total_force.x += forward.x * engine_force;
        total_force.z += forward.z * engine_force;
        
        // Aerodynamic drag
        let speed = vector3_length(&self.velocity);
        let drag_force = -0.5 * AIR_DENSITY * DRAG_COEFFICIENT * 2.5 * speed * speed;  // 2.5 m² frontal area
        let drag_direction = vector3_normalize(&self.velocity);
        total_force.x += drag_direction.x * drag_force;
        total_force.z += drag_direction.z * drag_force;
        
        // Gravity
        total_force.y += self.mass * GRAVITY;
        
        // Braking force
        if self.brake > 0.0 {
            let brake_force = -self.brake * 8000.0;  // N
            total_force.x += forward.x * brake_force;
            total_force.z += forward.z * brake_force;
        }
        
        // Update velocity and position
        let acceleration = Vector3 {
            x: total_force.x / self.mass,
            y: total_force.y / self.mass,
            z: total_force.z / self.mass,
        };
        
        self.velocity.x += acceleration.x * dt;
        self.velocity.y += acceleration.y * dt;
        self.velocity.z += acceleration.z * dt;
        
        self.position.x += self.velocity.x * dt;
        self.position.y += self.velocity.y * dt;
        self.position.z += self.velocity.z * dt;
        
        // Simple ground collision
        if self.position.y < 0.0 {
            self.position.y = 0.0;
            if self.velocity.y < 0.0 {
                self.velocity.y = 0.0;
            }
        }
        
        // Steering (simplified bicycle model)
        if self.steering_angle.abs() > 0.001 && speed > 1.0 {
            let angular_velocity = (speed / self.wheel_base) * self.steering_angle.tan();
            self.orientation.y += angular_velocity * dt;
            
            // Normalize angle
            while self.orientation.y > std::f32::consts::PI {
                self.orientation.y -= 2.0 * std::f32::consts::PI;
            }
            while self.orientation.y < -std::f32::consts::PI {
                self.orientation.y += 2.0 * std::f32::consts::PI;
            }
        }
        
        // Update RPM based on speed and gear
        if gear_ratio > 0.0 {
            let wheel_rpm = (speed * 60.0) / (2.0 * std::f32::consts::PI * 0.3);  // 0.3m wheel radius
            self.current_rpm = wheel_rpm * gear_ratio;
            self.current_rpm = self.current_rpm.clamp(800.0, self.max_rpm);
        }
    }
    
    fn get_forward_vector(&self) -> Vector3 {
        Vector3 {
            x: self.orientation.y.sin(),
            y: 0.0,
            z: self.orientation.y.cos(),
        }
    }
    
    fn transform_point(&self, local_point: &Vector3) -> Vector3 {
        // Simple transformation (would use proper matrix in real implementation)
        let cos_y = self.orientation.y.cos();
        let sin_y = self.orientation.y.sin();
        
        Vector3 {
            x: self.position.x + local_point.x * cos_y - local_point.z * sin_y,
            y: self.position.y + local_point.y,
            z: self.position.z + local_point.x * sin_y + local_point.z * cos_y,
        }
    }
    
    fn update_audio(&mut self) {
        unsafe {
            // Engine sound based on RPM
            let engine_pitch = 0.5 + (self.current_rpm / self.max_rpm) * 1.5;
            let engine_volume = 0.3 + self.throttle * 0.7;
            pwge_play_sound(self.engine_sound_id, engine_volume, engine_pitch);
            
            // Tire screeching
            let speed = vector3_length(&self.velocity);
            if self.brake > 0.5 && speed > 5.0 {
                pwge_play_sound(self.brake_sound_id, 0.6, 1.0);
            }
            
            if self.steering_angle.abs() > 0.5 && speed > 10.0 {
                let tire_volume = (self.steering_angle.abs() - 0.5) * 0.8;
                pwge_play_sound(self.tire_sound_id, tire_volume, 1.0);
            }
        }
    }
}

impl PWGEComponent for CarPhysicsComponent {
    fn init(&mut self, ctx: &ComponentContext) -> Result<(), ComponentError> {
        unsafe {
            pwge_log(LOG_INFO, b"Initializing Car Physics Component\0".as_ptr(), 33);
            
            // Load 3D models
            let car_model_data = include_bytes!("../assets/car.model");
            self.car_mesh_id = pwge_load_mesh(car_model_data.as_ptr(), car_model_data.len() as u32);
            
            let wheel_model_data = include_bytes!("../assets/wheel.model");
            self.wheel_mesh_id = pwge_load_mesh(wheel_model_data.as_ptr(), wheel_model_data.len() as u32);
            
            // Load audio
            let engine_audio = include_bytes!("../assets/engine.wav");
            self.engine_sound_id = pwge_load_sound(engine_audio.as_ptr(), engine_audio.len() as u32);
            
            let tire_audio = include_bytes!("../assets/tire_screech.wav");
            self.tire_sound_id = pwge_load_sound(tire_audio.as_ptr(), tire_audio.len() as u32);
            
            let brake_audio = include_bytes!("../assets/brake.wav");
            self.brake_sound_id = pwge_load_sound(brake_audio.as_ptr(), brake_audio.len() as u32);
        }
        
        Ok(())
    }
    
    fn update(&mut self, ctx: &ComponentContext, input: &InputState) -> Result<(), ComponentError> {
        self.handle_input(input);
        self.update_physics(ctx.delta_time);
        self.update_audio();
        
        Ok(())
    }
    
    fn render(&mut self, ctx: &ComponentContext) -> Result<(), ComponentError> {
        unsafe {
            // Set up 3D camera behind car
            let camera_offset = Vector3 { x: 0.0, y: 2.0, z: -5.0 };
            let camera_pos = Vector3 {
                x: self.position.x + camera_offset.x,
                y: self.position.y + camera_offset.y,
                z: self.position.z + camera_offset.z,
            };
            
            pwge_set_camera(&camera_pos, &self.position, &Vector3 { x: 0.0, y: 1.0, z: 0.0 });
            pwge_set_projection(60.0, 480.0 / 272.0, 0.1, 1000.0);
            
            // Render car
            let car_transform = create_transform_matrix(
                self.position,
                self.orientation,
                Vector3 { x: 1.0, y: 1.0, z: 1.0 }
            );
            pwge_draw_mesh(self.car_mesh_id, &car_transform);
            
            // Render wheels
            for i in 0..4 {
                let wheel_world_pos = self.transform_point(&self.wheel_positions[i]);
                let mut wheel_rotation = self.orientation;
                
                // Front wheels turn with steering
                if i < 2 {
                    wheel_rotation.y += self.steering_angle;
                }
                
                let wheel_transform = create_transform_matrix(
                    wheel_world_pos,
                    wheel_rotation,
                    Vector3 { x: 1.0, y: 1.0, z: 1.0 }
                );
                pwge_draw_mesh(self.wheel_mesh_id, &wheel_transform);
            }
        }
        
        Ok(())
    }
    
    fn network_message(&mut self, ctx: &ComponentContext, msg: &NetworkMessage) -> Result<(), ComponentError> {
        // Handle multiplayer synchronization
        match msg.msg_type {
            3 => {  // Player input from other cars
                // Apply other player's input to their car instance
            }
            4 => {  // Car state update
                // Sync position/velocity for lag compensation
            }
            _ => {}
        }
        
        Ok(())
    }
    
    fn suspend(&mut self, ctx: &ComponentContext) -> Result<(), ComponentError> {
        // PSP is being suspended - save critical state
        Ok(())
    }
    
    fn resume(&mut self, ctx: &ComponentContext) -> Result<(), ComponentError> {
        // PSP resumed - restore state
        Ok(())
    }
    
    fn cleanup(&mut self, _ctx: &ComponentContext) {
        // Cleanup handled by runtime
    }
}

#[no_mangle]
pub extern "C" fn pwge_component_create() -> *mut dyn PWGEComponent {
    Box::into_raw(Box::new(CarPhysicsComponent::new()))
}

#[no_mangle]
pub extern "C" fn pwge_component_destroy(component: *mut dyn PWGEComponent) {
    if !component.is_null() {
        unsafe {
            let _ = Box::from_raw(component);
        }
    }
}
```

### Multiplayer Racing Game Manifest

```toml
# examples/racing/pwge.toml

[game]
name = "PSP WASM Racer"
version = "1.2.0"
author = "PWGE Racing Team"
description = "High-performance 3D racing with multiplayer support"
target_fps = 60
memory_budget = "24MB"
networking = true
requires_wifi = true

[[component]]
id = "car_physics"
source = "car_physics/target/wasm32-unknown-unknown/release/car_physics.wasm"
priority = "critical"
max_memory = "4MB"
max_cycles_per_frame = 100000

[component.capabilities]
input = ["analog_stick", "buttons"]
audio = ["engine_channel", "sfx_channel_0", "sfx_channel_1"]
graphics = ["mesh_renderer", "3d_transforms"]
network = ["peer_to_peer", "state_sync"]

[[component]]
id = "track_renderer"
source = "track_renderer/
# Dreamcast WASM Game Engine (DWGE)
## Flight-Inspired WebAssembly Runtime for Sega Dreamcast

**Project Codename:** "Flight Retro"  
**Target Platform:** Sega Dreamcast (SH-4, PowerVR CLX2, 16MB+8MB RAM)  
**Design Philosophy:** Modern component architecture meets 1999 hardware constraints  
**Created:** May 31, 2025

---

## Executive Summary

The Dreamcast WASM Game Engine (DWGE) is a revolutionary project that brings modern WebAssembly-based game development to the Sega Dreamcast. Inspired by the Flight application host architecture, DWGE provides a component-based game engine where game logic is written in modern languages (Rust, C++, AssemblyScript) and compiled to WebAssembly, then executed on Dreamcast hardware through a custom WASM runtime.

### Key Innovation
- **First WASM runtime for 1990s console hardware**
- **Component-based architecture adapted for extreme memory constraints**
- **Modern development workflow targeting retro hardware**
- **Hybrid interpretation/compilation approach for SH-4 processor**

---

## Technical Architecture

### System Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    Game Application                         │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐    │
│  │   Player    │ │    Audio    │ │     Rendering       │    │
│  │ Controller  │ │   System    │ │     Engine          │    │
│  │ (WASM)      │ │  (WASM)     │ │     (WASM)          │    │
│  └─────────────┘ └─────────────┘ └─────────────────────┘    │
├─────────────────────────────────────────────────────────────┤
│                  DWGE Runtime Core                          │
│  ┌──────────────────────┐ ┌─────────────────────────────┐   │
│  │   WASM Interpreter   │ │    Component Manager        │   │
│  │   - Bytecode decode  │ │    - Lifecycle management   │   │
│  │   - Stack machine    │ │    - Inter-component comm   │   │
│  │   - Memory mgmt      │ │    - Resource allocation    │   │
│  └──────────────────────┘ └─────────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                 Dreamcast Hardware Layer                    │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌───────┐ │
│  │  Maple  │ │ PowerVR │ │  AICA   │ │   VMU   │ │  GD   │ │
│  │   Bus   │ │  CLX2   │ │  Audio  │ │ Storage │ │ ROM   │ │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └───────┘ │
└─────────────────────────────────────────────────────────────┘
```

### Memory Architecture

**Total Available:** 24MB (16MB main + 8MB video)
**Allocation Strategy:**
```
┌─────────────────────────────────────────────────────────┐
│                  16MB Main RAM                          │
├─────────────────────────────────────────────────────────┤
│  System/KOS (2MB)                                       │
├─────────────────────────────────────────────────────────┤
│  DWGE Runtime (1MB)                                     │
├─────────────────────────────────────────────────────────┤
│  WASM Linear Memory Pool (8MB)                         │
│  ├─ Component A Memory (2MB max)                       │
│  ├─ Component B Memory (2MB max)                       │
│  ├─ Component C Memory (2MB max)                       │
│  └─ Shared Memory Pool (2MB)                           │
├─────────────────────────────────────────────────────────┤
│  Audio Buffers (1MB)                                   │
├─────────────────────────────────────────────────────────┤
│  Game Assets/Scratch (4MB)                             │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│                   8MB Video RAM                         │
├─────────────────────────────────────────────────────────┤
│  Front Buffer 640x480x16 (600KB)                       │
├─────────────────────────────────────────────────────────┤
│  Back Buffer 640x480x16 (600KB)                        │
├─────────────────────────────────────────────────────────┤
│  Texture Memory (6.8MB)                                │
│  ├─ Texture Atlas 1 (2MB)                              │
│  ├─ Texture Atlas 2 (2MB)                              │
│  ├─ Font/UI Textures (512KB)                           │
│  └─ Dynamic Textures (2.3MB)                           │
└─────────────────────────────────────────────────────────┘
```

---

## WASM Runtime Implementation

### Core Interpreter Design

```c
// dreamcast-wasm/include/dwge_runtime.h

typedef struct {
    uint32_t opcode;
    union {
        int32_t i32_const;
        int64_t i64_const;
        float f32_const;
        double f64_const;
        uint32_t local_idx;
        uint32_t global_idx;
        struct {
            uint32_t align;
            uint32_t offset;
        } memarg;
    } operand;
} WasmInstruction;

typedef struct {
    uint8_t* memory;
    uint32_t memory_size;
    uint32_t memory_max;
    int64_t* stack;
    uint32_t stack_ptr;
    uint32_t stack_max;
    WasmInstruction* instructions;
    uint32_t instruction_count;
    uint32_t program_counter;
    uint32_t call_depth;
} WasmExecutionContext;

// Optimized for SH-4 processor
typedef struct {
    WasmExecutionContext ctx;
    uint32_t component_id;
    uint8_t priority;
    uint32_t max_cycles_per_frame;
    uint32_t cycles_used;
    ComponentState state;
} DWGEComponent;
```

### SH-4 Optimized Instruction Dispatch

```c
// Fast instruction dispatch using SH-4 branch prediction
static inline int dwge_execute_instruction(WasmExecutionContext* ctx) {
    WasmInstruction* instr = &ctx->instructions[ctx->program_counter];
    
    // Use SH-4's branch target buffer efficiently
    switch (instr->opcode & 0xFF) {
        case WASM_OP_I32_CONST:
            ctx->stack[++ctx->stack_ptr] = instr->operand.i32_const;
            break;
            
        case WASM_OP_I32_ADD: {
            int32_t b = ctx->stack[ctx->stack_ptr--];
            int32_t a = ctx->stack[ctx->stack_ptr];
            ctx->stack[ctx->stack_ptr] = a + b;
            break;
        }
        
        case WASM_OP_I32_LOAD: {
            uint32_t addr = ctx->stack[ctx->stack_ptr] + instr->operand.memarg.offset;
            if (addr + 4 > ctx->memory_size) return DWGE_ERROR_BOUNDS;
            ctx->stack[ctx->stack_ptr] = *(uint32_t*)(ctx->memory + addr);
            break;
        }
        
        // ... 200+ WebAssembly instructions
        
        default:
            return DWGE_ERROR_INVALID_OPCODE;
    }
    
    ctx->program_counter++;
    return DWGE_SUCCESS;
}
```

### Memory Management

```c
// Custom memory allocator designed for WASM linear memory model
typedef struct {
    uint8_t* base_ptr;
    uint32_t total_size;
    uint32_t used_size;
    uint32_t page_count;  // 64KB pages
    uint8_t page_bitmap[128];  // Track allocated pages
} WasmMemoryManager;

// Efficient page-based allocation
void* dwge_memory_alloc(WasmMemoryManager* mgr, uint32_t size) {
    uint32_t pages_needed = (size + 65535) / 65536;
    uint32_t start_page = dwge_find_free_pages(mgr, pages_needed);
    
    if (start_page == INVALID_PAGE) {
        return NULL;  // Out of memory
    }
    
    // Mark pages as allocated
    for (uint32_t i = 0; i < pages_needed; i++) {
        dwge_set_page_allocated(mgr, start_page + i);
    }
    
    return mgr->base_ptr + (start_page * 65536);
}
```

---

## Component System

### Component Manifest Format

```toml
# dwge.toml - Dreamcast WASM Game Manifest

[game]
name = "retro-platformer"
version = "1.0.0"
author = "Indie Dev Studio"
description = "Modern 2D platformer for Dreamcast"
target_fps = 60
memory_budget = "12MB"

[[component]]
id = "player_controller"
source = "target/wasm32-unknown-unknown/release/player.wasm"
priority = "high"
max_memory = "1MB"
max_cycles_per_frame = 50000

[component.capabilities]
input = ["controller_0"]
audio = ["sfx_channel_0", "sfx_channel_1"]
graphics = ["sprite_renderer"]

[[component]]
id = "audio_system"
source = "target/wasm32-unknown-unknown/release/audio.wasm"
priority = "medium"
max_memory = "512KB"
max_cycles_per_frame = 20000

[component.capabilities]
audio = ["music_channel", "sfx_mix"]
storage = ["vmu_save_data"]

[[component]]
id = "rendering_engine"  
source = "target/wasm32-unknown-unknown/release/renderer.wasm"
priority = "high"
max_memory = "2MB"
max_cycles_per_frame = 100000

[component.capabilities]
graphics = ["tile_renderer", "sprite_renderer", "particle_system"]
memory = ["texture_cache", "vertex_buffer"]

[graphics]
resolution = "640x480"
color_depth = 16
texture_format = "rgb565"
max_textures = 256
tile_map_size = "64x64"

[audio]
sample_rate = 44100
channels = 2
buffer_size = 1024
max_voices = 64

[input]
controllers = 4
analog_deadzone = 0.1
button_repeat_delay = 200

[storage]
save_format = "binary"
vmu_slots = 4
max_save_size = "32KB"
```

### Component Interface Definition

```rust
// dwge-api/src/lib.rs - Rust API for component development

#[repr(C)]
pub struct ComponentContext {
    pub component_id: u32,
    pub delta_time: f32,
    pub frame_count: u64,
    pub memory_available: u32,
    pub cycles_remaining: u32,
}

#[repr(C)]
pub struct InputState {
    pub controller: [ControllerState; 4],
    pub keyboard: u64,  // Bit field for keyboard (if connected)
}

#[repr(C)]
pub struct ControllerState {
    pub buttons: u32,
    pub analog_x: i8,
    pub analog_y: i8,
    pub trigger_l: u8,
    pub trigger_r: u8,
    pub connected: bool,
}

// Component lifecycle traits
pub trait DWGEComponent {
    fn init(&mut self, ctx: &ComponentContext) -> Result<(), ComponentError>;
    fn update(&mut self, ctx: &ComponentContext, input: &InputState) -> Result<(), ComponentError>;
    fn render(&mut self, ctx: &ComponentContext) -> Result<(), ComponentError>;
    fn cleanup(&mut self, ctx: &ComponentContext);
}

// Host function imports - provided by DWGE runtime
extern "C" {
    // Graphics API
    fn dwge_draw_sprite(texture_id: u32, x: f32, y: f32, scale: f32) -> i32;
    fn dwge_draw_tile(tile_id: u32, x: u32, y: u32) -> i32;
    fn dwge_load_texture(data: *const u8, len: u32) -> u32;
    fn dwge_set_camera(x: f32, y: f32, zoom: f32) -> i32;
    
    // Audio API
    fn dwge_play_sound(sound_id: u32, volume: f32, pitch: f32) -> i32;
    fn dwge_play_music(music_id: u32, loop_mode: bool) -> i32;
    fn dwge_load_sound(data: *const u8, len: u32) -> u32;
    
    // Storage API
    fn dwge_save_data(vmu_slot: u32, data: *const u8, len: u32) -> i32;
    fn dwge_load_data(vmu_slot: u32, buffer: *mut u8, max_len: u32) -> i32;
    
    // Debug API
    fn dwge_log(level: u32, message: *const u8, len: u32);
    fn dwge_get_performance_counters(counters: *mut PerformanceCounters);
}
```

### Example Component Implementation

```rust
// player_controller/src/lib.rs

use dwge_api::*;

struct PlayerController {
    position: (f32, f32),
    velocity: (f32, f32),
    on_ground: bool,
    sprite_id: u32,
    jump_sound_id: u32,
}

impl DWGEComponent for PlayerController {
    fn init(&mut self, ctx: &ComponentContext) -> Result<(), ComponentError> {
        // Load player sprite
        let sprite_data = include_bytes!("../assets/player.png");
        self.sprite_id = unsafe { dwge_load_texture(sprite_data.as_ptr(), sprite_data.len() as u32) };
        
        // Load jump sound
        let jump_data = include_bytes!("../assets/jump.wav");
        self.jump_sound_id = unsafe { dwge_load_sound(jump_data.as_ptr(), jump_data.len() as u32) };
        
        self.position = (320.0, 240.0);  // Center of screen
        self.velocity = (0.0, 0.0);
        self.on_ground = false;
        
        Ok(())
    }
    
    fn update(&mut self, ctx: &ComponentContext, input: &InputState) -> Result<(), ComponentError> {
        let controller = &input.controller[0];
        
        if !controller.connected {
            return Ok(());
        }
        
        // Movement physics
        const SPEED: f32 = 200.0;
        const JUMP_FORCE: f32 = -500.0;
        const GRAVITY: f32 = 1200.0;
        
        // Horizontal movement
        let input_x = controller.analog_x as f32 / 128.0;
        self.velocity.0 = input_x * SPEED;
        
        // Jumping
        if (controller.buttons & BUTTON_A) != 0 && self.on_ground {
            self.velocity.1 = JUMP_FORCE;
            self.on_ground = false;
            
            unsafe {
                dwge_play_sound(self.jump_sound_id, 0.8, 1.0);
            }
        }
        
        // Apply gravity
        if !self.on_ground {
            self.velocity.1 += GRAVITY * ctx.delta_time;
        }
        
        // Update position
        self.position.0 += self.velocity.0 * ctx.delta_time;
        self.position.1 += self.velocity.1 * ctx.delta_time;
        
        // Simple ground collision
        if self.position.1 >= 400.0 {
            self.position.1 = 400.0;
            self.velocity.1 = 0.0;
            self.on_ground = true;
        }
        
        Ok(())
    }
    
    fn render(&mut self, ctx: &ComponentContext) -> Result<(), ComponentError> {
        unsafe {
            dwge_draw_sprite(self.sprite_id, self.position.0, self.position.1, 1.0);
        }
        Ok(())
    }
    
    fn cleanup(&mut self, _ctx: &ComponentContext) {
        // Cleanup resources if needed
    }
}

// Export component for DWGE runtime
#[no_mangle]
pub extern "C" fn dwge_component_create() -> *mut dyn DWGEComponent {
    Box::into_raw(Box::new(PlayerController {
        position: (0.0, 0.0),
        velocity: (0.0, 0.0),
        on_ground: false,
        sprite_id: 0,
        jump_sound_id: 0,
    }))
}
```

---

## Hardware Integration Layer

### PowerVR Graphics Interface

```c
// dwge-graphics/pvr_interface.c

typedef struct {
    uint16_t* framebuffer;
    uint16_t* backbuffer;
    uint32_t width;
    uint32_t height;
    uint32_t texture_cache_size;
    pvr_ptr_t texture_memory;
} DWGEGraphicsContext;

// Optimized sprite rendering for PowerVR tile-based renderer
void dwge_pvr_draw_sprite(uint32_t texture_id, float x, float y, float scale) {
    pvr_vertex_t vert[4];
    DWGETexture* tex = &texture_cache[texture_id];
    
    // Calculate UV coordinates and screen positions
    float half_width = (tex->width * scale) * 0.5f;
    float half_height = (tex->height * scale) * 0.5f;
    
    // Top-left
    vert[0].flags = PVR_CMD_VERTEX;
    vert[0].x = x - half_width;
    vert[0].y = y - half_height;
    vert[0].z = 1.0f;
    vert[0].u = 0.0f;
    vert[0].v = 0.0f;
    vert[0].argb = 0xFFFFFFFF;
    
    // Top-right
    vert[1].flags = PVR_CMD_VERTEX;
    vert[1].x = x + half_width;
    vert[1].y = y - half_height;
    vert[1].z = 1.0f;
    vert[1].u = 1.0f;
    vert[1].v = 0.0f;
    vert[1].argb = 0xFFFFFFFF;
    
    // Bottom-left
    vert[2].flags = PVR_CMD_VERTEX;
    vert[2].x = x - half_width;
    vert[2].y = y + half_height;
    vert[2].z = 1.0f;
    vert[2].u = 0.0f;
    vert[2].v = 1.0f;
    vert[2].argb = 0xFFFFFFFF;
    
    // Bottom-right (end of strip)
    vert[3].flags = PVR_CMD_VERTEX_EOL;
    vert[3].x = x + half_width;
    vert[3].y = y + half_height;
    vert[3].z = 1.0f;
    vert[3].u = 1.0f;
    vert[3].v = 1.0f;
    vert[3].argb = 0xFFFFFFFF;
    
    // Submit to PowerVR
    pvr_prim(&vert[0], sizeof(pvr_vertex_t));
    pvr_prim(&vert[1], sizeof(pvr_vertex_t));
    pvr_prim(&vert[2], sizeof(pvr_vertex_t));
    pvr_prim(&vert[3], sizeof(pvr_vertex_t));
}

// Efficient texture loading with format conversion
uint32_t dwge_pvr_load_texture(const uint8_t* data, uint32_t len, TextureFormat format) {
    uint32_t texture_id = dwge_allocate_texture_id();
    DWGETexture* tex = &texture_cache[texture_id];
    
    // Parse texture header (custom format)
    tex->width = *(uint32_t*)data;
    tex->height = *(uint32_t*)(data + 4);
    const uint8_t* pixel_data = data + 8;
    
    // Convert to PowerVR format (RGB565 or ARGB4444)
    uint32_t pixel_count = tex->width * tex->height;
    uint32_t pvr_size = pixel_count * 2;  // 16-bit pixels
    
    tex->pvr_texture = pvr_mem_malloc(pvr_size);
    if (!tex->pvr_texture) {
        return INVALID_TEXTURE_ID;
    }
    
    // Convert pixels to PowerVR format
    uint16_t* pvr_pixels = (uint16_t*)tex->pvr_texture;
    for (uint32_t i = 0; i < pixel_count; i++) {
        uint8_t r = pixel_data[i * 4 + 0];
        uint8_t g = pixel_data[i * 4 + 1];
        uint8_t b = pixel_data[i * 4 + 2];
        uint8_t a = pixel_data[i * 4 + 3];
        
        // Convert RGBA8888 to RGB565
        pvr_pixels[i] = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    }
    
    return texture_id;
}
```

### AICA Audio System

```c
// dwge-audio/aica_interface.c

typedef struct {
    uint8_t* sample_data;
    uint32_t sample_length;
    uint32_t sample_rate;
    uint8_t channels;
    uint8_t bits_per_sample;
    bool looping;
} DWGEAudioClip;

typedef struct {
    DWGEAudioClip* clip;
    uint32_t position;
    float volume;
    float pitch;
    bool active;
    uint8_t channel;
} DWGEAudioVoice;

static DWGEAudioVoice audio_voices[64];
static DWGEAudioClip audio_clips[256];
static uint32_t next_clip_id = 0;

// Load audio clip into AICA memory
uint32_t dwge_aica_load_sound(const uint8_t* data, uint32_t len) {
    if (next_clip_id >= 256) return INVALID_SOUND_ID;
    
    DWGEAudioClip* clip = &audio_clips[next_clip_id];
    
    // Parse WAV header (simplified)
    const uint8_t* wav_data = data + 44;  // Skip WAV header
    uint32_t wav_size = len - 44;
    
    // Allocate AICA memory
    clip->sample_data = (uint8_t*)snd_mem_malloc(wav_size);
    if (!clip->sample_data) return INVALID_SOUND_ID;
    
    // Copy sample data to AICA RAM
    memcpy(clip->sample_data, wav_data, wav_size);
    clip->sample_length = wav_size;
    clip->sample_rate = 44100;  // Fixed for simplicity
    clip->channels = 1;
    clip->bits_per_sample = 16;
    clip->looping = false;
    
    return next_clip_id++;
}

// Play sound using AICA hardware
int32_t dwge_aica_play_sound(uint32_t sound_id, float volume, float pitch) {
    if (sound_id >= next_clip_id) return -1;
    
    // Find free voice
    int8_t voice_channel = -1;
    for (int i = 0; i < 64; i++) {
        if (!audio_voices[i].active) {
            voice_channel = i;
            break;
        }
    }
    
    if (voice_channel == -1) return -1;  // No free voices
    
    DWGEAudioVoice* voice = &audio_voices[voice_channel];
    DWGEAudioClip* clip = &audio_clips[sound_id];
    
    voice->clip = clip;
    voice->position = 0;
    voice->volume = volume;
    voice->pitch = pitch;
    voice->active = true;
    voice->channel = voice_channel;
    
    // Configure AICA channel
    AICA_CHANNEL(voice_channel).cmd = AICA_CH_CMD_STOP;
    AICA_CHANNEL(voice_channel).base = (uint32_t)clip->sample_data;
    AICA_CHANNEL(voice_channel).type = AICA_SM_16BIT;
    AICA_CHANNEL(voice_channel).length = clip->sample_length / 2;  // 16-bit samples
    AICA_CHANNEL(voice_channel).loop = 0;
    AICA_CHANNEL(voice_channel).loopstart = 0;
    AICA_CHANNEL(voice_channel).loopend = clip->sample_length / 2;
    AICA_CHANNEL(voice_channel).freq = (uint32_t)(44100.0f * pitch);
    AICA_CHANNEL(voice_channel).vol = (uint32_t)(255.0f * volume);
    AICA_CHANNEL(voice_channel).pan = 128;  // Center
    AICA_CHANNEL(voice_channel).cmd = AICA_CH_CMD_START;
    
    return voice_channel;
}
```

### Maple Bus Input System

```c
// dwge-input/maple_interface.c

typedef struct {
    maple_device_t* device;
    cont_state_t* state;
    cont_state_t previous_state;
    bool connected;
    uint8_t port;
} DWGEController;

static DWGEController controllers[4];

void dwge_maple_init() {
    // Initialize Maple Bus
    maple_init();
    
    // Scan for controllers
    for (int i = 0; i < 4; i++) {
        controllers[i].device = maple_enum_type(i, MAPLE_FUNC_CONTROLLER);
        if (controllers[i].device) {
            controllers[i].state = (cont_state_t*)maple_dev_status(controllers[i].device);
            controllers[i].connected = true;
            controllers[i].port = i;
        } else {
            controllers[i].connected = false;
        }
    }
}

void dwge_maple_update() {
    for (int i = 0; i < 4; i++) {
        if (controllers[i].connected) {
            // Store previous state for edge detection
            controllers[i].previous_state = *controllers[i].state;
            
            // Update current state
            if (maple_dev_status(controllers[i].device) < 0) {
                controllers[i].connected = false;
            }
        }
    }
}

InputState dwge_maple_get_input() {
    InputState input = {0};
    
    for (int i = 0; i < 4; i++) {
        if (controllers[i].connected) {
            cont_state_t* state = controllers[i].state;
            
            // Map Dreamcast controller to DWGE input format
            input.controller[i].buttons = 0;
            
            if (state->buttons & CONT_A) input.controller[i].buttons |= DWGE_BUTTON_A;
            if (state->buttons & CONT_B) input.controller[i].buttons |= DWGE_BUTTON_B;
            if (state->buttons & CONT_X) input.controller[i].buttons |= DWGE_BUTTON_X;
            if (state->buttons & CONT_Y) input.controller[i].buttons |= DWGE_BUTTON_Y;
            if (state->buttons & CONT_START) input.controller[i].buttons |= DWGE_BUTTON_START;
            if (state->buttons & CONT_DPAD_UP) input.controller[i].buttons |= DWGE_BUTTON_UP;
            if (state->buttons & CONT_DPAD_DOWN) input.controller[i].buttons |= DWGE_BUTTON_DOWN;
            if (state->buttons & CONT_DPAD_LEFT) input.controller[i].buttons |= DWGE_BUTTON_LEFT;
            if (state->buttons & CONT_DPAD_RIGHT) input.controller[i].buttons |= DWGE_BUTTON_RIGHT;
            
            // Analog stick
            input.controller[i].analog_x = state->joyx;
            input.controller[i].analog_y = state->joyy;
            
            // Triggers
            input.controller[i].trigger_l = state->ltrig;
            input.controller[i].trigger_r = state->rtrig;
            
            input.controller[i].connected = true;
        }
    }
    
    return input;
}
```

---

## Performance Optimization

### SH-4 Specific Optimizations

```c
// SH-4 has excellent branch prediction - optimize for it
static inline void dwge_prefetch_instruction(WasmExecutionContext* ctx, uint32_t offset) {
    // SH-4 cache line is 32 bytes
    __builtin_prefetch(&ctx->instructions[ctx->program_counter + offset], 0, 3);
}

// Use SH-4's MAC (Multiply-Accumulate) instructions for WASM f32 ops
static inline float dwge_sh4_mac_f32_multiply_add(float a, float b, float c) {
    float result;
    asm volatile (
        "fmov %1, fr0\n\t"
        "fmov %2, fr1\n\t"
        "fmov %3, fr2\n\t"
        "fmac fr0, fr1, fr2\n\t"
        "fmov fr2, %0"
        : "=f" (result)
        : "f" (a), "f" (b), "f" (c)
        : "fr0", "fr1", "fr2"
    );
    return result;
}

// Optimized WASM stack operations using SH-4's efficient memory access
static inline void dwge_stack_push_i32(WasmExecutionContext* ctx, int32_t value) {
    // Use SH-4's pre-increment addressing mode
    asm volatile (
        "mov.l %1, @-r15"
        :
        : "r" (ctx->stack), "r" (value)
        : "memory"
    );
    ctx->stack_ptr++;
}

static inline int32_t dwge_stack_pop_i32(WasmExecutionContext* ctx) {
    int32_t result;
    asm volatile (
        "mov.l @r15+, %0"
        : "=r" (result)
        :
        : "memory"
    );
    ctx->stack_ptr--;
    return result;
}

// Cache-optimized instruction fetching
typedef struct {
    WasmInstruction cache[32];  // One SH-4 cache line worth
    uint32_t base_pc;
    uint32_t valid_count;
} InstructionCache;

static InstructionCache icache = {0};

static inline WasmInstruction* dwge_fetch_instruction(WasmExecutionContext* ctx) {
    uint32_t pc = ctx->program_counter;
    
    // Check if instruction is in cache
    if (pc >= icache.base_pc && pc < icache.base_pc + icache.valid_count) {
        return &icache.cache[pc - icache.base_pc];
    }
    
    // Cache miss - reload cache line
    icache.base_pc = pc;
    icache.valid_count = (ctx->instruction_count - pc > 32) ? 32 : (ctx->instruction_count - pc);
    
    // Use SH-4's burst mode for efficient cache fill
    memcpy(icache.cache, &ctx->instructions[pc], icache.valid_count * sizeof(WasmInstruction));
    
    return &icache.cache[0];
}
```

### Memory Pool Management

```c
// dwge-memory/pool_allocator.c

typedef struct MemoryBlock {
    struct MemoryBlock* next;
    uint32_t size;
    bool free;
} MemoryBlock;

typedef struct {
    uint8_t* memory_base;
    uint32_t total_size;
    MemoryBlock* free_list;
    MemoryBlock* blocks;
    uint32_t block_count;
    uint32_t bytes_allocated;
    uint32_t peak_usage;
} MemoryPool;

// Component-specific memory pools
static MemoryPool component_pools[8];
static MemoryPool shared_pool;

void dwge_memory_init() {
    // Initialize component pools (1MB each)
    for (int i = 0; i < 8; i++) {
        component_pools[i].memory_base = (uint8_t*)malloc(1024 * 1024);
        component_pools[i].total_size = 1024 * 1024;
        component_pools[i].free_list = (MemoryBlock*)component_pools[i].memory_base;
        component_pools[i].free_list->next = NULL;
        component_pools[i].free_list->size = 1024 * 1024 - sizeof(MemoryBlock);
        component_pools[i].free_list->free = true;
        component_pools[i].bytes_allocated = 0;
        component_pools[i].peak_usage = 0;
    }
    
    // Initialize shared pool (4MB)
    shared_pool.memory_base = (uint8_t*)malloc(4 * 1024 * 1024);
    shared_pool.total_size = 4 * 1024 * 1024;
    shared_pool.free_list = (MemoryBlock*)shared_pool.memory_base;
    shared_pool.free_list->next = NULL;
    shared_pool.free_list->size = 4 * 1024 * 1024 - sizeof(MemoryBlock);
    shared_pool.free_list->free = true;
    shared_pool.bytes_allocated = 0;
    shared_pool.peak_usage = 0;
}

// Fast allocation using best-fit algorithm optimized for game patterns
void* dwge_memory_alloc(uint32_t component_id, uint32_t size) {
    if (component_id >= 8) return NULL;
    
    MemoryPool* pool = &component_pools[component_id];
    
    // Align to 16-byte boundaries for SH-4 efficiency
    size = (size + 15) & ~15;
    
    MemoryBlock* block = pool->free_list;
    MemoryBlock* prev = NULL;
    
    // Find best-fit block
    MemoryBlock* best_block = NULL;
    MemoryBlock* best_prev = NULL;
    uint32_t best_size = UINT32_MAX;
    
    while (block) {
        if (block->free && block->size >= size) {
            if (block->size < best_size) {
                best_block = block;
                best_prev = prev;
                best_size = block->size;
            }
        }
        prev = block;
        block = block->next;
    }
    
    if (!best_block) {
        // Try shared pool as fallback
        return dwge_shared_memory_alloc(size);
    }
    
    // Split block if necessary
    if (best_block->size > size + sizeof(MemoryBlock) + 64) {
        MemoryBlock* new_block = (MemoryBlock*)((uint8_t*)best_block + sizeof(MemoryBlock) + size);
        new_block->size = best_block->size - size - sizeof(MemoryBlock);
        new_block->free = true;
        new_block->next = best_block->next;
        
        best_block->size = size;
        best_block->next = new_block;
    }
    
    best_block->free = false;
    pool->bytes_allocated += best_block->size;
    
    if (pool->bytes_allocated > pool->peak_usage) {
        pool->peak_usage = pool->bytes_allocated;
    }
    
    return (uint8_t*)best_block + sizeof(MemoryBlock);
}

void dwge_memory_free(uint32_t component_id, void* ptr) {
    if (!ptr || component_id >= 8) return;
    
    MemoryPool* pool = &component_pools[component_id];
    MemoryBlock* block = (MemoryBlock*)((uint8_t*)ptr - sizeof(MemoryBlock));
    
    block->free = true;
    pool->bytes_allocated -= block->size;
    
    // Coalesce with adjacent free blocks
    dwge_memory_coalesce(pool, block);
}

// Garbage collection for WASM linear memory
void dwge_memory_gc(uint32_t component_id) {
    if (component_id >= 8) return;
    
    MemoryPool* pool = &component_pools[component_id];
    
    // Simple mark-and-sweep GC
    // Mark all reachable memory from WASM stack and globals
    dwge_gc_mark_phase(pool, component_id);
    
    // Sweep unreachable blocks
    dwge_gc_sweep_phase(pool);
    
    // Compact memory to reduce fragmentation
    dwge_memory_compact(pool);
}
```

### Frame Timing and Component Scheduling

```c
// dwge-core/scheduler.c

typedef struct {
    uint32_t component_id;
    uint32_t priority;  // 0 = highest, 255 = lowest
    uint32_t max_cycles;
    uint32_t cycles_used;
    uint64_t last_update_time;
    ComponentState state;
    DWGEComponent* component;
} ScheduledComponent;

typedef struct {
    ScheduledComponent components[16];
    uint32_t component_count;
    uint64_t frame_start_time;
    uint64_t frame_target_time;  // 16.67ms for 60fps
    uint32_t total_cycles_per_frame;
    bool frame_skip_enabled;
} ComponentScheduler;

static ComponentScheduler scheduler;

void dwge_scheduler_init() {
    scheduler.component_count = 0;
    scheduler.frame_target_time = 16667;  // 16.667ms in microseconds
    scheduler.total_cycles_per_frame = 1000000;  // 1M cycles at 200MHz = 5ms budget
    scheduler.frame_skip_enabled = true;
}

// Time-sliced component execution
void dwge_scheduler_update() {
    scheduler.frame_start_time = timer_us_gettime64();
    uint64_t current_time = scheduler.frame_start_time;
    
    // Sort components by priority (stable sort to maintain order for same priority)
    dwge_sort_components_by_priority();
    
    uint32_t cycles_remaining = scheduler.total_cycles_per_frame;
    
    for (uint32_t i = 0; i < scheduler.component_count; i++) {
        ScheduledComponent* comp = &scheduler.components[i];
        
        if (comp->state != COMPONENT_ACTIVE) continue;
        
        // Check if we have cycles remaining for this component
        uint32_t cycles_for_component = (comp->max_cycles < cycles_remaining) 
            ? comp->max_cycles : cycles_remaining;
            
        if (cycles_for_component < 1000) {
            // Not enough cycles - defer to next frame
            comp->state = COMPONENT_DEFERRED;
            continue;
        }
        
        // Execute component with cycle limit
        uint32_t cycles_before = dwge_get_cycle_count();
        
        ComponentContext ctx = {
            .component_id = comp->component_id,
            .delta_time = (current_time - comp->last_update_time) / 1000000.0f,
            .frame_count = scheduler.frame_start_time / scheduler.frame_target_time,
            .cycles_remaining = cycles_for_component
        };
        
        InputState input = dwge_maple_get_input();
        
        // Update component
        ComponentError result = comp->component->update(comp->component, &ctx, &input);
        
        uint32_t cycles_after = dwge_get_cycle_count();
        uint32_t cycles_used = cycles_after - cycles_before;
        
        comp->cycles_used = cycles_used;
        comp->last_update_time = current_time;
        cycles_remaining -= cycles_used;
        
        if (result != COMPONENT_SUCCESS) {
            comp->state = COMPONENT_ERROR;
            dwge_log(LOG_ERROR, "Component %d update failed: %d", comp->component_id, result);
        }
        
        // Check frame time budget
        current_time = timer_us_gettime64();
        if (current_time - scheduler.frame_start_time > scheduler.frame_target_time * 0.8) {
            // 80% of frame time used - defer remaining components
            for (uint32_t j = i + 1; j < scheduler.component_count; j++) {
                scheduler.components[j].state = COMPONENT_DEFERRED;
            }
            break;
        }
    }
    
    // Render phase - always execute render components
    dwge_scheduler_render_phase();
}

void dwge_scheduler_render_phase() {
    pvr_wait_ready();
    pvr_scene_begin();
    
    // Render in priority order
    for (uint32_t i = 0; i < scheduler.component_count; i++) {
        ScheduledComponent* comp = &scheduler.components[i];
        
        if (comp->component->render) {
            ComponentContext ctx = {
                .component_id = comp->component_id,
                .delta_time = 0.0f,  // Not relevant for render
                .frame_count = scheduler.frame_start_time / scheduler.frame_target_time,
                .cycles_remaining = 10000  // Small budget for render
            };
            
            comp->component->render(comp->component, &ctx);
        }
    }
    
    pvr_scene_finish();
}
```

---

## Development Toolchain

### Build System

```makefile
# Makefile for DWGE projects

# Dreamcast toolchain paths
DC_ARM_CC = arm-eabi-gcc
DC_ARM_AS = arm-eabi-as
DC_ARM_LD = arm-eabi-ld
DC_SH_CC = sh-elf-gcc
DC_SH_AS = sh-elf-as
DC_SH_LD = sh-elf-ld

# WASM toolchain
WASM_CC = clang
WASM_TARGET = --target=wasm32-unknown-unknown
WASM_FLAGS = -O3 -flto -nostdlib -Wl,--no-entry -Wl,--allow-undefined

# Project structure
SRC_DIR = src
WASM_DIR = wasm-components
BUILD_DIR = build
DIST_DIR = dist

# DWGE runtime sources
DWGE_SOURCES = \
    $(SRC_DIR)/dwge-runtime/interpreter.c \
    $(SRC_DIR)/dwge-runtime/component_manager.c \
    $(SRC_DIR)/dwge-runtime/memory_manager.c \
    $(SRC_DIR)/dwge-graphics/pvr_interface.c \
    $(SRC_DIR)/dwge-audio/aica_interface.c \
    $(SRC_DIR)/dwge-input/maple_interface.c \
    $(SRC_DIR)/dwge-core/scheduler.c

# WASM component sources
WASM_COMPONENTS = \
    $(WASM_DIR)/player_controller \
    $(WASM_DIR)/audio_system \
    $(WASM_DIR)/rendering_engine

# Build targets
.PHONY: all clean wasm-components dreamcast-binary cdi

all: dreamcast-binary cdi

# Build WASM components
wasm-components:
	@echo "Building WASM components..."
	@for component in $(WASM_COMPONENTS); do \
		echo "Building $component..."; \
		cd $component && \
		$(WASM_CC) $(WASM_TARGET) $(WASM_FLAGS) \
			-I../../include/dwge-api \
			src/lib.rs \
			-o ../../$(BUILD_DIR)/$(basename $component).wasm; \
		cd ../..; \
	done

# Build Dreamcast binary
dreamcast-binary: wasm-components
	@echo "Building Dreamcast binary..."
	@mkdir -p $(BUILD_DIR)
	$(DC_SH_CC) -ml -m4-single-only -O3 -ffast-math \
		-I$(KOS_BASE)/include \
		-I$(KOS_BASE)/kernel/arch/dreamcast/include \
		-I$(KOS_BASE)/addons/include \
		-Iinclude \
		$(DWGE_SOURCES) \
		src/main.c \
		-L$(KOS_BASE)/lib/dreamcast \
		-L$(KOS_BASE)/addons/lib/dreamcast \
		-lkallisti -lgcc \
		-o $(BUILD_DIR)/dwge_game.elf

# Create CDI image
cdi: dreamcast-binary
	@echo "Creating CDI image..."
	@mkdir -p $(DIST_DIR)
	
	# Convert ELF to BIN
	$(DC_SH_OBJCOPY) -R .stack -O binary $(BUILD_DIR)/dwge_game.elf $(BUILD_DIR)/1ST_READ.BIN
	
	# Scramble binary
	$(KOS_BASE)/utils/scramble/scramble $(BUILD_DIR)/1ST_READ.BIN $(BUILD_DIR)/1ST_READ.BIN
	
	# Create ISO
	mkisofs -C 0,11702 -V "DWGE_GAME" -G $(BUILD_DIR)/IP.BIN \
		-r -J -l -o $(BUILD_DIR)/game.iso $(BUILD_DIR)/
	
	# Convert to CDI
	cdi4dc $(BUILD_DIR)/game.iso $(DIST_DIR)/game.cdi

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)

# Development helpers
dev-server:
	@echo "Starting development server for component hot-reload..."
	cd tools && python dev_server.py

test-emulator:
	@echo "Testing in emulator..."
	lxdream-nitro $(DIST_DIR)/game.cdi

flash-dcload:
	@echo "Uploading via dc-load..."
	dc-tool -x $(BUILD_DIR)/dwge_game.elf

# Asset pipeline
assets:
	@echo "Processing game assets..."
	cd tools && python asset_processor.py

# Component development
new-component:
	@read -p "Component name: " name; \
	tools/create_component.sh $name

# Documentation
docs:
	@echo "Generating documentation..."
	cd docs && mdbook build
```

### Asset Processing Pipeline

```python
# tools/asset_processor.py

import os
import struct
import json
from PIL import Image
import wave

class DWGEAssetProcessor:
    def __init__(self):
        self.output_dir = "build/assets"
        self.texture_atlas_size = 1024
        self.max_texture_size = 512
        
    def process_all_assets(self):
        """Process all assets for DWGE deployment"""
        os.makedirs(self.output_dir, exist_ok=True)
        
        # Process textures
        self.process_textures("assets/textures")
        
        # Process audio
        self.process_audio("assets/audio")
        
        # Generate asset manifest
        self.generate_manifest()
        
    def process_textures(self, texture_dir):
        """Convert textures to Dreamcast-optimized format"""
        textures = []
        atlas_images = []
        
        for filename in os.listdir(texture_dir):
            if filename.lower().endswith(('.png', '.jpg', '.jpeg')):
                img_path = os.path.join(texture_dir, filename)
                img = Image.open(img_path)
                
                # Convert to RGB565 format
                if img.mode != 'RGB':
                    img = img.convert('RGB')
                
                # Resize if too large
                if img.width > self.max_texture_size or img.height > self.max_texture_size:
                    img.thumbnail((self.max_texture_size, self.max_texture_size), Image.Resampling.LANCZOS)
                
                # Ensure dimensions are power of 2 for PowerVR
                width = self.next_power_of_2(img.width)
                height = self.next_power_of_2(img.height)
                
                if width != img.width or height != img.height:
                    img = img.resize((width, height), Image.Resampling.LANCZOS)
                
                textures.append({
                    'name': os.path.splitext(filename)[0],
                    'image': img,
                    'width': width,
                    'height': height
                })
        
        # Pack textures into atlases
        atlases = self.pack_texture_atlas(textures)
        
        # Save atlases in DWGE format
        for i, atlas in enumerate(atlases):
            self.save_texture_atlas(atlas, f"atlas_{i}.dtex")
    
    def pack_texture_atlas(self, textures):
        """Pack multiple textures into texture atlases"""
        # Simple bin-packing algorithm
        atlases = []
        current_atlas = {
            'image': Image.new('RGB', (self.texture_atlas_size, self.texture_atlas_size)),
            'textures': [],
            'x': 0,
            'y': 0,
            'row_height': 0
        }
        
        for texture in textures:
            # Check if texture fits in current row
            if current_atlas['x'] + texture['width'] > self.texture_atlas_size:
                # Move to next row
                current_atlas['x'] = 0
                current_atlas['y'] += current_atlas['row_height']
                current_atlas['row_height'] = 0
            
            # Check if texture fits in atlas
            if current_atlas['y'] + texture['height'] > self.texture_atlas_size:
                # Start new atlas
                atlases.append(current_atlas)
                current_atlas = {
                    'image': Image.new('RGB', (self.texture_atlas_size, self.texture_atlas_size)),
                    'textures': [],
                    'x': 0,
                    'y': 0,
                    'row_height': 0
                }
            
            # Place texture in atlas
            current_atlas['image'].paste(texture['image'], (current_atlas['x'], current_atlas['y']))
            
            # Record texture location
            current_atlas['textures'].append({
                'name': texture['name'],
                'x': current_atlas['x'],
                'y': current_atlas['y'],
                'width': texture['width'],
                'height': texture['height'],
                'u1': current_atlas['x'] / self.texture_atlas_size,
                'v1': current_atlas['y'] / self.texture_atlas_size,
                'u2': (current_atlas['x'] + texture['width']) / self.texture_atlas_size,
                'v2': (current_atlas['y'] + texture['height']) / self.texture_atlas_size
            })
            
            # Update position
            current_atlas['x'] += texture['width']
            current_atlas['row_height'] = max(current_atlas['row_height'], texture['height'])
        
        atlases.append(current_atlas)
        return atlases
    
    def save_texture_atlas(self, atlas, filename):
        """Save texture atlas in DWGE format"""
        output_path = os.path.join(self.output_dir, filename)
        
        # Convert to RGB565
        rgb_data = atlas['image'].tobytes()
        rgb565_data = self.convert_to_rgb565(rgb_data)
        
        with open(output_path, 'wb') as f:
            # DWGE texture header
            f.write(b'DTEX')  # Magic number
            f.write(struct.pack('<I', 1))  # Version
            f.write(struct.pack('<I', self.texture_atlas_size))  # Width
            f.write(struct.pack('<I', self.texture_atlas_size))  # Height
            f.write(struct.pack('<I', len(atlas['textures'])))  # Texture count
            
            # Texture entries
            for texture in atlas['textures']:
                name_bytes = texture['name'].encode('utf-8')
                f.write(struct.pack('<I', len(name_bytes)))
                f.write(name_bytes)
                f.write(struct.pack('<ffff', texture['u1'], texture['v1'], texture['u2'], texture['v2']))
                f.write(struct.pack('<II', texture['width'], texture['height']))
            
            # Pixel data
            f.write(rgb565_data)
    
    def convert_to_rgb565(self, rgb_data):
        """Convert RGB888 to RGB565"""
        rgb565_data = bytearray()
        
        for i in range(0, len(rgb_data), 3):
            r = rgb_data[i] >> 3      # 5 bits
            g = rgb_data[i+1] >> 2    # 6 bits  
            b = rgb_data[i+2] >> 3    # 5 bits
            
            rgb565 = (r << 11) | (g << 5) | b
            rgb565_data.extend(struct.pack('<H', rgb565))
        
        return bytes(rgb565_data)
    
    def process_audio(self, audio_dir):
        """Convert audio to AICA-compatible format"""
        for filename in os.listdir(audio_dir):
            if filename.lower().endswith('.wav'):
                audio_path = os.path.join(audio_dir, filename)
                self.convert_audio(audio_path, filename)
    
    def convert_audio(self, input_path, filename):
        """Convert WAV to AICA format"""
        with wave.open(input_path, 'rb') as wav:
            frames = wav.readframes(wav.getnframes())
            sample_rate = wav.getframerate()
            channels = wav.getnchannels()
            sample_width = wav.getsampwidth()
        
        # Convert to 16-bit mono 44.1kHz
        if sample_rate != 44100 or channels != 1 or sample_width != 2:
            # Use resampling (simplified - would use proper audio library)
            pass
        
        output_path = os.path.join(self.output_dir, f"{os.path.splitext(filename)[0]}.daudio")
        
        with open(output_path, 'wb') as f:
            f.write(b'DAUD')  # Magic number
            f.write(struct.pack('<I', 1))  # Version
            f.write(struct.pack('<I', 44100))  # Sample rate
            f.write(struct.pack('<H', 1))  # Channels
            f.write(struct.pack('<H', 16))  # Bits per sample
            f.write(struct.pack('<I', len(frames)))  # Data size
            f.write(frames)
    
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
            'audio': []
        }
        
        # Scan generated assets
        for filename in os.listdir(self.output_dir):
            if filename.endswith('.dtex'):
                manifest['textures'].append(filename)
            elif filename.endswith('.daudio'):
                manifest['audio'].append(filename)
        
        manifest_path = os.path.join(self.output_dir, 'manifest.json')
        with open(manifest_path, 'w') as f:
            json.dump(manifest, f, indent=2)

if __name__ == "__main__":
    processor = DWGEAssetProcessor()
    processor.process_all_assets()
    print("Asset processing complete!")
```

### Component Generator Tool

```bash
#!/bin/bash
# tools/create_component.sh

COMPONENT_NAME=$1

if [ -z "$COMPONENT_NAME" ]; then
    echo "Usage: $0 <component_name>"
    exit 1
fi

COMPONENT_DIR="wasm-components/$COMPONENT_NAME"

echo "Creating DWGE component: $COMPONENT_NAME"

# Create directory structure
mkdir -p "$COMPONENT_DIR/src"
mkdir -p "$COMPONENT_DIR/assets"

# Create Cargo.toml
cat > "$COMPONENT_DIR/Cargo.toml" << EOF
[package]
name = "$COMPONENT_NAME"
version = "0.1.0"
edition = "2021"

[lib]
crate-type = ["cdylib"]

[dependencies]
dwge-api = { path = "../../dwge-api" }

[profile.release]
opt-level = 3
lto = true
codegen-units = 1
panic = "abort"
EOF

# Create lib.rs template
cat > "$COMPONENT_DIR/src/lib.rs" << EOF
use dwge_api::*;

pub struct ${COMPONENT_NAME^}Component {
    // Component state here
    initialized: bool,
}

impl ${COMPONENT_NAME^}Component {
    pub fn new() -> Self {
        Self {
            initialized: false,
        }
    }
}

impl DWGEComponent for ${COMPONENT_NAME^}Component {
    fn init(&mut self, ctx: &ComponentContext) -> Result<(), ComponentError> {
        unsafe {
            dwge_log(LOG_INFO, format!("Initializing {} component", "$COMPONENT_NAME").as_ptr(), 0);
        }
        
        self.initialized = true;
        Ok(())
    }
    
    fn update(&mut self, ctx: &ComponentContext, input: &InputState) -> Result<(), ComponentError> {
        if !self.initialized {
            return Err(ComponentError::NotInitialized);
        }
        
        // Update logic here
        
        Ok(())
    }
    
    fn render(&mut self, ctx: &ComponentContext) -> Result<(), ComponentError> {
        if !self.initialized {
            return Ok(());
        }
        
        // Render logic here
        
        Ok(())
    }
    
    fn cleanup(&mut self, _ctx: &ComponentContext) {
        self.initialized = false;
    }
}

// Export component for DWGE runtime
#[no_mangle]
pub extern "C" fn dwge_component_create() -> *mut dyn DWGEComponent {
    Box::into_raw(Box::new(${COMPONENT_NAME^}Component::new()))
}

#[no_mangle]
pub extern "C" fn dwge_component_destroy(component: *mut dyn DWGEComponent) {
    if !component.is_null() {
        unsafe {
            let _ = Box::from_raw(component);
        }
    }
}
EOF

# Create component manifest
cat > "$COMPONENT_DIR/component.toml" << EOF
[component]
id = "$COMPONENT_NAME"
name = "${COMPONENT_NAME^} Component"
description = "Generated DWGE component"
version = "0.1.0"
priority = "medium"
max_memory = "1MB"
max_cycles_per_frame = 25000

[capabilities]
input = []
audio = []
graphics = []
storage = []
EOF

echo "Component '$COMPONENT_NAME' created successfully!"
echo "Edit $COMPONENT_DIR/src/lib.rs to implement your component logic."
echo "Add to your dwge.toml manifest to include in your game."
```

---

## Performance Benchmarks and Targets

### Expected Performance Characteristics

```
╭─────────────────────────────────────────────────────────╮
│                DWGE Performance Targets                 │
├─────────────────────────────────────────────────────────┤
│ Frame Rate: 60 FPS (16.67ms per frame)                 │
│ WASM Execution: 5ms budget per frame                   │
│ Graphics Rendering: 8ms budget per frame               │
│ Audio Processing: 2ms budget per frame                 │
│ System Overhead: 1.67ms budget per frame               │
├─────────────────────────────────────────────────────────┤
│ Memory Usage:                                           │
│   DWGE Runtime: 1MB                                    │
│   WASM Components: 8MB total                           │
│   Texture Memory: 6.8MB                                │
│   Audio Buffers: 1MB                                   │
├─────────────────────────────────────────────────────────┤
│ WASM Performance:                                       │
│   Instructions/sec: ~10M (optimistic)                  │
│   Function calls/frame: 10,000                         │
│   Memory allocations/frame: 100                        │
│   Component switches/frame: 8                          │
╰─────────────────────────────────────────────────────────╯
```

### Benchmark Test Suite

```c
// tools/benchmark/dwge_benchmarks.c

#include "dwge_runtime.h"
#include <kos.h>

typedef struct {
    const char* name;
    uint64_t (*benchmark_func)(void);
    uint64_t target_time_us;
    uint64_t actual_time_us;
    bool passed;
} BenchmarkTest;

// WASM instruction execution benchmark
uint64_t benchmark_wasm_execution() {
    WasmExecutionContext ctx;
    dwge_init_test_context(&ctx);
    
    // Load test WASM module with arithmetic operations
    uint8_t test_wasm[] = {
        // Simple arithmetic loop: for(i=0; i<1000; i++) { sum += i * 2; }
        0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,  // WASM header
        // ... test bytecode
    };
    
    dwge_load_module(&ctx, test_wasm, sizeof(test_wasm));
    
    uint64_t start_time = timer_us_gettime64();
    
    // Execute 1000 iterations
    for (int i = 0; i < 1000; i++) {
        dwge_call_function(&ctx, "test_function", NULL, 0);
    }
    
    uint64_t end_time = timer_us_gettime64();
    
    dwge_cleanup_context(&ctx);
    return end_time - start_time;
}

// Memory allocation benchmark
uint64_t benchmark_memory_allocation() {
    dwge_memory_init();
    
    uint64_t start_time = timer_us_gettime64();
    
    // Allocate and free 1000 blocks of various sizes
    void* ptrs[1000];
    for (int i = 0; i < 1000; i++) {
        ptrs[i] = dwge_memory_alloc(0, (i % 10 + 1) * 64);  // 64-640 bytes
    }
    
    for (int i = 0; i < 1000; i++) {
        dwge_memory_free(0, ptrs[i]);
    }
    
    uint64_t end_time = timer_us_gettime64();
    return end_time - start_time;
}

// Graphics rendering benchmark
uint64_t benchmark_sprite_rendering() {
    pvr_init_defaults();
    
    uint64_t start_time = timer_us_gettime64();
    
    pvr_wait_ready();
    pvr_scene_begin();
    
    // Render 1000 sprites
    for (int i = 0; i < 1000; i++) {
        float x = (i % 32) * 20.0f;
        float y = (i / 32) * 20.0f;
        dwge_pvr_draw_sprite(0, x, y, 1.0f);
    }
    
    pvr_scene_finish();
    
    uint64_t end_time = timer_us_gettime64();
    return end_time - start_time;
}

// Component switching benchmark  
uint64_t benchmark_component_switching() {
    ComponentScheduler scheduler;
    dwge_scheduler_init(&scheduler);
    
    // Create 8 test components
    for (int i = 0; i < 8; i++) {
        ScheduledComponent comp;
        comp.component_id = i;
        comp.priority = i;
        comp.max_cycles = 10000;
        comp.state = COMPONENT_ACTIVE;
        dwge_scheduler_add_component(&scheduler, &comp);
    }
    
    uint64_t start_time = timer_us_gettime64();
    
    // Execute 100 scheduling cycles
    for (int i = 0; i < 100; i++) {
        dwge_scheduler_update(&scheduler);
    }
    
    uint64_t end_time = timer_us_gettime64();
    return end_time - start_time;
}

// Audio mixing benchmark
uint64_t benchmark_audio_mixing() {
    dwge_aica_init();
    
    // Load test sounds
    uint32_t sound_ids[8];
    for (int i = 0; i < 8; i++) {
        sound_ids[i] = dwge_aica_load_test_sound(440 + i * 110);  // Different frequencies
    }
    
    uint64_t start_time = timer_us_gettime64();
    
    // Play all sounds simultaneously (stress test mixing)
    for (int i = 0; i < 8; i++) {
        dwge_aica_play_sound(sound_ids[i], 0.5f, 1.0f);
    }
    
    // Let them play for a short time
    thd_sleep(100);  // 100ms
    
    uint64_t end_time = timer_us_gettime64();
    return end_time - start_time;
}

static BenchmarkTest benchmarks[] = {
    {"WASM Execution", benchmark_wasm_execution, 5000, 0, false},         // 5ms target
    {"Memory Allocation", benchmark_memory_allocation, 1000, 0, false},   // 1ms target
    {"Sprite Rendering", benchmark_sprite_rendering, 8000, 0, false},     // 8ms target
    {"Component Switching", benchmark_component_switching, 500, 0, false}, // 0.5ms target
    {"Audio Mixing", benchmark_audio_mixing, 2000, 0, false},             // 2ms target
};

void dwge_run_benchmarks() {
    printf("DWGE Performance Benchmarks\n");
    printf("===========================\n\n");
    
    int passed = 0;
    int total = sizeof(benchmarks) / sizeof(BenchmarkTest);
    
    for (int i = 0; i < total; i++) {
        printf("Running %s... ", benchmarks[i].name);
        
        benchmarks[i].actual_time_us = benchmarks[i].benchmark_func();
        benchmarks[i].passed = benchmarks[i].actual_time_us <= benchmarks[i].target_time_us;
        
        if (benchmarks[i].passed) {
            printf("PASS (%llu/%llu us)\n", 
                   benchmarks[i].actual_time_us, 
                   benchmarks[i].target_time_us);
            passed++;
        } else {
            printf("FAIL (%llu/%llu us) - %.1fx over target\n",
                   benchmarks[i].actual_time_us,
                   benchmarks[i].target_time_us,
                   (float)benchmarks[i].actual_time_us / benchmarks[i].target_time_us);
        }
    }
    
    printf("\nBenchmark Results: %d/%d passed (%.1f%%)\n", 
           passed, total, (float)passed / total * 100.0f);
    
    if (passed == total) {
        printf("All benchmarks passed! DWGE performance targets met.\n");
    } else {
        printf("Some benchmarks failed. Performance optimization needed.\n");
    }
}
```

---

## Deployment and Distribution

### CDI Creation Pipeline

```python
# tools/build_cdi.py

import os
import struct
import subprocess
import shutil
from datetime import datetime

class DreamcastCDIBuilder:
    def __init__(self):
        self.build_dir = "build"
        self.dist_dir = "dist"
        self.assets_dir = "build/assets"
        
    def build_cdi(self, game_name, version):
        """Complete CDI build pipeline"""
        print(f"Building {game_name} v{version} for Dreamcast...")
        
        # Ensure directories exist
        os.makedirs(self.build_dir, exist_ok=True)
        os.makedirs(self.dist_dir, exist_ok=True)
        
        # Generate IP.BIN (Initial Program)
        self.generate_ip_bin(game_name, version)
        
        # Create file system structure
        self.create_filesystem()
        
        # Copy assets
        self.copy_assets()
        
        # Generate 1ST_READ.BIN
        self.prepare_executable()
        
        # Create ISO image
        iso_path = self.create_iso(game_name)
        
        # Convert to CDI
        cdi_path = self.create_cdi(iso_path, game_name, version)
        
        print(f"CDI created: {cdi_path}")
        return cdi_path
    
    def generate_ip_bin(self, game_name, version):
        """Generate Initial Program (IP.BIN) for Dreamcast boot"""
        ip_data = bytearray(32768)  # 32KB IP.BIN
        
        # Hardware info section
        ip_data[0:16] = b'SEGA SEGAKATANA '
        
        # Product info
        product_info = f"{game_name[:10]:<10}V{version[:6]:<6}"
        ip_data[16:32] = product_info.encode('ascii')[:16].ljust(16, b' ')
        
        # Release date
        release_date = datetime.now().strftime('%Y%m%d')
        ip_data[32:40] = release_date.encode('ascii')
        
        # Device info
        ip_data[64:80] = b'JUE             '  # Region (Japan, USA, Europe)
        
        # Boot code (simplified - jumps to 1ST_READ.BIN)
        boot_code = [
            0x6000, 0x4f22,  # mov.l @r0+, r4; sts.l pr, @-r15
            0x4f06, 0x8b0c,  # jmp @r4; bf $+24
            0x0009, 0x0000,  # nop; nop
        ]
        
        for i, instruction in enumerate(boot_code):
            struct.pack_into('<H', ip_data, 0x100 + i*2, instruction)
        
        # Write IP.BIN
        with open(f"{self.build_dir}/IP.BIN", 'wb') as f:
            f.write(ip_data)
    
    def create_filesystem(self):
        """Create Dreamcast filesystem structure"""
        fs_structure = [
            "0GDTEX",    # Texture data
            "0GDAUDIO",  # Audio data
            "0WDATA",    # WASM component data
            "0SAVES",    # Save game data
        ]
        
        for directory in fs_structure:
            os.makedirs(f"{self.build_dir}/{directory}", exist_ok=True)
    
    def copy_assets(self):
        """Copy processed assets to build directory"""
        if os.path.exists(self.assets_dir):
            # Copy textures
            for file in os.listdir(self.assets_dir):
                if file.endswith('.dtex'):
                    shutil.copy2(f"{self.assets_dir}/{file}", f"{self.build_dir}/0GDTEX/")
                elif file.endswith('.daudio'):
                    shutil.copy2(f"{self.assets_dir}/{file}", f"{self.build_dir}/0GDAUDIO/")
        
        # Copy WASM components
        wasm_dir = "build"
        for file in os.listdir(wasm_dir):
            if file.endswith('.wasm'):
                shutil.copy2(f"{wasm_dir}/{file}", f"{self.build_dir}/0WDATA/")
        
        # Copy manifest
        if os.path.exists(f"{self.assets_dir}/manifest.json"):
            shutil.copy2(f"{self.assets_dir}/manifest.json", f"{self.build_dir}/")
    
    def prepare_executable(self):
        """Prepare and scramble the main executable"""
        # The ELF should already be built by the Makefile
        elf_path = f"{self.build_dir}/dwge_game.elf"
        
        if not os.path.exists(elf_path):
            raise FileNotFoundError(f"Executable not found: {elf_path}")
        
        # Convert ELF to binary
        subprocess.run([
            "sh-elf-objcopy", 
            "-R", ".stack", 
            "-O", "binary", 
            elf_path, 
            f"{self.build_dir}/1ST_READ.BIN"
        ], check=True)
        
        # Scramble the binary (Dreamcast copy protection)
        subprocess.run([
            "scramble", 
            f"{self.build_dir}/1ST_READ.BIN", 
            f"{self.build_dir}/1ST_READ.BIN"
        ], check=True)
    
    def create_iso(self, game_name):
        """Create ISO image from filesystem"""
        iso_path = f"{self.build_dir}/{game_name.lower().replace(' ', '_')}.iso"
        
        # Use mkisofs to create the ISO
        subprocess.run([
            "mkisofs",
            "-C", "0,11702",  # Dreamcast specific
            "-V", game_name[:32],  # Volume label
            "-G", f"{self.build_dir}/IP.BIN",  # Initial program
            "-r",  # Rock Ridge extensions
            "-J",  # Joliet extensions  
            "-l",  # Allow 31 character filenames
            "-o", iso_path,
            self.build_dir
        ], check=True)
        
        return iso_path
    
    def create_cdi(self, iso_path, game_name, version):
        """Convert ISO to CDI format"""
        cdi_filename = f"{game_name.lower().replace(' ', '_')}_v{version}.cdi"
        cdi_path = f"{self.dist_dir}/{cdi_filename}"
        
        # Use cdi4dc tool to convert
        subprocess.run([
            "cdi4dc",
            iso_path,
            cdi_path
        ], check=True)
        
        return cdi_path

if __name__ == "__main__":
    import sys
    
    if len(sys.argv) != 3:
        print("Usage: python build_cdi.py <game_name> <version>")
        sys.exit(1)
    
    game_name = sys.argv[1]
    version = sys.argv[2]
    
    builder = DreamcastCDIBuilder()
    cdi_path = builder.build_cdi(game_name, version)
    
    print(f"\n✓ CDI build complete: {cdi_path}")
    print(f"✓ Ready for burning to CD-R or loading in emulator")
```

### Debugging and Development Tools

```python
# tools/debug_monitor.py

import socket
import struct
import json
import time
from dataclasses import dataclass
from typing import Dict, List, Optional

@dataclass
class ComponentStats:
    component_id: int
    name: str
    cycles_used: int
    memory_used: int
    state: str
    last_error: str

@dataclass
class SystemStats:
    frame_time_ms: float
    fps: float
    total_memory_used: int
    texture_memory_used: int
    audio_voices_active: int
    wasm_instructions_per_second: int

class DWGEDebugMonitor:
    def __init__(self, host='192.168.1.100', port=31313):
        """Connect to DWGE debug interface on Dreamcast"""
        self.host = host
        self.port = port
        self.socket = None
        self.connected = False
        
    def connect(self):
        """Connect to Dreamcast debug interface"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(5.0)
            self.socket.connect((self.host, self.port))
            self.connected = True
            print(f"Connected to DWGE debug interface at {self.host}:{self.port}")
            return True
        except Exception as e:
            print(f"Failed to connect: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from debug interface"""
        if self.socket:
            self.socket.close()
            self.connected = False
    
    def send_command(self, command: str, data: dict = None) -> dict:
        """Send debug command to DWGE runtime"""
        if not self.connected:
            return None
        
        message = {
            'command': command,
            'data': data or {}
        }
        
        try:
            # Send message
            json_data = json.dumps(message).encode('utf-8')
            length = struct.pack('<I', len(json_data))
            self.socket.send(length + json_data)
            
            # Receive response
            response_length = struct.unpack('<I', self.socket.recv(4))[0]
            response_data = self.socket.recv(response_length).decode('utf-8')
            return json.loads(response_data)
            
        except Exception as e:
            print(f"Command failed: {e}")
            return None
    
    def get_system_stats(self) -> Optional[SystemStats]:
        """Get current system performance statistics"""
        response = self.send_command('get_system_stats')
        if response and response.get('success'):
            data = response['data']
            return SystemStats(
                frame_time_ms=data['frame_time_ms'],
                fps=data['fps'],
                total_memory_used=data['total_memory_used'],
                texture_memory_used=data['texture_memory_used'],
                audio_voices_active=data['audio_voices_active'],
                wasm_instructions_per_second=data['wasm_instructions_per_second']
            )
        return None
    
    def get_component_stats(self) -> List[ComponentStats]:
        """Get statistics for all components"""
        response = self.send_command('get_component_stats')
        if response and response.get('success'):
            stats = []
            for comp_data in response['data']['components']:
                stats.append(ComponentStats(
                    component_id=comp_data['id'],
                    name=comp_data['name'],
                    cycles_used=comp_data['cycles_used'],
                    memory_used=comp_data['memory_used'],
                    state=comp_data['state'],
                    last_error=comp_data['last_error']
                ))
            return stats
        return []
    
    def reload_component(self, component_id: int) -> bool:
        """Hot-reload a component (development mode only)"""
        response = self.send_command('reload_component', {'component_id': component_id})
        return response and response.get('success', False)
    
    def set_breakpoint(self, component_id: int, function_name: str) -> bool:
        """Set debugging breakpoint in WASM component"""
        response = self.send_command('set_breakpoint', {
            'component_id': component_id,
            'function': function_name
        })
        return response and response.get('success', False)
    
    def get_wasm_callstack(self, component_id: int) -> List[str]:
        """Get current WASM call stack for component"""
        response = self.send_command('get_callstack', {'component_id': component_id})
        if response and response.get('success'):
            return response['data']['callstack']
        return []
    
    def run_monitor_loop(self):
        """Run interactive monitoring loop"""
        print("DWGE Debug Monitor")
        print("==================")
        print("Commands:")
        print("  stats    - Show system statistics")
        print("  comp     - Show component statistics") 
        print("  reload X - Reload component X")
        print("  break X func - Set breakpoint in component X function")
        print("  stack X  - Show call stack for component X")
        print("  quit     - Exit monitor")
        print()
        
        while self.connected:
            try:
                command = input("dwge> ").strip().split()
                if not command:
                    continue
                
                if command[0] == 'quit':
                    break
                elif command[0] == 'stats':
                    self.show_system_stats()
                elif command[0] == 'comp':
                    self.show_component_stats()
                elif command[0] == 'reload' and len(command) > 1:
                    comp_id = int(command[1])
                    if self.reload_component(comp_id):
                        print(f"Component {comp_id} reloaded successfully")
                    else:
                        print(f"Failed to reload component {comp_id}")
                elif command[0] == 'break' and len(command) > 2:
                    comp_id = int(command[1])
                    func_name = command[2]
                    if self.set_breakpoint(comp_id, func_name):
                        print(f"Breakpoint set in component {comp_id} function {func_name}")
                    else:
                        print(f"Failed to set breakpoint")
                elif command[0] == 'stack' and len(command) > 1:
                    comp_id = int(command[1])
                    callstack = self.get_wasm_callstack(comp_id)
                    print(f"Call stack for component {comp_id}:")
                    for i, frame in enumerate(callstack):
                        print(f"  {i}: {frame}")
                else:
                    print("Unknown command")
                    
            except KeyboardInterrupt:
                break
            except Exception as e:
                print(f"Error: {e}")
        
        self.disconnect()
    
    def show_system_stats(self):
        """Display current system statistics"""
        stats = self.get_system_stats()
        if stats:
            print(f"Frame Time: {stats.frame_time_ms:.2f}ms")
            print(f"FPS: {stats.fps:.1f}")
            print(f"Memory Used: {stats.total_memory_used // 1024}KB / 16MB")
            print(f"Texture Memory: {stats.texture_memory_used // 1024}KB / 8MB")
            print(f"Audio Voices: {stats.audio_voices_active} / 64")
            print(f"WASM Instructions/sec: {stats.wasm_instructions_per_second:,}")
        else:
            print("Failed to get system stats")
    
    def show_component_stats(self):
        """Display component statistics"""
        components = self.get_component_stats()
        if components:
            print(f"{'ID':<3} {'Name':<20} {'State':<10} {'Cycles':<8} {'Memory':<8} {'Error'}")
            print("-" * 70)
            for comp in components:
                memory_kb = comp.memory_used // 1024
                error = comp.last_error[:20] if comp.last_error else "None"
                print(f"{comp.component_id:<3} {comp.name:<20} {comp.state:<10} "
                      f"{comp.cycles_used:<8} {memory_kb:<8} {error}")
        else:
            print("Failed to get component stats")

if __name__ == "__main__":
    import sys
    
    host = sys.argv[1] if len(sys.argv) > 1 else '192.168.1.100'
    
    monitor = DWGEDebugMonitor(host)
    if monitor.connect():
        monitor.run_monitor_loop()
    else:
        print("Failed to connect to DWGE debug interface")
        print("Make sure your Dreamcast is connected to the network")
        print("and DWGE is running in debug mode")
```

---

## Example Game Implementation

### Complete 2D Platformer Example

```rust
// examples/platformer/player_component/src/lib.rs

use dwge_api::*;

const GRAVITY: f32 = 1200.0;
const JUMP_FORCE: f32 = -400.0;
const MOVE_SPEED: f32 = 200.0;
const TERMINAL_VELOCITY: f32 = 600.0;

pub struct PlayerComponent {
    position: (f32, f32),
    velocity: (f32, f32),
    on_ground: bool,
    facing_right: bool,
    
    // Sprites
    idle_sprite: u32,
    run_sprite: u32,
    jump_sprite: u32,
    
    // Audio
    jump_sound: u32,
    land_sound: u32,
    
    // Animation
    animation_time: f32,
    current_frame: u32,
    
    // Input buffering
    jump_buffer_time: f32,
    coyote_time: f32,
}

impl PlayerComponent {
    pub fn new() -> Self {
        Self {
            position: (100.0, 300.0),
            velocity: (0.0, 0.0),
            on_ground: false,
            facing_right: true,
            idle_sprite: 0,
            run_sprite: 0,
            jump_sprite: 0,
            jump_sound: 0,
            land_sound: 0,
            animation_time: 0.0,
            current_frame: 0,
            jump_buffer_time: 0.0,
            coyote_time: 0.0,
        }
    }
    
    fn handle_input(&mut self, input: &InputState, delta_time: f32) {
        let controller = &input.controller[0];
        
        if !controller.connected {
            return;
        }
        
        // Horizontal movement
        let mut input_x = 0.0f32;
        
        if (controller.buttons & DWGE_BUTTON_LEFT) != 0 {
            input_x -= 1.0;
        }
        if (controller.buttons & DWGE_BUTTON_RIGHT) != 0 {
            input_x += 1.0;
        }
        
        // Analog stick support
        let analog_x = controller.analog_x as f32 / 128.0;
        if analog_x.abs() > 0.1 {
            input_x = analog_x;
        }
        
        // Update facing direction
        if input_x > 0.1 {
            self.facing_right = true;
        } else if input_x < -0.1 {
            self.facing_right = false;
        }
        
        // Apply movement
        self.velocity.0 = input_x * MOVE_SPEED;
        
        // Jump input buffering
        if (controller.buttons & DWGE_BUTTON_A) != 0 {
            self.jump_buffer_time = 0.1; // 100ms buffer
        } else {
            self.jump_buffer_time -= delta_time;
        }
        
        // Coyote time (grace period after leaving ground)
        if self.on_ground {
            self.coyote_time = 0.1; // 100ms coyote time
        } else {
            self.coyote_time -= delta_time;
        }
        
        // Jump logic
        if self.jump_buffer_time > 0.0 && self.coyote_time > 0.0 {
            self.velocity.1 = JUMP_FORCE;
            self.on_ground = false;
            self.jump_buffer_time = 0.0;
            self.coyote_time = 0.0;
            
            unsafe {
                dwge_play_sound(self.jump_sound, 0.7, 1.0);
            }
        }
    }
    
    fn update_physics(&mut self, delta_time: f32) {
        // Apply gravity
        if !self.on_ground {
            self.velocity.1 += GRAVITY * delta_time;
            
            // Terminal velocity
            if self.velocity.1 > TERMINAL_VELOCITY {
                self.velocity.1 = TERMINAL_VELOCITY;
            }
        }
        
        // Update position
        self.position.0 += self.velocity.0 * delta_time;
        self.position.1 += self.velocity.1 * delta_time;
        
        // Simple ground collision (y = 400)
        let was_on_ground = self.on_ground;
        
        if self.position.1 >= 400.0 {
            self.position.1 = 400.0;
            
            if self.velocity.1 > 0.0 {
                self.velocity.1 = 0.0;
                self.on_ground = true;
                
                // Landing sound
                if !was_on_ground {
                    unsafe {
                        dwge_play_sound(self.land_sound, 0.5, 1.0);
                    }
                }
            }
        } else {
            self.on_ground = false;
        }
        
        // Screen boundaries
        if self.position.0 < 0.0 {
            self.position.0 = 0.0;
        } else if self.position.0 > 640.0 {
            self.position.0 = 640.0;
        }
    }
    
    fn update_animation(&mut self, delta_time: f32) {
        self.animation_time += delta_time;
        
        // Simple frame-based animation
        if self.animation_time > 0.1 { // 10 FPS animation
            self.current_frame = (self.current_frame + 1) % 4;
            self.animation_time = 0.0;
        }
    }
    
    fn get_current_sprite(&self) -> u32 {
        if !self.on_ground {
            self.jump_sprite
        } else if self.velocity.0.abs() > 10.0 {
            self.run_sprite
        } else {
            self.idle_sprite
        }
    }
}

impl DWGEComponent for PlayerComponent {
    fn init(&mut self, ctx: &ComponentContext) -> Result<(), ComponentError> {
        unsafe {
            dwge_log(LOG_INFO, b"Initializing Player Component\0".as_ptr(), 26);
            
            // Load player sprites
            let idle_data = include_bytes!("../assets/player_idle.png");
            self.idle_sprite = dwge_load_texture(idle_data.as_ptr(), idle_data.len() as u32);
            
            let run_data = include_bytes!("../assets/player_run.png");
            self.run_sprite = dwge_load_texture(run_data.as_ptr(), run_data.len() as u32);
            
            let jump_data = include_bytes!("../assets/player_jump.png");
            self.jump_sprite = dwge_load_texture(jump_data.as_ptr(), jump_data.len() as u32);
            
            // Load audio
            let jump_audio = include_bytes!("../assets/jump.wav");
            self.jump_sound = dwge_load_sound(jump_audio.as_ptr(), jump_audio.len() as u32);
            
            let land_audio = include_bytes!("../assets/land.wav");
            self.land_sound = dwge_load_sound(land_audio.as_ptr(), land_audio.len() as u32);
        }
        
        Ok(())
    }
    
    fn update(&mut self, ctx: &ComponentContext, input: &InputState) -> Result<(), ComponentError> {
        self.handle_input(input, ctx.delta_time);
        self.update_physics(ctx.delta_time);
        self.update_animation(ctx.delta_time);
        
        Ok(())
    }
    
    fn render(&mut self, ctx: &ComponentContext) -> Result<(), ComponentError> {
        let sprite_id = self.get_current_sprite();
        let scale = if self.facing_right { 1.0 } else { -1.0 };
        
        unsafe {
            dwge_draw_sprite(sprite_id, self.position.0, self.position.1, scale);
        }
        
        Ok(())
    }
    
    fn cleanup(&mut self, _ctx: &ComponentContext) {
        // Cleanup handled automatically by DWGE runtime
    }
}

#[no_mangle]
pub extern "C" fn dwge_component_create() -> *mut dyn DWGEComponent {
    Box::into_raw(Box::new(PlayerComponent::new()))
}

#[no_mangle]
pub extern "C" fn dwge_component_destroy(component: *mut dyn DWGEComponent) {
    if !component.is_null() {
        unsafe {
            let _ = Box::from_raw(component);
        }
    }
}
```

### World/Level Component

```rust
// examples/platformer/world_component/src/lib.rs

use dwge_api::*;

const TILE_SIZE: f32 = 32.0;
const WORLD_WIDTH: usize = 40;
const WORLD_HEIGHT: usize = 15;

pub struct WorldComponent {
    tiles: [[u8; WORLD_WIDTH]; WORLD_HEIGHT],
    tile_textures: [u32; 16],
    background_texture: u32,
    camera_x: f32,
    camera_y: f32,
}

impl WorldComponent {
    pub fn new() -> Self {
        Self {
            tiles: [[0; WORLD_WIDTH]; WORLD_HEIGHT],
            tile_textures: [0; 16],
            background_texture: 0,
            camera_x: 0.0,
            camera_y: 0.0,
        }
    }
    
    fn load_level(&mut self) {
        // Simple level layout
        for y in 0..WORLD_HEIGHT {
            for x in 0..WORLD_WIDTH {
                if y == WORLD_HEIGHT - 3 {
                    self.tiles[y][x] = 1; // Ground tile
                } else if y > WORLD_HEIGHT - 3 {
                    self.tiles[y][x] = 2; // Dirt tile
                } else if x == 10 && y == WORLD_HEIGHT - 7 {
                    self.tiles[y][x] = 3; // Platform
                } else if x == 20 && y >= WORLD_HEIGHT - 6 && y <= WORLD_HEIGHT - 4 {
                    self.tiles[y][x] = 1; // Small tower
                } else {
                    self.tiles[y][x] = 0; // Empty
                }
            }
        }
    }
}

impl DWGEComponent for WorldComponent {
    fn init(&mut self, ctx: &ComponentContext) -> Result<(), ComponentError> {
        unsafe {
            dwge_log(LOG_INFO, b"Initializing World Component\0".as_ptr(), 27);
            
            // Load background
            let bg_data = include_bytes!("../assets/background.png");
            self.background_texture = dwge_load_texture(bg_data.as_ptr(), bg_data.len() as u32);
            
            // Load tile textures
            let tile_data = include_bytes!("../assets/tileset.png");
            self.tile_textures[0] = 0; // Empty tile
            for i in 1..16 {
                self.tile_textures[i] = dwge_load_texture(tile_data.as_ptr(), tile_data.len() as u32);
            }
        }
        
        self.load_level();
        Ok(())
    }
    
    fn update(&mut self, ctx: &ComponentContext, _input: &InputState) -> Result<(), ComponentError> {
        // Update camera (follow player - would need inter-component communication)
        // For now, static camera
        self.camera_x = 0.0;
        self.camera_y = 0.0;
        
        Ok(())
    }
    
    fn render(&mut self, ctx: &ComponentContext) -> Result<(), ComponentError> {
        unsafe {
            // Set camera
            dwge_set_camera(self.camera_x, self.camera_y, 1.0);
            
            // Draw background
            dwge_draw_sprite(self.background_texture, 320.0, 240.0, 1.0);
            
            // Draw tiles
            for y in 0..WORLD_HEIGHT {
                for x in 0..WORLD_WIDTH {
                    let tile_id = self.tiles[y][x];
                    if tile_id > 0 {
                        let world_x = x as f32 * TILE_SIZE;
                        let world_y = y as f32 * TILE_SIZE;
                        dwge_draw_tile(self.tile_textures[tile_id as usize], x as u32, y as u32);
                    }
                }
            }
        }
        
        Ok(())
    }
    
    fn cleanup(&mut self, _ctx: &ComponentContext) {
        // Cleanup handled by runtime
    }
}

#[no_mangle]
pub extern "C" fn dwge_component_create() -> *mut dyn DWGEComponent {
    Box::into_raw(Box::new(WorldComponent::new()))
}
```

### Game Manifest

```toml
# examples/platformer/dwge.toml

[game]
name = "Retro Platformer"
version = "1.0.0"
author = "DWGE Examples"
description = "A classic 2D platformer showcasing DWGE capabilities"
target_fps = 60
memory_budget = "10MB"

[[component]]
id = "world"
source = "world_component/target/wasm32-unknown-unknown/release/world_component.wasm"
priority = "low"
max_memory = "2MB"
max_cycles_per_frame = 20000

[component.capabilities]
graphics = ["tile_renderer", "sprite_renderer"]

[[component]]
id = "player"
source = "player_component/target/wasm32-unknown-unknown/release/player_component.wasm"
priority = "high"
max_memory = "1MB"
max_cycles_per_frame = 30000

[component.capabilities]
input = ["controller_0"]
audio = ["sfx_channel_0", "sfx_channel_1"]
graphics = ["sprite_renderer"]

[[component]]
id = "audio_manager"
source = "audio_component/target/wasm32-unknown-unknown/release/audio_component.wasm"
priority = "medium"
max_memory = "512KB"
max_cycles_per_frame = 15000

[component.capabilities]
audio = ["music_channel", "sfx_mix"]

[graphics]
resolution = "640x480"
color_depth = 16
texture_format = "rgb565"
max_textures = 128

[audio]
sample_rate = 44100
channels = 2
buffer_size = 1024
max_voices = 32

[input]
controllers = 1
analog_deadzone = 0.15
```

---

## Future Roadmap and Extensions

### Phase 2: Advanced Features

```markdown
## DWGE 2.0 Roadmap

### Networking Extensions
- **Peer-to-peer multiplayer** (2-4 Dreamcasts via crossover cable)
- **Internet play** (via broadband adapter)
- **Custom UDP protocol** for low-latency game state sync
- **Anti-cheat validation** through deterministic simulation

### Graphics Enhancements
- **3D rendering support** using PowerVR CLX2 hardware features
- **Advanced lighting models** (per-vertex and texture-based)
- **Particle systems** with hardware acceleration
- **Multi-texturing** and texture animations

### Audio Improvements
- **3D spatial audio** using AICA's built-in DSP
- **Real-time audio synthesis** (FM, subtractive, granular)
- **Music streaming** from GD-ROM with seamless loops
- **Voice chat** over network connections

### Development Tools
- **Visual component editor** (drag-and-drop interface)
- **Real-time performance profiler** with flame graphs
- **Asset optimization pipeline** with automatic compression
- **Cross-platform deployment** (Windows, Linux, macOS tools)

### Extended Hardware Support
- **VMU integration** (mini-games, save data visualization)
- **Keyboard/mouse support** for development and debug
- **Network adapter optimization** for faster data transfer
- **Custom peripherals** (light guns, arcade sticks, etc.)
```

### Community and Ecosystem

```markdown
## DWGE Community Initiative

### Open Source Components
- **Core runtime** (MIT license)
- **Standard library** of common game components
- **Asset processing tools** and format specifications
- **Documentation** and tutorial materials

### Developer Resources
- **Component marketplace** for sharing reusable modules
- **Best practices guide** for performance optimization
- **Porting guides** from other platforms
- **Hardware interfacing** documentation

### Contest and Recognition
- **Annual DWGE Game Jam** (retro-themed competition)
- **Performance optimization challenges**
- **Technical innovation awards**
- **Community showcase** events

### Educational Outreach
- **University partnerships** for game development courses
- **Workshop materials** for teaching WebAssembly concepts
- **Historical preservation** projects for Dreamcast software
- **Cross-generational** knowledge transfer programs
```

---

## Technical Appendices

### Appendix A: WebAssembly Instruction Set Coverage

```c
// DWGE supports subset of WebAssembly 1.0 specification
// Optimized for game development use cases

// Supported Instructions:
// - Numeric: i32/i64/f32/f64 operations (200+ instructions)
// - Memory: load/store operations with alignment
// - Control: blocks, loops, branches, calls
// - Variable: local/global get/set
// - Parametric: drop, select

// Unsupported Instructions:
// - SIMD (v128 operations) - requires newer hardware
// - Atomic operations - single-threaded execution model
// - Reference types - not needed for game logic
// - Bulk memory operations - optimization for later

// Performance Notes:
// - Interpreter executes ~10M instructions/second on SH-4
// - Function calls have ~100 cycle overhead
// - Memory access aligned to 4-byte boundaries optimal
// - Branch prediction helps with loop-heavy code
```

### Appendix B: Memory Layout Specification

```
Dreamcast Memory Map for DWGE:

0x8C000000 - 0x8C1FFFFF (2MB):   KallistiOS + System
0x8C200000 - 0x8C2FFFFF (1MB):   DWGE Runtime Code
0x8C300000 - 0x8CAFFFFF (8MB):   WASM Linear Memory Pool
0x8CB00000 - 0x8CBFFFFF (1MB):   Audio Sample Buffer
0x8CC00000 - 0x8CFFFFFF (4MB):   Asset Loading / Scratch

Video Memory (PowerVR):
0xA5000000 - 0xA5095FFF (600KB): Front Buffer (640x480x16)
0xA5096000 - 0xA512BFFF (600KB): Back Buffer (640x480x16)
0xA512C000 - 0xA57FFFFF (6.8MB): Texture Memory

Memory Protection:
- WASM components cannot access system memory
- Each component isolated to 2MB max allocation
- Bounds checking on all memory operations
- Stack overflow protection via guard pages
```

### Appendix C: Performance Optimization Guidelines

```markdown
## DWGE Performance Best Practices

### WASM Component Optimization
1. **Minimize function calls** across component boundaries
2. **Use local variables** instead of global state when possible
3. **Avoid frequent memory allocations** during gameplay
4. **Prefer integer math** over floating-point when appropriate
5. **Use component-local caching** for frequently accessed data

### Graphics Optimization
1. **Batch sprite draws** by texture to reduce state changes
2. **Use texture atlases** to minimize texture memory usage
3. **Sort draws by depth** to enable early Z-rejection
4. **Avoid overdraw** by culling off-screen objects
5. **Use 16-bit textures** (RGB565) for better memory bandwidth

### Audio Optimization
1. **Pre-load frequently used sounds** at startup
2. **Use lower sample rates** (22kHz) for sound effects
3. **Compress audio data** using ADPCM when possible
4. **Limit concurrent voices** to 8-16 for optimal mixing
5. **Use audio streaming** for background music

### Memory Management
1. **Pool allocate objects** to avoid fragmentation
2. **Use stack allocation** for temporary data
3. **Free resources immediately** when no longer needed
4. **Monitor peak memory usage** during development
5. **Implement custom allocators** for specific use cases
```

---

## Conclusion

The Dreamcast WASM Game Engine (DWGE) represents a unique fusion of cutting-edge WebAssembly technology with classic 1990s console hardware. While the technical challenges are immense, the resulting system would offer unprecedented capabilities:

**Revolutionary Aspects:**
- First WebAssembly runtime for retro hardware
- Modern component-based architecture on legacy systems
- Cross-platform development targeting historical hardware
- Educational bridge between modern and retro development

**Practical Applications:**
- Homebrew game development with modern tools
- Academic research into runtime optimization
- Historical software preservation projects
- Cross-generational knowledge transfer

**Technical Achievement:**
- Sub-10ms WASM execution budget on 200MHz hardware
- 60fps performance with multiple concurrent components
- Memory-efficient runtime in under 1MB footprint
- Hardware-optimized instruction interpretation

While this project would require significant engineering effort and deep expertise in both WebAssembly internals and Dreamcast hardware programming, it would result in a truly unique development platform that brings the best of modern software architecture to beloved retro hardware.

The DWGE would not just be a game engine—it would be a testament to the enduring principles of good software design, proving that elegant architectures can transcend hardware generations and continue to provide value decades after their inception.

**Total Development Estimate:** 18-24 months with a dedicated team of 3-4 experienced systems programmers.

**Community Impact:** High - would revitalize Dreamcast homebrew scene and demonstrate WebAssembly's versatility beyond web browsers.

**Technical Legacy:** Pioneering work in constrained-resource WebAssembly execution that could inform embedded systems and IoT device programming.

---

*This document represents a comprehensive technical design for bringing Flight's component architecture to Sega Dreamcast through WebAssembly. While ambitious, every aspect has been carefully considered for feasibility within the hardware constraints of the legendary console.*
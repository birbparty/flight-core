# UWRC Component Model Implementation

## Overview

The Component Model is UWRC's most innovative feature - it implements the cutting-edge WebAssembly Component Model specification to enable true polyglot composition of game components. This document details the complete implementation strategy based on the wazero fork approach described in the Flight project documentation.

## Why Component Model vs Traditional WASM Modules

### Traditional WASM Modules (Limited)
```cpp
// Old approach - basic modules with simple types
auto module = load_wasm_module("physics.wasm");
auto update_func = module.get_function("update");

// Primitive interface - just numbers
std::vector<uint64_t> args = {reinterpret_cast<uint64_t>(&game_state)};
auto result = update_func.call(args);
```

### Component Model (Revolutionary)
```cpp
// New approach - rich type system with true interop
auto physics_component = ComponentRegistry::load("physics-engine@2.1.0");
auto renderer_component = ComponentRegistry::load("vulkan-renderer@3.0.0");

// Type-safe interface with complex types
RigidBodyDef body_def{
    .mass = 1.0f,
    .position = {10.0f, 20.0f, 0.0f},
    .shape = CollisionShape::Box{width: 1.0f, height: 2.0f, depth: 1.0f}
};

auto body_handle = physics_component.add_rigid_body(body_def);
renderer_component.attach_visual(body_handle, "player_model.mesh");
```

## Core Architecture

### Component Model Runtime Structure
```cpp
// uwrc-component-model/include/uwrc/component_model/runtime.h

namespace uwrc::component_model {

class ComponentModelRuntime {
private:
    // Core subsystems
    std::unique_ptr<CanonicalABI> canonical_abi_;
    std::unique_ptr<ResourceTable> resource_table_;
    std::unique_ptr<TypeSystem> type_system_;
    std::unique_ptr<ComponentLinker> linker_;
    std::unique_ptr<InterfaceRegistry> interface_registry_;
    
    // Component management
    std::unordered_map<ComponentId, std::unique_ptr<ComponentInstance>> instances_;
    std::unordered_map<std::string, std::unique_ptr<ComponentDefinition>> definitions_;
    
    // Performance optimization
    std::unique_ptr<CallSiteCache> call_cache_;
    std::unique_ptr<TypeConversionCache> type_cache_;

public:
    // Component lifecycle
    Result<ComponentInstance*> instantiate_component(
        const ComponentDefinition& definition,
        const InstantiationConfig& config = {}
    );
    
    Result<void> destroy_component(ComponentId id);
    
    // Component linking and composition
    Result<void> link_components(
        ComponentId consumer,
        const std::string& import_name,
        ComponentId provider,
        const std::string& export_name
    );
    
    Result<CompositeComponent> compose_components(
        const std::vector<ComponentId>& components,
        const CompositionGraph& graph
    );
    
    // Interface management
    Result<void> register_host_interface(
        const std::string& interface_name,
        std::unique_ptr<HostInterface> interface
    );
    
    // Type system access
    TypeSystem& types() { return *type_system_; }
    ResourceTable& resources() { return *resource_table_; }
};

} // namespace uwrc::component_model
```

## Canonical ABI Implementation

The Canonical ABI is the bridge between Component Model types and core WebAssembly. This is one of the most complex parts of the implementation.

### Core Canonical ABI Class
```cpp
// uwrc-component-model/include/uwrc/component_model/canonical_abi.h

class CanonicalABI {
private:
    MemoryDriver* memory_;
    Function* realloc_func_;
    StringEncoding encoding_;
    
    // Optimization caches
    mutable std::unordered_map<TypeId, TypeInfo> type_info_cache_;
    mutable std::unordered_map<std::string, EncodedString> string_cache_;

public:
    // Lifting operations (Core WASM → Component Model)
    Result<Value> lift_value(uint32_t offset, const ComponentType& type) const;
    Result<std::string> lift_string(uint32_t ptr, uint32_t len) const;
    Result<std::vector<Value>> lift_list(uint32_t ptr, uint32_t len, const ComponentType& elem_type) const;
    Result<Record> lift_record(uint32_t ptr, const RecordType& type) const;
    Result<Variant> lift_variant(uint32_t discriminant, uint32_t payload_ptr, const VariantType& type) const;
    
    // Lowering operations (Component Model → Core WASM)
    Result<uint32_t> lower_value(const Value& value, const ComponentType& type);
    Result<std::pair<uint32_t, uint32_t>> lower_string(const std::string& str);
    Result<std::pair<uint32_t, uint32_t>> lower_list(const std::vector<Value>& list, const ComponentType& elem_type);
    Result<uint32_t> lower_record(const Record& record, const RecordType& type);
    Result<std::pair<uint32_t, uint32_t>> lower_variant(const Variant& variant, const VariantType& type);
    
    // Memory management for canonical ABI
    Result<uint32_t> allocate(uint32_t size, uint32_t alignment);
    void deallocate(uint32_t ptr, uint32_t size);
    
private:
    // Implementation helpers
    uint32_t align_to(uint32_t offset, uint32_t alignment) const;
    uint32_t size_of_type(const ComponentType& type) const;
    uint32_t alignment_of_type(const ComponentType& type) const;
};
```

### String Handling Implementation
```cpp
// String lifting with proper encoding support
Result<std::string> CanonicalABI::lift_string(uint32_t ptr, uint32_t len) const {
    if (len == 0) {
        return std::string{};
    }
    
    // Check cache first for frequently used strings
    auto cache_key = std::make_pair(ptr, len);
    if (auto it = string_cache_.find(cache_key); it != string_cache_.end()) {
        return it->second.decoded_string;
    }
    
    // Read raw bytes from memory
    std::vector<uint8_t> bytes(len);
    auto read_result = memory_->read(ptr, bytes.data(), len);
    if (!read_result.is_ok()) {
        return read_result.error();
    }
    
    // Decode based on string encoding (UTF-8, UTF-16, Latin1)
    std::string decoded;
    switch (encoding_) {
        case StringEncoding::UTF8:
            decoded = decode_utf8(bytes);
            break;
        case StringEncoding::UTF16:
            decoded = decode_utf16(bytes);
            break;
        case StringEncoding::Latin1:
            decoded = decode_latin1(bytes);
            break;
    }
    
    // Cache for future use (with LRU eviction)
    if (string_cache_.size() > MAX_STRING_CACHE_SIZE) {
        evict_least_recently_used_string();
    }
    string_cache_[cache_key] = {decoded, std::chrono::steady_clock::now()};
    
    return decoded;
}

// String lowering with automatic memory allocation
Result<std::pair<uint32_t, uint32_t>> CanonicalABI::lower_string(const std::string& str) {
    if (str.empty()) {
        return std::make_pair(0u, 0u);
    }
    
    // Encode string to bytes
    std::vector<uint8_t> encoded_bytes;
    switch (encoding_) {
        case StringEncoding::UTF8:
            encoded_bytes = encode_utf8(str);
            break;
        case StringEncoding::UTF16:
            encoded_bytes = encode_utf16(str);
            break;
        case StringEncoding::Latin1:
            encoded_bytes = encode_latin1(str);
            break;
    }
    
    // Allocate memory using component's realloc
    uint32_t ptr = allocate(encoded_bytes.size(), 1);
    if (ptr == 0) {
        return Error{ErrorCategory::Memory, "Failed to allocate string memory"};
    }
    
    // Write bytes to allocated memory
    auto write_result = memory_->write(ptr, encoded_bytes.data(), encoded_bytes.size());
    if (!write_result.is_ok()) {
        deallocate(ptr, encoded_bytes.size());
        return write_result.error();
    }
    
    return std::make_pair(ptr, static_cast<uint32_t>(encoded_bytes.size()));
}
```

### Complex Type Handling
```cpp
// Record (struct) lifting
Result<Record> CanonicalABI::lift_record(uint32_t ptr, const RecordType& type) const {
    Record record;
    uint32_t offset = 0;
    
    for (const auto& field : type.fields()) {
        // Align to field requirements
        offset = align_to(offset, alignment_of_type(field.type()));
        
        // Lift field value
        auto field_value = lift_value(ptr + offset, field.type());
        if (!field_value.is_ok()) {
            return field_value.error();
        }
        
        record.set_field(field.name(), field_value.value());
        offset += size_of_type(field.type());
    }
    
    return record;
}

// Variant (tagged union) lifting
Result<Variant> CanonicalABI::lift_variant(
    uint32_t discriminant, 
    uint32_t payload_ptr, 
    const VariantType& type) const {
    
    if (discriminant >= type.cases().size()) {
        return Error{ErrorCategory::Parse, "Invalid variant discriminant"};
    }
    
    const auto& variant_case = type.cases()[discriminant];
    
    // Handle unit variants (no payload)
    if (!variant_case.payload_type()) {
        return Variant{variant_case.name(), std::nullopt};
    }
    
    // Lift payload for non-unit variants
    auto payload_value = lift_value(payload_ptr, *variant_case.payload_type());
    if (!payload_value.is_ok()) {
        return payload_value.error();
    }
    
    return Variant{variant_case.name(), payload_value.value()};
}

// List lifting with proper element handling
Result<std::vector<Value>> CanonicalABI::lift_list(
    uint32_t ptr, 
    uint32_t len, 
    const ComponentType& elem_type) const {
    
    std::vector<Value> elements;
    elements.reserve(len);
    
    uint32_t elem_size = size_of_type(elem_type);
    uint32_t elem_align = alignment_of_type(elem_type);
    
    for (uint32_t i = 0; i < len; ++i) {
        uint32_t elem_offset = ptr + i * elem_size;
        
        // Ensure proper alignment for this element
        elem_offset = align_to(elem_offset, elem_align);
        
        auto element = lift_value(elem_offset, elem_type);
        if (!element.is_ok()) {
            return element.error();
        }
        
        elements.push_back(element.value());
    }
    
    return elements;
}
```

## Resource Handle Management

Resource handles are a critical Component Model feature that provides ownership semantics similar to Rust.

### Resource Table Implementation
```cpp
// uwrc-component-model/include/uwrc/component_model/resource_table.h

class ResourceTable {
private:
    struct ResourceEntry {
        ResourceTypeId type_id;
        std::unique_ptr<void, std::function<void(void*)>> data;
        bool owned;
        std::atomic<int32_t> borrow_count{0};
        ComponentId owner;
        std::chrono::steady_clock::time_point created_at;
        
        ResourceEntry(ResourceTypeId tid, std::unique_ptr<void, std::function<void(void*)>> d, 
                     bool o, ComponentId own)
            : type_id(tid), data(std::move(d)), owned(o), owner(own)
            , created_at(std::chrono::steady_clock::now()) {}
    };
    
    mutable std::shared_mutex table_mutex_;
    std::unordered_map<ResourceHandle, std::unique_ptr<ResourceEntry>> entries_;
    std::atomic<ResourceHandle> next_handle_{1};
    
    // Resource type registry
    std::unordered_map<ResourceTypeId, ResourceTypeInfo> type_registry_;

public:
    // Create owned resource handle
    template<typename T>
    ResourceHandle create_own(ComponentId owner, std::unique_ptr<T> resource) {
        auto handle = next_handle_.fetch_add(1);
        auto deleter = [](void* ptr) { delete static_cast<T*>(ptr); };
        auto entry = std::make_unique<ResourceEntry>(
            get_type_id<T>(),
            std::unique_ptr<void, std::function<void(void*)>>(resource.release(), deleter),
            true,  // owned
            owner
        );
        
        std::unique_lock lock(table_mutex_);
        entries_[handle] = std::move(entry);
        return handle;
    }
    
    // Borrow resource (increment reference count)
    template<typename T>
    Result<BorrowedResource<T>> borrow(ResourceHandle handle) {
        std::shared_lock lock(table_mutex_);
        
        auto it = entries_.find(handle);
        if (it == entries_.end()) {
            return Error{ErrorCategory::Runtime, "Invalid resource handle"};
        }
        
        auto& entry = *it->second;
        
        // Type check
        if (entry.type_id != get_type_id<T>()) {
            return Error{ErrorCategory::Runtime, "Resource type mismatch"};
        }
        
        // Increment borrow count
        entry.borrow_count.fetch_add(1);
        
        return BorrowedResource<T>{
            static_cast<T*>(entry.data.get()),
            handle,
            this
        };
    }
    
    // Drop owned resource
    Result<void> drop_own(ResourceHandle handle, ComponentId dropper) {
        std::unique_lock lock(table_mutex_);
        
        auto it = entries_.find(handle);
        if (it == entries_.end()) {
            return Error{ErrorCategory::Runtime, "Invalid resource handle"};
        }
        
        auto& entry = *it->second;
        
        // Verify ownership
        if (!entry.owned || entry.owner != dropper) {
            return Error{ErrorCategory::Runtime, "Cannot drop resource - not owner"};
        }
        
        // Check for outstanding borrows
        if (entry.borrow_count.load() > 0) {
            return Error{ErrorCategory::Runtime, "Cannot drop resource - outstanding borrows"};
        }
        
        // Resource destructor called automatically via unique_ptr
        entries_.erase(it);
        return {};
    }
    
private:
    void return_borrow(ResourceHandle handle) {
        std::shared_lock lock(table_mutex_);
        auto it = entries_.find(handle);
        if (it != entries_.end()) {
            it->second->borrow_count.fetch_sub(1);
        }
    }
    
    template<typename T>
    friend class BorrowedResource;
};

// RAII wrapper for borrowed resources
template<typename T>
class BorrowedResource {
private:
    T* resource_;
    ResourceHandle handle_;
    ResourceTable* table_;

public:
    BorrowedResource(T* resource, ResourceHandle handle, ResourceTable* table)
        : resource_(resource), handle_(handle), table_(table) {}
    
    ~BorrowedResource() {
        if (table_) {
            table_->return_borrow(handle_);
        }
    }
    
    // Move-only type
    BorrowedResource(const BorrowedResource&) = delete;
    BorrowedResource& operator=(const BorrowedResource&) = delete;
    
    BorrowedResource(BorrowedResource&& other) noexcept
        : resource_(other.resource_), handle_(other.handle_), table_(other.table_) {
        other.table_ = nullptr;
    }
    
    BorrowedResource& operator=(BorrowedResource&& other) noexcept {
        if (this != &other) {
            if (table_) {
                table_->return_borrow(handle_);
            }
            resource_ = other.resource_;
            handle_ = other.handle_;
            table_ = other.table_;
            other.table_ = nullptr;
        }
        return *this;
    }
    
    T* operator->() const { return resource_; }
    T& operator*() const { return *resource_; }
    T* get() const { return resource_; }
};
```

## Type System Implementation

The Component Model has a rich type system that goes far beyond WebAssembly's basic number types.

### Component Type Hierarchy
```cpp
// uwrc-component-model/include/uwrc/component_model/types.h

enum class TypeKind {
    // Primitive types
    Bool, S8, U8, S16, U16, S32, U32, S64, U64, F32, F64, Char,
    
    // Composite types
    String, List, Record, Variant, Tuple, Flags, Enum, Option, Result,
    
    // Resource types
    Own, Borrow
};

class ComponentType {
public:
    virtual ~ComponentType() = default;
    virtual TypeKind kind() const = 0;
    virtual uint32_t size() const = 0;
    virtual uint32_t alignment() const = 0;
    virtual std::string to_string() const = 0;
    
    // Type equality and compatibility
    virtual bool equals(const ComponentType& other) const = 0;
    virtual bool is_subtype_of(const ComponentType& other) const = 0;
};

// Primitive types
class PrimitiveType : public ComponentType {
private:
    TypeKind kind_;

public:
    explicit PrimitiveType(TypeKind kind) : kind_(kind) {}
    
    TypeKind kind() const override { return kind_; }
    
    uint32_t size() const override {
        switch (kind_) {
            case TypeKind::Bool:
            case TypeKind::S8:
            case TypeKind::U8: return 1;
            case TypeKind::S16:
            case TypeKind::U16: return 2;
            case TypeKind::S32:
            case TypeKind::U32:
            case TypeKind::F32:
            case TypeKind::Char: return 4;
            case TypeKind::S64:
            case TypeKind::U64:
            case TypeKind::F64: return 8;
            default: return 0;
        }
    }
    
    uint32_t alignment() const override { return size(); }
};

// Record type (struct)
class RecordType : public ComponentType {
public:
    struct Field {
        std::string name;
        std::unique_ptr<ComponentType> type;
        
        Field(std::string n, std::unique_ptr<ComponentType> t)
            : name(std::move(n)), type(std::move(t)) {}
    };

private:
    std::vector<Field> fields_;
    mutable std::optional<uint32_t> cached_size_;
    mutable std::optional<uint32_t> cached_alignment_;

public:
    explicit RecordType(std::vector<Field> fields) : fields_(std::move(fields)) {}
    
    TypeKind kind() const override { return TypeKind::Record; }
    
    const std::vector<Field>& fields() const { return fields_; }
    
    uint32_t size() const override {
        if (!cached_size_) {
            uint32_t total_size = 0;
            for (const auto& field : fields_) {
                total_size = align_to(total_size, field.type->alignment());
                total_size += field.type->size();
            }
            cached_size_ = total_size;
        }
        return *cached_size_;
    }
    
    uint32_t alignment() const override {
        if (!cached_alignment_) {
            uint32_t max_alignment = 1;
            for (const auto& field : fields_) {
                max_alignment = std::max(max_alignment, field.type->alignment());
            }
            cached_alignment_ = max_alignment;
        }
        return *cached_alignment_;
    }
    
    // Record subtyping: subtype can have additional fields
    bool is_subtype_of(const ComponentType& other) const override {
        if (other.kind() != TypeKind::Record) {
            return false;
        }
        
        const auto& other_record = static_cast<const RecordType&>(other);
        
        // Build map of supertype fields
        std::unordered_map<std::string, const ComponentType*> super_fields;
        for (const auto& field : other_record.fields()) {
            super_fields[field.name] = field.type.get();
        }
        
        // Check that all supertype fields exist and are compatible
        for (const auto& field : fields_) {
            auto it = super_fields.find(field.name);
            if (it != super_fields.end()) {
                if (!field.type->is_subtype_of(*it->second)) {
                    return false;
                }
            }
        }
        
        return true;
    }

private:
    static uint32_t align_to(uint32_t offset, uint32_t alignment) {
        return (offset + alignment - 1) & ~(alignment - 1);
    }
};

// Variant type (tagged union)
class VariantType : public ComponentType {
public:
    struct Case {
        std::string name;
        std::unique_ptr<ComponentType> payload_type;  // nullptr for unit variants
        
        Case(std::string n, std::unique_ptr<ComponentType> t)
            : name(std::move(n)), payload_type(std::move(t)) {}
    };

private:
    std::vector<Case> cases_;
    mutable std::optional<uint32_t> cached_size_;

public:
    explicit VariantType(std::vector<Case> cases) : cases_(std::move(cases)) {}
    
    TypeKind kind() const override { return TypeKind::Variant; }
    
    const std::vector<Case>& cases() const { return cases_; }
    
    uint32_t size() const override {
        if (!cached_size_) {
            // Size = discriminant + largest payload + padding
            uint32_t discriminant_size = 4;  // u32 discriminant
            uint32_t max_payload_size = 0;
            uint32_t max_payload_alignment = 1;
            
            for (const auto& case_ : cases_) {
                if (case_.payload_type) {
                    max_payload_size = std::max(max_payload_size, case_.payload_type->size());
                    max_payload_alignment = std::max(max_payload_alignment, case_.payload_type->alignment());
                }
            }
            
            // Align payload to its requirements
            uint32_t payload_offset = align_to(discriminant_size, max_payload_alignment);
            cached_size_ = payload_offset + max_payload_size;
        }
        return *cached_size_;
    }
    
    uint32_t alignment() const override {
        uint32_t max_alignment = 4;  // For discriminant
        for (const auto& case_ : cases_) {
            if (case_.payload_type) {
                max_alignment = std::max(max_alignment, case_.payload_type->alignment());
            }
        }
        return max_alignment;
    }
    
    // Variant subtyping: subtype must have subset of cases
    bool is_subtype_of(const ComponentType& other) const override {
        if (other.kind() != TypeKind::Variant) {
            return false;
        }
        
        const auto& other_variant = static_cast<const VariantType&>(other);
        
        // Build map of supertype cases
        std::unordered_map<std::string, const ComponentType*> super_cases;
        for (const auto& case_ : other_variant.cases()) {
            super_cases[case_.name] = case_.payload_type.get();
        }
        
        // Check that all subtype cases exist in supertype
        for (const auto& case_ : cases_) {
            auto it = super_cases.find(case_.name);
            if (it == super_cases.end()) {
                return false;  // Case doesn't exist in supertype
            }
            
            // Check payload compatibility
            if (case_.payload_type && it->second) {
                if (!case_.payload_type->is_subtype_of(*it->second)) {
                    return false;
                }
            } else if (case_.payload_type || it->second) {
                return false;  // One is unit, other is not
            }
        }
        
        return true;
    }

private:
    static uint32_t align_to(uint32_t offset, uint32_t alignment) {
        return (offset + alignment - 1) & ~(alignment - 1);
    }
};

// List type
class ListType : public ComponentType {
private:
    std::unique_ptr<ComponentType> element_type_;

public:
    explicit ListType(std::unique_ptr<ComponentType> element_type)
        : element_type_(std::move(element_type)) {}
    
    TypeKind kind() const override { return TypeKind::List; }
    
    const ComponentType& element_type() const { return *element_type_; }
    
    // Lists are represented as pointer + length
    uint32_t size() const override { return 8; }  // ptr(4) + len(4)
    uint32_t alignment() const override { return 4; }
    
    bool is_subtype_of(const ComponentType& other) const override {
        if (other.kind() != TypeKind::List) {
            return false;
        }
        
        const auto& other_list = static_cast<const ListType&>(other);
        return element_type_->is_subtype_of(other_list.element_type());
    }
};
```

## Component Instantiation Pipeline

This is the process of loading and starting a Component Model component.

### Component Definition Parser
```cpp
// uwrc-component-model/include/uwrc/component_model/parser.h

class ComponentParser {
public:
    static Result<ComponentDefinition> parse(std::span<const uint8_t> component_bytes);

private:
    // Parse WebAssembly component binary format
    static Result<ComponentDefinition> parse_component_binary(BinaryReader& reader);
    
    // Parse component sections
    static Result<void> parse_type_section(BinaryReader& reader, ComponentDefinition& def);
    static Result<void> parse_import_section(BinaryReader& reader, ComponentDefinition& def);
    static Result<void> parse_export_section(BinaryReader& reader, ComponentDefinition& def);
    static Result<void> parse_component_section(BinaryReader& reader, ComponentDefinition& def);
    static Result<void> parse_instance_section(BinaryReader& reader, ComponentDefinition& def);
    static Result<void> parse_alias_section(BinaryReader& reader, ComponentDefinition& def);
    static Result<void> parse_start_section(BinaryReader& reader, ComponentDefinition& def);
    
    // Type parsing helpers
    static Result<std::unique_ptr<ComponentType>> parse_component_type(BinaryReader& reader);
    static Result<RecordType::Field> parse_record_field(BinaryReader& reader);
    static Result<VariantType::Case> parse_variant_case(BinaryReader& reader);
};

struct ComponentDefinition {
    // Component metadata
    std::string name;
    std::string version;
    std::vector<std::string> authors;
    
    // Type definitions
    std::unordered_map<TypeId, std::unique_ptr<ComponentType>> types;
    
    // Interface definitions
    std::unordered_map<std::string, InterfaceDefinition> imports;
    std::unordered_map<std::string, InterfaceDefinition> exports;
    
    // Core modules within this component
    std::vector<CoreModuleDefinition> core_modules;
    
    // Component instances
    std::vector<ComponentInstanceDefinition> component_instances;
    
    // Start function (optional)
    std::optional<FunctionRef> start_function;
    
    // Memory requirements
    MemoryRequirements memory_requirements;
};

struct InterfaceDefinition {
    std::string name;
    std::unordered_map<std::string, FunctionSignature> functions;
    std::unordered_map<std::string, std::unique_ptr<ComponentType>> types;
    std::unordered_map<std::string, ResourceTypeDefinition> resources;
};
```

### Component Instantiation Process
```cpp
// The complete component instantiation pipeline
Result<std::unique_ptr<ComponentInstance>> ComponentModelRuntime::instantiate_component(
    const ComponentDefinition& definition,
    const InstantiationConfig& config) {
    
    // Phase 1: Validate component definition
    auto validation_result = type_system_->validate_component(definition);
    if (!validation_result.is_ok()) {
        return validation_result.error();
    }
    
    // Phase 2: Create component instance
    auto instance = std::make_unique<ComponentInstance>();
    instance->definition = &definition;
    instance->id = generate_component_id();
    
    // Phase 3: Set up component memory
    auto memory_setup = setup_component_memory(*instance, config);
    if (!memory_setup.is_ok()) {
        return memory_setup.error();
    }
    
    // Phase 4: Instantiate core modules
    for (const auto& core_module_def : definition.core_modules) {
        auto core_instance = instantiate_core_module(core_module_def, *instance);
        if (!core_instance.is_ok()) {
            return core_instance.error();
        }
        instance->core_instances[core_module_def.name] = std::move(core_instance.value());
    }
    
    // Phase 5: Set up Canonical ABI
    auto abi_setup = setup_canonical_abi(*instance);
    if (!abi_setup.is_ok()) {
        return abi_setup.error();
    }
    
    // Phase 6: Create resource table for this component
    instance->resource_table = std::make_unique<ComponentResourceTable>(instance->id);
    
    // Phase 7: Resolve imports
    auto import_resolution = resolve_imports(*instance);
    if (!import_resolution.is_ok()) {
        return import_resolution.error();
    }
    
    // Phase 8: Set up exports
    auto export_setup = setup_exports(*instance);
    if (!export_setup.is_ok()) {
        return export_setup.error();
    }
    
    // Phase 9: Call start function if present
    if (definition.start_function) {
        auto start_result = call_start_function(*instance, *definition.start_function);
        if (!start_result.is_ok()) {
            return start_result.error();
        }
    }
    
    // Phase 10: Register instance
    ComponentId id = instance->id;
    instances_[id] = std::move(instance);
    
    return instances_[id].get();
}

Result<void> ComponentModelRuntime::setup_canonical_abi(ComponentInstance& instance) {
    // Find the component's realloc function
    Function* realloc_func = nullptr;
    
    for (const auto& [name, core_instance] : instance.core_instances) {
        if (auto func = core_instance->find_function("cabi_realloc")) {
            realloc_func = func;
            break;
        }
    }
    
    if (!realloc_func) {
        return Error{ErrorCategory::Parse, "Component missing required cabi_realloc function"};
    }
    
    // Create Canonical ABI instance
    instance.canonical_abi = std::make_unique<CanonicalABI>(
        instance.memory.get(),
        realloc_func,
        StringEncoding::UTF8  // Default encoding
    );
    
    return {};
}
```

## Performance Optimizations

### Call Site Caching
```cpp
// Cache frequently called component functions to avoid lookup overhead
class CallSiteCache {
private:
    struct CallSite {
        ComponentId source_component;
        ComponentId target_component;
        std::string function_name;
        
        // Performance metrics
        std::atomic<uint64_t> call_count{0};
        std::atomic<uint64_t> total_time_ns{0};
        
        // Cached function pointer and type conversion
        Function* cached_function = nullptr;
        std::unique_ptr<TypeConverter> cached_converter;
        
        // JIT-compiled adapter (if available)
        std::unique_ptr<CompiledAdapter> jit_adapter;
    };
    
    std::unordered_map<CallSiteKey, std::unique_ptr<CallSite>> call_sites_;
    mutable std::shared_mutex cache_mutex_;

public:
    Result<CallSite*> get_or_create_call_site(
        ComponentId source,
        ComponentId target,
        const std::string& function_name
    );
    
    // Optimize hot call sites with JIT compilation
    void optimize_hot_call_sites();
    
    // Statistics
    std::vector<CallSiteStats> get_performance_stats() const;
};
```

### Type Conversion Caching
```cpp
// Cache expensive type conversions
class TypeConversionCache {
private:
    // Cache key for type conversions
    struct ConversionKey {
        TypeId source_type;
        TypeId target_type;
        bool operator==(const ConversionKey& other) const {
            return source_type == other.source_type && target_type == other.target_type;
        }
    };
    
    struct ConversionEntry {
        std::function<Result<Value>(const Value&)> converter;
        std::chrono::steady_clock::time_point last_used;
        uint64_t use_count = 0;
    };
    
    std::unordered_map<ConversionKey, ConversionEntry> cache_;
    mutable std::shared_mutex cache_mutex_;

public:
    // Get cached converter or create new one
    std::function<Result<Value>(const Value&)> get_converter(
        const ComponentType& source_type,
        const ComponentType& target_type
    );
    
    // Periodic cleanup of unused converters
    void cleanup_unused_converters(std::chrono::seconds max_age);
};
```

## Testing Strategy for Component Model

### Unit Tests for Core Functionality
```cpp
// Test canonical ABI operations
TEST(CanonicalABI, StringRoundTrip) {
    auto memory = create_test_memory();
    auto abi = create_test_abi(memory.get());
    
    std::string original = "Hello, Component Model! 🎮";
    
    // Lower string to memory
    auto lowered = abi.lower_string(original);
    ASSERT_TRUE(lowered.is_ok());
    
    auto [ptr, len] = lowered.value();
    
    // Lift string back from memory
    auto lifted = abi.lift_string(ptr, len);
    ASSERT_TRUE(lifted.is_ok());
    EXPECT_EQ(lifted.value(), original);
}

TEST(ResourceTable, OwnershipSemantics) {
    ResourceTable table;
    ComponentId owner = 1;
    
    // Create owned resource
    auto resource = std::make_unique<TestResource>(42);
    auto handle = table.create_own(owner, std::move(resource));
    
    // Borrow resource
    {
        auto borrowed = table.borrow<TestResource>(handle);
        ASSERT_TRUE(borrowed.is_ok());
        EXPECT_EQ(borrowed.value()->value, 42);
        
        // Should not be able to drop while borrowed
        auto drop_result = table.drop_own(handle, owner);
        EXPECT_FALSE(drop_result.is_ok());
    } // Borrow goes out of scope
    
    // Now drop should succeed
    auto drop_result = table.drop_own(handle, owner);
    EXPECT_TRUE(drop_result.is_ok());
}
```

### Integration Tests
```cpp
TEST(ComponentIntegration, PhysicsToRendererPipeline) {
    auto runtime = create_test_runtime();
    
    // Load physics component
    auto physics_def = load_test_component("test_physics.wasm");
    auto physics = runtime.instantiate_component(physics_def);
    ASSERT_TRUE(physics.is_ok());
    
    // Load renderer component  
    auto renderer_def = load_test_component("test_renderer.wasm");
    auto renderer = runtime.instantiate_component(renderer_def);
    ASSERT_TRUE(renderer.is_ok());
    
    // Link physics output to renderer input
    auto link_result = runtime.link_components(
        renderer.value()->id, "render_data",
        physics.value()->id, "get_render_objects"
    );
    ASSERT_TRUE(link_result.is_ok());
    
    // Simulate physics step
    auto sim_result = physics.value()->call("simulate_step", 0.016f);
    ASSERT_TRUE(sim_result.is_ok());
    
    // Render frame (should automatically get physics data)
    auto render_result = renderer.value()->call("render_frame");
    ASSERT_TRUE(render_result.is_ok());
}
```

This Component Model implementation provides the foundation for true polyglot game development on retro hardware. Components written in different languages can seamlessly interact with full type safety and proper resource management.

The next step would be implementing the memory management system that supports this Component Model architecture efficiently on resource-constrained platforms.

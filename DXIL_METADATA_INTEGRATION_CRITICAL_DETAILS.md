# DXIL Metadata Integration - Critical Implementation Details

## Overview
This document covers the 7 critical details that must be handled for safe and correct DXIL metadata integration into CFGStructurizer.

---

## 1. Block Index Mapping - Critical Issue

### The Problem
DXIL metadata contains block IDs (uint32_t), but CFGStructurizer works with CFGNode* pointers. We need bidirectional mapping.

### Solution Architecture

**Data Structure in CFGStructurizer**:
```cpp
class CFGStructurizer
{
private:
    // NEW: Metadata integration members
    const LLVMBC::Module* llvm_module;
    UnorderedMap<uint32_t, CFGNode*> metadata_id_to_node;
    UnorderedMap<CFGNode*, uint32_t> node_to_metadata_id;
    
    // Build the mapping during initialization
    void build_metadata_mapping();
};
```

**CFGNode Extension**:
```cpp
struct CFGNode
{
    // ...existing fields...
    
    // NEW: Metadata tracking
    uint32_t metadata_block_id = UINT32_MAX;  // ID from DXIL
    bool has_metadata = false;
};
```

### Implementation

**In CFGStructurizer Constructor**:
```cpp
CFGStructurizer::CFGStructurizer(
    CFGNode *entry,
    CFGNodePool &pool_,
    SPIRVModule &module_,
    const LLVMBC::Module* llvm_module_)  // NEW parameter
    : entry_block(entry)
    , pool(pool_)
    , module(module_)
    , llvm_module(llvm_module_)
{
    if (llvm_module && llvm_module->has_cfg_data())
    {
        build_metadata_mapping();
    }
}
```

**Build Mapping Function**:
```cpp
void CFGStructurizer::build_metadata_mapping()
{
    if (!llvm_module)
        return;
    
    const auto& headers = llvm_module->get_cfg_headers();
    const auto& merges = llvm_module->get_cfg_merges();
    const auto& continues = llvm_module->get_cfg_continues();
    const auto& hints = llvm_module->get_cfg_hints();
    
    // Collect all metadata block IDs
    UnorderedSet<uint32_t> metadata_ids;
    for (uint32_t id : headers)
        metadata_ids.insert(id);
    for (uint32_t id : merges)
        metadata_ids.insert(id);
    for (uint32_t id : continues)
        metadata_ids.insert(id);
    
    // Map each ID to corresponding CFGNode
    for (auto *node : all_nodes)
    {
        // Match node to metadata ID through:
        // 1. Node's original block index
        // 2. Post-order number
        // 3. Position in CFG traversal
        
        for (uint32_t id : metadata_ids)
        {
            if (matches_metadata_id(node, id))
            {
                node->metadata_block_id = id;
                node->has_metadata = true;
                metadata_id_to_node[id] = node;
                node_to_metadata_id[node] = id;
                LOGI("Mapped metadata ID %u to CFGNode %p\n", id, node);
                break;
            }
        }
    }
}

bool CFGStructurizer::matches_metadata_id(CFGNode *node, uint32_t id)
{
    // Strategy 1: Direct block number match
    if (node->block_number == id)
        return true;
    
    // Strategy 2: Post-order index match
    if (find_postorder_index(node) == id)
        return true;
    
    // Strategy 3: Semantic matching (fallback)
    // - Check if node has same successors
    // - Check if node has same predecessors
    // - Check block type consistency
    
    return semantic_match_to_metadata(node, id);
}
```

### Critical Validation

```cpp
bool CFGStructurizer::validate_metadata_mapping()
{
    if (!llvm_module || !llvm_module->has_cfg_data())
        return true;  // No metadata, nothing to validate
    
    const auto& headers = llvm_module->get_cfg_headers();
    const auto& merges = llvm_module->get_cfg_merges();
    
    // Validation 1: All metadata IDs map to CFGNodes
    for (uint32_t header_id : headers)
    {
        if (metadata_id_to_node.find(header_id) == metadata_id_to_node.end())
        {
            LOGE("Metadata header ID %u has no CFGNode mapping!\n", header_id);
            return false;
        }
    }
    
    // Validation 2: No duplicate mappings
    UnorderedSet<CFGNode*> mapped_nodes;
    for (auto& [id, node] : metadata_id_to_node)
    {
        if (mapped_nodes.count(node))
        {
            LOGE("CFGNode %p mapped to multiple metadata IDs!\n", node);
            return false;
        }
        mapped_nodes.insert(node);
    }
    
    // Validation 3: Mapped nodes are reachable
    for (auto& [id, node] : metadata_id_to_node)
    {
        if (!is_reachable_from_entry(node))
        {
            LOGE("Mapped node %p (ID %u) not reachable from entry!\n", node, id);
            return false;
        }
    }
    
    return true;
}
```

---

## 2. When to Call has_cfg_data() - Decision Tree

### The Problem
Metadata may not exist. We need to know when it's safe to use.

### Decision Tree

```cpp
bool CFGStructurizer::should_use_metadata()
{
    // Check 1: Is module available?
    if (!llvm_module)
    {
        LOGI("No LLVM module available, using fallback\n");
        return false;
    }
    
    // Check 2: Does module have CFG data?
    if (!llvm_module->has_cfg_data())
    {
        LOGI("Module has no CFG data, using fallback\n");
        return false;
    }
    
    // Check 3: Did mapping succeed?
    if (!mapping_is_valid)
    {
        LOGI("Metadata mapping validation failed, using fallback\n");
        return false;
    }
    
    // Check 4: Is metadata reasonably complete?
    // (At least headers or merges should be present)
    if (llvm_module->get_cfg_headers().empty() &&
        llvm_module->get_cfg_merges().empty())
    {
        LOGI("Metadata too sparse, using fallback\n");
        return false;
    }
    
    return true;  // Safe to use metadata!
}
```

### When to Check

**On Every Major Operation**:
```cpp
bool CFGStructurizer::find_loops(unsigned pass)
{
    bool iteration_successful = false;
    
    // ALWAYS check before using metadata
    if (should_use_metadata())
    {
        LOGI("Using metadata for loop detection (pass %u)\n", pass);
        iteration_successful = find_loops_with_metadata(pass);
    }
    
    // Fallback if metadata not available or failed
    if (!iteration_successful && llvm_module == nullptr)
    {
        iteration_successful = find_loops_original(pass);
    }
    
    return iteration_successful;
}
```

### Caching the Decision

```cpp
class CFGStructurizer
{
private:
    bool use_metadata = false;
    bool use_metadata_checked = false;
    
public:
    bool should_use_metadata_cached()
    {
        if (!use_metadata_checked)
        {
            use_metadata = should_use_metadata();
            use_metadata_checked = true;
            LOGI("CFG metadata decision cached: %s\n",
                 use_metadata ? "use" : "skip");
        }
        return use_metadata;
    }
};
```

---

## 3. Validation of Metadata - Safety Checks

### The Problem
Metadata could be malformed, mismatched, or from a different shader. Must validate before use.

### Comprehensive Validation Suite

```cpp
struct MetadataValidation
{
    bool valid = false;
    String error_message;
    int error_code = 0;
    
    enum ErrorCode
    {
        NO_ERROR = 0,
        INVALID_BLOCK_ID = 1,
        UNMAPPED_ID = 2,
        CIRCULAR_DEPENDENCY = 3,
        ORPHANED_BLOCK = 4,
        INCONSISTENT_MERGE = 5,
        INCONSISTENT_CONTINUE = 6,
        MISSING_HEADERS = 7,
        CORRUPTED_DATA = 8,
    };
};

MetadataValidation CFGStructurizer::validate_metadata_complete()
{
    MetadataValidation result;
    
    if (!llvm_module || !llvm_module->has_cfg_data())
    {
        result.valid = true;  // No metadata = valid (no-op)
        return result;
    }
    
    const auto& headers = llvm_module->get_cfg_headers();
    const auto& merges = llvm_module->get_cfg_merges();
    const auto& continues = llvm_module->get_cfg_continues();
    
    // Validation 1: Non-empty headers
    if (headers.empty())
    {
        result.error_code = MetadataValidation::MISSING_HEADERS;
        result.error_message = "CFG headers list is empty";
        return result;
    }
    
    // Validation 2: All IDs are valid block references
    for (uint32_t header_id : headers)
    {
        if (!is_valid_block_id(header_id))
        {
            result.error_code = MetadataValidation::INVALID_BLOCK_ID;
            result.error_message = String::Format(
                "Invalid header block ID: %u", header_id);
            return result;
        }
    }
    
    // Validation 3: All IDs map to CFGNodes
    for (uint32_t id : headers)
    {
        if (metadata_id_to_node.find(id) == metadata_id_to_node.end())
        {
            result.error_code = MetadataValidation::UNMAPPED_ID;
            result.error_message = String::Format(
                "Header ID %u doesn't map to CFGNode", id);
            return result;
        }
    }
    
    // Validation 4: Merge IDs are valid
    for (uint32_t merge_id : merges)
    {
        if (!is_valid_block_id(merge_id))
        {
            result.error_code = MetadataValidation::INVALID_BLOCK_ID;
            result.error_message = String::Format(
                "Invalid merge block ID: %u", merge_id);
            return result;
        }
    }
    
    // Validation 5: Continue IDs are valid
    for (uint32_t continue_id : continues)
    {
        if (!is_valid_block_id(continue_id))
        {
            result.error_code = MetadataValidation::INVALID_BLOCK_ID;
            result.error_message = String::Format(
                "Invalid continue block ID: %u", continue_id);
            return result;
        }
    }
    
    // Validation 6: No circular dependencies
    if (has_circular_merge_dependency())
    {
        result.error_code = MetadataValidation::CIRCULAR_DEPENDENCY;
        result.error_message = "Circular merge dependency detected";
        return result;
    }
    
    // Validation 7: Merge targets are reachable
    for (uint32_t header_id : headers)
    {
        CFGNode* header = metadata_id_to_node[header_id];
        CFGNode* merge = find_merge_for_header(header);
        
        if (merge && !can_reach(header, merge))
        {
            result.error_code = MetadataValidation::INCONSISTENT_MERGE;
            result.error_message = String::Format(
                "Header %u cannot reach its merge", header_id);
            return result;
        }
    }
    
    // All checks passed
    result.valid = true;
    return result;
}

bool CFGStructurizer::is_valid_block_id(uint32_t id) const
{
    // Block ID should be in reasonable range
    return id < all_nodes.size();
}

bool CFGStructurizer::has_circular_merge_dependency()
{
    const auto& merges = llvm_module->get_cfg_merges();
    
    for (uint32_t merge_id : merges)
    {
        CFGNode* merge = metadata_id_to_node[merge_id];
        
        // Check if merge can reach itself through CFG
        if (can_reach_without_exit(merge, merge))
        {
            return true;  // Circular!
        }
    }
    
    return false;
}
```

### Integration Point

```cpp
bool CFGStructurizer::run()
{
    // Validate metadata early
    MetadataValidation validation = validate_metadata_complete();
    if (!validation.valid)
    {
        LOGW("Metadata validation failed: %s (code %d)\n",
             validation.error_message.c_str(),
             validation.error_code);
        use_metadata = false;  // Disable metadata use
    }
    
    // Continue with algorithm...
}
```

---

## 4. Multiple Functions - Per-Function Metadata

### The Problem
A module may contain multiple functions. Metadata is per-function, not per-module.

### Solution

**Function-Level Metadata Storage**:
```cpp
// In LLVMBC::Module
struct FunctionMetadata
{
    String function_name;
    std::vector<uint32_t> cfg_headers;
    std::vector<uint32_t> cfg_merges;
    std::vector<uint32_t> cfg_continues;
    std::vector<uint32_t> cfg_hints;
};

class Module
{
private:
    // NEW: Per-function metadata
    UnorderedMap<String, FunctionMetadata> function_metadata_map;
    
public:
    void set_function_cfg_data(
        const String& function_name,
        std::vector<uint32_t>&& h,
        std::vector<uint32_t>&& m,
        std::vector<uint32_t>&& c,
        std::vector<uint32_t>&& hi)
    {
        FunctionMetadata& meta = function_metadata_map[function_name];
        meta.function_name = function_name;
        meta.cfg_headers = std::move(h);
        meta.cfg_merges = std::move(m);
        meta.cfg_continues = std::move(c);
        meta.cfg_hints = std::move(hi);
    }
    
    const FunctionMetadata* get_function_cfg_data(
        const String& function_name) const
    {
        auto it = function_metadata_map.find(function_name);
        if (it != function_metadata_map.end())
            return &it->second;
        return nullptr;
    }
};
```

**CFGStructurizer Adjustment**:
```cpp
class CFGStructurizer
{
private:
    const LLVMBC::Module* llvm_module;
    String current_function_name;
    const FunctionMetadata* function_metadata = nullptr;
    
public:
    CFGStructurizer(
        CFGNode *entry,
        CFGNodePool &pool_,
        SPIRVModule &module_,
        const LLVMBC::Module* llvm_module_,
        const String& function_name_)  // NEW
        : entry_block(entry)
        , pool(pool_)
        , module(module_)
        , llvm_module(llvm_module_)
        , current_function_name(function_name_)
    {
        if (llvm_module && llvm_module->has_cfg_data())
        {
            function_metadata = llvm_module->get_function_cfg_data(
                function_name);
            
            if (function_metadata)
            {
                LOGI("Found metadata for function: %s\n",
                     function_name.c_str());
                build_metadata_mapping();
            }
        }
    }
    
    bool has_function_metadata() const
    {
        return function_metadata != nullptr;
    }
    
    const std::vector<uint32_t>& get_cfg_headers() const
    {
        if (!has_function_metadata())
            return empty_vector;  // Return empty if no metadata
        return function_metadata->cfg_headers;
    }
};
```

**Usage Example**:
```cpp
// When creating CFGStructurizer for a specific function
CFGStructurizer* structurizer = new CFGStructurizer(
    entry_block,
    node_pool,
    spv_module,
    llvm_module,
    "my_shader_main"  // Function name
);

if (structurizer->has_function_metadata())
{
    LOGI("Using metadata for shader_main\n");
}
```

---

## 5. CFGNode Metadata Integration - Struct Extension

### The Problem
CFGNode needs to know its metadata ID for efficient lookups.

### CFGNode Extension

**Header File** (`node.hpp`):
```cpp
struct CFGNode
{
    // ...existing fields...
    
    // Dominance and control flow
    Vector<CFGNode *> predecessors;
    Vector<CFGNode *> successors;
    CFGNode *immediate_dominator = nullptr;
    CFGNode *immediate_post_dominator = nullptr;
    
    // NEW: Metadata fields
    struct MetadataInfo
    {
        uint32_t block_id = UINT32_MAX;
        bool is_loop_header = false;
        bool is_merge_block = false;
        bool is_continue_block = false;
        bool is_exit_block = false;
        
        bool has_metadata() const
        {
            return block_id != UINT32_MAX;
        }
    };
    
    MetadataInfo metadata;
    
    // Helpers
    bool is_metadata_loop_header() const
    {
        return metadata.is_loop_header && metadata.has_metadata();
    }
    
    bool is_metadata_merge() const
    {
        return metadata.is_merge_block && metadata.has_metadata();
    }
};
```

**Initialization**:
```cpp
void CFGStructurizer::annotate_nodes_with_metadata()
{
    if (!has_function_metadata())
        return;
    
    const auto& headers = function_metadata->cfg_headers;
    const auto& merges = function_metadata->cfg_merges;
    const auto& continues = function_metadata->cfg_continues;
    
    // Mark loop headers
    for (uint32_t header_id : headers)
    {
        CFGNode* node = metadata_id_to_node[header_id];
        if (node)
        {
            node->metadata.block_id = header_id;
            node->metadata.is_loop_header = true;
        }
    }
    
    // Mark merge blocks
    for (uint32_t merge_id : merges)
    {
        CFGNode* node = metadata_id_to_node[merge_id];
        if (node)
        {
            node->metadata.block_id = merge_id;
            node->metadata.is_merge_block = true;
        }
    }
    
    // Mark continue blocks
    for (uint32_t continue_id : continues)
    {
        CFGNode* node = metadata_id_to_node[continue_id];
        if (node)
        {
            node->metadata.block_id = continue_id;
            node->metadata.is_continue_block = true;
        }
    }
}
```

---

## 6. Order of Operations - Fallback Strategy

### The Problem
We need clear order: check metadata first, fall back gracefully if it fails.

### Decision Flow Diagram

```
┌─────────────────────────────────┐
│ CFGStructurizer::run()          │
└────────────┬────────────────────┘
             │
             ▼
┌─────────────────────────────────┐
│ Is llvm_module available?       │
└─┬─────────────────────────────┬─┘
  │ YES                       NO│
  ▼                            ▼
┌──────────────────┐   ┌──────────────────┐
│ has_cfg_data()   │   │ Use original     │
└─┬────────────┬───┘   │ algorithm only   │
  │ YES     NO│        └──────────────────┘
  ▼           ▼
┌──┐   ┌──────────────────┐
│  │   │ Use original     │
│  │   │ algorithm only   │
│  │   └──────────────────┘
│  │
│  └─→ Validate mapping
      ▼
     ┌──────────────────┐
     │ Valid?           │
     └─┬────────────┬───┘
       │ YES     NO│
       ▼           ▼
    USE META   FALLBACK
```

### Implementation

```cpp
class CFGStructurizer
{
private:
    enum MetadataState
    {
        UNKNOWN,        // Not yet checked
        AVAILABLE,      // Available and validated
        UNAVAILABLE,    // Not available
        INVALID,        // Available but invalid
        FAILED,         // Was available but use failed
    };
    
    MetadataState metadata_state = UNKNOWN;
    
    void check_metadata_availability()
    {
        if (metadata_state != UNKNOWN)
            return;  // Already checked
        
        // Step 1: Module check
        if (!llvm_module)
        {
            metadata_state = UNAVAILABLE;
            LOGI("No LLVM module\n");
            return;
        }
        
        // Step 2: CFG data check
        if (!llvm_module->has_cfg_data())
        {
            metadata_state = UNAVAILABLE;
            LOGI("No CFG data in module\n");
            return;
        }
        
        // Step 3: Function metadata check
        if (!has_function_metadata())
        {
            metadata_state = UNAVAILABLE;
            LOGI("No metadata for function: %s\n",
                 current_function_name.c_str());
            return;
        }
        
        // Step 4: Build mapping
        if (!build_metadata_mapping())
        {
            metadata_state = FAILED;
            LOGW("Failed to build metadata mapping\n");
            return;
        }
        
        // Step 5: Validate
        MetadataValidation validation = validate_metadata_complete();
        if (!validation.valid)
        {
            metadata_state = INVALID;
            LOGW("Metadata validation failed: %s\n",
                 validation.error_message.c_str());
            return;
        }
        
        // All checks passed
        metadata_state = AVAILABLE;
        LOGI("Metadata available and validated\n");
    }

public:
    bool can_use_metadata()
    {
        check_metadata_availability();
        return metadata_state == AVAILABLE;
    }
    
    void mark_metadata_failed()
    {
        metadata_state = FAILED;
    }
};
```

### Usage Pattern

```cpp
bool CFGStructurizer::find_loops(unsigned pass)
{
    // Always try metadata first if available
    if (can_use_metadata())
    {
        LOGI("Attempting to use metadata for loop detection\n");
        
        try
        {
            if (find_loops_with_metadata(pass))
                return true;
        }
        catch (const std::exception& e)
        {
            LOGW("Metadata-based loop detection failed: %s\n", e.what());
            mark_metadata_failed();
        }
    }
    
    // Fallback to original algorithm
    LOGI("Using fallback algorithm for loop detection\n");
    return find_loops_original(pass);
}
```

---

## 7. Testing with Real DXIL - Validation Strategy

### The Problem
No guarantee metadata format matches assumptions. Must test with real DXIL.

### Real DXIL Test Suite

**Test File Structure**:
```cpp
// test_dxil_metadata_integration.cpp

#include "gtest/gtest.h"
#include "cfg_structurizer.hpp"
#include "bc/module.hpp"

class DIXLMetadataIntegrationTest : public ::testing::Test
{
protected:
    LLVMBC::Module* load_test_dxil(const char* filename);
    void validate_metadata_format(const LLVMBC::Module* module);
};
```

**Test 1: Metadata Format**:
```cpp
TEST_F(DIXLMetadataIntegrationTest, MetadataFormatValidation)
{
    // Real DXIL shader
    auto* module = load_test_dxil("simple_loop.hlsl");
    ASSERT_NE(nullptr, module);
    
    EXPECT_TRUE(module->has_cfg_data());
    
    const auto& headers = module->get_cfg_headers();
    const auto& merges = module->get_cfg_merges();
    
    // Validate format
    EXPECT_FALSE(headers.empty()) << "Should have loop headers";
    EXPECT_FALSE(merges.empty()) << "Should have merge blocks";
    
    // Validate IDs are reasonable
    for (uint32_t id : headers)
        EXPECT_LT(id, 1000) << "Block ID too large";
    
    for (uint32_t id : merges)
        EXPECT_LT(id, 1000) << "Block ID too large";
}
```

**Test 2: Mapping**:
```cpp
TEST_F(DIXLMetadataIntegrationTest, BlockIDMapping)
{
    auto* module = load_test_dxil("nested_loops.hlsl");
    
    CFGStructurizer structurizer(entry, pool, spv_module, module, "main");
    
    EXPECT_TRUE(structurizer.validate_metadata_mapping());
    
    // All header IDs should map to nodes
    const auto& headers = module->get_cfg_headers();
    for (uint32_t header_id : headers)
    {
        auto* node = structurizer.get_node_for_metadata_id(header_id);
        EXPECT_NE(nullptr, node) << "Header ID " << header_id
                                 << " should map to CFGNode";
    }
}
```

**Test 3: Fallback Safety**:
```cpp
TEST_F(DIXLMetadataIntegrationTest, FallbackSafety)
{
    // Test 1: Corrupted metadata
    auto* module = load_test_dxil("simple_loop.hlsl");
    corrupt_metadata(module);  // Make it invalid
    
    CFGStructurizer structurizer(entry, pool, spv_module, module, "main");
    
    // Should detect corruption and fall back
    EXPECT_FALSE(structurizer.can_use_metadata());
    
    // Should still work with fallback
    bool result = structurizer.run();
    EXPECT_TRUE(result) << "Should fall back gracefully";
}
```

**Test 4: Performance**:
```cpp
TEST_F(DIXLMetadataIntegrationTest, PerformanceComparison)
{
    auto* module = load_test_dxil("complex_shader.hlsl");
    
    // With metadata
    auto start_with = clock();
    CFGStructurizer fast(entry, pool, spv_module, module, "main");
    fast.run();
    auto time_with = clock() - start_with;
    
    // Without metadata (nullptr)
    auto start_without = clock();
    CFGStructurizer slow(entry, pool, spv_module, nullptr, "main");
    slow.run();
    auto time_without = clock() - start_without;
    
    // Should be faster
    EXPECT_LT(time_with, time_without)
        << "Metadata optimization should be faster";
    
    float speedup = (float)time_without / time_with;
    EXPECT_GT(speedup, 1.5f) << "Expected at least 1.5x speedup";
    
    LOGI("Performance: %.2fx speedup (with: %ld, without: %ld)\n",
         speedup, time_with, time_without);
}
```

**Test 5: Real Shader Examples**:
```cpp
TEST_F(DIXLMetadataIntegrationTest, RealShaderExamples)
{
    struct ShaderTest
    {
        const char* filename;
        bool has_loops;
        bool has_selections;
        int expected_headers;
    };
    
    ShaderTest tests[] =
    {
        {"simple_loop.hlsl", true, false, 1},
        {"nested_loops.hlsl", true, false, 2},
        {"if_else.hlsl", false, true, 0},
        {"complex.hlsl", true, true, 3},
    };
    
    for (const auto& test : tests)
    {
        SCOPED_TRACE(test.filename);
        
        auto* module = load_test_dxil(test.filename);
        ASSERT_NE(nullptr, module);
        
        if (test.has_loops)
        {
            EXPECT_FALSE(module->get_cfg_headers().empty())
                << test.filename << " should have loop headers";
        }
        
        EXPECT_EQ(test.expected_headers,
                  module->get_cfg_headers().size())
            << test.filename << " header count mismatch";
    }
}
```

### Real DXIL Metadata Examples

**Example 1: Simple Loop**:
```
DXIL: !dx.controlFlowAnnotations
  Headers: [0]
  Merges: [2]
  Continues: []
  
CFG:
  Block 0 (header): if (cond) goto 1 else goto 2
  Block 1 (body):   ... goto 0 (back edge)
  Block 2 (merge):  ... return
```

**Example 2: Nested Loops**:
```
DXIL: !dx.controlFlowAnnotations
  Headers: [0, 1]
  Merges: [2, 3]
  Continues: [1]
  
CFG:
  Block 0 (outer header): if (...) goto 1 else goto 3
  Block 1 (inner header): if (...) goto 2 else goto 0 (back to outer)
  Block 2 (inner merge):  goto 0
  Block 3 (outer merge):  return
```

### Test Execution

```bash
# Build tests
cmake --build . --config Debug --target dxil_metadata_tests

# Run tests
./bin/dxil_metadata_tests --gtest_filter="DIXLMetadataIntegrationTest.*"

# With verbose output
./bin/dxil_metadata_tests \
  --gtest_filter="DIXLMetadataIntegrationTest.*" \
  -v
```

---

## Complete Integration Checklist

- [ ] **Block Mapping**
  - [ ] CFGNode extended with metadata_block_id
  - [ ] Bidirectional mapping (ID ↔ Node*) implemented
  - [ ] Mapping validation in place
  - [ ] Mapping tested with real shaders

- [ ] **Metadata Availability**
  - [ ] has_cfg_data() called appropriately
  - [ ] Function-level metadata retrieval
  - [ ] Metadata state machine implemented
  - [ ] Fallback paths verified

- [ ] **Validation**
  - [ ] Metadata format validation
  - [ ] Circular dependency check
  - [ ] Block ID validity check
  - [ ] Reachability verification

- [ ] **Multi-Function Support**
  - [ ] Per-function metadata storage
  - [ ] Function name passed to CFGStructurizer
  - [ ] Metadata retrieval by function
  - [ ] Multiple functions tested

- [ ] **CFGNode Integration**
  - [ ] MetadataInfo struct added
  - [ ] Annotation function implemented
  - [ ] Helper methods added
  - [ ] Type safety maintained

- [ ] **Order of Operations**
  - [ ] Metadata state enum defined
  - [ ] Check sequence implemented
  - [ ] Fallback logic verified
  - [ ] Error handling tested

- [ ] **Real DXIL Testing**
  - [ ] Test suite written
  - [ ] Simple loops tested
  - [ ] Nested loops tested
  - [ ] Edge cases covered
  - [ ] Performance measured

---

## Summary

These 7 critical details ensure:

✅ **Correctness**: Proper mapping and validation
✅ **Safety**: Graceful fallback mechanisms
✅ **Performance**: Real benefit from metadata
✅ **Robustness**: Handles all shader types
✅ **Maintainability**: Clear decision points
✅ **Testability**: Comprehensive validation

All must be implemented before using DXIL metadata in production.

# CFG Structurizer Deep Dive - Complete Analysis

## Overview

The **CFG Structurizer** is a sophisticated component in the dxil-spirv compiler that transforms an unstructured control flow graph (CFG) from DXIL bytecode into a structured form suitable for SPIR-V code generation. It's one of the most complex pieces of the compilation pipeline.

---

## File Structure

### Files
- **`cfg_structurizer.hpp`** - Class definition and interface
- **`cfg_structurizer.cpp`** - Implementation (9712 lines)

### Location
```
subprojects/dxil-spirv/cfg_structurizer.hpp
subprojects/dxil-spirv/cfg_structurizer.cpp
```

---

## Core Concepts

### What is CFG Structurization?

**Problem**: DXIL bytecode uses unstructured control flow (arbitrary jumps, multiple exits)

**Goal**: Convert to SPIR-V structured control flow (nested blocks with clear entry/exit points)

**Solution**: CFGStructurizer performs complex graph transformations to:
1. Identify loops and selection structures
2. Rewrite complex control flow patterns
3. Insert ladder blocks where needed
4. Handle break/continue semantics
5. Maintain SSA form throughout

---

## Class Architecture

### CFGStructurizer Class

```cpp
class CFGStructurizer
{
public:
    // Constructor
    CFGStructurizer(CFGNode *entry, CFGNodePool &pool, SPIRVModule &module);
    
    // Main operations
    bool run();                              // Full structurization
    bool run_trivial();                      // Simple cases only
    void traverse(BlockEmissionInterface &iface);  // Emit structured code
    CFGNode *get_entry_block() const;        // Get structured entry point
    
    // Specialized transformations
    bool rewrite_rov_lock_region();          // ROV (Rasterizer Ordered Views)
    void rewrite_auto_group_shared_barrier(); // Group shared memory barriers
    void flatten_subgroup_shuffles();         // Simplify subgroup ops
    void fixup_loop_header_undef_phis();      // Fix undefined PHI values in loops
    
private:
    // Core data
    CFGNode *entry_block;
    CFGNode *exit_block;
    CFGNodePool &pool;
    SPIRVModule &module;
    
    // Analysis data structures
    Vector<CFGNode *> forward_post_visit_order;   // For dominance
    Vector<CFGNode *> backward_post_visit_order;  // For post-dominance
    Vector<uint32_t> reachability_bitset;         // Reachability analysis
    unsigned reachability_stride = 0;
    UnorderedSet<const CFGNode *> reachable_nodes;
    UnorderedSet<const CFGNode *> structured_loop_merge_targets;
};
```

---

## Key Data Structures

### CFGNode (from node.hpp - not shown)

Each node represents a basic block with:
- **Instructions**: Vector of operations
- **Terminator**: Branch/switch/return instruction
- **Predecessors/Successors**: Edge connections
- **PHI nodes**: Merge point value selections
- **Dominance info**: Immediate dominator, post-dominator
- **Loop info**: Back edges, loop headers
- **Merge type**: None/Selection/Loop

### Analysis Results

```cpp
struct LoopAnalysis {
    Vector<CFGNode *> direct_exits;           // Direct exits from loop
    Vector<CFGNode *> inner_direct_exits;     // Exits inside inner loops
    Vector<CFGNode *> dominated_exit;         // Dominated exit edges
    Vector<CFGNode *> inner_dominated_exit;   // In nested loops
    Vector<CFGNode *> non_dominated_exit;     // Non-dominated edges
    Vector<CFGNode *> dominated_continue_exit; // Continue block edges
};

struct LoopMergeAnalysis {
    CFGNode *merge;                    // Target merge block
    CFGNode *weak_merge;              // Weakly dominated merge
    CFGNode *dominated_merge;         // Strongly dominated merge
    CFGNode *infinite_continue_ladder; // For infinite loops
};

enum class LoopExitType {
    Exit,                 // Normal loop exit
    Merge,               // Merge point
    Escape,              // Escape outer construct
    MergeToInfiniteLoop, // To infinite loop
    InnerLoopExit,       // Inner loop exit
    InnerLoopMerge,      // Inner loop merge
    InnerLoopFalsePositive
};
```

---

## Main Algorithm - `run()`

### Phase 1: Analysis

```
1. visit() - Forward depth-first traversal
   - Compute post-order traversal
   - Identify back edges
   
2. backwards_visit() - Reverse traversal
   - Compute reverse post-order
   
3. build_immediate_dominators() - Dominator tree
   - Immediate dominator for each node
   
4. build_immediate_post_dominators() - Post-dominator tree
   - Immediate post-dominator for each node
   
5. build_reachability() - Reachability analysis
   - Compute which nodes can reach which nodes
```

### Phase 2: Structurization (Iterative)

```
Loop until no changes:
    1. find_loops() - Identify loop structures
       - Find natural loop headers
       - Determine loop merges
       - Handle multiple back edges
       
    2. find_selection_merges() - Identify selection (if/else)
       - Find selection headers
       - Determine merge points
       
    3. structurize() - Apply transformations
       - Rewrite complex exits
       - Insert ladder blocks
       - Fix merge targets
       
    4. Repeat until all structures identified
```

### Phase 3: Post-Processing

```
1. split_merge_blocks() - Split complex merges
2. eliminate_degenerate_blocks() - Remove no-op blocks
3. cleanup_breaking_phi_constructs() - Clean PHI nodes
4. insert_phi() - Add PHI nodes at merge points
5. fixup_broken_value_dominance() - Fix SSA form
6. rewrite_invalid_loop_breaks() - Correct break edges
7. eliminate_degenerate_switch_merges() - Clean switch blocks
```

### Phase 4: Specialized Transforms

```
1. rewrite_transposed_loops() - Fix transposed loop patterns
2. serialize_interleaved_merge_scopes() - Serialize nested merges
3. serialize_interleaved_early_returns() - Handle early returns
4. propagate_branch_control_hints() - Add branch hints
5. remove_unused_ssa() - Clean up dead code
```

---

## Critical Methods

### 1. Loop Detection & Analysis

```cpp
LoopAnalysis analyze_loop(CFGNode *node) const
{
    // Returns:
    // - direct_exits: Edges leaving the loop
    // - dominated_exits: Post-dominated by loop
    // - continue_exits: To continue block
}

LoopMergeAnalysis analyze_loop_merge(CFGNode *node, 
                                     const LoopAnalysis &analysis)
{
    // Determines where loop merges to
    // Handles infinite loops specially
}
```

### 2. Dominance Analysis

```cpp
void build_immediate_dominators()
{
    // Computes idom[n] = immediate dominator of n
    // Used to determine control dependence
}

void build_immediate_post_dominators()
{
    // Computes idom_post[n] = immediate post-dominator
    // Used to find merge points
}
```

### 3. Reachability Analysis

```cpp
bool query_reachability(const CFGNode &from, const CFGNode &to) const
{
    // Can control flow reach 'to' from 'from'?
    // Uses bitset for efficiency
}

void build_reachability()
{
    // Builds bitset representation of reachability
    // Allows O(1) reachability queries
}
```

### 4. Ladder Block Creation

```cpp
CFGNode *create_ladder_block(CFGNode *header, CFGNode *node, const char *tag)
{
    // Creates intermediate block for complex control flow
    // Example: 
    //   Original: header → node1, node2, merge
    //   Modified: header → ladder → {node1, node2} → merge
}
```

### 5. PHI Insertion

```cpp
void insert_phi()
{
    // Inserts PHI nodes at merge points
    // Maintains SSA form
    // Handles multiple predecessors
}

void fixup_phi(PHINode &node)
{
    // Fixes incorrect PHI node placement
    // Ensures all reaching definitions are handled
}
```

---

## Special Transformations

### ROV (Rasterizer Ordered Views) Handling

```cpp
bool rewrite_rov_lock_region()
{
    // Detects ROV locking pattern:
    //   BeginInvariantRegion()
    //   ... code ...
    //   EndInvariantRegion()
    //
    // Transforms to ensure single entry/exit
    // Maintains lock semantics for correctness
}

bool find_single_entry_exit_lock_region(
    CFGNode *&idom, CFGNode *&pdom, 
    const Vector<CFGNode *> &rov_blocks)
{
    // Ensures ROV region has single entry/exit
    // Rejects loops unless trivially safe
}
```

### Group Shared Memory Barriers

```cpp
void rewrite_auto_group_shared_barrier()
{
    // Handles group.Barrier() operations
    // Ensures barriers appear at well-defined points
    // Prevents barrier misalignment in branching code
}
```

### Subgroup Operations

```cpp
void flatten_subgroup_shuffles()
{
    // Simplifies subgroup shuffle operations
    // Converts complex shuffle patterns to simpler forms
    // Improves compatibility with SPIR-V
}
```

### Loop Header PHI Fixup

```cpp
void fixup_loop_header_undef_phis()
{
    // Problem: Loop header PHI references undefined values
    // 
    // Solution: Insert initialization blocks
    // Example:
    //   // Before
    //   loop_header:
    //     %val = phi [%undef, %entry], [%new_val, %back]
    //   
    //   // After
    //   entry:
    //     %val = some_init_value
    //     br loop_header
    //   loop_header:
    //     %val = phi [%val, %entry], [%new_val, %back]
}
```

---

## Complex Control Flow Patterns

### 1. Transposed Loops

**What are they?**
Loops where the exit condition is checked at the end instead of beginning.

```
// Transposed loop (unstructured)
Entry → Body → Check → Exit

// Structured equivalent
Entry → Header(Check) → Body → Back → Header
```

**Solution**: `rewrite_transposed_loops()`

### 2. Interleaved Merges

**What are they?**
Multiple merge constructs that cross each other's boundaries.

```
If1 { ... If2 { ... } ... }  // Nested - OK
If1 { ... } If2 { ... If1_merge ... }  // Interleaved - BAD
```

**Solution**: `serialize_interleaved_merge_scopes()`

### 3. Early Returns

**What are they?**
Multiple return statements in different control flow paths.

```cpp
if (condition) {
    return value1;  // Early return
}
// ... more code ...
return value2;  // Main return
```

**Solution**: `serialize_interleaved_early_returns()`

### 4. Multiple Back Edges

**What are they?**
A single loop header with multiple back edges (multiple continue points).

```cpp
loop_header:
    if (cond1) { ... break; }
    if (cond2) { ... continue; }  // back edge 1
    if (cond3) { ... continue; }  // back edge 2
    break;  // back edge 3
```

**Solution**: `rewrite_multiple_back_edges()`

---

## Algorithm Complexity

### Time Complexity
- **Analysis Phase**: O(V + E) where V=nodes, E=edges
- **Dominance**: O((V + E) * α(V)) ≈ O(V + E)
- **Each Iteration**: O(V + E)
- **Total**: O(Iterations * (V + E))

**Iterations depend on**:
- Nesting depth of control structures
- Complexity of CFG
- Number of transformations needed

Typical: 3-10 iterations, but complex shaders can require 50+

### Space Complexity
- **Reachability bitset**: O(V²/64) bits ≈ O(V²)
- **Post-order arrays**: O(V)
- **Graph structures**: O(V + E)

Total: **O(V² + E)** where V² comes from reachability analysis

---

## Key Invariants

### 1. Structured Control Flow
After structurization:
- Every loop has clear header and merge
- Every selection has clear header and merge
- No arbitrary jumps remain
- All back edges identified

### 2. SSA Form
- Every value defined once
- PHI nodes at merge points
- Dominance-based correctness
- Value use dominated by definition

### 3. Edge Properties
- No crossing edges between merge scopes
- Back edges only to loop headers
- Forward edges respect dominance
- All edges have defined semantics

### 4. Block Properties
- Every block reachable from entry
- No dead code
- PHI nodes only at merge points
- Terminator at end of each block

---

## Debugging & Logging

### Logging Functions

```cpp
void log_cfg(const char *tag) const
{
    // Prints text representation of CFG
    // Shows structure and terminators
    // Useful for debugging transformations
}

void log_cfg_graphviz(const char *path) const
{
    // Outputs DOT format graph
    // Can visualize with graphviz
    // Shows all blocks and edges
}
```

### Debug Macros

```cpp
#ifdef PHI_DEBUG
static void validate_phi(const PHI &phi)
{
    // Validates PHI node correctness
    // Checks all predecessors covered
    // Verifies type consistency
}
#endif
```

---

## Real-World Example

### Input: Unstructured Loop with Early Exit

```cpp
// DXIL pseudocode
Entry:
    i = 0
    br Header

Header:
    if (i >= 10) br Exit
    if (i == 5) br Exit  // Early exit
    br Body

Body:
    i = i + 1
    if (i > 100) br Header  // Back edge 1
    br Header  // Back edge 2

Exit:
    return i
```

### Structurizer Output: Structured Loop

```
Entry:
    i = 0
    br LoopHeader

LoopHeader:  // Loop merge point
    i = phi [0, Entry], [i_new, LoopBody]
    if (i >= 10) br LoopMerge
    br LoopBody

LoopBody:
    if (i == 5) br LoopMerge  // Converted to normal exit
    i_new = i + 1
    br LoopHeader  // Single back edge

LoopMerge:
    return i
```

**Key transformations**:
1. Early exit converted to loop exit
2. Multiple back edges merged to single point
3. PHI node created at loop header
4. All dominance relationships preserved

---

## Integration Points

### Input
- **From**: LLVMBCParser produces unstructured CFG
- **Type**: CFGNode graph with arbitrary edges
- **Properties**: May have multiple back edges, cross-jumping, etc.

### Output
- **To**: SPIRVModule for code generation
- **Type**: Structured CFGNode graph
- **Properties**: SPIR-V compatible structure

### Usage Pattern

```cpp
// In dxil_converter.cpp
CFGStructurizer structurizer(entry_block, node_pool, spirv_module);

if (structurizer.run()) {
    // Structurization successful
    structurizer.traverse(emission_interface);
    CFGNode *structured_entry = structurizer.get_entry_block();
    // Use structured_entry for code generation
} else {
    // Fallback to trivial mode
    structurizer.run_trivial();
}
```

---

## Performance Considerations

### Hot Paths
1. **Query reachability** - Called millions of times
   - Optimized with bitset
   - Stride-based access patterns
   
2. **Dominance queries** - Called frequently
   - Pre-computed during build phase
   - O(1) traversal after computation

3. **PHI insertion** - Critical for correctness
   - Iterative until fixed point
   - May require multiple passes

### Optimization Techniques
- **Lazy evaluation**: Skip analysis of unreachable nodes
- **Bitsets**: Fast reachability queries
- **Caching**: Store computed results
- **Early exit**: Stop iterating when converged

---

## Common Issues & Solutions

### Issue 1: Infinite Loop Detection

**Problem**: Infinite loops with no back edge to header

**Solution**: 
```cpp
LoopMergeAnalysis analyze_loop_merge(...)
{
    // Detects infinite loops
    // Sets infinite_continue_ladder if needed
}
```

### Issue 2: PHI Node Mismatch

**Problem**: PHI node has wrong number of predecessors

**Solution**:
```cpp
void fixup_phi(PHINode &node)
{
    // Ensures PHI node covers all predecessors
    // Adds missing incoming values
}
```

### Issue 3: Broken Dominance

**Problem**: Value use dominated by definition incorrect after transforms

**Solution**:
```cpp
void fixup_broken_value_dominance()
{
    // Inserts additional PHI nodes
    // Ensures all uses dominated by defs
}
```

### Issue 4: Invalid Loop Breaks

**Problem**: Break from within nested loop to wrong target

**Solution**:
```cpp
bool rewrite_invalid_loop_breaks()
{
    // Identifies incorrect break targets
    // Rewrites to correct merge blocks
    // Inserts ladder blocks as needed
}
```

---

## Future Enhancements

1. **Improved Loop Detection**
   - Natural loops detection
   - Nested loop hierarchy

2. **Better Switch Handling**
   - Optimize sparse switches
   - Convert to if-else chains

3. **Performance Optimization**
   - Reduce iteration count
   - Faster convergence detection

4. **Debug Support**
   - Better logging and diagnostics
   - Visualization tools
   - Validation checks

---

## Summary

The CFGStructurizer is a sophisticated graph transformation engine that:

✅ **Analyzes** unstructured control flow graphs
✅ **Identifies** loops and selections
✅ **Transforms** complex patterns to SPIR-V compatible form
✅ **Maintains** SSA form throughout
✅ **Handles** special cases (ROV, barriers, subgroups)
✅ **Validates** correctness invariants

**Result**: Production compiler-quality structurization from DXIL to SPIR-V

---

## Recommended Reading Order

1. **Start**: This document (overview)
2. **Next**: `cfg_structurizer.hpp` (class interface)
3. **Then**: `run()` method in cpp (main algorithm)
4. **Deep Dive**: Specific transformation methods
5. **Debug**: Logging and validation code


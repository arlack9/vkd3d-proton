# CFG Structurizer - Visual Algorithm Guide

## Quick Reference: Main Execution Flow

```
┌─────────────────────────────────────────────────────────────────┐
│  CFGStructurizer::run()                                         │
│  Main structurization algorithm                                 │
└────────────────────┬────────────────────────────────────────────┘
                     │
         ┌───────────┴───────────┐
         │                       │
         ▼                       ▼
    ┌─────────────┐         ┌──────────────┐
    │ run_trivial │         │ Full Analysis│
    └─────────────┘         └──────┬───────┘
                                   │
            ┌──────────────────────┼──────────────────────┐
            │                      │                      │
            ▼                      ▼                      ▼
      ┌──────────┐          ┌────────────┐         ┌──────────┐
      │ Analysis │          │Iterative   │         │Post-Proc │
      │ Phase    │          │Transform   │         │Phase     │
      └──────────┘          └────────────┘         └──────────┘
```

---

## Phase 1: Analysis Pipeline

```
CFG Input (Unstructured)
         │
         ▼
    visit()
    └─ Forward DFS traversal
    └─ Build post-order sequence
    └─ Mark back edges
         │
         ▼
    backwards_visit()
    └─ Reverse DFS traversal
    └─ Build reverse post-order
         │
         ▼
    build_immediate_dominators()
    └─ Compute idom for each node
    └─ Answer: "Who dominates whom?"
         │
         ▼
    build_immediate_post_dominators()
    └─ Compute idom_post for each node
    └─ Answer: "What must execute after?"
         │
         ▼
    build_reachability()
    └─ Compute reachability bitset
    └─ Answer: "Can A reach B?"
         │
         ▼
    Analysis Complete ✓
```

---

## Phase 2: Iterative Structurization

```
Loop until convergence:

    Step 1: find_loops()
    ├─ Identify loop headers (back edges target)
    ├─ Analyze loop structure
    ├─ Determine merge targets
    └─ Mark unrewritable merges
        │
        ▼
    Step 2: find_selection_merges()
    ├─ Identify if/else structures
    ├─ Find selection headers
    ├─ Determine merge points
    └─ Classify merge types
        │
        ▼
    Step 3: structurize()
    ├─ Apply transformations
    ├─ Rewrite complex exits
    ├─ Insert ladder blocks
    ├─ Update dominance info
    └─ Recompute CFG
        │
        ▼
    Step 4: Check Convergence
    ├─ Any changes made?
    └─ Yes → Repeat | No → Done
```

---

## Phase 3: Post-Processing Pipeline

```
After structurization converges:

    split_merge_blocks()
    └─ Break complex merge blocks
    └─ Creates intermediate blocks
         │
         ▼
    eliminate_degenerate_blocks()
    └─ Remove empty blocks
    └─ Merge single-predecessor blocks
         │
         ▼
    cleanup_breaking_phi_constructs()
    └─ Remove unnecessary PHI blocks
    └─ Merge PHI-only blocks
         │
         ▼
    insert_phi()
    └─ Add PHI nodes at merge points
    └─ Compute liveness
    └─ Ensure SSA form
         │
         ▼
    fixup_broken_value_dominance()
    └─ Fix SSA violations
    └─ Insert additional PHIs
         │
         ▼
    rewrite_invalid_loop_breaks()
    └─ Verify break targets
    └─ Fix incorrect jumps
         │
         ▼
    eliminate_degenerate_switch_merges()
    └─ Clean switch statements
         │
         ▼
    Post-Processing Complete ✓
```

---

## Detailed Algorithm: find_loops()

```
For each node in CFG:
    
    If node has incoming back edges:
        This is a LOOP HEADER
        
        1. Find all back edges
           └─ Edges from node to itself or descendants
        
        2. Analyze loop structure
           ┌─ Direct exits (leave loop)
           ├─ Dominated exits (inside loop)
           ├─ Non-dominated exits (escape outer)
           └─ Continue edges (back to header)
        
        3. Determine merge point
           ├─ Post-dominator of all loop blocks
           ├─ May not exist (infinite loop)
           └─ May be implicit (multiple merges)
        
        4. Handle special cases
           ├─ Multiple back edges → Create unified continue block
           ├─ No back edges → Not actually a loop
           ├─ Infinite loop → Special handling
           └─ Nested loops → Process inner first
```

---

## Detailed Algorithm: find_selection_merges()

```
For each node without back edges:
    
    If node has conditional terminator:
        This is a SELECTION HEADER (if statement)
        
        1. Analyze both branches
           ├─ Get true branch target
           ├─ Get false branch target
           └─ Track which nodes reachable from each
        
        2. Find merge point
           ├─ Node where both branches reconverge
           ├─ Post-dominator of both targets
           ├─ Must be dominated by header
           └─ Must not be in inner loop
        
        3. Classify merge
           ├─ Simple merge (both branches reach it)
           ├─ Asymmetric (one branch exits)
           ├─ Complex (branches loop back)
           └─ Broken (no clear merge)
        
        4. Update control flow
           ├─ Mark merge point
           ├─ Set merge type to Selection
           └─ Ready for code generation
```

---

## Dominance Analysis Visualization

```
Example CFG:
    A ─┬─→ B ─┬─→ D ─┐
       │      │      │
       └──→ C ─┴──→ E ─┘
                     │
                     ▼
                     F

Dominance Relationships:
    A dominates all (is entry)
    B is post-dominated by F (must exit through F)
    C is post-dominated by E (must exit through E)
    D and E both post-dominated by F
    
    idom[A] = A (entry)
    idom[B] = A
    idom[C] = A
    idom[D] = B
    idom[E] = max(B, C)  = A (common ancestor)
    idom[F] = E
    
    idom_post[F] = F (exit)
    idom_post[E] = F
    idom_post[D] = F
    idom_post[C] = E (must reach F through E)
    idom_post[B] = F (may reach F through D or E)
    idom_post[A] = A
```

---

## Loop Transformation Example

### Input: Multiple Back Edges

```
        Entry
         │
         ▼
    ┌─→ Header ←──┐
    │   /    \    │ back edge 2
    │  /      \   │
    │ ▼        ▼  │
    │Body1  Body2─┘
    │ │       │
    │ └───┬───┘
    │     │
    │     ▼
    │    Check
    │    /    \
    └───      ▼
            Exit
```

### Output: Single Back Edge

```
        Entry
         │
         ▼
    ┌─→ Header ←──┐
    │   /    \    │
    │  /      \   ▼
    │ ▼        ▼ Continue
    │Body1  Body2   │
    │ │       │     │
    │ └───┬───┘     │
    │     │◄────────┘
    │     ▼
    │    Check
    │    /    \
    └───      ▼
            Exit
```

**Transformation**:
1. Create `Continue` block
2. Redirect Body2's back edge to Continue
3. Continue always branches to Header
4. Now single back edge: Continue → Header

---

## SSA Form: PHI Node Insertion

### Problem: Multiple Reaching Definitions

```
        Entry
         │
         ▼
    a = 1
         │
    ┌────┴────┐
    │         │
    ▼         ▼
if(cond)    b = 2
  │         a = b (redefines a)
  │         │
  └────┬────┘
       │
       ▼
      Merge
   (a = ?)  ← What is a here?
```

### Solution: PHI Node

```
        Entry
         │
         ▼
    a = 1
         │
    ┌────┴────┐
    │         │
    ▼         ▼
if(cond)    b = 2
  │         a = b
  │         │
  └────┬────┘
       │
       ▼
      Merge
  a = phi(a_from_true, a_from_false)
  
  Where:
  - a_from_true = a₁ (from true branch) = 1
  - a_from_false = a₂ (from false branch) = b = 2
```

---

## Ladder Block Creation

### Problem: Complex Control Flow

```
Header
  / \
 /   \
B1   B2
 \   /
  \ /
 Merge   ← Both B1 and B2 exit directly

But structured control flow needs:
Header
  │
Ladder ← Single point to dispatch
  / \
 /   \
B1   B2
 \   /
  \ /
 Merge
```

### Creation Process

```cpp
CFGNode *create_ladder_block(CFGNode *header, CFGNode *node, const char *tag)
{
    // 1. Create new block
    CFGNode *ladder = pool.create_node();
    ladder->name = tag;
    
    // 2. Redirect predecessors dominated by header
    for (auto *pred : node->pred) {
        if (header->dominates(pred)) {
            // Redirect: pred → ladder instead of pred → node
            retarget_succ_from(ladder, node);
        }
    }
    
    // 3. Connect ladder to node
    ladder->succ.push_back(node);
    node->pred.push_back(ladder);
    
    return ladder;
}
```

**Before**:
```
Header → Body1 → Merge
      → Body2 ↗
```

**After**:
```
Header → Ladder → Body1 → Merge
            ↓
          Body2 → Merge
```

---

## Transposed Loop Transformation

### What is a Transposed Loop?

```
// Transposed (body before condition)
Entry
  │
  ▼
Body
  │
  ▼
Condition
 / \
/   \ (true)
│    ▼
│   Exit
│
└─→ Back to Body (false)
```

### Why It's A Problem

- Not SPIR-V compatible
- Condition must be at start of loop
- Hard to analyze

### How It's Fixed

```
// Structured (condition at start)
Entry
  │
  ▼
Header
  │ (true)
  ├───→ Exit
  │ (false)
  ▼
Body
  │
  ▼
Back to Header
```

**Transformation Steps**:
1. Create new Header block
2. Move condition to header
3. Body becomes loop body
4. Condition branches to both exit and body
5. Body has single back edge to header

---

## Early Return Handling

### Problem: Multiple Returns

```cpp
if (x > 0) {
    return x;      // Return 1
}
if (y > 0) {
    return y;      // Return 2
}
return z;          // Return 3
```

### Unstructured CFG

```
Entry → If1 ─true→ Return1
         │
         false
         ↓
        If2 ─true→ Return2
         │
         false
         ↓
        Return3
```

### Structured CFG

```
Entry → SelectionMerge1
         /          \
       true        false
       ↓            ↓
    Return1   SelectionMerge2
               /          \
             true        false
             ↓            ↓
          Return2      Return3
```

---

## ROV (Rasterizer Ordered Views) Transformation

### Pattern Recognition

```cpp
// Input DXIL code
BeginInvariantRegion(counter, &lock_out);
    ... (code accessing memory with ordering)
EndInvariantRegion(counter, lock_out);
```

### Structurization Challenge

- ROV lock must not be broken by control flow
- If statements inside ROV region must be handled specially
- Branches must reunite before EndInvariantRegion

### Transformation

```
Before:
    BeginROV()
    │
    ├─ If cond
    │  ├─ A
    │  ├─ B (different length!)
    │  └─ Merge
    │
    EndROV()

After:
    BeginROV()
    │
    ├─ Ladder
    │  ├─ If cond → A → Merge
    │  └─      → B → Merge
    │
    EndROV()
```

---

## Reachability Analysis

### Bitset Representation

For N nodes, create N×N bitset where:
```
reachability[i][j] = true if node[i] can reach node[j]
```

### Query Optimization

```cpp
bool query_reachability(const CFGNode &from, const CFGNode &to) const
{
    // O(1) lookup using bitset
    size_t from_idx = node_to_index(from);
    size_t to_idx = node_to_index(to);
    
    // Bitset access: stride-based for cache efficiency
    size_t bit_offset = to_idx;
    size_t element = bit_offset / 32;  // 32 bits per element
    size_t bit = bit_offset % 32;
    
    return (reachability_bitset[from_idx * reachability_stride + element] 
            >> bit) & 1;
}
```

### Space-Time Tradeoff

**Option 1: Adjacency Matrix** (Current)
- Space: O(V²)
- Query: O(1)
- Build: O(V + E)

**Option 2: On-demand DFS**
- Space: O(1)
- Query: O(V + E)
- Build: O(1)

Used: Option 1 (fast queries are critical)

---

## Debugging: CFG Visualization

### Text Dump

```cpp
void log_cfg(const char *tag) const
{
    // Produces output like:
    
    // Block Header [id=0, entries=1]
    //   Operations: 5
    //   Terminator: Conditional br
    //   Successors: Header_body, Header_exit
    //   Merge: Selection merge to ExitBlock
    //
    // Block Body [id=1, entries=1]
    //   Operations: 3
    //   Terminator: Unconditional br
    //   Successors: Header
    //   Merge: None (back edge to Header)
}
```

### GraphViz Output

```cpp
void log_cfg_graphviz(const char *path) const
{
    // Produces DOT format:
    
    // digraph {
    //   node_0 [label="Header\nentry"];
    //   node_1 [label="Body"];
    //   node_exit [label="Exit\nexit"];
    //   
    //   node_0 -> node_1 [label="true"];
    //   node_0 -> node_exit [label="false"];
    //   node_1 -> node_0 [label="back"];
    // }
    
    // Render with: dot -Tpng -o cfg.png cfg.dot
}
```

---

## Summary Matrix

| Aspect | Implementation |
|--------|-----------------|
| **Main Loop** | Iterative until convergence |
| **Dominance** | Lengauer-Tarjan algorithm |
| **Analysis** | O(V + E) per iteration |
| **Iterations** | 3-50+ depending on CFG |
| **Total Time** | O(Iterations × (V + E)) |
| **Space** | O(V² + E) for reachability |
| **PHI Insertion** | Iterative algorithm |
| **Convergence** | Guaranteed (monotonic) |

---

## Next Steps for Deep Learning

1. **Study `run()`** - Main orchestration
2. **Study `find_loops()`** - Loop detection
3. **Study `structurize()`** - Transformation logic
4. **Study `insert_phi()`** - SSA maintenance
5. **Trace** through simple example
6. **Profile** on complex shaders
7. **Optimize** hot paths


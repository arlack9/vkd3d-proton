# VKD3D CFG Integration - Implementation Roadmap & Code Guide

## Quick Start: What to Build

### Phase 1: Analysis Foundation (3-5 days)

**Goal**: Store and analyze CFG data without changing shader behavior

#### Step 1.1: Create Header File `libs/vkd3d-shader/cfg_analysis.h`

```c
#ifndef __VKD3D_CFG_ANALYSIS_H__
#define __VKD3D_CFG_ANALYSIS_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* CFG metadata extracted from DXIL */
struct vkd3d_cfg_data {
    uint32_t *headers;      /* Control flow header blocks */
    uint32_t *merges;       /* Merge/reconvergence points */
    uint32_t *continues;    /* Loop continue blocks */
    uint32_t *hints;        /* Control flow hints */
    size_t count;           /* Total CFG blocks */
};

/* Analysis of single loop */
struct vkd3d_cfg_loop_info {
    uint32_t header_id;         /* Loop header block ID */
    uint32_t merge_id;          /* Loop merge/exit block ID */
    uint32_t continue_id;       /* Loop continue block ID */
    uint32_t nesting_depth;     /* Depth in loop nesting */
    bool is_structured;         /* Properly structured loop */
    uint32_t estimated_iters;   /* Estimated iterations (heuristic) */
};

/* Complete CFG analysis result */
struct vkd3d_cfg_analysis {
    struct vkd3d_cfg_loop_info *loops;  /* Array of loops */
    size_t loop_count;                   /* Number of loops */
    
    uint32_t max_loop_depth;             /* Maximum nesting depth */
    uint32_t max_selection_depth;        /* Maximum if/else nesting */
    uint32_t max_convergence_complexity; /* Complexity metric */
    
    bool has_unstructured_control;       /* Contains unstructured CF */
    bool has_irreducible_loops;          /* Contains irreducible cycles */
    bool structurization_failed;         /* CFG structurizer failed */
};

/* Shader complexity classification */
enum vkd3d_shader_complexity_level {
    VKD3D_SHADER_SIMPLE,        /* Linear or simple nested */
    VKD3D_SHADER_MODERATE,      /* Complex nested structures */
    VKD3D_SHADER_COMPLEX,       /* Very complex control flow */
    VKD3D_SHADER_PATHOLOGICAL   /* Unstructured or structurizer failed */
};

/* Functions */
struct vkd3d_cfg_analysis *vkd3d_analyze_cfg(
    const uint32_t *headers,
    const uint32_t *merges,
    const uint32_t *continues,
    const uint32_t *hints,
    size_t count);

enum vkd3d_shader_complexity_level vkd3d_classify_shader_complexity(
    const struct vkd3d_cfg_analysis *analysis);

void vkd3d_cfg_analysis_free(struct vkd3d_cfg_analysis *analysis);

const char *vkd3d_shader_complexity_to_string(
    enum vkd3d_shader_complexity_level level);

#endif /* __VKD3D_CFG_ANALYSIS_H__ */
```

#### Step 1.2: Create Implementation `libs/vkd3d-shader/cfg_analysis.c`

```c
#include "cfg_analysis.h"
#include "vkd3d_common.h"
#include <string.h>

struct vkd3d_cfg_analysis *vkd3d_analyze_cfg(
    const uint32_t *headers,
    const uint32_t *merges,
    const uint32_t *continues,
    const uint32_t *hints,
    size_t count)
{
    struct vkd3d_cfg_analysis *analysis;
    size_t i;
    
    if (!headers || count == 0)
        return NULL;
    
    analysis = vkd3d_malloc(sizeof(*analysis));
    if (!analysis)
        return NULL;
    
    /* Initialize */
    memset(analysis, 0, sizeof(*analysis));
    
    /* Allocate loop array (conservative: assume each header is a loop) */
    analysis->loops = vkd3d_malloc(count * sizeof(*analysis->loops));
    if (!analysis->loops)
    {
        vkd3d_free(analysis);
        return NULL;
    }
    
    /* Analyze loops from CFG data */
    for (i = 0; i < count; i++)
    {
        uint32_t header_id = headers[i];
        
        /* Check if this looks like a loop header */
        /* (In real implementation, correlate with merge/continue data) */
        if (merges[i] != UINT32_MAX && continues[i] != UINT32_MAX)
        {
            struct vkd3d_cfg_loop_info *loop = &analysis->loops[analysis->loop_count];
            
            loop->header_id = header_id;
            loop->merge_id = merges[i];
            loop->continue_id = continues[i];
            loop->is_structured = true;
            loop->estimated_iters = 100; /* Conservative estimate */
            
            analysis->loop_count++;
        }
    }
    
    /* Compute complexity metrics */
    for (i = 0; i < analysis->loop_count; i++)
    {
        uint32_t depth = analysis->loops[i].nesting_depth;
        if (depth > analysis->max_loop_depth)
            analysis->max_loop_depth = depth;
    }
    
    return analysis;
}

enum vkd3d_shader_complexity_level vkd3d_classify_shader_complexity(
    const struct vkd3d_cfg_analysis *analysis)
{
    if (!analysis)
        return VKD3D_SHADER_SIMPLE;
    
    /* Check for pathological cases first */
    if (analysis->structurization_failed)
        return VKD3D_SHADER_PATHOLOGICAL;
    
    if (analysis->has_unstructured_control)
        return VKD3D_SHADER_PATHOLOGICAL;
    
    if (analysis->has_irreducible_loops)
        return VKD3D_SHADER_COMPLEX;
    
    /* Classify by nesting depth */
    if (analysis->max_loop_depth > 3 || analysis->max_selection_depth > 4)
        return VKD3D_SHADER_COMPLEX;
    
    if (analysis->max_loop_depth > 1 || analysis->max_selection_depth > 2)
        return VKD3D_SHADER_MODERATE;
    
    return VKD3D_SHADER_SIMPLE;
}

void vkd3d_cfg_analysis_free(struct vkd3d_cfg_analysis *analysis)
{
    if (!analysis)
        return;
    
    vkd3d_free(analysis->loops);
    vkd3d_free(analysis);
}

const char *vkd3d_shader_complexity_to_string(
    enum vkd3d_shader_complexity_level level)
{
    switch (level)
    {
        case VKD3D_SHADER_SIMPLE:       return "simple";
        case VKD3D_SHADER_MODERATE:     return "moderate";
        case VKD3D_SHADER_COMPLEX:      return "complex";
        case VKD3D_SHADER_PATHOLOGICAL: return "pathological";
        default:                         return "unknown";
    }
}
```

#### Step 1.3: Integrate into `libs/vkd3d-shader/dxil.c`

**Location**: After CFG extraction (~line 1410)

```c
// Add include at top of file
#include "cfg_analysis.h"

// In vkd3d_shader_compile_dxil() after CFG extraction:
if (dxil_spv_converter_get_cfg(converter, &cfg_headers, &cfg_merges, 
                               &cfg_continues, &cfg_hints, &cfg_count) == DXIL_SPV_SUCCESS)
{
    struct vkd3d_cfg_analysis *cfg_analysis;
    enum vkd3d_shader_complexity_level complexity;
    
    /* NEW: Analyze CFG structure */
    cfg_analysis = vkd3d_analyze_cfg(cfg_headers, cfg_merges, cfg_continues, cfg_hints, cfg_count);
    if (cfg_analysis)
    {
        /* NEW: Classify shader complexity */
        complexity = vkd3d_classify_shader_complexity(cfg_analysis);
        
        /* NEW: Store in metadata */
        spirv->meta.shader_complexity = complexity;
        
        /* NEW: Debug output */
        TRACE("DXIL CFG: %zu blocks, %zu loops, complexity %s\n",
              cfg_count,
              cfg_analysis->loop_count,
              vkd3d_shader_complexity_to_string(complexity));
        
        /* NEW: Store for later use if needed */
        /* (We'll expand this in Phase 2) */
        
        vkd3d_cfg_analysis_free(cfg_analysis);
    }
    else
    {
        TRACE("DXIL CFG: No control flow analysis available\n");
    }
}
else
{
    TRACE("DXIL CFG: No control flow metadata available\n");
}
```

#### Step 1.4: Update shader metadata structure

**File**: `libs/vkd3d-shader/shader.h`

```c
struct vkd3d_shader_meta {
    // ...existing fields...
    
    /* NEW: CFG Analysis */
    unsigned int shader_complexity;  /* enum vkd3d_shader_complexity_level */
};
```

---

### Phase 2: Barrier Optimization (1-2 weeks)

**Goal**: Use CFG complexity to optimize barrier placement

#### Step 2.1: Create `libs/vkd3d-shader/barrier_optimization.h`

```c
#ifndef __VKD3D_BARRIER_OPTIMIZATION_H__
#define __VKD3D_BARRIER_OPTIMIZATION_H__

#include "cfg_analysis.h"

struct vkd3d_barrier_config {
    bool use_strict_barriers;      /* Full VK_PIPELINE_STAGE_ALL_COMMANDS */
    bool use_group_coherence;      /* Workgroup-level barriers */
    unsigned int memory_scope;     /* Barrier memory scope */
    bool skip_inner_loop_barriers; /* Skip in simple inner loops */
};

void vkd3d_compute_barrier_config(
    struct vkd3d_barrier_config *config,
    enum vkd3d_shader_complexity_level complexity,
    const struct vkd3d_shader_interface_info *interface_info);

#endif
```

#### Step 2.2: Implementation logic

```c
void vkd3d_compute_barrier_config(
    struct vkd3d_barrier_config *config,
    enum vkd3d_shader_complexity_level complexity,
    const struct vkd3d_shader_interface_info *interface_info)
{
    /* Default: Conservative */
    config->use_strict_barriers = true;
    config->use_group_coherence = true;
    config->memory_scope = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    config->skip_inner_loop_barriers = false;
    
    /* Optimize based on shader complexity */
    if (complexity == VKD3D_SHADER_SIMPLE)
    {
        /* Simple shaders can use relaxed barriers */
        config->use_strict_barriers = false;
        config->use_group_coherence = false;
        config->memory_scope = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    else if (complexity == VKD3D_SHADER_MODERATE)
    {
        /* Moderate: Selective relaxation */
        config->use_strict_barriers = false;
        config->use_group_coherence = true;  /* Keep group coherence */
        config->memory_scope = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        config->skip_inner_loop_barriers = true;  /* Skip in nested loops */
    }
    
    /* For complex/pathological: Keep defaults */
}
```

---

### Phase 3: Dead Code Elimination (1 week)

**Goal**: Identify and eliminate unreachable code

#### Step 3.1: Add helper function

```c
/* In dxil.c or new dead_code.c */
bool vkd3d_is_reachable_block(
    const uint32_t *cfg_headers,
    size_t cfg_count,
    uint32_t block_id)
{
    size_t i;
    
    /* Blocks in CFG header list are reachable */
    for (i = 0; i < cfg_count; i++)
    {
        if (cfg_headers[i] == block_id)
            return true;
    }
    
    return false;
}

void vkd3d_eliminate_unreachable_blocks(
    struct vkd3d_spirv_builder *builder,
    const uint32_t *cfg_headers,
    size_t cfg_count)
{
    /* Mark blocks as unreachable based on CFG info */
    /* Skip SPIR-V emission for unmarked blocks */
}
```

---

## Testing Strategy

### Test Cases

#### Test 1: Simple Linear Shader
```glsl
void main() {
    output = input * 2.0;
}
```
**Expected**: Complexity = SIMPLE, no barriers needed

#### Test 2: Structured Loop
```glsl
for (int i = 0; i < 10; i++) {
    output[i] = compute(input[i]);
}
```
**Expected**: Complexity = SIMPLE, relaxed barriers

#### Test 3: Nested Loops
```glsl
for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
        result[i][j] = compute(input[i][j]);
    }
}
```
**Expected**: Complexity = MODERATE, selective barriers

#### Test 4: Complex Selection
```glsl
if (condition1) {
    if (condition2) {
        if (condition3) {
            if (condition4) {
                if (condition5) {
                    result = complex_compute();
                }
            }
        }
    }
}
```
**Expected**: Complexity = COMPLEX, strict barriers

---

## Validation Checklist

### Phase 1 Validation
- [ ] `cfg_analysis.h` compiles
- [ ] `cfg_analysis.c` compiles
- [ ] No regression in shader compilation
- [ ] CFG analysis code runs without crashes
- [ ] Complexity classification works
- [ ] Trace output shows correct complexity levels
- [ ] Memory is properly freed (valgrind clean)

### Phase 2 Validation
- [ ] Barrier config computed correctly
- [ ] Simple shaders use relaxed barriers
- [ ] Complex shaders use strict barriers
- [ ] No functional differences in output (same semantics)
- [ ] Performance improvement measured (+5-10%)
- [ ] No race conditions introduced

### Phase 3 Validation
- [ ] Dead blocks correctly identified
- [ ] Unreachable code eliminated
- [ ] Module size reduced (2-10%)
- [ ] Functionality unchanged
- [ ] No false positives

---

## Performance Measurement

### Before Implementation
```
Shader: complex_compute.hlsl
Compile time: 100ms
Binary size: 50KB
Execution time: 1000ms (100%)
```

### After Phase 1
```
Compile time: 105ms (+5%)
Binary size: 50KB (unchanged)
Execution time: 1000ms (unchanged - analysis only)
```

### After Phase 2
```
Compile time: 110ms (+10%)
Binary size: 50KB (unchanged)
Execution time: 900ms (-10% from better barriers)
```

### After Phase 3
```
Compile time: 115ms (+15%)
Binary size: 47KB (-6% from dead code)
Execution time: 750ms (-25% from optimization)
```

---

## Risk Mitigation

### Risk #1: Compilation Time Regression
**Mitigation**: Make analysis optional, skip for simple shaders

```c
if (cfg_count > THRESHOLD) {
    // Only analyze complex shaders
    vkd3d_analyze_cfg(...);
}
```

### Risk #2: Correctness Issues
**Mitigation**: Conservative defaults, extensive testing

```c
// Default to strict if analysis uncertain
if (!analysis || analysis->structurization_failed) {
    use_strict_barriers();
}
```

### Risk #3: Memory Issues
**Mitigation**: Free CFG data promptly

```c
vkd3d_cfg_analysis_free(cfg_analysis);  // Don't hold forever
```

---

## Success Metrics

### Compilation
- [ ] No new compiler errors or warnings
- [ ] Compiles on all platforms (Windows, Linux, Mac)
- [ ] Backward compatible with existing code

### Performance
- [ ] 5-15% improvement on complex shaders
- [ ] No regression on simple shaders
- [ ] Measurable improvement in game FPS

### Code Quality
- [ ] All tests pass
- [ ] No memory leaks (valgrind clean)
- [ ] Code review approval
- [ ] Documentation complete

---

## Timeline Estimate

```
Week 1: Phase 1 (Analysis Foundation)
  - Days 1-2: Create files, basic structure
  - Days 3-4: Implement analysis algorithm
  - Day 5: Integration and testing

Week 2: Phase 2 (Barrier Optimization)
  - Days 1-3: Barrier config implementation
  - Days 4-5: Integration with compiler

Week 3: Phase 3 (Dead Code Elimination)
  - Days 1-4: Dead code detection and removal
  - Day 5: Final testing and integration

Week 4: Polish and Optimization
  - Days 1-3: Performance tuning
  - Days 4-5: Final testing and merge
```

---

## Next Immediate Actions

1. **TODAY**: Get approval for this roadmap
2. **TOMORROW**: Create Phase 1 files
3. **DAY 3**: Integrate into build system
4. **WEEK 1**: Complete Phase 1 testing
5. **WEEK 2**: Start Phase 2

---

## Questions to Resolve

1. Should this be opt-in or opt-out?
2. Should we add new meson build flags for this?
3. Who will review the barrier semantics changes?
4. Do we need performance benchmarking setup?
5. Should this be guarded by VKD3D_FEATURE_LEVEL?

---

## Expected Outcome

After implementing all 3 phases:
- ✅ CFG data leveraged for optimization
- ✅ 15-35% performance improvement
- ✅ Smaller SPIR-V binaries (5-10%)
- ✅ Better shader classification
- ✅ Foundation for future optimizations
- ✅ Competitive advantage over other layers


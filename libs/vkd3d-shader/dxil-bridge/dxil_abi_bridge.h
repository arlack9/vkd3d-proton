// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef DXIL_ABI_BRIDGE_H
#define DXIL_ABI_BRIDGE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Version information
#define DXIL_ABI_BRIDGE_VERSION_MAJOR 1
#define DXIL_ABI_BRIDGE_VERSION_MINOR 0

// Opaque handles
typedef struct dxil_context_t dxil_context_t;
typedef struct dxil_module_t dxil_module_t;
typedef struct dxil_resource_t dxil_resource_t;
typedef struct dxil_signature_t dxil_signature_t;
typedef struct dxil_signature_element_t dxil_signature_element_t;
typedef struct dxil_function_t dxil_function_t;
typedef struct dxil_root_signature_t dxil_root_signature_t;

// ============================================================================
// Enums matching DXIL ABI (from DxilConstants.h)
// ============================================================================

typedef enum dxil_resource_kind {
    DXIL_RESOURCE_KIND_INVALID = 0,
    DXIL_RESOURCE_KIND_TEXTURE_1D,
    DXIL_RESOURCE_KIND_TEXTURE_2D,
    DXIL_RESOURCE_KIND_TEXTURE_2D_MS,
    DXIL_RESOURCE_KIND_TEXTURE_3D,
    DXIL_RESOURCE_KIND_TEXTURE_CUBE,
    DXIL_RESOURCE_KIND_TEXTURE_1D_ARRAY,
    DXIL_RESOURCE_KIND_TEXTURE_2D_ARRAY,
    DXIL_RESOURCE_KIND_TEXTURE_2D_MS_ARRAY,
    DXIL_RESOURCE_KIND_TEXTURE_CUBE_ARRAY,
    DXIL_RESOURCE_KIND_TYPED_BUFFER,
    DXIL_RESOURCE_KIND_RAW_BUFFER,
    DXIL_RESOURCE_KIND_STRUCTURED_BUFFER,
    DXIL_RESOURCE_KIND_CBUFFER,
    DXIL_RESOURCE_KIND_SAMPLER,
    DXIL_RESOURCE_KIND_TBUFFER,
    DXIL_RESOURCE_KIND_RT_ACCELERATION_STRUCTURE,
    DXIL_RESOURCE_KIND_FEEDBACK_TEXTURE_2D,
    DXIL_RESOURCE_KIND_FEEDBACK_TEXTURE_2D_ARRAY,
    DXIL_RESOURCE_KIND_NUM_ENTRIES
} dxil_resource_kind_t;

typedef enum dxil_component_type {
    DXIL_COMPONENT_TYPE_INVALID = 0,
    DXIL_COMPONENT_TYPE_I1 = 1,
    DXIL_COMPONENT_TYPE_I16 = 2,
    DXIL_COMPONENT_TYPE_U16 = 3,
    DXIL_COMPONENT_TYPE_I32 = 4,
    DXIL_COMPONENT_TYPE_U32 = 5,
    DXIL_COMPONENT_TYPE_I64 = 6,
    DXIL_COMPONENT_TYPE_U64 = 7,
    DXIL_COMPONENT_TYPE_F16 = 8,
    DXIL_COMPONENT_TYPE_F32 = 9,
    DXIL_COMPONENT_TYPE_F64 = 10,
    DXIL_COMPONENT_TYPE_SNORM_F16 = 11,
    DXIL_COMPONENT_TYPE_UNORM_F16 = 12,
    DXIL_COMPONENT_TYPE_SNORM_F32 = 13,
    DXIL_COMPONENT_TYPE_UNORM_F32 = 14,
    DXIL_COMPONENT_TYPE_SNORM_F64 = 15,
    DXIL_COMPONENT_TYPE_UNORM_F64 = 16,
    DXIL_COMPONENT_TYPE_PACKED_S8X32 = 17,
    DXIL_COMPONENT_TYPE_PACKED_U8X32 = 18,
    // SM 6.9 additions
    DXIL_COMPONENT_TYPE_I8 = 19,
    DXIL_COMPONENT_TYPE_U8 = 20,
    DXIL_COMPONENT_TYPE_F8_E4M3 = 21,
    DXIL_COMPONENT_TYPE_F8_E5M2 = 22,
} dxil_component_type_t;

typedef enum dxil_resource_class {
    DXIL_RESOURCE_CLASS_SRV = 0,
    DXIL_RESOURCE_CLASS_UAV = 1,
    DXIL_RESOURCE_CLASS_CBUFFER = 2,
    DXIL_RESOURCE_CLASS_SAMPLER = 3,
    DXIL_RESOURCE_CLASS_INVALID = 4
} dxil_resource_class_t;

typedef enum dxil_semantic_kind {
    DXIL_SEMANTIC_ARBITRARY = 0,
    DXIL_SEMANTIC_VERTEX_ID,
    DXIL_SEMANTIC_INSTANCE_ID,
    DXIL_SEMANTIC_POSITION,
    DXIL_SEMANTIC_RENDER_TARGET_ARRAY_INDEX,
    DXIL_SEMANTIC_VIEW_PORT_ARRAY_INDEX,
    DXIL_SEMANTIC_CLIP_DISTANCE,
    DXIL_SEMANTIC_CULL_DISTANCE,
    DXIL_SEMANTIC_OUTPUT_CONTROL_POINT_ID,
    DXIL_SEMANTIC_DOMAIN_LOCATION,
    DXIL_SEMANTIC_PRIMITIVE_ID,
    DXIL_SEMANTIC_GS_INSTANCE_ID,
    DXIL_SEMANTIC_SAMPLE_INDEX,
    DXIL_SEMANTIC_IS_FRONT_FACE,
    DXIL_SEMANTIC_COVERAGE,
    DXIL_SEMANTIC_INNER_COVERAGE,
    DXIL_SEMANTIC_TARGET,
    DXIL_SEMANTIC_DEPTH,
    DXIL_SEMANTIC_DEPTH_LESS_EQUAL,
    DXIL_SEMANTIC_DEPTH_GREATER_EQUAL,
    DXIL_SEMANTIC_STENCIL_REF,
    DXIL_SEMANTIC_DISPATCH_THREAD_ID,
    DXIL_SEMANTIC_GROUP_ID,
    DXIL_SEMANTIC_GROUP_INDEX,
    DXIL_SEMANTIC_GROUP_THREAD_ID,
    DXIL_SEMANTIC_TESS_FACTOR,
    DXIL_SEMANTIC_INSIDE_TESS_FACTOR,
    DXIL_SEMANTIC_VIEW_ID,
    DXIL_SEMANTIC_BARYCENTRICS,
    DXIL_SEMANTIC_SHADING_RATE,
    DXIL_SEMANTIC_CULL_PRIMITIVE,
    DXIL_SEMANTIC_START_VERTEX_LOCATION,
    DXIL_SEMANTIC_START_INSTANCE_LOCATION,
    DXIL_SEMANTIC_INVALID
} dxil_semantic_kind_t;

typedef enum dxil_interpolation_mode {
    DXIL_INTERPOLATION_UNDEFINED = 0,
    DXIL_INTERPOLATION_CONSTANT = 1,
    DXIL_INTERPOLATION_LINEAR = 2,
    DXIL_INTERPOLATION_LINEAR_CENTROID = 3,
    DXIL_INTERPOLATION_LINEAR_NOPERSPECTIVE = 4,
    DXIL_INTERPOLATION_LINEAR_NOPERSPECTIVE_CENTROID = 5,
    DXIL_INTERPOLATION_LINEAR_SAMPLE = 6,
    DXIL_INTERPOLATION_LINEAR_NOPERSPECTIVE_SAMPLE = 7,
    DXIL_INTERPOLATION_INVALID = 8
} dxil_interpolation_mode_t;

typedef enum dxil_shader_kind {
    DXIL_SHADER_PIXEL = 0,
    DXIL_SHADER_VERTEX,
    DXIL_SHADER_GEOMETRY,
    DXIL_SHADER_HULL,
    DXIL_SHADER_DOMAIN,
    DXIL_SHADER_COMPUTE,
    DXIL_SHADER_LIBRARY,
    DXIL_SHADER_RAY_GENERATION,
    DXIL_SHADER_INTERSECTION,
    DXIL_SHADER_ANY_HIT,
    DXIL_SHADER_CLOSEST_HIT,
    DXIL_SHADER_MISS,
    DXIL_SHADER_CALLABLE,
    DXIL_SHADER_MESH,
    DXIL_SHADER_AMPLIFICATION,
    DXIL_SHADER_NODE,
    DXIL_SHADER_INVALID
} dxil_shader_kind_t;

typedef enum dxil_root_parameter_type {
    DXIL_ROOT_PARAMETER_DESCRIPTOR_TABLE = 0,
    DXIL_ROOT_PARAMETER_32BIT_CONSTANTS = 1,
    DXIL_ROOT_PARAMETER_CBV = 2,
    DXIL_ROOT_PARAMETER_SRV = 3,
    DXIL_ROOT_PARAMETER_UAV = 4
} dxil_root_parameter_type_t;

typedef enum dxil_descriptor_range_type {
    DXIL_DESCRIPTOR_RANGE_SRV = 0,
    DXIL_DESCRIPTOR_RANGE_UAV = 1,
    DXIL_DESCRIPTOR_RANGE_CBV = 2,
    DXIL_DESCRIPTOR_RANGE_SAMPLER = 3
} dxil_descriptor_range_type_t;

// ============================================================================
// Resource Information
// ============================================================================

typedef struct dxil_resource_info {
    uint32_t id;                          // Register ID (t0, u0, etc.)
    uint32_t space_id;                    // Register space
    dxil_resource_kind_t kind;            // Texture, buffer, etc.
    dxil_resource_class_t class_type;     // SRV, UAV, CBuffer, Sampler
    dxil_component_type_t component_type; // I32, F32, etc.
    uint32_t element_stride;              // Bytes per element (structured buffers)
    uint32_t sample_count;                // MSAA sample count
    bool is_globally_coherent;
    bool has_counter;                     // Append/consume buffer
    bool is_rov;                          // Rasterizer Ordered View
    bool has_atomic64_use;
} dxil_resource_info_t;

// ============================================================================
// Signature Element Information (Shader I/O)
// ============================================================================

typedef struct dxil_signature_element_info {
    char semantic_name[64];               // e.g., "SV_Position", "TEXCOORD0"
    uint32_t semantic_index;              // For arrays: TEXCOORD0, TEXCOORD1, etc.
    dxil_semantic_kind_t semantic_kind;
    dxil_interpolation_mode_t interpolation_mode;
    uint32_t row;                         // Register row (r0, r1, etc.)
    uint32_t col;                         // Component (X=0, Y=1, Z=2, W=3)
    uint32_t start_row;                   // Packing start
    uint32_t start_col;
    uint32_t data_width_bits;             // 16 or 32
    bool is_used;
    uint32_t output_stream;               // For GS multiple streams
} dxil_signature_element_info_t;

// ============================================================================
// Function/Opcode Information
// ============================================================================

typedef struct dxil_function_info {
    char name[256];                       // Function name
    dxil_shader_kind_t shader_kind;
    uint32_t num_resources;
    uint32_t num_inputs;
    uint32_t num_outputs;
    uint32_t num_patch_constants;
    uint32_t feature_flags;               // Feature bitmask
} dxil_function_info_t;

// ============================================================================
// Root Signature Information
// ============================================================================

typedef struct dxil_root_descriptor_table_range {
    dxil_descriptor_range_type_t range_type;
    uint32_t num_descriptors;
    uint32_t base_shader_register;
    uint32_t register_space;
    uint32_t offset_in_table;
} dxil_root_descriptor_table_range_t;

typedef struct dxil_root_parameter_info {
    dxil_root_parameter_type_t type;
    dxil_shader_kind_t shader_visibility;  // Which stages see this
    
    union {
        struct {
            uint32_t num_ranges;
            dxil_root_descriptor_table_range_t *ranges;
        } descriptor_table;
        struct {
            uint32_t shader_register;
            uint32_t register_space;
            uint32_t num_32bit_values;
        } constants;
        struct {
            uint32_t shader_register;
            uint32_t register_space;
        } descriptor;
    };
} dxil_root_parameter_info_t;

typedef struct dxil_root_signature_info {
    uint32_t version;                     // 1_0 or 1_1
    uint32_t num_parameters;
    dxil_root_parameter_info_t *parameters;
    uint32_t num_static_samplers;
    uint32_t flags;                       // DxilRootSignatureFlags
} dxil_root_signature_info_t;

// ============================================================================
// Context Management
// ============================================================================

// Create context from DXIL binary (LLVM bitcode)
dxil_context_t* dxil_create_context(const uint8_t* data, size_t size);

// Create context from file path
dxil_context_t* dxil_create_context_from_file(const char* path);

// Destroy context and free all resources
void dxil_destroy_context(dxil_context_t* ctx);

// Get last error message
const char* dxil_get_last_error(dxil_context_t* ctx);

// ============================================================================
// Module Information
// ============================================================================

// Get DXIL version
void dxil_get_version(dxil_context_t* ctx, uint32_t* major, uint32_t* minor);

// Get shader kind and model
dxil_shader_kind_t dxil_get_shader_kind(dxil_context_t* ctx);
uint32_t dxil_get_shader_model_major(dxil_context_t* ctx);
uint32_t dxil_get_shader_model_minor(dxil_context_t* ctx);

// Get entry function info
dxil_function_info_t* dxil_get_entry_function_info(dxil_context_t* ctx);
void dxil_free_function_info(dxil_function_info_t* info);

// ============================================================================
// Resource Queries
// ============================================================================

uint32_t dxil_get_num_resources(dxil_context_t* ctx);
dxil_resource_info_t* dxil_get_resource(dxil_context_t* ctx, uint32_t index);
void dxil_free_resource_info(dxil_resource_info_t* info);

// Find resource by register and space
dxil_resource_info_t* dxil_find_resource_by_register(dxil_context_t* ctx, 
    uint32_t reg_id, uint32_t space_id, dxil_resource_class_t class_type);
void dxil_free_resource_info(dxil_resource_info_t* info);

// ============================================================================
// Signature Queries (Shader I/O)
// ============================================================================

uint32_t dxil_get_num_input_signature_elements(dxil_context_t* ctx);
uint32_t dxil_get_num_output_signature_elements(dxil_context_t* ctx);
uint32_t dxil_get_num_patch_constant_signature_elements(dxil_context_t* ctx);

dxil_signature_element_info_t* dxil_get_input_signature_element(dxil_context_t* ctx, uint32_t index);
dxil_signature_element_info_t* dxil_get_output_signature_element(dxil_context_t* ctx, uint32_t index);
dxil_signature_element_info_t* dxil_get_patch_constant_element(dxil_context_t* ctx, uint32_t index);

void dxil_free_signature_element_info(dxil_signature_element_info_t* info);

// ============================================================================
// Root Signature Queries
// ============================================================================

bool dxil_has_root_signature(dxil_context_t* ctx);
dxil_root_signature_info_t* dxil_get_root_signature(dxil_context_t* ctx);
void dxil_free_root_signature_info(dxil_root_signature_info_t* info);

// ============================================================================
// Utility Functions
// ============================================================================

// Get enum name strings (for debugging)
const char* dxil_resource_kind_to_string(dxil_resource_kind_t kind);
const char* dxil_component_type_to_string(dxil_component_type_t type);
const char* dxil_semantic_kind_to_string(dxil_semantic_kind_t kind);
const char* dxil_shader_kind_to_string(dxil_shader_kind_t kind);

#ifdef __cplusplus
}
#endif

#endif // DXIL_ABI_BRIDGE_H
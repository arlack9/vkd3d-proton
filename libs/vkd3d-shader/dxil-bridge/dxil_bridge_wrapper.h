/*
 * DXIL Bridge Wrapper for vkd3d
 * Uses dxil_abi_bridge library to parse DXIL shaders
 */

#ifndef DXIL_BRIDGE_WRAPPER_H
#define DXIL_BRIDGE_WRAPPER_H

#include "dxil-bridge/dxil_abi_bridge.h"
#include "vkd3d_shader_private.h"
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

struct dxil_bridge_context {
    dxil_context_t* dxil_ctx;
    dxil_function_info_t* func_info;
    uint32_t num_resources;
    uint32_t num_inputs;
    uint32_t num_outputs;
};

// Parse DXIL shader and extract metadata
static inline struct dxil_bridge_context* dxil_bridge_parse(const uint8_t* dxil_data, size_t dxil_size) {
    struct dxil_bridge_context* ctx = calloc(1, sizeof(*ctx));
    if (!ctx) return NULL;
    
    ctx->dxil_ctx = dxil_create_context(dxil_data, dxil_size);
    if (!ctx->dxil_ctx) {
        free(ctx);
        return NULL;
    }
    
    ctx->func_info = dxil_get_entry_function_info(ctx->dxil_ctx);
    ctx->num_resources = dxil_get_num_resources(ctx->dxil_ctx);
    ctx->num_inputs = dxil_get_num_input_signature_elements(ctx->dxil_ctx);
    ctx->num_outputs = dxil_get_num_output_signature_elements(ctx->dxil_ctx);
    
    return ctx;
}

// Get shader kind for Vulkan pipeline creation
static inline VkShaderStageFlagBits dxil_bridge_get_shader_stage(struct dxil_bridge_context* ctx) {
    if (!ctx) return 0;
    
    dxil_shader_kind_t kind = dxil_get_shader_kind(ctx->dxil_ctx);
    
    switch (kind) {
        case DXIL_SHADER_VERTEX:   return VK_SHADER_STAGE_VERTEX_BIT;
        case DXIL_SHADER_PIXEL:    return VK_SHADER_STAGE_FRAGMENT_BIT;
        case DXIL_SHADER_COMPUTE:  return VK_SHADER_STAGE_COMPUTE_BIT;
        case DXIL_SHADER_GEOMETRY: return VK_SHADER_STAGE_GEOMETRY_BIT;
        case DXIL_SHADER_HULL:     return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case DXIL_SHADER_DOMAIN:   return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case DXIL_SHADER_MESH:     return VK_SHADER_STAGE_MESH_BIT_EXT;
        case DXIL_SHADER_AMPLIFICATION: return VK_SHADER_STAGE_TASK_BIT_EXT;
        default: return 0;
    }
}

// Get shader model
static inline void dxil_bridge_get_shader_model(struct dxil_bridge_context* ctx, 
                                                 uint32_t* major, uint32_t* minor) {
    if (!ctx) return;
    *major = dxil_get_shader_model_major(ctx->dxil_ctx);
    *minor = dxil_get_shader_model_minor(ctx->dxil_ctx);
}

// Check if shader requires specific Vulkan features
static inline uint32_t dxil_bridge_get_required_features(struct dxil_bridge_context* ctx) {
    if (!ctx) return 0;
    
    uint32_t features = 0;
    
    // Check shader model for raytracing
    uint32_t major = dxil_get_shader_model_major(ctx->dxil_ctx);
    uint32_t minor = dxil_get_shader_model_minor(ctx->dxil_ctx);
    
    if (major > 6 || (major == 6 && minor >= 5)) {
        features |= VK_FEATURE_RAY_TRACING_PIPELINE_BIT;
    }
    
    // Check resources for double precision
    uint32_t num_res = ctx->num_resources;
    for (uint32_t i = 0; i < num_res; i++) {
        dxil_resource_info_t* res = dxil_get_resource(ctx->dxil_ctx, i);
        if (res) {
            if (res->component_type == DXIL_COMPONENT_TYPE_F64) {
                features |= VK_FEATURE_SHADER_FLOAT64_BIT;
            }
            if (res->has_atomic64_use) {
                features |= VK_FEATURE_SHADER_INT64_BIT;
            }
            dxil_free_resource_info(res);
        }
    }
    
    return features;
}

// Get root signature info
static inline bool dxil_bridge_has_root_signature(struct dxil_bridge_context* ctx) {
    if (!ctx) return false;
    return dxil_has_root_signature(ctx->dxil_ctx);
}

// Clean up
static inline void dxil_bridge_destroy(struct dxil_bridge_context* ctx) {
    if (!ctx) return;
    
    if (ctx->func_info) dxil_free_function_info(ctx->func_info);
    if (ctx->dxil_ctx) dxil_destroy_context(ctx->dxil_ctx);
    free(ctx);
}

#ifdef __cplusplus
}
#endif

#endif /* DXIL_BRIDGE_WRAPPER_H */
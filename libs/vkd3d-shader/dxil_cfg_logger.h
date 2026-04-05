// dxil_cfg_logger.h
#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void dxil_cfg_log_metadata(const char* game_name,
                           const char* shader_name,
                           const uint32_t* headers,
                           const uint32_t* merges,
                           const uint32_t* continues,
                           const uint32_t* hints,
                           size_t block_count);

void dxil_cfg_log_init(const char* log_path);
void dxil_cfg_log_close(void);

#ifdef __cplusplus
}
#endif
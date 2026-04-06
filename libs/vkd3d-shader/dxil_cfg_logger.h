// dxil_cfg_logger.h
#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Path handling utilities */
void dxil_cfg_log_get_safe_path(char* buffer, size_t buffer_size, const char* filename);

/* Logging initialization and cleanup */
void dxil_cfg_log_init_with_safe_path(const char* filename);
void dxil_cfg_log_metadata(const char* game_name,
                           const char* shader_name,
                           const uint32_t* headers,
                           const uint32_t* merges,
                           const uint32_t* continues,
                           const uint32_t* hints,
                           size_t block_count);

void dxil_cfg_log_init(const char* log_path);
void dxil_cfg_log_close(void);

/* Legacy compatibility - deprecated */
void dxil_cfg_log_set_game(const char* game_name);

#ifdef __cplusplus
}
#endif
// dxil_cfg_logger.c
#include "dxil_cfg_logger.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #define PATH_MAX 260
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #ifndef PATH_MAX
        #define PATH_MAX 4096
    #endif
#endif

static FILE* g_log_file = NULL;
static const char* g_game_name = "Unknown";

/* Get safe path for log file - uses current directory or temp directory */
void dxil_cfg_log_get_safe_path(char* buffer, size_t buffer_size, const char* filename)
{
    if (!buffer || buffer_size < 10 || !filename)
        return;

    /* Try current working directory first */
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        snprintf(buffer, buffer_size, "%s/%s", cwd, filename);
        return;
    }

    /* Fallback to temp directory or just filename */
#ifdef _WIN32
    char temp_dir[PATH_MAX];
    DWORD ret = GetTempPathA(sizeof(temp_dir), temp_dir);
    if (ret > 0 && ret < sizeof(temp_dir))
    {
        snprintf(buffer, buffer_size, "%s%s", temp_dir, filename);
        return;
    }
#else
    const char* temp = getenv("TMPDIR");
    if (!temp) temp = "/tmp";
    snprintf(buffer, buffer_size, "%s/%s", temp, filename);
    return;
#endif

    /* Last resort: just use filename */
    snprintf(buffer, buffer_size, "%s", filename);
}

void dxil_cfg_log_init_with_safe_path(const char* filename)
{
    char safe_path[PATH_MAX];
    dxil_cfg_log_get_safe_path(safe_path, sizeof(safe_path), filename ? filename : "dxil_cfg_log.txt");
    dxil_cfg_log_init(safe_path);
}

void dxil_cfg_log_init(const char* log_path)
{
    if (g_log_file) fclose(g_log_file);
    
    /* Validate path before opening */
    if (!log_path || strlen(log_path) == 0)
        log_path = "dxil_cfg_log.txt";
    
    g_log_file = fopen(log_path, "w");
    if (g_log_file) {
        time_t start_time = time(NULL);
        fprintf(g_log_file, "DXIL CFG Metadata Log\n");
        fprintf(g_log_file, "Started: %s", ctime(&start_time));
        fprintf(g_log_file, "========================================\n\n");
        fflush(g_log_file);
    }
}

void dxil_cfg_log_set_game(const char* game_name)
{
    g_game_name = game_name ? game_name : "Unknown";
}

void dxil_cfg_log_metadata(const char* game_name,
                           const char* shader_name,
                           const uint32_t* headers,
                           const uint32_t* merges,
                           const uint32_t* continues,
                           const uint32_t* hints,
                           size_t block_count)
{
    if (!g_log_file) return;
    
    const char* actual_game = game_name ? game_name : g_game_name;
    
    fprintf(g_log_file, "========================================\n");
    fprintf(g_log_file, "Game: %s\n", actual_game);
    fprintf(g_log_file, "Shader: %s\n", shader_name ? shader_name : "unknown");
    fprintf(g_log_file, "Block Count: %zu\n", block_count);
    fprintf(g_log_file, "----------------------------------------\n");
    
    if (headers && merges && continues && hints && block_count > 0) {
        fprintf(g_log_file, "Index | Header | Merge | Continue | Hint\n");
        fprintf(g_log_file, "------|--------|-------|----------|-----\n");
        
        for (size_t i = 0; i < block_count && i < 1000; i++) {
            fprintf(g_log_file, "%5zu | %6u | %5u | %8u | %4u\n",
                    i, headers[i], merges[i], continues[i], hints[i]);
        }
        
        if (block_count > 1000) {
            fprintf(g_log_file, "... and %zu more blocks\n", block_count - 1000);
        }
    } else {
        fprintf(g_log_file, "NO VALID CFG DATA\n");
    }
    
    fprintf(g_log_file, "========================================\n\n");
    fflush(g_log_file);
}

void dxil_cfg_log_close(void)
{
    if (g_log_file) {
        time_t close_time = time(NULL);
        fprintf(g_log_file, "Log closed: %s", ctime(&close_time));
        fclose(g_log_file);
        g_log_file = NULL;
    }
}
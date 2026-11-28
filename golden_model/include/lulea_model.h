#ifndef LULEA_MODEL_H
#define LULEA_MODEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { LULEA_MODEL_STAGES = 6 };

typedef struct lulea_model lulea_model_t;

typedef struct
{
    bool valid;
    uint8_t port;
} lulea_result_t;

// Load the nine packed cache_gen files. Returns NULL on error.
// Pass NULL and 0 if no error message is needed.
lulea_model_t *lulea_model_create(const char *cache_dir, char *error, size_t error_size);
void lulea_model_destroy(lulea_model_t *model);

// Clear the pipeline registers. The cache contents persist across reset.
void lulea_model_reset(lulea_model_t *model);

// Advance one rising edge and return the result visible after that edge.
// An IP accepted on edge n has its result on edge n+5. valid=false inserts a bubble.
lulea_result_t lulea_model_tick(lulea_model_t *model, bool valid, uint32_t ip);

#ifdef __cplusplus
}
#endif

#endif

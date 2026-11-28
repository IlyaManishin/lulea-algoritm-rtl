#include "lulea_model.h"

#include "cache_images.h"
#include "popcount.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    chunk_t chunk;
    chunk_sum_t chunk_sum;
    uint16_t ip_tail;
    uint8_t bit_pos;
    uint8_t port;
    bool bypass;
    bool valid;
} rank_reg_t;

typedef struct
{
    seq_cell_t cell;
    uint16_t ip_tail;
    bool valid;
} seq_reg_t;

typedef struct
{
    rank_reg_t l1_rank;
    seq_reg_t l1_seq;
    rank_reg_t l2_rank;
    seq_reg_t l2_seq;
    rank_reg_t l3_rank;
    lulea_result_t l3_seq;
} pipeline_regs_t;

struct lulea_model
{
    pipeline_regs_t regs;
    cache_images_t caches;
};

static inline uint32_t seq_addr(const rank_reg_t *stage)
{
    // Keep bits [bit_pos:0]. The XOR gives a shift in [0, 63].
    const chunk_t mask = UINT64_MAX >> (stage->bit_pos ^ CHUNK_BIT_MASK);
    return stage->chunk_sum + popcount_64(stage->chunk & mask);
}

static inline rank_reg_t bypass_rank(seq_cell_t cell)
{
    const rank_reg_t next = {
        .port = (uint8_t)cell,
        .bypass = true,
        .valid = true,
    };
    return next;
}

static inline rank_reg_t read_l1_rank(const lulea_model_t *model, bool valid, uint32_t ip)
{
    if (!valid)
    {
        const rank_reg_t bubble = {0};
        return bubble;
    }

    const uint32_t word_addr = ip >> 20;
    const rank_reg_t next = {
        .chunk = model->caches.l1_pop[word_addr],
        .chunk_sum = model->caches.l1_chunk_sum[word_addr],
        .ip_tail = (uint16_t)(ip & 0x3fffU),
        .bit_pos = (uint8_t)((ip >> 14) & CHUNK_BIT_MASK),
        .valid = true,
    };
    return next;
}

static inline seq_reg_t read_l1_seq(const lulea_model_t *model)
{
    const rank_reg_t *stage = &model->regs.l1_rank;
    if (!stage->valid)
    {
        const seq_reg_t bubble = {0};
        return bubble;
    }

    const seq_reg_t next = {
        .cell = model->caches.l1_seq[seq_addr(stage)],
        .ip_tail = stage->ip_tail,
        .valid = true,
    };
    return next;
}

static inline rank_reg_t read_l2_rank(const lulea_model_t *model)
{
    const seq_reg_t *stage = &model->regs.l1_seq;
    if (!stage->valid)
    {
        const rank_reg_t bubble = {0};
        return bubble;
    }
    if ((stage->cell & L1_PTR_FLAG) == 0)
    {
        return bypass_rank(stage->cell);
    }

    const uint32_t word_addr = stage->cell & (L1_PTR_FLAG - 1U);
    const rank_reg_t next = {
        .chunk = model->caches.l2_pop[word_addr],
        .chunk_sum = model->caches.l2_chunk_sum[word_addr],
        .ip_tail = (uint8_t)stage->ip_tail,
        .bit_pos = (uint8_t)(stage->ip_tail >> 8),
        .valid = true,
    };
    return next;
}

static inline seq_reg_t read_l2_seq(const lulea_model_t *model)
{
    const rank_reg_t *stage = &model->regs.l2_rank;
    if (!stage->valid)
    {
        const seq_reg_t bubble = {0};
        return bubble;
    }

    const seq_reg_t next = {
        .cell = stage->bypass ? stage->port : model->caches.l2_seq[seq_addr(stage)],
        .ip_tail = stage->ip_tail,
        .valid = true,
    };
    return next;
}

static inline rank_reg_t read_l3_rank(const lulea_model_t *model)
{
    const seq_reg_t *stage = &model->regs.l2_seq;
    if (!stage->valid)
    {
        const rank_reg_t bubble = {0};
        return bubble;
    }
    if ((stage->cell & L2_PTR_FLAG) == 0)
    {
        return bypass_rank(stage->cell);
    }

    const uint32_t block = stage->cell & (L2_PTR_FLAG - 1U);
    const uint32_t word_addr = (block << 2) | (stage->ip_tail >> 6);
    const rank_reg_t next = {
        .chunk = model->caches.l3_pop[word_addr],
        .chunk_sum = model->caches.l3_chunk_sum[word_addr],
        .bit_pos = (uint8_t)(stage->ip_tail & CHUNK_BIT_MASK),
        .valid = true,
    };
    return next;
}

static inline lulea_result_t read_l3_seq(const lulea_model_t *model)
{
    const rank_reg_t *stage = &model->regs.l3_rank;
    if (!stage->valid)
    {
        const lulea_result_t bubble = {0};
        return bubble;
    }

    const lulea_result_t next = {
        .valid = true,
        .port = stage->bypass ? stage->port : (uint8_t)model->caches.l3_seq[seq_addr(stage)],
    };
    return next;
}

lulea_model_t *lulea_model_create(const char *cache_dir, char *error, size_t error_size)
{
    lulea_model_t *model = calloc(1, sizeof(*model));
    if (model == NULL)
    {
        if (error != NULL && error_size != 0)
        {
            snprintf(error, error_size, "Out of memory creating Lulea model");
        }
        return NULL;
    }
    if (!cache_images_load(&model->caches, cache_dir, error, error_size))
    {
        lulea_model_destroy(model);
        return NULL;
    }
    return model;
}

void lulea_model_destroy(lulea_model_t *model)
{
    free(model);
}

void lulea_model_reset(lulea_model_t *model)
{
    const pipeline_regs_t empty = {0};
    model->regs = empty;
}

lulea_result_t lulea_model_tick(lulea_model_t *model, bool valid, uint32_t ip)
{
    // Each stage reads the previous stage before its register is overwritten.
    model->regs.l3_seq = read_l3_seq(model);
    model->regs.l3_rank = read_l3_rank(model);
    model->regs.l2_seq = read_l2_seq(model);
    model->regs.l2_rank = read_l2_rank(model);
    model->regs.l1_seq = read_l1_seq(model);
    model->regs.l1_rank = read_l1_rank(model, valid, ip);
    return model->regs.l3_seq;
}

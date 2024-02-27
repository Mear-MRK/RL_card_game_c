#include "RL_model.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "log.h"

static inline uint8_t *wrt2byt(const void *obj, size_t sz, uint8_t *bytes)
{
    assert(obj);
    assert(bytes);
    assert(sz);
    memcpy(bytes, obj, sz);
    return bytes + sz;
}

int RL_save_models(const RL_model *rl_mdl_arr[], int nbr_models, const char *filepath)
{
    assert(rl_mdl_arr);
    assert(nbr_models > 0);
    assert(filepath);
    assert(strlen(filepath) != 0);

    FILE *file = fopen(filepath, "wb");
    if (!file)
    {
        log_msg(LOG_ERR, "RL_save_models: can't open '%s' for writing; %s\n", filepath, strerror(errno));
        return -2;
    }

    size_t size = sizeof(size_t) + sizeof(nbr_models);
    for (int m = 0; m < nbr_models; m++)
    {
        size += sizeof(RL_model_type);
        size += sizeof(rl_mdl_arr[m]->train_prop);
        size += nn_model_serial_size(&rl_mdl_arr[m]->model);
    }

    uint8_t *const init_bytes = (uint8_t *)calloc(size, 1);
    if (!init_bytes)
    {
        log_msg(LOG_ERR, "RL_save_models: can't allocate enough buffer.\n");
        fclose(file);
        return -1;
    }
    uint8_t *bytes = init_bytes;
    bytes = wrt2byt(&size, sizeof(size_t), bytes);
    bytes = wrt2byt(&nbr_models, sizeof(nbr_models), bytes);
    for (int m = 0; m < nbr_models; m++)
    {
        bytes = wrt2byt(&rl_mdl_arr[m]->type, sizeof(rl_mdl_arr[m]->type), bytes);
        bytes = wrt2byt(&rl_mdl_arr[m]->train_prop, sizeof(rl_mdl_arr[m]->train_prop), bytes);
        bytes = nn_model_serialize(&rl_mdl_arr[m]->model, bytes);
    }

    size_t sz = fwrite(init_bytes, 1, size, file);
    fclose(file);
    free((void *)init_bytes);

    if (sz != size)
    {
        log_msg(LOG_ERR, "RL_save_models: seems '%s' not fully written; %s\n", filepath, strerror(errno));
        return -3;
    }
    log_msg(LOG_INF, "RL_save_models: %s saved successfully.\n", filepath);
    return 0;
}

static inline const uint8_t *rd_byt(void *obj, size_t sz, const uint8_t *bytes)
{
    assert(bytes);
    assert(sz);
    if (!obj)
        return bytes + sz;
    memcpy(obj, bytes, sz);
    return bytes + sz;
}

int RL_load_models(RL_model *rl_mdl_arr[], const char *filepath)
{
    assert(rl_mdl_arr);
    assert(filepath);
    assert(strlen(filepath) != 0);

    FILE *file = fopen(filepath, "rb");
    if (!file)
    {
        log_msg(LOG_ERR, "RL_load_models: can't open '%s' for reading; %s\n", filepath, strerror(errno));
        return -2;
    }
    size_t size = 0;
    fread(&size, sizeof(size_t), 1, file);
    size_t sz = size - sizeof(size_t);
    uint8_t *const init_bytes = (uint8_t *)malloc(sz);
    if (!init_bytes)
    {
        log_msg(LOG_ERR, "RL_load_models: can't allocate enough buffer.\n");
        fclose(file);
        return -1;
    }
    uint8_t *bytes = init_bytes;
    if (fread(bytes, 1, sz, file) != sz)
    {
        log_msg(LOG_ERR, "RL_load_models: can't read '%s' (completely); %s\n", filepath, strerror(errno));
        fclose(file);
        free((void *)init_bytes);
        return -3;
    }
    fclose(file);
    int nbr_models = 0;
    bytes = rd_byt(&nbr_models, sizeof(nbr_models), bytes);
    for (int m = 0; m < nbr_models; m++)
    {
        bytes = rd_byt(&rl_mdl_arr[m]->type, sizeof(rl_mdl_arr[m]->type), bytes);
        bytes = rd_byt(&rl_mdl_arr[m]->train_prop, sizeof(rl_mdl_arr[m]->train_prop), bytes);
        bytes = nn_model_deserialize(&rl_mdl_arr[m]->model, bytes);
    }
    free((void *)init_bytes);
    log_msg(LOG_INF, "RL_load_models: models loaded from %s successfully.\n", filepath);
    return 0;
}

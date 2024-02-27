#pragma once

#include "state.h"
#include "table.h"
#include "game.h"

#include "stat_act_array.h"

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

float *calltrump_stat_into_float_arr(const state *state, float *arr);

float *calltrump_act_into_float_arr(suit a, float *arr);

float *calltrump_stat_act_into_float_arr(const state *state, suit a, float *arr);

suit *sort_sut_ord(suit sut_ord[N_SUT], suit led, suit trump);

float *stat_into_float_arr(const state *state, const suit sut_ord[N_SUT], float *arr);

float *act_into_float_arr(const card *a, const suit sut_ord[N_SUT], float *arr);

float *stat_act_into_float_arr(const state *state, const card *a, float *arr);

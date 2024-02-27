#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "hand.h"
#include "table.h"

#define N_PLAYERS 4u
#define N_TEAMS 2u
#define N_PLPT 2u
#define N_DELT 13u
#define N_INIT_DELT 5u

#define N_ROUNDS 13u
#define N_TRICKS 13u

#define NON_PLAYER 10101u


bool is_act_legal(const hand *hand, const table *table, const card *c);

int cmp_card(const card *cl, const card *cr, suit led, suit trump);

unsigned eval_table(table *table, suit trump);

#include "agent.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "game.h"

typedef struct ag_inter_intern_struct
{
    int round_score;
} ag_inter_intern;

static agent_t *construct(agent_t *agent, const void *param)
{
    assert(agent);
    agent->intern = calloc(1, sizeof(ag_inter_intern));
    assert(agent->intern);
    return agent;
}

static void destruct(agent_t *agent)
{
    free(agent->intern);
    agent->intern = NULL;
    agent->id = -1;
}

static void init(agent_t *agent, const void* param)
{
    ((ag_inter_intern *)agent->intern)->round_score = 0;
}

static suit_t call_trump(agent_t *agent)
{
    char hand_str[128];
    hand_to_str(agent->state->hand_p, hand_str);
    printf("Player %d, your init hand:\n%s\n", agent->id, hand_str);
    char trump_str[16];
    printf("Player %d, call the trump: ", agent->id);
    fgets(trump_str, 16, stdin);
    return suit_from_str(trump_str);
}

static char *table_show_str(table_t *table, char *show_str, int player_id)
{
    assert(table);
    assert(show_str);
    assert(player_id >= 0 && player_id < table->nbr_players);
    char buff[32], buff_1[32], buff_2[32];
    table_cleanse(table);
    int op_n = (player_id + 1) % table->nbr_players;
    int op_b = (player_id + 3) % table->nbr_players;
    int co = (player_id + 2) % table->nbr_players;
    show_str[0] = 0;
    sprintf(buff, "  %s  \n", card_to_str(table->card_arr + co, buff_1));
    strcat(show_str, buff);
    sprintf(buff, "%s  %s\n", card_to_str(table->card_arr + op_b, buff_1),
            card_to_str(table->card_arr + op_n, buff_2));
    strcat(show_str, buff);
    sprintf(buff, "  %s  ", card_to_str(table->card_arr + player_id, buff_1));
    strcat(show_str, buff);
    return show_str;
}

static card_t act(agent_t *agent)
{
    assert(agent);
    char tbl_str[64];
    table_show_str(agent->state->table_p, tbl_str, agent->id);
    char hand_str[128];
    hand_to_str(agent->state->hand_p, hand_str);
    printf("Trump %c Led suit %c ", SUT_CHR[*agent->state->trump_p], SUT_CHR[agent->state->table_p->led]);
    printf("Your round team score %d\n", ((ag_inter_intern *)agent->intern)->round_score);
    printf("Player %d, your hand:\n%s\n", agent->id, hand_str);
    printf("Player %d, table:\n%s\n", agent->id, tbl_str);
    char card_str[16];
    card_t c = NON_CARD;
    while (!is_act_legal(agent->state->hand_p, agent->state->table_p, &c))
    {
        printf("Player %d, which card you play? ", agent->id);
        fgets(card_str, 16, stdin);
        c = card_from_str(card_str);
    }

    return c;
}

static void gain(agent_t *agent, float reward)
{
    char tbl_str[64];
    table_show_str(agent->state->table_p, tbl_str, agent->id);
    printf("Player %d, end table:\n%s\n", agent->id, tbl_str);

    if (reward)
    {
        ((ag_inter_intern *)(agent->intern))->round_score++;
        printf("Player %d, your team took the trick!\n", agent->id);
    }
    else
    {
        printf("Player %d, your team lost the trick!\n", agent->id);
    }
}

const agent_class agent_interact = {.construct = construct, .destruct = destruct, 
.init = init, .call_trump = call_trump, .act = act, .gain = gain};

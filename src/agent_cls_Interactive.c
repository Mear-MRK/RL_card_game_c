#include "agent_cls_Interactive.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "game.h"

typedef struct ag_inter_private
{
    int round_score;
    int game_score;
} ag_inter_private;

static agent *constructor(agent *agent, const void *param)
{
    assert(agent);
    agent->private = calloc(1, sizeof(ag_inter_private));
    assert(agent->private);
    return agent;
}

static void destructor(agent *agent)
{
    assert(agent);
    free(agent->private);
    agent->private = NULL;
    agent->ply_id = -1;
}

static void init_episode(agent *agent, const void *param)
{
    ((ag_inter_private *)agent->private)->game_score = 0;
}

static void init_round(agent *agent, const void *param)
{
    ((ag_inter_private *)agent->private)->round_score = 0;
}

static suit call_trump(agent *agent)
{
    char hand_str[128];
    hand_to_str(agent->state->p_hand, hand_str);
    printf("Player %d, your init hand:\n%s\n", agent->ply_id, hand_str);
    char trump_str[16];
    printf("Player %d, call the trump: ", agent->ply_id);
    fgets(trump_str, 16, stdin);
    return suit_from_str(trump_str);
}

static char *table_show_str(const table *table, char *show_str, int player_id)
{
    assert(table);
    assert(show_str);
    assert(player_id >= 0 && player_id < table->nbr_players);
    char buff[32], buff_1[32], buff_2[32];
    // table_cleanse(table);
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

static card act(agent *agent)
{
    assert(agent);
    char tbl_str[64];
    table_show_str(agent->state->p_table, tbl_str, agent->ply_id);
    char hand_str[128];
    hand_to_str(agent->state->p_hand, hand_str);
    printf("Trump %c Led suit %c ", SUT_CHR[*agent->state->p_trump], SUT_CHR[agent->state->p_table->led]);
    printf("Your round team score %d\n", ((ag_inter_private *)agent->private)->round_score);
    printf("Player %d, your hand:\n%s\n", agent->ply_id, hand_str);
    printf("Player %d, table:\n%s\n", agent->ply_id, tbl_str);
    char card_str[16];
    card c = NON_CARD;
    while (!is_act_legal(agent->state->p_hand, agent->state->p_table, &c))
    {
        printf("Player %d, which card you play? ", agent->ply_id);
        fgets(card_str, 16, stdin);
        c = card_from_str(card_str);
    }

    return c;
}

static void trick_gain(agent *agent, float reward)
{
    char tbl_str[64];
    table_show_str(agent->state->p_table, tbl_str, agent->ply_id);
    printf("Player %d, end table:\n%s\n", agent->ply_id, tbl_str);

    if (reward)
    {
        ((ag_inter_private *)(agent->private))->round_score++;
        printf("Player %d, your team took the trick!\n", agent->ply_id);
    }
    else
    {
        printf("Player %d, your team lost the trick!\n", agent->ply_id);
    }
}

static void round_gain(agent *agent, float reward)
{
    char tbl_str[64];
    ag_inter_private *private = (ag_inter_private *)(agent->private);
    assert(private->round_score == (int)reward);
    printf("Player %d, your team score at the end of this round: %g.\n", agent->ply_id, reward);
    if (private->round_score > N_RNK / 2)
        private->game_score++;
}

const agent_class agent_cls_Interactive = 
{.cls_id = 2,
.name = "INTERACTIVE",
.construct = constructor, .destruct = destructor, 
.init_episode = init_episode, .init_round = init_round, 
.call_trump = call_trump, .act = act, 
.trick_gain = trick_gain, .round_gain = round_gain, .finalize_episode = NULL,
.to_string = NULL};

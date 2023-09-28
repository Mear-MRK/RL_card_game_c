#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <math.h>

#include "card.h"
#include "deck.h"
#include "game.h"
#include "agent.h"
#include "agent_cls_Rand.h"
#include "agent_cls_Interactive.h"
#include "agent_cls_RL.h"

#include "log.h"

#include "pcg.h"
#include "rnd_seed.h"

static agent_class parse_ag_ch(char ag_ch)
{
    switch (ag_ch)
    {
    case 'n':
        return agent_cls_RL;
    case 'r':
        return agent_cls_Rand;
    case 'i':
        return agent_cls_Interactive;
    default:
        fprintf(stderr, "Wrong agent char!\n");
        exit(EXIT_FAILURE);
    }
}

#define MAX(x,y) (((x) >= (y)) ? (x) : (y))

int main(int argc, char **argv)
{
    time_t main_began_time = time(NULL);

    agent_cls_RL_construct_param_t RL_cnst_param[4];
    for (int pl = 0; pl < 4; pl++)
        agent_cls_RL_construct_param_clear(RL_cnst_param + pl);
    log_type log_level = LOG_WRN;
    int nbr_games = 1000;
    char inp_agent_str[] = "nnnn";
    char inp_train[] = "1111";

    switch (argc)
    {
    // case 6:
    //     RL_cnst_param.init_eps = strtof(argv[5], NULL);
    case 5:
        strncpy(inp_agent_str, argv[4], 4);
    case 4:
        if (strcmp(argv[3], "0") == 0)
            strcpy(inp_train, "0000");
        else if (strcmp(argv[3], "1") == 0)
            strcpy(inp_train, "1111");
        else if (strcmp(argv[3], "01") == 0)
            strcpy(inp_train, "0101");
        else if (strcmp(argv[3], "10") == 0)
            strcpy(inp_train, "1010");
        else
            memcpy(inp_train, argv[3], MAX(4, strlen(argv[3])));
    case 3:
        log_level = strtol(argv[2], NULL, 10);
        log_set_level(log_level);
    case 2:
        if (strcmp(argv[1], "-h") == 0)
        {
            printf("options: [nbr_episodes] [log_level] [train bit-str] [agents string] [init eps]\n");
            exit(EXIT_SUCCESS);
        }
        nbr_games = strtol(argv[1], NULL, 10);
        assert(nbr_games > 0);
    }

    printf("\nProgram began at %s\n", ctime(&main_began_time));

    pcg_seed(gen_seed());

    hand_t ar_hand[N_PLY];
    memset(ar_hand, 0, N_PLY * sizeof(hand_t));

    hand_t played;
    hand_clear(&played);

    state_t ar_state[N_PLY];
    memset(ar_state, 0, N_PLY * sizeof(state_t));

    table_t table;
    card_t tbl[N_PLY];
    table_init(&table, tbl, N_PLY);
    table_clear(&table);
    table_cleanse(&table);

    suit_t trump = NON_SUT;

    agent_t agent[N_PLY];
    memset(agent, 0, N_PLY * sizeof(agent_t));

    for (int pl = 0; pl < N_PLY; pl++)
    {
        ar_state[pl].p_hand = ar_hand + pl;
        ar_state[pl].p_played = &played;
        ar_state[pl].p_table = &table;
        ar_state[pl].p_trump = &trump;

        const agent_class ag_cl = parse_ag_ch(inp_agent_str[pl]);
        RL_cnst_param[pl].train = inp_train[pl] == '1';
        const void *param = (inp_agent_str[pl] == 'n') ? RL_cnst_param + pl : NULL;
        agent_construct(agent + pl, &ag_cl, pl, param);
        char str_buff[64];
        str_buff[0] = 0;
        printf("%s constructed.\n", agent[pl].name);
    }
    puts("");

    deck_t deck;
    deck_construct(&deck, N_CRD);

    char buff_str[256] = {0};
    int scores[N_TEAM] = {0};
    bool show_progress = true;
    float progress_inc = 10;
    float progress_limit = progress_inc;
    time_t start_time = time(NULL);
    for (int game = 0; game < nbr_games; game++)
    {
        log_msg(LOG_INF, "Game %d ####\n", game);
        int game_scores[N_TEAM];
        memset(game_scores, 0, N_TEAM * sizeof(int));
        int round_leader = pcg_uint32() % N_PLY;
        for (int i = 0; i < N_PLY; i++)
        {
            int pl = (i + round_leader) % N_PLY;
            agent_init_episode(agent + pl, ar_state + pl, NULL);
        }
        for (int round = 0; round < N_RND; round++)
        {
            log_msg(LOG_INF, "Game %d Round %d ====\n", game, round);
            deck_shuffle(&deck, pcg_uint32);
            // Initial 5-cards
            for (int pl = 0; pl < N_PLY; pl++)
            {
                hand_clear(ar_hand + pl);
                hand_add_card_arr(ar_hand + pl, deck.card_arr + pl * N_DELT, N_INIT_DELT);
                log_msg(LOG_DBG, "Game %d Round %d Player %d init hand:\n%s\n", game, round, pl,
                        hand_to_str(ar_hand + pl, buff_str));
            }

            for (int i = 0; i < N_PLY; i++)
            {
                int pl = (i + round_leader) % N_PLY;
                agent_init_round(agent + pl, NULL);
            }
            log_msg(LOG_DBG, "Game %d Round %d Lead player %d\n", game, round, round_leader);
            trump = agent_call_trump(agent + round_leader);
            assert(trump >= 0 && trump < N_SUT);
            for (int pl = 0; pl < N_PLY; pl++)
            {
                hand_add_card_arr(ar_hand + pl, deck.card_arr + pl * N_DELT + N_INIT_DELT, N_DELT - N_INIT_DELT);
                log_msg(LOG_DBG, "Game %d Round %d Player %d init hand:\n%s\n", game, round, pl,
                        hand_to_str(ar_hand + pl, buff_str));
            }
            log_msg(LOG_DBG, "Game %d Round %d Trump: %d\n", game, round, trump);
            hand_clear(&played);
            int round_scores[N_TEAM];
            memset(round_scores, 0, N_TEAM * sizeof(int));
            int leader = round_leader;
            for (int trick = 0; trick < N_TRK; trick++)
            {
                log_msg(LOG_INF, "Game %d Round %d Trick %d ----\n", game, round, trick);
                table_clear(&table);
                table.trick_id = trick;
                table.leader = leader;
                for (int ord = 0; ord < N_PLY; ord++)
                {
                    int pl = (ord + leader) % N_PLY;
                    ar_state[pl].play_ord = ord;
                    card_t c = agent_act(agent + pl);
#ifdef DEBUG
                    if (!is_act_legal(ar_state[pl].p_hand, &table, &c))
                    {
                        log_msg(LOG_ERR, "Game %d Round %d Trick %d Player %d act is illegal: %s\n",
                                game, round, trick, pl, card_to_str(&c, buff_str));
                        log_msg(LOG_ERR, "led %c, hand:\n%s\n", SUT_CHR[table.led],
                                hand_to_str(agent[pl].state->p_hand, buff_str));
                        exit(-1);
                    }
#endif
                    table_put(&table, pl, &c);
                    hand_remove(ar_state[pl].p_hand, &c);
#ifdef DEBUG
                    log_msg(LOG_DBG, "Game %d Round %d Trick %d Player %d played %s\n", game, round, trick, pl,
                            card_to_str(&c, buff_str));
                    log_msg(LOG_DBG, "Game %d Round %d Trick %d Player %d hand:\n%s\n", game, round, trick, pl,
                            hand_to_str(ar_hand + pl, buff_str));
                    log_msg(LOG_DBG, "Game %d Round %d Trick %d Table: %s\n", game, round, trick,
                            table_to_str(&table, buff_str));
#endif
                }
                hand_add_card_arr(&played, table.card_arr, table.nbr_players);
                leader = eval_table(&table, trump);
                int trick_taker_team = leader % N_PLPT;
                round_scores[trick_taker_team]++;
                log_msg(LOG_INF, "Game %d Round %d Trick %d Trick taker team: %d\n", game, round, trick,
                        trick_taker_team);
                for (int pl = 0; pl < N_PLY; pl++)
                {
                    int team = pl % N_PLPT;
                    agent_trick_gain(agent + pl, team == trick_taker_team);
                }
            } // trick
            int round_winner_team = -1;
            int round_winner_score = 0;
            for (int t = 0; t < N_TEAM; t++)
                if (round_scores[t] > round_winner_score)
                {
                    round_winner_score = round_scores[t];
                    round_winner_team = t;
                }
            log_msg(LOG_INF, "Game %d Round %d winner team: %d\n", game, round, round_winner_team);
            for (int pl = 0; pl < N_PLY; pl++)
            {
                int t = pl % N_PLPT;
                agent_round_gain(agent + pl, round_scores[t]);
            }
            if (round_leader % N_PLPT != round_winner_team)
                round_leader = (round_leader + 1) % N_PLY;
            game_scores[round_winner_team]++;
        } // round
        int winner_team = -1;
        int winner_score = 0;
        for (int t = 0; t < N_TEAM; t++)
            if (game_scores[t] > winner_score)
            {
                winner_score = game_scores[t];
                winner_team = t;
            }
        scores[winner_team]++;
        log_msg(LOG_INF, "Game %d winner team: %d\n", game, winner_team);
        for (int pl = 0; pl < N_PLY; pl++)
            agent_finalize(agent + pl, NULL);

        if (show_progress)
        {
            float progress = (game + 1.0f) * 100.0f / nbr_games;
            if (progress >= progress_limit)
            {
                printf("PROGRESS: %5.1f%%  t: %6lds \twin ratio (team0/team1): %7.3f\n", progress,
                       (long)difftime(time(NULL), start_time),
                       (float)scores[0] / scores[1]);
                progress_limit += progress_inc;
            }
        }
    } // game
    puts("");
    for (int t = 0; t < N_TEAM; t++)
    {
        // log_msg(LOG_INF, "Team %d score: %d\n", t, scores[t]);
        printf("Team %d score: %d\n", t, scores[t]);
    }
    puts("");

    for (int pl = 0; pl < N_PLY; pl++)
    {
        agent_destruct(agent + pl);
    }
    deck_destruct(&deck);

    time_t main_end_time = time(NULL);
    printf("\nProgram ended at %s\n", ctime(&main_end_time));

    return EXIT_SUCCESS;
}
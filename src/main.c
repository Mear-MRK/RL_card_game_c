#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>

#include "card.h"
#include "deck.h"
#include "game.h"
#include "agent.h"
#include "agent_rnd.h"
#include "agent_interact.h"
#include "agent_RL.h"

#include "log.h"

#include "pcg.h"
#include "rnd_seed.h"

int main(int argc, char **argv)
{

    log_set_level(warning);

    int nbr_games = 1000;
    if (argc > 1)
    {
        nbr_games = atoi(argv[1]);
        assert(nbr_games > 0);
    }
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

        agent[pl].class = (pl % N_PLPT == 1) ? agent_RL : agent_rnd; // agent_interact;
        agent_construct(agent + pl, pl, NULL);
    }

    deck_t deck;
    deck_construct(&deck, N_CRD);

    char buff_str[256] = {0};
    int scores[N_TEAM] = {0};
    for (int game = 0; game < nbr_games; game++)
    {
        log_msg(info, "Game %d ####\n", game);
        int game_scores[N_TEAM];
        memset(game_scores, 0, N_TEAM * sizeof(int));
        int round_leader = pcg_uint32() % N_PLY;
        for (int round = 0; round < N_RND; round++)
        {
            log_msg(info, "Game %d Round %d ====\n", game, round);
            deck_shuffle(&deck, pcg_uint32);
            // Initial 5-cards
            for (int pl = 0; pl < N_PLY; pl++)
            {
                hand_clear(ar_hand + pl);
                hand_add_card_arr(ar_hand + pl, deck.card_arr + pl * N_DELT, N_INIT_DELT);
                log_msg(debug, "Game %d Round %d Player %d init hand:\n%s\n", game, round, pl,
                        hand_to_str(ar_hand + pl, buff_str));
            }

            for (int i = 0; i < N_PLY; i++)
            {
                int pl = (i + round_leader) % N_PLY;
                agent_init(agent + pl, ar_state + pl, NULL);
            }
            log_msg(debug, "Game %d Round %d Lead player %d\n", game, round, round_leader);
            trump = agent_call_trump(agent + round_leader);
            assert(trump >= 0 && trump < N_SUT);
            for (int pl = 0; pl < N_PLY; pl++)
            {
                hand_add_card_arr(ar_hand + pl, deck.card_arr + pl * N_DELT + N_INIT_DELT, N_DELT - N_INIT_DELT);
                log_msg(debug, "Game %d Round %d Player %d init hand:\n%s\n", game, round, pl,
                        hand_to_str(ar_hand + pl, buff_str));
            }
            log_msg(debug, "Game %d Round %d Trump: %d\n", game, round, trump);

            int round_scores[N_TEAM];
            memset(round_scores, 0, N_TEAM * sizeof(int));
            int leader = round_leader;
            for (int trick = 0; trick < N_TRK; trick++)
            {
                log_msg(info, "Game %d Round %d Trick %d ----\n", game, round, trick);
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
                        log_msg(error, "Game %d Round %d Trick %d Player %d act is illegal: %s\n",
                                game, round, trick, pl, card_to_str(&c, buff_str));
                        log_msg(error, "led %c, hand:\n%s\n", SUT_CHR[table.led],
                                hand_to_str(agent[pl].state->p_hand, buff_str));
                        exit(-1);
                    }
#endif
                    table_put(&table, pl, &c);
                    hand_remove(ar_state[pl].p_hand, &c);
                    log_msg(debug, "Game %d Round %d Trick %d Player %d played %s\n", game, round, trick, pl,
                            card_to_str(&c, buff_str));
                    log_msg(debug, "Game %d Round %d Trick %d Player %d hand:\n%s\n", game, round, trick, pl,
                            hand_to_str(ar_hand + pl, buff_str));
                    log_msg(debug, "Game %d Round %d Trick %d Table: %s\n", game, round, trick,
                            table_to_str(&table, buff_str));
                }
                hand_add_card_arr(&played, table.card_arr, table.nbr_players);
                leader = eval_table(&table, trump);
                int trick_taker_team = leader % N_PLPT;
                round_scores[trick_taker_team]++;
                log_msg(info, "Game %d Round %d Trick %d Trick taker team: %d\n", game, round, trick,
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
            log_msg(info, "Game %d Round %d winner team: %d\n", game, round, round_winner_team);
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
        log_msg(info, "Game %d winner team: %d\n", game, winner_team);
    } // game

    for (int t = 0; t < N_TEAM; t++)
    {
        log_msg(info, "Team %d score: %d\n", t, scores[t]);
        printf("Team %d score: %d\n", t, scores[t]);
    }

    for (int pl = 0; pl < N_PLY; pl++)
    {
        agent_destruct(agent + pl);
    }
    deck_destruct(&deck);

    return EXIT_SUCCESS;
}
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
#include "agent_cls_Sound.h"

#include "log.h"

#include "pcg.h"
#include "rnd_seed.h"

#define MAX(x, y) (((x) >= (y)) ? (x) : (y))
#define MIN(x, y) (((x) <= (y)) ? (x) : (y))

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
    case 's':
        return agent_cls_Sound;
    default:
        fprintf(stderr, "Wrong agent char!\n");
        exit(EXIT_FAILURE);
    }
}

static bool train_RL_model(RL_model_t *rl_mdl, float fill_factor, RL_training_params_t *param)
{
    bool train = fill_factor == 0 || RL_replay_buffer_near_full(&rl_mdl->replay_buff, fill_factor);
    if (train)
    {
        RL_train(rl_mdl, param);
        RL_replay_buffer_clear(&rl_mdl->replay_buff);
        rl_mdl->nbr_training++;
    }
    return train;
}

int main(int argc, char *argv[])
{
    time_t main_began_time = time(NULL);

    log_type log_level = LOG_WRN;
    unsigned nbr_episodes = 3000;
    char inp_agent_str[] = "nnnn";
    char models_filepath[512] = {0};
    strcpy(models_filepath, "models.dat");
    bool training = true;
    float train_buffer_fill_factor = 0.95;
    bool reset_training_counter = false;

    switch (argc)
    {
    case 7:
        strcpy(models_filepath, argv[6]);
    case 6:
        reset_training_counter = (bool)atoi(argv[5]);
    case 5:
        strncpy(inp_agent_str, argv[4], 4);
    case 4:
        training = (bool)atoi(argv[3]);
    case 3:
        log_level = strtol(argv[2], NULL, 10);
        log_set_level(log_level);
    case 2:
        if (strcmp(argv[1], "-h") == 0)
        {
            printf("options: [nbr_episodes] [log_level] [training] [agents_string] [reset_training_counter] [models_filepath]\n");
            exit(EXIT_SUCCESS);
        }
        nbr_episodes = strtol(argv[1], NULL, 10);
        assert(nbr_episodes > 0);
    }
    unsigned nbr_episodes_in_re_buff = MIN(nbr_episodes / 10, 1000);
    if (!nbr_episodes_in_re_buff)
        nbr_episodes_in_re_buff = 1;
    unsigned nbr_hid_layers = 2;
    nn_activ_t activ = nn_activ_SIGMOID;
    float dropout = 0.1;
    nn_optim_class optim_cls = nn_optim_cls_ADAM;

    printf("\nProgram began at %s\n", ctime(&main_began_time));

    pcg_seed(gen_seed());

    hand_t ar_hand[N_PLAYERS];
    memset(ar_hand, 0, N_PLAYERS * sizeof(hand_t));

    hand_t played;
    hand_clear(&played);

    state_t ar_state[N_PLAYERS];
    memset(ar_state, 0, N_PLAYERS * sizeof(state_t));

    table_t table;
    card_t tbl[N_PLAYERS];
    table_init(&table, tbl, N_PLAYERS);
    table_clear(&table);
    // table_cleanse(&table);

    suit_t trump = NON_SUT;

    agent_t agent[N_PLAYERS];
    memset(agent, 0, N_PLAYERS * sizeof(agent_t));

    agent_RL_models_t rl_models;
    memset(&rl_models, 0, sizeof(agent_RL_models_t));
    agent_RL_models_construct(&rl_models, nbr_episodes_in_re_buff);
    if (agent_RL_models_load(&rl_models, models_filepath, &optim_cls))
        agent_RL_models_nn_construct(&rl_models, nbr_hid_layers, &activ, dropout, &optim_cls);

    agent_RL_construct_param_t ag_RL_cnst_param;
    agent_RL_construct_param_clear(&ag_RL_cnst_param);
    ag_RL_cnst_param.rl_models = &rl_models;
    ag_RL_cnst_param.train = training;
    ag_RL_cnst_param.reset_training_counter = reset_training_counter;
    ag_RL_cnst_param.discunt_factor = 0.1; // only applies to new models

    RL_training_params_t train_param;
    RL_trianing_params_clear(&train_param);
    train_param.nbr_epochs = 1;
    train_param.batch_size = 39;

    agent_Snd_construct_param_t ag_Snd_cnst_param;
    agent_Snd_construct_param_clear(&ag_Snd_cnst_param);
    ag_Snd_cnst_param.rl_mdls = &rl_models;
    ag_Snd_cnst_param.training = training;

    for (unsigned pl = 0; pl < N_PLAYERS; pl++)
    {
        ar_state[pl].p_hand = ar_hand + pl;
        ar_state[pl].p_played = &played;
        ar_state[pl].p_table = &table;
        ar_state[pl].p_trump = &trump;

        const agent_class ag_cl = parse_ag_ch(inp_agent_str[pl]);
        const void *param;
        param = NULL;
        if (inp_agent_str[pl] == 'n')
            param = &ag_RL_cnst_param;
        if (inp_agent_str[pl] == 's' && training)
            param = &ag_Snd_cnst_param;
        agent_construct(agent + pl, &ag_cl, pl, param);
        printf("%s constructed.\n", agent[pl].name);
    }
    puts("");

    log_msg(LOG_INF, "Agents info:\n");
    for (unsigned pl = 0; pl < N_PLAYERS; pl++)
    {
        char str_buff[1024];
        log_msg(LOG_INF, "<<<<<\n%s\n>>>>>\n", agent_to_str(agent + pl, str_buff));
    }

    deck_t deck;
    deck_construct(&deck, N_CRD);

    char buff_str[256] = {0};

    unsigned scores[N_TEAMS] = {0};

    bool auto_save = true;
    double auto_save_interval = 5 * 60; // 5min
    bool show_progress = true;
    float progress_inc = 10;
    float progress_limit = progress_inc;
    if (show_progress)
    {
        printf("PROGRESS: %5.1f%%\n", 0.0);
    }
    unsigned last_scores[N_TEAMS] = {0};
    bool trained[4] = {0}; // 4: nbr of models

    time_t start_time = time(NULL);
    time_t p_time = start_time;
    time_t ls_time = start_time;
    for (unsigned game = 0; game < nbr_episodes; game++)
    {
        log_msg(LOG_INF, "Game %d ####\n", game);
        unsigned game_scores[N_TEAMS];
        memset(game_scores, 0, N_TEAMS * sizeof(unsigned));
        unsigned round_leader = pcg_uint32() % N_PLAYERS;
        for (unsigned i = 0; i < N_PLAYERS; i++)
        {
            unsigned pl = (i + round_leader) % N_PLAYERS;
            agent_init_episode(agent + pl, ar_state + pl, NULL);
        }
        for (unsigned round = 0; round < N_ROUNDS; round++)
        {
            log_msg(LOG_INF, "Game %d Round %d ====\n", game, round);
            deck_shuffle(&deck, pcg_uint32);
            // Initial 5-cards
            for (unsigned pl = 0; pl < N_PLAYERS; pl++)
            {
                hand_clear(ar_hand + pl);
                hand_add_card_arr(ar_hand + pl, deck.card_arr + pl * N_DELT, N_INIT_DELT);
                log_msg(LOG_DBG, "Game %d Round %d Player %d init hand:\n%s\n", game, round, pl,
                        hand_to_str(ar_hand + pl, buff_str));
            }

            for (unsigned i = 0; i < N_PLAYERS; i++)
            {
                unsigned pl = (i + round_leader) % N_PLAYERS;
                agent_init_round(agent + pl, NULL);
            }
            log_msg(LOG_DBG, "Game %d Round %d Lead player %d\n", game, round, round_leader);
            trump = agent_call_trump(agent + round_leader);
            log_msg(LOG_DBG, "Game %d Round %d Lead player %d called trump %c\n", game, round, round_leader, SUT_CHR[trump]);
            assert(trump >= 0 && trump < N_SUT);
            for (unsigned pl = 0; pl < N_PLAYERS; pl++)
            {
                hand_add_card_arr(ar_hand + pl, deck.card_arr + pl * N_DELT + N_INIT_DELT, N_DELT - N_INIT_DELT);
                log_msg(LOG_DBG, "Game %d Round %d Player %d init hand:\n%s\n", game, round, pl,
                        hand_to_str(ar_hand + pl, buff_str));
            }
            hand_clear(&played);
            unsigned round_scores[N_TEAMS];
            memset(round_scores, 0, N_TEAMS * sizeof(unsigned));
            unsigned trick_leader = round_leader;
            for (unsigned trick = 0; trick < N_TRICKS; trick++)
            {
                log_msg(LOG_INF, "Game %d Round %d Trick %d ----\n", game, round, trick);
                table_clear(&table);
                table.trick_id = trick;
                table.leader = trick_leader;
                for (unsigned ord = 0; ord < N_PLAYERS; ord++)
                {
                    unsigned pl = (ord + trick_leader) % N_PLAYERS;
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
                } // puting cards on the table
                hand_add_card_arr(&played, table.card_arr, table.nbr_players);
                trick_leader = eval_table(&table, trump);
                unsigned trick_taker_team = trick_leader % N_PLPT;
                round_scores[trick_taker_team]++;
                log_msg(LOG_INF, "Game %d Round %d Trick %d Trick taker team: %d\n", game, round, trick,
                        trick_taker_team);
                for (unsigned pl = 0; pl < N_PLAYERS; pl++)
                {
                    unsigned team = pl % N_PLPT;
                    agent_trick_gain(agent + pl, team == trick_taker_team);
                }
            } // trick
            unsigned round_winner_team = -1;
            unsigned round_winner_score = 0;
            for (unsigned t = 0; t < N_TEAMS; t++)
                if (round_scores[t] > round_winner_score)
                {
                    round_winner_score = round_scores[t];
                    round_winner_team = t;
                }
            log_msg(LOG_INF, "Game %d Round %d winner team: %d\n", game, round, round_winner_team);
            for (unsigned pl = 0; pl < N_PLAYERS; pl++)
            {
                unsigned t = pl % N_PLPT;
                agent_round_gain(agent + pl, round_scores[t]);
            }
            if (training)
            {
                round_leader = pcg_uint32() % N_PLAYERS;
            }
            else if (round_leader % N_PLPT != round_winner_team)
                round_leader = (round_leader + 1) % N_PLAYERS;
            game_scores[round_winner_team]++;
        } // round
        unsigned winner_team = 10101u;
        unsigned winner_score = 0;
        for (unsigned t = 0; t < N_TEAMS; t++)
            if (game_scores[t] > winner_score)
            {
                winner_score = game_scores[t];
                winner_team = t;
            }
        scores[winner_team]++;
        log_msg(LOG_INF, "Game %d winner team: %d\n", game, winner_team);
        for (unsigned pl = 0; pl < N_PLAYERS; pl++)
            agent_finalize(agent + pl, NULL);
        if (training)
        {
            // puts("Training...");
            trained[0] = trained[0] ||
                         train_RL_model(&rl_models.rl_calltrump, train_buffer_fill_factor, &train_param);
            trained[1] = trained[1] ||
                         train_RL_model(&rl_models.rl_distinct, train_buffer_fill_factor, &train_param);
            trained[2] = trained[2] ||
                         train_RL_model(&rl_models.rl_leader, train_buffer_fill_factor, &train_param);
            trained[3] = trained[3] ||
                         train_RL_model(&rl_models.rl_trumpleads, train_buffer_fill_factor, &train_param);
        }

        float progress = (game + 1.0f) * 100.0f / nbr_episodes;

        if (progress >= progress_limit)
        {
            time_t c_time = time(NULL);
            double delta_t;
            if (show_progress)
            {
                delta_t = difftime(c_time, p_time);
                printf("PROGRESS: %5.1f%%  delta_t: %5lds  win ratio (team0/team1): %5.2f  ",
                       progress,
                       (long)delta_t,
                       (float)(scores[0] - last_scores[0]) / (scores[1] - last_scores[1]));
                last_scores[0] = scores[0];
                last_scores[1] = scores[1];
                p_time = c_time;

                if (trained[0] || trained[1] || trained[2] || trained[3])
                    printf("trained=%d%d%d%d  ", trained[0], trained[1], trained[2], trained[3]);
            }
            memset(trained, 0, sizeof(trained));

            delta_t = difftime(c_time, ls_time);
            if (training && auto_save && (delta_t >= auto_save_interval) && !(progress >= 90))
            {
                int save_err = agent_RL_models_save(&rl_models, models_filepath);
                ls_time = c_time;
                if (!save_err && show_progress)
                    printf("auto_save  ");
            }

            if (show_progress)
                puts("");
            progress_limit += progress_inc;
        }
    } // game
    puts("");
    for (unsigned t = 0; t < N_TEAMS; t++)
    {
        // log_msg(LOG_INF, "Team %d score: %d\n", t, scores[t]);
        printf("Team %d score: %d\n", t, scores[t]);
    }
    puts("");

    if (training)
    {
        printf("Final possible trainings... ");
        fflush(stdout);
        train_RL_model(&rl_models.rl_calltrump, 0, &train_param);
        train_RL_model(&rl_models.rl_distinct, 0, &train_param);
        train_RL_model(&rl_models.rl_trumpleads, 0, &train_param);
        train_RL_model(&rl_models.rl_leader, 0, &train_param);
        puts("done.");
        int save_err = agent_RL_models_save(&rl_models, models_filepath);
        if (!save_err)
            puts("Final save: Models have been saved.");
    }

    for (unsigned pl = 0; pl < N_PLAYERS; pl++)
    {
        agent_destruct(agent + pl);
    }
    deck_destruct(&deck);

    agent_RL_models_destruct(&rl_models);

    time_t main_end_time = time(NULL);
    printf("\nProgram ended at %s\n", ctime(&main_end_time));

    return EXIT_SUCCESS;
}

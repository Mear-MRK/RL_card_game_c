#include "agent_cls_RL_nn_max_Q.h"

#include "agent_RL.h"

static agent* constructor(agent* ag, const void *param)
{
    assert(ag);
    ag->public = calloc(1, sizeof(agent_RL_models));
    assert(ag->public);
}

static suit call_trump(agent *ag)
{
    assert(ag);
    return Spade;
}

static card act(agent *ag)
{
    assert(ag);
    card c;
    card_from_sut_rnk(&c, Spade, ag->state->p_hand->st_card[Spade][0]);
    return c;
}


static agent *constructor(agent *agent, const void *param)
{
    assert(agent);
    assert(param);
    agent->private = calloc(1, sizeof(agent_RL_private));
    assert(agent->private);
    agent_RL_private *private = (agent_RL_private *)agent->private;
    const agent_RL_max_Q_construct_param *inp_param = (const agent_RL_max_Q_construct_param *)param;
    assert(inp_param->rl_models);

    private->rl_mdls = inp_param->rl_models;
    intern_rep_buf_construct(&private->rebf_calltrump, N_ROUNDS, private->rl_mdls->rl_calltrump.model.input_size);
    intern_rep_buf_construct(&private->rebf_distinct, N_ROUNDS * N_TRICKS, private->rl_mdls->rl_distinct.model.input_size);
    intern_rep_buf_construct(&private->rebf_leader, N_ROUNDS * N_TRICKS, private->rl_mdls->rl_leader.model.input_size);
    intern_rep_buf_construct(&private->rebf_trumpleads, N_ROUNDS * N_TRICKS, private->rl_mdls->rl_trumpleads.model.input_size);
    private->train = true;
    // private->no_exploration = false;
    private->eps_offset = 0;
    private->eps_delta = 3;

    if (inp_param->train >= 0)
        private->train = (bool)inp_param->train;
    if (inp_param->eps_delta > 0)
        private->eps_delta = inp_param->eps_delta;
    if (inp_param->eps_offset != INT32_MAX)
        private->eps_offset = inp_param->eps_offset;
    if (inp_param->reset_training_counter > 0)
    {
        private->rl_mdls->rl_calltrump.nbr_training = 0;
        private->rl_mdls->rl_distinct.nbr_training = 0;
        private->rl_mdls->rl_leader.nbr_training = 0;
        private->rl_mdls->rl_trumpleads.nbr_training = 0;
    }
    if (private->rl_mdls->new && inp_param->discunt_factor >= 0 && inp_param->discunt_factor <= 1)
    {
        private->rl_mdls->rl_calltrump.discunt_factor = inp_param->discunt_factor;
        private->rl_mdls->rl_distinct.discunt_factor = inp_param->discunt_factor;
        private->rl_mdls->rl_leader.discunt_factor = inp_param->discunt_factor;
        private->rl_mdls->rl_trumpleads.discunt_factor = inp_param->discunt_factor;
    }

    return agent;
}


const agent_class agent_cls_RL =
    {.cls_id = 92,
     .name = "RL_max_Q",
     .construct = constructor,
     .destruct = destructor,
     .init_episode = init_episode,
     .init_round = NULL,
     .call_trump = call_trump,
     .act = act,
     .trick_gain = trick_gain,
     .round_gain = round_gain,
     .finalize_episode = finalize_episode,
     .to_string = agent_RL_to_str};
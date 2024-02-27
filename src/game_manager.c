typedef struct agent_Snd_private
{
    intern_rep_buf rebf_calltrump;
    intern_rep_buf rebf_distinct;
    intern_rep_buf rebf_trumpleads;
    intern_rep_buf rebf_leader;
    bool imitation;
    FLT_TYP stat_act[6 * N_CRD];
} agent_Snd_private;

    private->imitation = false;

    if (param->training >= 0)
        private->imitation = (bool)param->training;

    private->rl_mdls = param->rl_mdls;

    intern_rep_buf_construct(&private->rebf_calltrump, N_ROUNDS, N_CRD + N_SUT);
    intern_rep_buf_construct(&private->rebf_distinct, N_ROUNDS * N_TRICKS, 6 * N_CRD);
    intern_rep_buf_construct(&private->rebf_leader, N_ROUNDS * N_TRICKS, 3 * N_CRD);
    intern_rep_buf_construct(&private->rebf_trumpleads, N_ROUNDS * N_TRICKS, 6 * N_CRD);

        if (agent->private)
    {
        agent_Snd_private *private = (agent_Snd_private *)agent->private;
        intern_rep_buf_destruct(&private->rebf_calltrump);
        intern_rep_buf_destruct(&private->rebf_distinct);
        intern_rep_buf_destruct(&private->rebf_leader);
        intern_rep_buf_destruct(&private->rebf_trumpleads);
        free(agent->private);
        agent->private = NULL;
    }

        if (reward == 0)
        return;

    assert(agent);
    agent_Snd_private *private = (agent_Snd_private *)agent->private;

    if (!private || !private->imitation)
        return;
    intern_rep_buf_update(&private->rebf_trumpleads, reward, private->rl_mdls->rl_calltrump.discunt_factor);
    intern_rep_buf_update(&private->rebf_distinct, reward, private->rl_mdls->rl_distinct.discunt_factor);
    intern_rep_buf_update(&private->rebf_leader, reward, private->rl_mdls->rl_leader.discunt_factor);

        assert(agent);
    agent_Snd_private *private = (agent_Snd_private *)agent->private;

    if (!private || !private->imitation)
        return;

    intern_rep_buf_update(&private->rebf_calltrump, reward, 0);

    intern_rep_buf_new_round(&private->rebf_calltrump);
    intern_rep_buf_new_round(&private->rebf_distinct);
    intern_rep_buf_new_round(&private->rebf_trumpleads);
    intern_rep_buf_new_round(&private->rebf_leader);

        agent_Snd_private *private = (agent_Snd_private *)agent->private;
    if (private && private->imitation)
    {
        intern_rep_buf_append_to_replay_buff(&private->rebf_calltrump, &private->rl_mdls->rl_calltrump.replay_buff);
        intern_rep_buf_append_to_replay_buff(&private->rebf_distinct, &private->rl_mdls->rl_distinct.replay_buff);
        intern_rep_buf_append_to_replay_buff(&private->rebf_trumpleads, &private->rl_mdls->rl_trumpleads.replay_buff);
        intern_rep_buf_append_to_replay_buff(&private->rebf_leader, &private->rl_mdls->rl_leader.replay_buff);
    }

        if (private && private->imitation)
    {
        calltrump_stat_act_into_float_arr(agent->state, trump, private->stat_act);
        intern_rep_buf_add(&private->rebf_calltrump, private->stat_act);
    }

    static inline intern_rep_buf *choose_buff(agent_Snd_private *private, suit led, suit trump)
{
    assert(private);
    if (led == NON_SUT)
        return &private->rebf_leader;
    if (led == trump)
        return &private->rebf_trumpleads;
    return &private->rebf_distinct;
}

void agent_Snd_construct_param_clear(agent_Snd_construct_param *param)
{
    assert(param);
    param->training = -1;
    param->rl_mdls = NULL;
}


    if (private && private->imitation)
    {
        stat_act_into_float_arr(agent->state, &a, private->stat_act);
        intern_rep_buf_add(choose_buff(private, led, trump), private->stat_act);
    }
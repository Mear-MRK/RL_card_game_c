#pragma once

#include "config.h"

#include "state.h"

typedef struct agent agent;

typedef agent *(*construct_func)(agent *agent, const void *param);
typedef void (*destruct_func)(agent *agent);
typedef void (*init_func)(agent *, const void *param);
typedef suit (*call_trump_func)(agent *);
typedef card (*act_func)(agent *);
typedef void (*gain_func)(agent *, FLT_TYP reward);
typedef void (*finalize_func)(agent *, const void *param);
typedef char *(*to_string_func)(const agent *, char *out_str);

typedef int class_id;
typedef int player_id;

#define agent_class_NAME_MAX_LEN 15

typedef struct agent_class
{
	class_id cls_id;
	char name[agent_class_NAME_MAX_LEN + 1];
	construct_func construct;
	destruct_func destruct;
	init_func init_episode;
	init_func init_round;
	call_trump_func call_trump;
	act_func act;
	gain_func trick_gain;
	gain_func round_gain;
	finalize_func finalize_episode;
	to_string_func to_string;
} agent_class;

#define agent_NAME_MAX_LEN 31

struct agent
{
	player_id ply_id;
	agent_class class;
	char name[agent_NAME_MAX_LEN + 1];
	state *state;

	void *public;
	void *private;
};

bool agent_is_of_class(const agent *ag, const agent_class *cls);

agent *agent_construct(agent *agent, const agent_class *ag_class,
						 player_id ply_id, const void *param);

void agent_destruct(agent *agent);

void agent_init_episode(agent *agent, state *state, const void *param);

void agent_init_round(agent *agent, const void *param);

suit agent_call_trump(agent *agent);

card agent_act(agent *agent);

void agent_trick_gain(agent *agent, float reward);

void agent_round_gain(agent *agent, float reward);

void agent_finalize(agent *agent, const void *param);

char *agent_to_str(const agent *agent, char *out_str);

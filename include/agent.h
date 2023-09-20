#include <stdint.h>

#ifndef AGENT_H_
#define AGENT_H_ 1

#include "state.h"

typedef struct agent_struct agent_t;

typedef agent_t *(*construct_func)(agent_t *agent, const void *param);
typedef void (*destruct_func)(agent_t *agent);
typedef void (*init_func)(agent_t *, const void *param);
typedef suit_t (*call_trump_func)(agent_t *);
typedef card_t (*act_func)(agent_t *);
typedef void (*gain_func)(agent_t *, float reward);

typedef struct agent_class_struct
{
	construct_func construct;
	destruct_func destruct;
	init_func init;
	call_trump_func call_trump;
	act_func act;
	gain_func trick_gain;
	gain_func round_gain;
} agent_class;

struct agent_struct
{
	agent_class class;
	int id;
	state_t *state;
	void *intern;
};

agent_t *agent_construct(agent_t *agent, int player_id, const void *param);
void agent_destruct(agent_t *agent);
void agent_init(agent_t *agent, state_t *state, const void *param);
suit_t agent_call_trump(agent_t *agent);
card_t agent_act(agent_t *agent);
void agent_trick_gain(agent_t *agent, float reward);
void agent_round_gain(agent_t *agent, float reward);

#endif /* AGENT_H_ */

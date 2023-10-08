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
typedef void (*finalize_func)(agent_t *, const void *param);
typedef char *(*to_string_func)(const agent_t *, char *out_str);

typedef int class_id;

#define agent_class_NAME_MAX_LEN 15

typedef struct agent_class_struct
{
	class_id uniq_id;
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

struct agent_struct
{
	unsigned player_id;
	unsigned unique_id;
	agent_class class;
	char name[agent_NAME_MAX_LEN + 1];
	state_t *state;
	void *intern;
};

bool agent_is_of_class(const agent_t *ag, const agent_class* cls);

agent_t *agent_construct(agent_t *agent, const agent_class* ag_class, unsigned player_id, const void *param);
void agent_destruct(agent_t *agent);
void agent_init_episode(agent_t *agent, state_t *state, const void *param);
void agent_init_round(agent_t *agent, const void *param);
suit_t agent_call_trump(agent_t *agent);
card_t agent_act(agent_t *agent);
void agent_trick_gain(agent_t *agent, float reward);
void agent_round_gain(agent_t *agent, float reward);
void agent_finalize(agent_t *agent, const void* param);
char *agent_to_str(const agent_t *agent, char *out_str);

#endif /* AGENT_H_ */

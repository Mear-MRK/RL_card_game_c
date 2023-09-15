#ifndef CARD_H_
#define CARD_H_ 1

#include <stdbool.h>

#define N_SUT 4
#define N_RNK 13
#define N_CRD 52

typedef enum suit_enum
{
    NON_SUT = -1,
    Spade,
    Heart,
    Club,
    Diamond,
} suit_t;

typedef enum rank_enum
{
    NON_RNK = -1,
    two,
    three,
    four,
    five,
    six,
    seven,
    eight,
    nine,
    ten,
    Jack,
    Queen,
    King,
    Ace
} rank_t;


extern const char RNK_CHR[];
extern const char SUT_CHR[];

typedef struct card_struct
{
    suit_t sut;
    rank_t rnk;
} card_t;

#define NON_CARD ((card_t){.sut = NON_SUT, .rnk = NON_RNK})

bool card_is_equal(const card_t *, const card_t *);
bool card_is_none(const card_t *);
bool card_is_valid(const card_t *);

char *card_to_str(const card_t *card, char *c_str);
card_t card_from_str(const char *c_str);

suit_t suit_from_str(const char *sut_str);
rank_t rank_from_str(const char *rnk_str);


#endif /* CARD_H_ */
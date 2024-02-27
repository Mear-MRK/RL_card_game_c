#ifndef CARD_H_
#define CARD_H_ 1

#include <stdbool.h>
#include <stdint.h>

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
    UNKNOWN_SUT
} suit;

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
    Ace,
    UNKNOWN_RNK
} rank;

typedef int8_t cid;

#define NON_CID -14

extern const char RNK_CHR[];
extern const char SUT_CHR[];

typedef struct card
{
    suit sut;
    rank rnk;
    cid cid;
} card;

#define NON_CARD ((const card){.sut = NON_SUT, .rnk = NON_RNK, .cid = NON_CID})

bool is_suit_valid(suit s);
bool is_rank_valid(rank r);
bool is_cid_valid(cid id);
bool card_is_valid(const card *);

bool card_is_equal(const card *, const card *);
bool card_is_none(const card *);

card *card_from_cid(card* card, cid id);
card *card_from_sut_rnk(card* card, suit s, rank r);

char *card_to_str(const card *card, char *c_str);
card card_from_str(const char *c_str);

suit suit_from_str(const char *sut_str);
rank rank_from_str(const char *rnk_str);


#endif /* CARD_H_ */
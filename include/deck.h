#ifndef DECK_H_
#define DECK_H_ 1

#include <stdint.h>

#include "card.h"

typedef struct deck_struct
{
    int nbr_cards;
    card_t* card_arr;
} deck_t;


deck_t* deck_construct(deck_t* deck, int nbr_cards);
deck_t* deck_shuffle(deck_t* deck, uint32_t (*rnd_gen)(void));
void deck_destruct(deck_t* deck);


#endif /* DECK_H_ */

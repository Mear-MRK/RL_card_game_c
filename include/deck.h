#ifndef DECK_H_
#define DECK_H_ 1

#include <stdint.h>

#include "card.h"

typedef struct deck
{
    int nbr_cards;
    card* card_arr;
} deck;


deck* deck_construct(deck* deck, int nbr_cards);
deck* deck_shuffle(deck* deck, uint32_t (*rnd_gen)(void));
void deck_destruct(deck* deck);


#endif /* DECK_H_ */

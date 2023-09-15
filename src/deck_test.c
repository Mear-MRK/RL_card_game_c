
#include "deck.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static unsigned rnd(void) {
	return rand() << 15 | rand();
}

void deck_test(void) {
	puts("+++ deck test +++");
	deck_t deck;
	deck_construct(&deck, N_CRD + 9);
	char c_s[4];
	for (int i = 0; i < deck.nbr_cards; i++) {

		printf("%s ", card_to_str(deck.card_arr + i, c_s));
	}

	puts("");
	srand(time(NULL));
	deck_shuffle(&deck, rnd);
	for (int i = 0; i < deck.nbr_cards; i++) {
		card_to_str(deck.card_arr + i, c_s);
		printf("%s ", c_s);
	}
	deck_destruct(&deck);
	puts("\n^^^ deck test ^^^");
}

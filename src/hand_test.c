#include "hand.h"

#include <stdio.h>

void hand_test(void)
{
    puts("+++ hand_test +++");
    hand_t hand;
    char str[256];
    hand_clear(&hand);
    puts(hand_to_str(&hand, str));

    hand.n_card[0][1] = 2;
    hand.n_card[1][0] = 1;
    hand.n_card[1][5] = 1;
    hand.n_card[3][3] = 1;
    hand.n_card[3][11] = 1;
    puts(hand_to_str(&hand, str));

    card_t cards[3];
    cards[0].sut = 0;
    cards[0].rnk = 1;
    cards[1].sut = 1;
    cards[1].rnk = 7;
    cards[2].sut = 3;
    cards[2].rnk = 12;
    hand_add_arr(&hand, cards, 3);
    puts(hand_to_str(&hand, str));

    card_t c;
    c.sut = 0; c.rnk = 1;
    hand_remove(&hand, &c);
    c.sut = 2; c.rnk = 11;
    hand_remove(&hand, &c);
    c.sut = 3;
    hand_remove(&hand, &c);
    puts(hand_to_str(&hand, str));

    float fl[N_CRD] = {0};

    hand_to_float(&hand, fl);
    for (int s = 0; s < N_SUT; s++)
    {
        printf("%c: ", SUT_CHR[s]);
        for (int r = 0; r < N_RNK; r++)
        {
            printf("%g ", fl[s * N_RNK + r]);
        }
        puts("");
    }

    puts("^^^ hand_test ^^^");
}
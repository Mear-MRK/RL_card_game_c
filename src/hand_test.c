#include "hand.h"

#include <stdio.h>

void hand_test(void)
{
    puts("+++ hand_test +++");
    hand_t hand;
    char str[256];
    hand_clear(&hand);
    puts(hand_to_str(&hand, str));
    puts("----");
    card_t c_ar[] = {{0, 1, 1}, {1, 0, 13}, {1, 5, 18}, {3, 3, 42}, {3, 11, 50}};
    hand_add_card_arr(&hand, c_ar, 5);
    puts(hand_to_str(&hand, str));
    puts("----");
    hand_clear(&hand);
    cid_t cids[] = {0, 3, 7, 51, 23};
    hand_add_cid_arr(&hand, cids, 5);
    puts(hand_to_str(&hand, str));
    puts("----");
    card_t c;
    card_from_sut_rnk(&c, 0, 3);
    hand_remove(&hand, &c);
    card_from_sut_rnk(&c, 2, 11);
    hand_remove(&hand, &c);
    card_from_sut_rnk(&c, 0, 3);
    hand_remove(&hand, &c);
    puts(hand_to_str(&hand, str));
    puts("----");
    puts(hand_to_str_v2(&hand, str));
    puts("----");
    card_t cr_ar[52];
    int n = hand_to_card_arr(&hand, NULL, cr_ar);
    for(int i = 0; i < n; i++)
        printf("%s ", card_to_str(cr_ar + i, str));
    puts("\n----");
    float fl[N_CRD] = {0};
    suit_t sut_ord[N_SUT] = {1, 3, 0, 2};
    bool sut_select[N_SUT] = {false, true, false, true};
    hand_to_float(&hand, sut_select, sut_ord, fl);
    for (int s = 0; s < N_SUT; s++)
    {
        suit_t ss = sut_ord[s];
        if(sut_select[ss])
            printf("%c: ", SUT_CHR[ss]);
        else
            printf("$: ");
        for (int r = 0; r < N_RNK; r++)
        {
            printf("%g ", fl[s * N_RNK + r]);
        }
        puts("");
    }

    puts("^^^ hand_test ^^^");
}
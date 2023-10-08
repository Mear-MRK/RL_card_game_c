#!/bin/bash

bin/main.out 10000 2 1 rnrr
bin/main.out 10000 2 1 rrnr
bin/main.out 10000 2 1 rrrn

for i in {1..2}
do
	bash ./run_all_perm_RL_ag.sh 10000
	bin/main.out 10000 2 1 nrrr
	bin/main.out 10000 2 1 rnrr
	bin/main.out 10000 2 1 rrnr
	bin/main.out 10000 2 1 rrrn
done


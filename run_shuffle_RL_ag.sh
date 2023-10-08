#!/bin/bash

for i in {1..100}; do
	bash shuffle_RL_ag.sh
	bin/main.out 10000 2 1111 nnnn 10
       	echo "--- rep $i/100 ---"
	echo
done

for i in {1..25}; do
	bash shuffle_RL_ag.sh
	bin/main.out 10000
	echo "--- rep $i/25 ---"
	echo
done


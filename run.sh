#!/bin/bash

for i in {1..10}
do
	bin/main.out 100000 2 1 snsn
	bin/main.out 100000 2 1 nnnn
done


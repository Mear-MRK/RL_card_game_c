#!/bin/bash

mv RL_ag0.dat t0
mv RL_ag2.dat t2
mv RL_ag1.dat RL_ag0.dat
mv RL_ag3.dat RL_ag2.dat
mv t0 RL_ag1.dat
mv t2 RL_ag3.dat


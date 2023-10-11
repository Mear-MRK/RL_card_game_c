#!/bin/bash

for m in {0..3}
do
    mv "RL_ag$m.dat" "tmp_$m"
done

my_array=("0" "1" "2" "3")

for i in {0..3}; do
    tmp=${my_array[0]}
    my_array[0]=${my_array[$i]}
    my_array[$i]=${tmp}
    for j in {1..3}; do
        tmp=${my_array[1]}
        my_array[1]=${my_array[$j]}
        my_array[$j]=${tmp}
        for k in {2..3}; do
            tmp=${my_array[2]}
            my_array[2]=${my_array[$k]}
            my_array[$k]=${tmp}
            echo "${my_array[@]}"
                for m in {0..3}
                do
                    n=${my_array[m]}
                    mv "tmp_$m" "RL_ag$n.dat"
                done
                bin/main.out "$@"
                for m in {0..3}
                do
                    n=${my_array[m]}
                    mv "RL_ag$n.dat" "tmp_$m"
                done
            tmp=${my_array[2]}
            my_array[2]=${my_array[$k]}
            my_array[$k]=${tmp}
        done
        tmp=${my_array[1]}
        my_array[1]=${my_array[$j]}
        my_array[$j]=${tmp}
    done
    tmp=${my_array[0]}
    my_array[0]=${my_array[$i]}
    my_array[$i]=${tmp}
done

for m in {0..3}
do
    mv "tmp_$m" "RL_ag$m.dat"
done
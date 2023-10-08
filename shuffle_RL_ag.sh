#!/bin/bash

shuffle_array() {
  local array=("$@")
  local array_size=${#array[@]}

  for ((i = array_size - 1; i > 0; i--)); do
    # Generate a random index between 0 and i (inclusive)
    rand_index=$((RANDOM % (i + 1)))

    # Swap array elements
    temp=${array[i]}
    array[i]=${array[rand_index]}
    array[rand_index]=$temp
  done

  # Print the shuffled array
  echo "${array[@]}"
}

my_array=("0" "1" "2" "3")
shuffled_array=($(shuffle_array "${my_array[@]}"))

# Print the shuffled array
echo "Shuffled Array: ${shuffled_array[@]}"

for i in {0..3}
do
  mv "RL_ag$i.dat" "tmp_$i"
done

for i in {0..3}
do
  j=${shuffled_array[i]}
  mv "tmp_$i" "RL_ag$j.dat"
done

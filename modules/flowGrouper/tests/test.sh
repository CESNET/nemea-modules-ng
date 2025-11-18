#!/bin/bash

function exit_with_error {
  pkill logger
  pkill logreplay
  pkill flowGrouper
  exit 1
}

function process_started {
  pid=$1
  if ! ps -p $pid > /dev/null
    then
      echo "Failed to start process"
      exit_with_error
  fi
}


function compare_files {
file1=$1
file2=$2

# Skip header and compare
tail -n +2 "$file1" > /tmp/f1_data
tail -n +2 "$file2" > /tmp/f2_data

# Create arrays to store flow keys
declare -A f1_keys
declare -A f2_keys

# Read file1 and store FLOW_GROUP_KEY (column 3) for each row
line_num=0
while IFS=',' read -r dst_ip src_ip flow_key rest; do
    ((line_num++))
    f1_keys[$line_num]=$flow_key
done < /tmp/f1_data

# Read file2 and store FLOW_GROUP_KEY (column 3) for each row
line_num=0
while IFS=',' read -r dst_ip src_ip flow_key rest; do
    ((line_num++))
    f2_keys[$line_num]=$flow_key
done < /tmp/f2_data

# Compare: if two rows have same flow_key in file1, they must have same flow_key in file2
errors=0
total_lines=${#f1_keys[@]}

for ((i=1; i<=total_lines; i++)); do
    for ((j=i+1; j<=total_lines; j++)); do
        # Check if same in file1
        if [ "${f1_keys[$i]}" == "${f1_keys[$j]}" ]; then
            # Must be same in file2
            if [ "${f2_keys[$i]}" != "${f2_keys[$j]}" ]; then
                echo "ERROR: Lines $i and $j have same FLOW_KEY in file1 (${f1_keys[$i]}) but different in file2 (${f2_keys[$i]} vs ${f2_keys[$j]})"
                ((errors++))
            fi
        else
            # Must be different in file2
            if [ "${f2_keys[$i]}" == "${f2_keys[$j]}" ]; then
                echo "ERROR: Lines $i and $j have different FLOW_KEY in file1 (${f1_keys[$i]} vs ${f1_keys[$j]}) but same in file2 (${f2_keys[$i]})"
                ((errors++))
            fi
        fi
    done
done
rm /tmp/f1_data /tmp/f2_data

if [ $errors -eq 0 ]; then
    return 0
else
    echo "FAILED: Found $errors inconsistencies"
    return 1
fi
}


data_path="$(dirname "$0")/testsData/"
flowGrouper=$1

set -e
trap 'echo "Command \"$BASH_COMMAND\" failed!"; exit_with_error' ERR
for input_file in $data_path/inputs/*; do
  index=$(echo "$input_file" | grep -o '[0-9]\+')
  echo "Running test $index"

  res_file="/tmp/res"
  logger -i "u:flowGrouper" -t -w $res_file &
  logger_pid=$!
  sleep 0.1

  process_started $logger_pid

  $flowGrouper \
    -i "u:din,u:flowGrouper" &

  detector_pid=$!
  sleep 0.1
  process_started $detector_pid

  logreplay -i "u:din" -f "$data_path/inputs/input$index.csv" 2>/dev/null &
  sleep 0.1
  process_started $!

  wait $logger_pid
  wait $detector_pid

  if [ -f "$res_file" ]; then
    if compare_files "$data_path/results/res$index.csv" "$res_file" ; then
      echo "Test $index passed"
    else
      echo "FLOW_GROUP_KEY equivalence patterns are NOT consistent"
      exit_with_error
    fi
  else
    echo "File $res_file not found"
    exit_with_error
  fi
done

echo "All tests passed"
exit 0

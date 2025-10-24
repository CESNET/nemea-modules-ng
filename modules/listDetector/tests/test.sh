#!/bin/bash

function exit_with_error {
  pkill logger
  pkill logreplay
  pkill listDetector
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

data_path="$(dirname "$0")/testsData/"
list_detector=$1

set -e
trap 'echo "Command \"$BASH_COMMAND\" failed!"; exit_with_error' ERR
for input_file in $data_path/inputs/*; do
  index=$(echo "$input_file" | grep -o '[0-9]\+')
  echo "Running test $index"

  res_file="/tmp/res"
  logger -i "u:listDetector" -w $res_file &
  logger_pid=$!
  sleep 0.1

  process_started $logger_pid

  $list_detector \
    -i "u:lr,u:listDetector" \
    -r "$data_path/rules/rule$index.csv" \
    -lm blacklist &

  detector_pid=$!
  sleep 0.1
  process_started $detector_pid

  logreplay -i "u:lr" -f "$data_path/inputs/input$index.csv" 2>/dev/null &
  sleep 0.1
  process_started $!

  wait $logger_pid
  wait $detector_pid

  if [ -f "$res_file" ]; then
    if ! cmp -s "$data_path/results/res$index.csv" "$res_file"; then
      echo "Files results/res$index.csv and $res_file are not equal"
      exit_with_error
    fi
  else
    echo "File $res_file not found"
    exit_with_error
  fi
done

echo "Running test 24 + 25"

res_file="/tmp/res"
logger  -i "u:listDetector" -w $res_file &
logger_pid=$!
sleep 0.1

process_started $logger_pid

touch /tmp/rules
cat "$data_path/rules/rule24.csv" > /tmp/rules

$list_detector \
  -i "u:lr,u:listDetector" \
  -r "/tmp/rules" \
  -lm blacklist \
  -ci 100 &

detector_pid=$!
sleep 0.1
process_started $detector_pid

logreplay -i "u:lr" -f "$data_path/inputs/input24.csv" 2>/dev/null -n
sleep 1

cat "$data_path/rules/rule25.csv" > /tmp/rules

logreplay -i "u:lr" -f "$data_path/inputs/input25.csv" 2>/dev/null

wait $detector_pid
wait $logger_pid

cat "$data_path/results/res24.csv" "$data_path/results/res25.csv" > "/tmp/res_expected24_25.csv"

if [ -f "$res_file" ]; then
  if ! cmp -s "/tmp/res_expected24_25.csv" "$res_file"; then
    echo "Files /tmp/res_expected24_25.csv and $res_file are not equal"
    exit_with_error
  fi
else
  echo "File $res_file not found"
  exit_with_error
fi

echo "All tests passed"
exit 0

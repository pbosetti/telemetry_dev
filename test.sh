#! /bin/zsh

for ((i=1; i<=10; i++)); do
  echo "{\"i\": $i, \"test\": \"ciao mondo\"}"
  sleep 0.1
done

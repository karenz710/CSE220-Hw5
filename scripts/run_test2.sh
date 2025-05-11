#!/usr/bin/env bash

timeout 30s ./build/server.poker_server 152 &
SERVER_PID=$!

sleep 1

./build/client.automated 0 < scripts/inputs/check2hands.txt &
./build/client.automated 1 < scripts/inputs/check2hands.txt &
./build/client.automated 2 < scripts/inputs/check2hands.txt &
./build/client.automated 3 < scripts/inputs/check2hands.txt &
./build/client.automated 4 < scripts/inputs/check2hands.txt &
./build/client.automated 5 < scripts/inputs/check2hands.txt &

wait

echo "All clients have finished."

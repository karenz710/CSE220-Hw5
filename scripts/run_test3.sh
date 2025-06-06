#!/usr/bin/env bash

timeout 30s ./build/server.poker_server 741 &
SERVER_PID=$!

sleep 1

./build/client.automated 0 < scripts/inputs/leave.txt &
./build/client.automated 1 < scripts/inputs/test3_p1.txt &
./build/client.automated 2 < scripts/inputs/leave.txt &
./build/client.automated 3 < scripts/inputs/leave.txt &
./build/client.automated 4 < scripts/inputs/test3_p4.txt &
./build/client.automated 5 < scripts/inputs/leave.txt &

wait

echo "All clients have finished."

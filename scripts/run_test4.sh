#!/usr/bin/env bash

timeout 30s ./build/server.poker_server 212389 &
SERVER_PID=$!

sleep 1

./build/client.automated 0 < scripts/inputs/test4_p0.txt &
./build/client.automated 1 < scripts/inputs/test4_p1.txt &
./build/client.automated 2 < scripts/inputs/test4_p2.txt &
./build/client.automated 3 < scripts/inputs/test4_p3.txt &
./build/client.automated 4 < scripts/inputs/test4_p4.txt &
./build/client.automated 5 < scripts/inputs/test4_p5.txt &

wait

echo "All clients have finished."

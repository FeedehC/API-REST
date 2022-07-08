#!/bin/bash -e

for i in {1..100}
do
 curl -X POST http://localhost:8537/increment &
done
wait
curl http://localhost:8537/print &
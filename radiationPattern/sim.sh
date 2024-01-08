#!/bin/bash

logfile="log.txt"
numthreads=6
exec_pooled()
{
    local time1=$(date +"%H:%M:%S")
    local time2=
    "$@" && time2=$(date +"%H:%M:%S") && echo "Finished '$@' ($time1 -- $time2)" >> $logfile &

    local pid=$$
    local children=$(ps -eo ppid | grep -w $pid | wc -w)
    children=$((children - 1))
    if [[ $children -ge $numthreads ]]; then
        wait -n
    fi
}

for h in 2 4 6 8 10 12 14; do
    for r in 0.25 0.5 0.75 1 2 3; do
        for c in "g" "c"; do
            echo "exec_pooled ./a.out $h 1e-6 0.15 $c 0.15 $r >> "results/${c}_h${h}cm_r${r}cm.txt" &"
        done
    done
done
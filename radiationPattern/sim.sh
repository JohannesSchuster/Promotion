#!/bin/bash

for h in 2 4 6 8 10 12 14; do
    for r in 0.25 0.5 0.75 1 2 3; do
        ./a.out $h 1e-6 0.15 c 0.15 $r >> "results/log_circle_h${h}cm_r${r}cm.txt" &
        ./a.out $h 1e-6 0.15 g 0.15 $r >> "results/log_gauss_h${h}cm_r${r}cm.txt" &
    done
done
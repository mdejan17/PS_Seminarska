#!/bin/bash

srun --cpus-per-task=1 t1
srun --cpus-per-task=4 -N1 t4
srun --cpus-per-task=16 -N1 t16
srun --cpus-per-task=32 -N1 t32

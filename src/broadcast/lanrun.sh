#!/bin/bash 

make
cp broadcast ~/cloud/
# mpirun -np 2 --hostfile ~/cloud/hosts ~/cloud/tob
mpirun -np 3 ~/cloud/broadcast broadcast
#!/bin/bash 

make
cp broadcast ~/cloud/
mpirun -np 2 --hostfile ~/cloud/hosts ddd ~/cloud/broadcast broadcast
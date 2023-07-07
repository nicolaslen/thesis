#!/bin/bash

make

tab=" --tab"
options=()

cmds[1]="./coregsp 0"
titles[1]="COREGSP - Cliente 0"
cmds[2]="./coregsp 1"
titles[2]="COREGSP - Cliente 1"
cmds[3]="./coregsp 2"
titles[3]="COREGSP - Cliente 2"

for i in 1 2 3; do
options+=($tab --title="${titles[i]}" -e "bash -c \"${cmds[i]} ; bash\"" )
done

cd ..
cd broadcast
gnome-terminal --tab --title="Broadcast" -e "bash -c \"mpirun -np 3 broadcast tob; bash\""
cd ..
cd coregsp
sleep 3
gnome-terminal "${options[@]}"

exit 0
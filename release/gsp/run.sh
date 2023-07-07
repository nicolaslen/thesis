#!/bin/bash

make

tab=" --tab"
options=()

cmds[1]="./gsp server"
titles[1]="GSP - Server"
cmds[2]="./gsp client 1 $1 $2"
titles[2]="GSP - Cliente 1"
cmds[3]="./gsp client 2 $1 $2"
titles[3]="GSP - Cliente 2"

for i in 1 2 3; do
options+=($tab --title="${titles[i]}" -e "bash -c \"${cmds[i]} ; bash\"" )
done

cd ..
cd broadcast
gnome-terminal --tab --title="Broadcast" -e "bash -c \"mpirun -np 3 broadcast tob; bash\""
cd ..
cd gsp
sleep 3
gnome-terminal "${options[@]}"

exit 0
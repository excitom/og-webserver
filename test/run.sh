#!/bin/bash
iterations=$1
processes=$2
echo "Iterations $iterations Processes $processes"
while [ $processes -gt 0 ]
do
	echo $processes
	./pound.pl $iterations 2>/dev/null && echo "DONE $processes" &
	processes=$(( $processes - 1 ))
done

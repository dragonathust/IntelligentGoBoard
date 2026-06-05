#!/bin/bash
x=0
while true
do let x=x+1
sudo ./igo_gomoku -b pbrain-pela -p /dev/ttyACM0
echo "Play turn $x end."
if [ $x -eq 3 ]; then
exit
fi
sleep 5
done

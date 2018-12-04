#!/bin/bash
cd "${0%/*}"

#1st arg = SNK veth last 2 characters
#2nd arg = iteration count
#3rd arg = packet rate
#4th arg = packet count
#5th arg = chain length
#6th arg = log filename
#7th arg = flows per second
#8th arg = reset udpEncap.src to ::1 (True/False)
#9th arg = cases to run=1 runs only 70B frame case, 2 runsh 70B and 512B, 3 runs 70B, 512B and 1500B cases (all)

clickFile=$([ $7 == 0 ] && echo "overtheline.click" || echo "overthelinefps.click -j 2 a=18")
clickFile=$([[ $7 -gt 0 && $8 -eq 0 ]] && echo "overthelinefpsnoreset.click -j 2 a=18" || echo $clickFile )
logFile=$([ $7 == 0 ] && echo $6 || echo "fps$7$6")

#Packet length 70
if [ $9 -gt 0 ]
then
for ((i = 1; i <= $2; i++)); do
   click $clickFile snkintf=$1 packetlen=8 packetrate=$3 packetcount=$4 chlen=$5 filename=$logFile flowspersecond=$7
   sleep 5
done
fi

#Packet length 512
if [ $9 -gt 1 ]
then
for ((i = 1; i <= $2; i++)); do
   click $clickFile snkintf=$1 packetlen=450 packetrate=$3 packetcount=$4 chlen=$5 filename=$logFile flowspersecond=$7
   sleep 5
done
fi

#Packet length 1500
if [ $9 -gt 2 ]
then
for ((i = 1; i <= $2; i++)); do
   click $clickFile snkintf=$1 packetlen=1438 packetrate=$3 packetcount=$4 chlen=$5 filename=$logFile flowspersecond=$7
   sleep 5
done
fi


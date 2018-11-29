#!/bin/bash
cd "${0%/*}"

#1st arg = SNK veth last 2 characters
#2nd arg = iteration count
#3rd arg = packet rate
#4th arg = packet count
#5th arg = chain length
#6th arg = log filename
#7th arg = flows per second

clickFile=$([ $7 == 0 ] && echo "overtheline.click" || echo "overthelinefps.click -j 2 a=18")

#Packet length 70
for ((i = 1; i <= $2; i++)); do
   click $clickFile snkintf=$1 packetlen=8 packetrate=$3 packetcount=$4 chlen=$5 filename=$6 flowspersecond=$7
   sleep 5
done

#Packet length 512
for ((i = 1; i <= $2; i++)); do
   click $clickFile snkintf=$1 packetlen=450 packetrate=$3 packetcount=$4 chlen=$5 filename=$6 flowspersecond=$7
   sleep 5
done

#Packet length 1500
for ((i = 1; i <= $2; i++)); do
   click $clickFile snkintf=$1 packetlen=1438 packetrate=$3 packetcount=$4 chlen=$5 filename=$6 flowspersecond=$7
   sleep 5
done


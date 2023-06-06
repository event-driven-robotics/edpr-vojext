#!/bin/bash
echo "Killing all VOJEXT tasks"
echo "======================="
killall edpr-vojext-demo
sleep 3
echo "Killed EDPR VOJEXT app"
killall atis-bridge-sdk
sleep 3
echo "ATIS-bridge killed"
killall yarpserver
echo "YARP server killed"

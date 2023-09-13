#!/bin/bash

echo "Run YARP server connected to ROS"
yarp namespace /vojext
yarp conf ${YARPSERVER_IP} 10000
yarpserver --ros &
sleep 1
echo "Run ATIS-bridge"
atis-bridge-sdk --s 60 --filter 0.01 --limit 2 &
sleep 5
echo "Run EDPR-VOJEXT application"
edpr-vojext-demo --vis false --f_vis 1

#!/bin/bash

echo "Run YARP server connected to ROS"
yarp conf ${ROS_MASTER_URI:7:-6} 10000
yarpserver --ros &
sleep 1
echo "Run ATIS-bridge"
atis-bridge-sdk --s 60 --filter 0.01 --limit 2 &
sleep 5
echo "Run EDPR-VOJEXT application"
edpr-vojext-demo --vis false &

#!/bin/bash

echo "Run YARP server connected to ROS"
yarp namespace /vojext
yarp conf ${YARPSERVER_IP} 10000
yarp detect
echo "Run EDPR-VOJEXT application"
edpr-vojext-demo --vis ${B_VIS} --f_det ${P_DETF} --f_vis ${P_VISF} --f_vel ${P_VELF} --f_img ${P_IMGF} --confidence ${P_CONF} 
#sleep 1
#python3 /usr/local/src/edpr-vojext/movenet_vojext.py

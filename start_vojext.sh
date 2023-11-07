#!/bin/bash

echo "Run YARP server connected to ROS"
yarp namespace /vojext
yarp conf ${YARPSERVER_IP} 10000
yarpserver --ros &
sleep 1
echo "Run ATIS-bridge"
atis-bridge-sdk --gen3 --s ${P_CAMS} --filter ${P_CAMF} --limit ${P_CAML} &
sleep 5
echo "Run EDPR-VOJEXT application"
edpr-vojext-demo --vis false --f_det ${P_DETF} --f_vis ${P_VISF} --f_vel ${P_VELF} --f_img ${P_IMGF} --confidence ${P_CONF}

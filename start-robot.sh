echo "Run YARP server connected to ROS"
yarp namespace /vojext
yarp conf ${YARPSERVER_IP} 10000
yarpserver --ros &
sleep 1
echo "Run ATIS-bridge"
atis-bridge-sdk --gen3 --s ${P_CAMS} --filter ${P_CAMF} --limit ${P_CAML} &

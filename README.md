# EDPR-VOJEXT

Event-driven Human Pose Estimation for Mobile Robots in Industry 

## Installation
The software was tested on Ubuntu 20.04.2 LTS

- Install [Docker Engine](https://docs.docker.com/engine/install/ubuntu)
- Download the repository and build the Docker image
    ```shell
    $ cd <workspace>
    $ git clone git@github.com:event-driven-robotics/edpr-vojext.git
    $ cd edpr-vojext
    $ docker build -t edpr-vojext - < Dockerfile
    ```
:bulb: `<workspace>` is the parent directory in which the repository is cloned

## Usage
- Run the Docker container and, inside it, run the pose detector
    ```shell
    $ xhost +
    ```
    ```shell
    $ docker run -it --privileged --network host -v /tmp/.X11-unix/:/tmp/.X11-unix -v /dev/bus/usb:/dev/bus/usb \
               -e DISPLAY=unix$DISPLAY -e ROS_MASTER_URI=http://172.17.0.1:11311 -e YARP_IP=172.17.0.1 \
               --name edpr-vojext-demo edpr-vojext
    ```
:bulb: set the ROS_MASTER_URI variable and the YARP_IP as needed for your set-up

- At the terminal inside the container run the following command to execute a script that will set up a yarpserver connected to ROS, run the atis-bridge to get the events from the camera and finally run the HPE application with a visualisation window. The application should automatically connect to required input and output YARP modules.
  ```shell 
  $ edpr-vojext/start_vojext.sh
  ```

- The application should run using the moveEnet event-driven HPE algorithm

- While the visualisation window is selected:
  - Press `ESC` to close
  - Press `e` to change visualisation

- The following command line options can be used if running the commands individually `edpr-vojext-demo --OPTION VALUE`
  - `--f_det <INT>` HPE detection rate [5]
  - `--f_vel <INT>` HPE velocity estimation rate [50]
  - `--f_vis <INT>` visualisation rate [20]
  - `--f_img <INT>` ROS image output rate [3]
  - `--w <INT> --h <INT>` to set the dataset sensor resolution [640 480]
  - `--pu <FLOAT> --muD <FLOAT>` to set the Kalman filter process and measurement uncertainty [10, 1]
  - `--vis <BOOL>` to see a visualisation of the algorithm

To stop the application itself, the camera and the yarpserver, the follwoing command should be run in a temrinal inside the docker:
```shell 
$ edpr-vojext/stop_vojext.sh
```

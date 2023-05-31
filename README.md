# EDPR-VOJEXT

Event-driven Human Pose Estimation for Mobile Robots in Industry 

## Installation
The software was tested on Ubuntu 20.04.2 LTS with an Nvidia GPU.

- Install the latest [Nvidia driver](https://github.com/NVIDIA/nvidia-docker/wiki/Frequently-Asked-Questions#how-do-i-install-the-nvidia-driver)
- Install [Docker Engine](https://docs.docker.com/engine/install/ubuntu)
- Install [Nvidia Docker Toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/install-guide.html#docker)
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
    $ docker run -it --privileged --network host -v /tmp/.X11-unix/:/tmp/.X11-unix -v /dev/bus/usb:/dev/bus/usb -e DISPLAY=unix$DISPLAY --runtime=nvidia --name edpr-vojext edpr-vojext-demo
    ```

- At the terminal inside the container run the following command to execute a script that will set up a yarpserver connected to ROS, run the atis-bridge to get the events from the camera and finally run the HPE application with a visualisation window. The application should automatically connect to required input and output YARP modules. This script also sets the value to the env variable `ROS_MASTER_URI`. It is either set to the default valaue (i.e. `http://127.0.0.1:11311`) using the `-d` flag, or manually set to a differnet value using the flag `-m`.
  ```shell 
  $ ./run_vojext [-d (default)] or [-m value (manual)]
  ```


- The application should run using the moveEnet event-driven HPE algorithm

- While the visualisation window is selected:
  - Press `e` to visualise alternate representations (toggle between eros and events)
  - Press `ESC` to close

- The following command line options can be used `edpr-april --OPTION VALUE`
  - `--detF <INT>` to modify the moveEnet detection rate [10]
  - `--w <INT> --h <INT>` to set the dataset sensor resolution [640 480]
  - `--pu <FLOAT> --muD <FLOAT>` to set the Kalman filter process and measurement uncertainty [0.001, 0.0004]

To stop the application itself, the camera and the yarpserver, the follwoing command should be run in a temrinal inside the docker:
```shell 
$ ./stop_vojext
```

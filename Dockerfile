# base image
FROM ubuntu:latest

MAINTAINER gaurvi goyal <gaurvi.goyal.iit.it>

RUN apt-get update


##########
# GL-HPE #
##########

ENV TZ=Europe/Rome
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone



# GL-HPE requirements 

# # Directories

RUN mkdir /code

RUN mkdir /data

RUN mkdir /data/DHP19

RUN mkdir /data/checkpoint

#VOLUME code/ volume thingy is not a good idea unless I have a better understanding of the command

WORKDIR code/

# # Packages

RUN apt-get install -y --no-install-recommends python3 python3-pip git

RUN apt-get update && apt-get install -y python3-opencv
RUN pip install opencv-python

RUN pip install albumentations \
h5py \
scikit-image \
scikit-learn \
scikit-video \
scipy \
torch>1 \
kornia \
hydra-core \
omegaconf \
opencv-python \
pytorch-lightning==1.1.6 \
torchvision \
tqdm \
numpy \
matplotlib \
segmentation_models_pytorch \
sklearn \
-e git+https://github.com/anibali/pose3d-utils#egg=pose3d_utils \
-e git+https://github.com/IIT-PAVIS/event_library#egg=event_library \
comet_ml 

# NVIDIA INSTALLATIONS- TO TEST

# RUN sudo apt install nvidia-cuda-toolkit


############
#   YARP   #
############

RUN echo "*************** building yarp ****************"

ARG EVENT_DRIVEN_VERSION=1.5
ARG YARP_VERSION=3.4.4
ARG YCM_VERSION=0.13.0
ARG BUILD_TYPE=Release
ARG SOURCE_FOLDER=/usr/local
ARG OPENGL=0

ENV DEBIAN_FRONTEND noninteractive 

RUN apt update

RUN apt install -y \
    apt-transport-https \
    ca-certificates \
    gnupg \
    software-properties-common \
    lsb-core
    
# Install useful packages
RUN apt install -y \
        build-essential \
        libssl-dev
        
# Install yarp dependencies
RUN apt install -y \
        libgsl-dev \
        libedit-dev \
        libace-dev \
        libeigen3-dev \
# Install QT5 for GUIS 
# (NOTE: may be not compatible with nvidia drivers when forwarding screen)
        qtbase5-dev \
        qt5-default \
        qtdeclarative5-dev \
        qtmultimedia5-dev \
        qml-module-qtquick2 \
        qml-module-qtquick-window2 \
        qml-module-qtmultimedia \
        qml-module-qtquick-dialogs \
        qml-module-qtquick-controls

RUN sh -c 'echo "deb http://www.icub.org/ubuntu `lsb_release -cs` contrib/science" > /etc/apt/sources.list.d/icub.list'
RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 57A5ACB6110576A6
RUN apt update
RUN apt install -y icub-common

RUN cd $SOURCE_FOLDER
RUN    git clone https://github.com/robotology/ycm.git && \
    cd ycm && \
    git checkout v$YCM_VERSION && \
    mkdir build && cd build && \
    cmake .. && \
    make -j `nproc` install

RUN cd $SOURCE_FOLDER && \
    git clone https://github.com/robotology/ycm.git && \
    cd ycm && \
    git checkout v$YCM_VERSION && \
    mkdir build && cd build && \
    cmake .. && \
    make -j `nproc` install


# Install YARP
RUN cd $SOURCE_FOLDER && \
    git clone https://github.com/robotology/yarp.git &&\
    cd yarp && \
    git checkout v$YARP_VERSION && \
    mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
          -DYARP_COMPILE_BINDINGS=OFF \
          -DCREATE_PYTHON=OFF \
          .. && \
    make -j `nproc` install

RUN yarp check
EXPOSE 10000/tcp 10000/udp

# Some QT-Apps don't show controls without this
ENV QT_X11_NO_MITSHM 1


# Install event-driven
RUN cd $SOURCE_FOLDER && \
    git clone https://github.com/robotology/event-driven.git && \
    cd event-driven && \
    git checkout v$EVENT_DRIVEN_VERSION && \
    mkdir build && cd build && \
    cmake -DVLIB_CLOCK_PERIOD_NS=1000 \
          -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
          .. && \
    make install -j$(nproc)


# add /usr/local/lib to the library path, so that libcaffe.so compiled with openpose will be used
# instead of the one provided by the nvidia/cuda docker image
ENV LD_LIBRARY_PATH /usr/local/lib:$LD_LIBRARY_PATH




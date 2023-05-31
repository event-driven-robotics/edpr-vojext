
# base image
FROM nvidia/cuda:11.5.2-cudnn8-devel-ubuntu20.04

ENV DEBIAN_FRONTEND noninteractive

# install basic indpendence
RUN apt update
RUN apt install -y build-essential libssl-dev software-properties-common 
RUN apt install -y cmake cmake-curses-gui vim nano git sudo openssh-client git
RUN apt install -y libboost-all-dev libmysqlclient-dev ffmpeg libsm6 libxext6 libcanberra-gtk-module 

##########
# PYTHON & PIP #
##########

# update python
ARG PYTHON_VERSION=3.8
RUN apt install -y python$PYTHON_VERSION python3-pip python3-dev

# create list of alternative python interpreters
RUN update-alternatives --install /usr/bin/python3 python3 /usr/bin/python$PYTHON_VERSION 1 && \
    update-alternatives --config python3 && \
    rm /usr/bin/python3 && \
    ln -s python$PYTHON_VERSION /usr/bin/python3
    
RUN pip3 install numpy

##########
# OPENCV C++ and Python
##########
RUN apt install -y libopencv-dev python3-opencv


###############
# NEUROMORHPIC CAMERA DRIVER #
###############
RUN add-apt-repository ppa:deadsnakes/ppa
RUN echo "deb [arch=amd64 trusted=yes] https://apt.prophesee.ai/dists/public/7l58osgr/ubuntu focal essentials" >> /etc/apt/sources.list;
RUN apt update
RUN apt install -y python3.7
RUN apt install -y metavision-*

############
#   INSTALLED FROM SOURCE   #
############

ARG SOURCE_FOLDER=/usr/local/src
ARG BUILD_TYPE=Release

ARG YCM_VERSION=v0.15.2
ARG YARP_VERSION=v3.8.0
ARG EVENT_DRIVEN_VERSION=master
ARG HPE_VERSION=main

RUN apt update

RUN apt install -y \
    apt-transport-https \
    ca-certificates \
    gnupg \
    lsb-core \
    swig

# Install yarp dependencies
RUN apt install -y \
        libgsl-dev \
        libedit-dev \
        libace-dev \
        libeigen3-dev \
        qtbase5-dev \
        qt5-default \
        qtdeclarative5-dev \
        qtmultimedia5-dev \
        qml-module-qtquick2 \
        qml-module-qtquick-window2 \
        qml-module-qtmultimedia \
        qml-module-qtquick-dialogs \
        qml-module-qtquick-controls

# git clone --depth 1 --branch <branch> url
# Install YCM
RUN cd $SOURCE_FOLDER && \
    git clone --depth 1 --branch $YCM_VERSION https://github.com/robotology/ycm.git &&\
    cd ycm && mkdir build && cd build && \
    cmake .. && make install -j$(nproc)

# Install YARP
RUN cd $SOURCE_FOLDER && \
    git clone --depth 1  --branch $YARP_VERSION https://github.com/robotology/yarp.git &&\
    cd yarp && mkdir build && cd build && \ 
    cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
          -DYARP_COMPILE_BINDINGS=ON \
          -DCREATE_PYTHON=ON \
          .. && \
    make install -j$(nproc)

RUN yarp check
EXPOSE 10000/tcp 10000/udp

# make yarp's python binding visible to python interpreter
ENV PYTHONPATH $SOURCE_FOLDER/yarp/build/lib/python3:$PYTHONPATH

# Some QT-Apps don't show controls without this
ENV QT_X11_NO_MITSHM 1


# install EVENT-DRIVEN
RUN cd $SOURCE_FOLDER && \
    git clone --depth 1 --branch $EVENT_DRIVEN_VERSION https://github.com/robotology/event-driven.git && \
    cd event-driven && mkdir build && cd build && \
    cmake -DVLIB_ENABLE_TS=OFF .. && make install -j$(nproc)
    
# install hpe-core
RUN cd $SOURCE_FOLDER && \
    git clone --depth 1 --branch $HPE_VERSION https://github.com/event-driven-robotics/hpe-core.git &&\
    cd hpe-core/core && mkdir build && cd build && \
    cmake .. && make install -j$(nproc)

# install movenet dependencies
RUN python3 -m pip install -r $SOURCE_FOLDER/hpe-core/example/movenet/requirements.txt

ENV PYTHONPATH "${PYTHONPATH}:$SOURCE_FOLDER/hpe-core"
    
# VOJEXT demo

RUN cd $SOURCE_FOLDER && \
    git clone --branch main https://github.com/event-driven-robotics/edpr-vojext.git 
    #&& \
    #cd edpr-vojext && mkdir build && cd build && \
    #cmake .. && make install -j$(nproc)
    
RUN apt autoremove && apt clean
RUN rm -rf /tmp/* /var/lib/apt/lists/* /var/tmp/*

WORKDIR $SOURCE_FOLDER

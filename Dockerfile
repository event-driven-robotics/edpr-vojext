# base image
FROM ubuntu:latest

MAINTAINER gaurvi goyal <gaurvi.goyal.iit.it>

RUN apt-get update


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



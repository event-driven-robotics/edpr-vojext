#!/usr/bin/env python3
import rospy
import numpy as np
import cv2
import time

from sensor_msgs.msg import Image
from sim_sem_msgs.msg import NChumanPose
from cv_bridge import CvBridge

def callback(data, img):
    n_joints = int(len(data.pose) / 2)
    # print(data.pose)
    poses = np.array(data.pose).reshape((n_joints, 2)).astype(int)
    img[:, :] = 0
    for p in poses:
        if p[0]>0 and p[1] > 0:
            cv2.circle(img, (p[0], p[1]), 5, (255, 0, 0), -1)
        else:
            cv2.circle(img, (10, 10), 5, (0, 0, 255), -1)

def evs_callback(data, img):
    bridge = CvBridge()
    cv_image = bridge.imgmsg_to_cv2(data, desired_encoding='8UC1')
    rgb_img = cv2.cvtColor(cv_image, cv2.COLOR_GRAY2BGR)
    img[:] = rgb_img[:]
    
def main():

    # TODO change
    skel_topic = "/pem/neuromorphic_camera/data"
    evs_topic = "/isim/neuromorphic_camera/eros"

    rospy.init_node('skel_image_publisher', anonymous=True)

    background = np.zeros((480, 640, 3), dtype=np.uint8)
    skel_img = np.zeros((480, 640, 3), dtype=np.uint8)
    rospy.Subscriber(skel_topic, NChumanPose, callback, skel_img)
    rospy.Subscriber(evs_topic, Image, evs_callback, background)

    while True:
        img = cv2.addWeighted(skel_img, 0.8, background, 1.0, 0.0)

        cv2.imshow("Image window", img)
        cv2.waitKey(1)

if __name__ == '__main__':
    main()
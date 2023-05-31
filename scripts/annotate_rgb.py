"""
Copyright (C) 2021 Event-driven Perception for Robotics
Author:
    Gaurvi Goyal

LICENSE GOES HERE
"""
import sys

sys.path.append('/home/ggoyal/code/hpe-core')
sys.path.append('/home/ggoyal/code/bimvee')

from tqdm import tqdm
import argparse
import cv2
import json
import pathlib
import os
import numpy as np

from datasets.utils.parsing import import_yarp_skeleton_data,batchIterator
from datasets.utils.events_representation import EROS
from datasets.utils.export import ensure_location, str2bool, get_hpecore_keypoints, get_center
from bimvee.importIitYarp import importIitYarp as import_dvs
from pycore.moveenet.visualization.visualization import add_skeleton


def remove_extra(pose):
    pose_out = np.reshape(pose,[-1,3])
    pose_out = pose_out[:,0:2]
    pose_out = np.reshape(pose_out,[-1])
    return pose_out
def create_annotated_video(video_path, data_vicon_file, output_path, skip=None, args=None):
    if skip == None:
        skip = 1
    else:
        skip = int(skip) + 1
    action_name = video_path.split(os.sep)[-1].split('.')[0]
    cap = cv2.VideoCapture(video_path)
    fps = cap.get(cv2.CAP_PROP_FPS)
    data_vicon = import_yarp_skeleton_data(pathlib.Path(data_vicon_file))
    dvs_file = args.dvs_file
    data_dvs = import_dvs(filePathOrName=dvs_file)
    data_dvs['data']['left']['dvs']['ts'] /= args.ts_scaler
    iterator = batchIterator(data_dvs['data']['left']['dvs'], data_vicon)
    # eros = EROS(kernel_size=args.eros_kernel, frame_width=args.frame_width, frame_height=args.frame_height)

    poses_movenet = []
    # if args.write_video:
    #     output_path_video = os.path.join(output_path,action_name+'.mp4')
    #     print(output_path_video)
    #     video_out = cv2.VideoWriter(output_path_video, cv2.VideoWriter_fourcc(*'avc1'), args.fps,
    #                                 (args.frame_width, args.frame_height))

    fi = 0
    while (cap.isOpened()):
        # Capture frame-by-frame

        for fi, (events, pose, batch_size) in enumerate(iterator):
            # for fi, (events, pose, ts) in enumerate(iterator):
            ret, frame = cap.read()
            # fps = cap.get(cv2.CAP_PROP_FPS)
            # frame_count = cap.get(cv2.CAP_PROP_FRAME_COUNT)
            #
            # print(fps)
            # print(frame_count)
            # exit()
            if ret == True:
                height, width, _ = frame.shape
            else:
                break
            if args.dev:
                print('frame: ', fi)
            # for ei in range(batch_size):
            #     eros.update(vx=int(events['x'][ei]), vy=int(events['y'][ei]))

            # if fi < 2:  # Almost empty images, not beneficial for training
            #     kps_old = get_movenet_keypoints(pose, args.frame_height, args.frame_width)
            #     continue

            if fi % skip != 0:
                continue

            # frame = eros.get_frame()
            sample_anno = {}
            sample_anno['img_name'] = action_name + '_' + str(fi).zfill(5) + '.jpg'
            sample_anno['ts'] = pose['ts']
            # sample_anno['keypoints'] = pose
            sample_anno['keypoints'] = remove_extra(get_hpecore_keypoints(pose, args.frame_height, args.frame_width))

            # print(sample_anno)
            # frame = cv2.GaussianBlur(frame, (args.gauss_kernel, args.gauss_kernel), 0)




            frame = add_skeleton(frame, sample_anno['keypoints'], (255, 0, 0), True, normalised=True)

            if args.dev:
                # keypoints = np.reshape(sample_anno['keypoints'], [-1, 3])
                # h, w, _ = frame.shape
                # for i in range(len(keypoints)):
                #     frame = cv2.circle(frame, [int(keypoints[i, 0] * w), int(keypoints[i, 1] * h)], 1, (255, 0, 0), 2)
                cv2.imshow('', frame)
                cv2.waitKey(1)
            # if args.write_video:
            #     video_out.write(frame)
            #     print('writing')
            if args.save_image:
                cv2.imwrite(os.path.join(output_path, sample_anno['img_name']), frame)
            if args.stop:
                if fi > args.stop:
                    break
        # When everything done, release the video capture object
        cap.release()

        # Closes all the frames
        cv2.destroyAllWindows()

    # if args.write_video:
    #     video_out.release()

    return



def main(args):

    if args.save_image:

        output_path_images = os.path.abspath(args.save_image)
        ensure_location(output_path_images)
    else:
        output_path_images = args.save_image


    input_video = os.path.abspath(args.input_video)
    data_vicon_file = args.input_anno
    process = True


    if not os.path.exists(input_video):
        print('skipping: ',  input_video, 'does not exist')
        process = False
    elif not os.path.exists(data_vicon_file):
        print('skipping: ',  data_vicon_file, 'does not exist')
        process = False

    if process:
        create_annotated_video(input_video, data_vicon_file, output_path_images, skip=args.skip_image, args=args)


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument('-frame_width', help='', default=640, type=int)
    parser.add_argument('-frame_height', help='', default=480, type=int)
    parser.add_argument('-skip_image', help='', default=None)
    # parser.add_argument("-write_video", type=str2bool, nargs='?', const=True, default=False, help="Save video.")
    parser.add_argument('-fps', help='', default=50, type=int)
    parser.add_argument("-dev", type=str2bool, nargs='?', const=True, default=False, help="Run in dev mode.")

    parser.add_argument('-input_video', help='Path to sample', default='', type=str)
    parser.add_argument('-dvs_file', help='Path to sample', default='', type=str)
    parser.add_argument('-input_anno', help='Path to sample', default='', type=str)
    parser.add_argument("-ts_scaler", help='', default=12.50, type=float)
    parser.add_argument('-save_image', help='Path to image folder', default=None, type=str)
    # parser.add_argument('-write_video', help='Path to video folder', default=None, type=str)
    parser.add_argument("-stop", type=str, default=None, help="early stop.")

    args = parser.parse_args()
    try:
        args = parser.parse_args()
    except argparse.ArgumentError:
        print('Catching an argumentError')

    args.dev = True
    args.input_video = '/home/ggoyal/data/h36m_cropped/cropped_video/cam4_S11_Phoning_3.mp4'
    args.input_anno = '/home/ggoyal/data/h36m_cropped/yarp/cam4_S11_Phoning_3/ch4GT50Hzskeleton/data.log'
    args.dvs_file = '/home/ggoyal/data/h36m_cropped/yarp/cam4_S11_Phoning_3/ch0dvs/'
    args.save_image = '/home/ggoyal/data/cvpr_experiments/rgb_anno/cam4_S11_Phoning_3/'
    args.stop = 2000
    main(args)



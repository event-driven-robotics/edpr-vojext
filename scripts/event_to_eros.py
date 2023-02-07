"""
Copyright (C) 2021 Event-driven Perception for Robotics
Author:
    Gaurvi Goyal

LICENSE GOES HERE
"""

from tqdm import tqdm
import argparse
import cv2
import json
import pathlib
import os
import numpy as np
import sys
sys.path.append('/home/ggoyal/code/hpe-core')
# sys.path.append('/home/ggoyal/code/hpe-core/example/movenet')
#
# from lib import init, MoveNet, Task
from datasets.utils.parsing import import_yarp_skeleton_data,batchIterator
from datasets.utils.events_representation import EROS
from datasets.utils.export import ensure_location, str2bool, get_movenet_keypoints, get_center
from bimvee.importIitYarp import importIitYarp as import_dvs
from bimvee.importIitYarp import importIitYarpBinaryDataLog

def create_ts_list(fps,ts):
    out = dict()
    out['ts'] = list()
    x = np.arange(ts[0],ts[-1],1/fps)
    for i in x:
        out['ts'].append(i)
    return out

def process(data_dvs_file, output_path, skip=None, args=None):

    if skip == None:
        skip = 1
    else:
        skip = int(skip) + 1

    data_dvs = import_dvs(filePathOrName=data_dvs_file)
    print('File imported.')
    # data_dvs = importIitYarpBinaryDataLog(filePathOrName=data_dvs_file)
    try:
        data_dvs['data']['left']['dvs']['ts'] /= args.ts_scaler
        side = 'left'
    except KeyError:
        data_dvs['data']['right']['dvs']['ts'] /= args.ts_scaler
        side = 'right'
    data_ts = create_ts_list(args.fps,data_dvs['data'][side]['dvs']['ts'])
    # print(f"{data_dvs_file.split('/')[-3:-1]}: \n start: {data_dvs['data'][side]['dvs']['ts'][0]} \n stop: {data_dvs['data'][side]['dvs']['ts'][-1]}")
    print(f"{data_dvs_file.split('/')[-3]}: \n start: {(-1)*data_dvs['data'][side]['dvs']['tsOffset']} \n duration: {data_dvs['data'][side]['dvs']['ts'][-1]}")
    iterator = batchIterator(data_dvs['data'][side]['dvs'], data_ts)
    eros = EROS(kernel_size=args.eros_kernel, frame_width=args.frame_width, frame_height=args.frame_height)

    poses_movenet = []
    if args.write_video:
        output_path_video = os.path.join(output_path,'eros-out.mp4')
        print(output_path_video)
        video_out = cv2.VideoWriter(output_path_video, cv2.VideoWriter_fourcc('M', 'J', 'P', 'G'), args.fps,
                                    (args.frame_width, args.frame_height))

    for fi, (events, pose, batch_size) in enumerate(iterator):
        print('frame: ', fi,'/',len(data_ts['ts']))

        # if args.dev:
        #     print('frame: ', fi)
        for ei in range(batch_size):
            eros.update(vx=int(events['x'][ei]), vy=int(events['y'][ei]))
        if fi % skip != 0:
            continue

        frame = eros.get_frame()
        frame = cv2.GaussianBlur(frame, (args.gauss_kernel, args.gauss_kernel), 0)

        if args.dev:
            # keypoints = np.reshape(sample_anno['keypoints'], [-1, 3])
            # h, w = frame.shape
            # for i in range(len(keypoints)):
            #     frame = cv2.circle(frame, [int(keypoints[i, 0] * w), int(keypoints[i, 1] * h)], 1, (255, 0, 0), 2)
            # frame = cv2.circle(frame, [int(sample_anno['center'][0] * w), int(sample_anno['center'][1] * h)], 1,
            #                    (255, 0, 0), 4)
            cv2.imshow('', frame)
            cv2.waitKey(1)
            if fi>50:
                break
        filename = os.path.basename(data_dvs_file)
        if args.write_images:
            cv2.imwrite(os.path.join(output_path, f'{filename}_{fi:04d}.jpg'), frame)
        if args.write_video:
            video_out.write(frame)
            print('writing')

    if args.write_video:
        video_out.release()

    return


def setup_testing_list(path):
    if not os.path.exists(path):
        return []
    with open(str(path), 'r+') as f:
        poses = json.load(f)
    files = [sample['original_sample'] for sample in poses]
    files_unique = set(files)
    return files_unique


def main(args):

    output_path = os.path.abspath(args.output_folder)
    ensure_location(output_path)
    input_data_dir = os.path.abspath(args.input_folder)

    print("=====", input_data_dir, "=====")
    if not os.path.exists(input_data_dir):
        print( input_data_dir, 'does not exist')
    else:
        process(input_data_dir, output_path, skip=args.skip_image, args=args)
    return


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument('-eros_kernel', help='', default=8, type=int)
    parser.add_argument('-frame_width', help='', default=640, type=int)
    parser.add_argument('-frame_height', help='', default=480, type=int)
    parser.add_argument('-gauss_kernel', help='', default=7, type=int)
    parser.add_argument('-skip_image', help='', default=None)
    parser.add_argument('-input', help='Path to input folder (with the data.log file in it)', default='/home/ggoyal/data/vojext_exp/binary/fatigue/alice/exp1-baseline/binaryevents.log', type=str)
    parser.add_argument('-output', help='Path to output folder', default='/home/ggoyal/data/vojext_exp/yarp/fatigue/alice/exp1-baseline/eros/', type=str)
    # parser.add_argument('-cfg', help='Path to movenet config file', default='/home/ggoyal/code/hpe-core/example/movenet/config.py', type=str)
    parser.add_argument("-write_images", type=str2bool, nargs='?', const=True, default=True, help="Save images.")
    parser.add_argument("-write_video", type=str2bool, nargs='?', const=True, default=True, help="Save video.")
    parser.add_argument('-fps', help='', default=50, type=int)
    parser.add_argument("-dev", type=str2bool, nargs='?', const=True, default=True, help="Run in dev mode.")
    parser.add_argument("-ts_scaler", help='', default=1.0, type=float)

    args = parser.parse_args()
    try:
        args = parser.parse_args()
    except argparse.ArgumentError:
        print('Catching an argumentError')
    main(args)



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
import sys, csv

sys.path.append('/home/ggoyal/code/hpe-core')
sys.path.append('/home/ggoyal/code/bimvee')
# sys.path.append('/home/ggoyal/code/hpe-core/example/movenet')
#
# from lib import init, MoveNet, Task
from datasets.utils.parsing import import_yarp_skeleton_data, batchIterator
from datasets.utils.events_representation import EROS, eventFrame
# from datasets.h36m.utils.parsing import movenet_to_hpecore
from datasets.utils.export import ensure_location, str2bool, get_movenet_keypoints, get_center
from bimvee.importIitYarp import importIitYarp as import_dvs
from bimvee.importIitYarp import importIitYarpBinaryDataLog

from pycore.moveenet import init, MoveNet, Task

from pycore.moveenet.config import cfg
from pycore.moveenet.visualization.visualization import add_skeleton, movenet_to_hpecore
from pycore.moveenet.utils.utils import arg_parser
from pycore.moveenet.task.task_tools import image_show, write_output, superimpose


def create_ts_list(fps, ts):
    out = dict()
    out['ts'] = list()
    x = np.arange(ts[0], ts[-1], 1 / fps)
    for i in x:
        out['ts'].append(i)
    return out

def get_representation(rep_name, args):
    if rep_name == 'eros':
        rep = EROS(kernel_size=args.eros_kernel, frame_width=args.frame_width, frame_height=args.frame_height)
    elif rep_name == 'ef':
        rep = eventFrame(frame_height=args.frame_height, frame_width=args.frame_width, n=args.n)
    else:
        print('Representation not found for this setup.')
        exit()
    return rep

def process(data_dvs_file, output_path, skip=None, args=None):
    init(cfg)
    model = MoveNet(num_classes=cfg["num_classes"],
                    width_mult=cfg["width_mult"],
                    mode='train')
    run_task = Task(cfg, model)
    run_task.modelLoad(cfg['ckpt'])

    if skip == None:
        skip = 1
    else:
        skip = int(skip) + 1

    files = os.listdir(data_dvs_file)
    if 'binaryevents.log' in files:
        data_dvs = importIitYarpBinaryDataLog(filePathOrName=os.path.join(data_dvs_file, 'binaryevents.log'))
    else:
        data_dvs = import_dvs(filePathOrName=os.path.join(data_dvs_file))
    print('File imported.')
    # print(data_dvs['data'].keys())
    channel = list(data_dvs['data'].keys())[0]
    data_dvs['data'][channel]['dvs']['ts'] /= args.ts_scaler
    data_ts = create_ts_list(args.fps, data_dvs['data'][channel]['dvs']['ts'])
    # print(
    #     f"{data_dvs_file}: \n start: {(-1) * data_dvs['data'][channel]['dvs']['tsOffset']} \n duration: {data_dvs['data'][channel]['dvs']['ts'][-1]}")
    filename = os.path.basename(data_dvs_file)
    # with open('/home/ggoyal/data/april_exp/fatigue/results_fatigue/offsets.csv', 'a') as f:
    #     row = []
    #     offset = str((-1)*data_dvs['data'][channel]['dvs']['tsOffset'])
    #     paths = os.path.normpath(data_dvs_file).split(os.path.sep)
    #     row.extend([os.path.join(paths[-2],paths[-1]), offset])
    #     print(row)
    #     writer = csv.writer(f, delimiter=' ')
    #     writer.writerow(row)
    # return

    iterator = batchIterator(data_dvs['data'][channel]['dvs'], data_ts)
    rep = get_representation(args.rep, args)

    poses_movenet = []
    if args.write_video:
        output_path_video = os.path.join(output_path, 'eros-out.mp4')
        print(output_path_video)
        video_out = cv2.VideoWriter(output_path_video, cv2.VideoWriter_fourcc('M', 'J', 'P', 'G'), args.fps,
                                    (args.frame_width, args.frame_height))

    for fi, (events, pose, batch_size) in enumerate(iterator):
        rep.reset_frame()
        if fi % 100 == 0:
            print('frame: ', fi, '/', len(data_ts['ts']))

        if args.stop:
            if fi > args.stop:
                break
        # if args.dev:
        #     print('frame: ', fi)

        for ei in range(batch_size):
            rep.update(vx=int(events['x'][ei]), vy=int(events['y'][ei]))
        if fi % skip != 0:
            continue

        frame = rep.get_frame()

        if args.rep == 'eros':
            frame = cv2.GaussianBlur(frame, (args.gauss_kernel, args.gauss_kernel), 0)
        if args.save_input:
            cv2.imwrite(os.path.join(os.path.abspath(args.save_input), f'{filename}_{fi:04d}.jpg'), frame*2)

        if args.write_csv is not None:
            pre = run_task.predict_online(frame, write_csv=args.write_csv, ts=data_ts['ts'][fi])
        else:
            pre = run_task.predict_online(frame, ts=data_ts['ts'][fi])
        # if args.dev:
        #     cv2.imshow('', frame)
        #     cv2.waitKey(1000)
        if args.dev or args.write_images or args.write_video:
            frame = add_skeleton(frame, pre['joints'], (0, 0, 255), True, normalised=False)
            # frame = add_skeleton(frame, pre['joints'], (0, 0, 255), normalised=False)
            # keypoints = np.reshape(pre['joints'], [-1, 2])
            # h, w = frame.shape
            #
            # for i in range(len(keypoints)):
            #     frame = cv2.circle(frame, [int(keypoints[i, 0]), int(keypoints[i, 1])], 1, (255, 0, 0), 2)
            # frame = cv2.circle(frame, [int(sample_anno['center'][0] * w), int(sample_anno['center'][1] * h)], 1,
            #                    (255, 0, 0), 4)
            if args.dev:
                cv2.imshow('', frame)
                cv2.waitKey(1)
            # if fi>500:
            #     break
        if args.write_images:
            cv2.imwrite(os.path.join(output_path, f'{filename}_{fi:04d}.jpg'), frame*2)
        if args.write_video:
            video_out.write(frame)

    if args.write_video:
        video_out.release()

    return


def main(args):

    output_path = os.path.abspath(args.output)
    ensure_location(output_path)
    if args.save_input:
        ensure_location(args.save_input)
    input_data_dir = os.path.abspath(args.input)

    print("=====", input_data_dir, "=====")
    if not os.path.exists(input_data_dir):
        print(input_data_dir, 'does not exist')
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
    parser.add_argument('-input', help='Path to input folder (with the data.log file in it)',
                        default='/home/ggoyal/data/vojext_exp/binary/fatigue/alice/exp1-baseline', type=str)
    parser.add_argument('-output', help='Path to output folder',
                        default='/home/ggoyal/data/vojext_exp/yarp/fatigue/alice/exp1-baseline/tester/', type=str)
    # parser.add_argument('-cfg', help='Path to movenet config file', default='/home/ggoyal/code/hpe-core/example/movenet/config.py', type=str)
    parser.add_argument("-write_images", type=str2bool, nargs='?', const=True, default=False, help="Save images.")
    parser.add_argument("-save_input", type=str, default=None, help="Set to a path if to be saved.")
    parser.add_argument("-write_video", type=str2bool, nargs='?', const=True, default=False, help="Save video.")
    parser.add_argument("-write_csv", type=str, default=None, help="path to the cvs file for result.")
    parser.add_argument("-ckpt", type=str, default='', help="path to the ckpt for moveenet.")
    parser.add_argument('-fps', help='', default=50, type=int)
    parser.add_argument('-stop', help='', default=None, type=int)
    parser.add_argument('-rep', help='Representation: eros or ef', default='eros', type=str)
    parser.add_argument('-n', help='', default=7500, type=int)
    parser.add_argument("-dev", type=str2bool, nargs='?', const=True, default=True, help="Run in dev mode.")
    parser.add_argument("-ts_scaler", help='', default=1.0, type=float)

    try:
        args = parser.parse_args()
    except argparse.ArgumentError:
        print('Catching an argumentError')

    # cfg = arg_parser(cfg)
    cfg['ckpt'] = args.ckpt
    if args.dev:
        cfg['ckpt'] = '/home/ggoyal/code/hpe-core/example/movenet/models/e97_valacc0.81209.pth'
        args.write_cvs = '/home/ggoyal/data/vojext_exp/yarp/fatigue/alice/exp1-baseline/tester/moveenet.cvs'
    main(args)

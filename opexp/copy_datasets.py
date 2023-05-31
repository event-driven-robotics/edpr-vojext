
import argparse
import cv2
import os
import pathlib
import shutil

# from bimvee.importIitYarp import importIitYarp
# from tqdm import tqdm

# TODO: add path to v2e folder to PYTHONPATH

def copyS9():
    server_location = '/run/user/1000/gvfs/smb-share:server=iiticubns010.iit.local,share=human_pose_estimation/Datasets/h36m/yarp'
    local_location = '/home/aglover/HPE/data/H36m'


    mpii_frames_path = pathlib.Path(server_location)
    mpii_frames_names = [str(fpath.name) for fpath in mpii_frames_path.glob('cam*_S9*')]

    for i in mpii_frames_names:
        print(i)
        if not os.path.exists(local_location + '/' + i):
            os.mkdir(local_location + '/' + i)
            shutil.copytree(server_location + '/' + i + '/ch0dvs', local_location + '/' + i + '/ch0dvs')
            try:
                shutil.copytree(server_location + '/' + i + '/ch2GT50Hzskeleton', local_location + '/' + i + '/ch2GT50Hzskeleton')
            except:
                shutil.copytree(server_location + '/' + i + '/ch4GT50Hzskeleton', local_location + '/' + i + '/ch2GT50Hzskeleton')

def copyS11():
    server_location = '/run/user/1000/gvfs/smb-share:server=iiticubns010.iit.local,share=human_pose_estimation/Datasets/h36m/yarp'
    local_location = '/home/aglover/HPE/data/H36m'


    mpii_frames_path = pathlib.Path(server_location)
    mpii_frames_names = [str(fpath.name) for fpath in mpii_frames_path.glob('cam*_S11*')]

    for i in mpii_frames_names:
        print(i)
        if not os.path.exists(local_location + '/' + i):
            os.mkdir(local_location + '/' + i)
            shutil.copytree(server_location + '/' + i + '/ch0dvs', local_location + '/' + i + '/ch0dvs')
            try:
                shutil.copytree(server_location + '/' + i + '/ch2GT50Hzskeleton', local_location + '/' + i + '/ch2GT50Hzskeleton')
            except:
                shutil.copytree(server_location + '/' + i + '/ch4GT50Hzskeleton', local_location + '/' + i + '/ch2GT50Hzskeleton')

def main(args):

    #video_frames_batches_path = pathlib.Path(args.f)
    #video_frames_batches_path = pathlib.Path(video_frames_batches_path.resolve())

    #mpii_frames_path = video_frames_batches_path

    copyS11()




if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument('-m', help='path to file mpii_human_pose_v1_sequences_keyframes.mat')
    parser.add_argument('-a', help='path to the MPII\'s annotation file')
    parser.add_argument('-f', help='path to the MPII\'s video frames batches')
    parser.add_argument('-o', help='path to the output folder')
    parser.add_argument('-n', help='number of frames preceding the annotated one that must be used by v2e', type=int, default=10)
    parser.add_argument('-v2e_dvs_res', help='resolution of v2e output', type=int, default=640)
    parser.add_argument('-v2e_batch_size', help='v2e batch size', type=int, default=6)

    args = parser.parse_args()
    main(args)

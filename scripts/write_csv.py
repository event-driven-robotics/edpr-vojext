"""
Copyright (C) 2021 Event-driven Perception for Robotics
Author:
    Gaurvi Goyal

LICENSE GOES HERE
"""

from os import listdir, system
from os.path import join

input_path = "/home/ggoyal/data/april_exp/fatigue"
# output_path = input_path


subjects = listdir(input_path)
for subject in subjects:
    experiments = listdir(join(input_path, subject))
    for experiment in experiments:

        input_folder = join(input_path, subject, experiment)
        output_folder = join(input_path, 'results', subject)
        output_csv = join(input_path,'results', subject, experiment+'.csv')
        command = f"/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
        -ckpt /home/ggoyal/code/hpe-core/example/movenet/models/e97_valacc0.81209.pth \
        -dev False -input {input_folder} -output {output_folder} -write_csv {output_csv}"

        print(f'\n \n {input_folder} Started. \n \n')
        system(command)
        print(f'\n \n {input_folder} Completed \n \n')


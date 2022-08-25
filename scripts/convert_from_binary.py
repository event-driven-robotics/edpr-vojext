from bimvee.importIitYarp import importIitYarpBinaryDataLog
from bimvee.exportIitYarp import exportIitYarp
from os import listdir
from os.path import join
import argparse




# input_path = "/home/ggoyal/data/vojext_exp/binary/franco/exp2/binaryevents.log"
input_path = "/home/ggoyal/data/vojext_exp/binary/"
# output_path = "/home/ggoyal/data/vojext_exp/yarp/franco/exp2"
output_path = "/home/ggoyal/data/vojext_exp/yarp/"


subjects = listdir(input_path)
for subject in subjects:
    experiments = listdir(join(input_path,subject))
    for experiment in experiments:
        input_path_i = join(input_path, subject, experiment,'binaryevents.log')
        output_path_i = join(output_path, subject, experiment)
        print(input_path_i, output_path_i)
        if 'franco' in input_path_i:
            continue
        data = importIitYarpBinaryDataLog(filePathOrName=input_path_i)
        exportIitYarp(data, exportFilePath=output_path_i)

#
# parser = argparse.ArgumentParser(description='Converter from binary to yarp format')
# parser.add_argument('--input', '-i', dest='input_path', type=str, required=True,
#                     help='Path to input file')
# parser.add_argument('--output', '-o', dest='output_path', type=str, required=True,
#                     help='Path to output file')
#
# args = parser.parse_args()
#
# data = importIitYarpBinaryDataLog(filePathOrName=input_path)
# exportIitYarp(data, exportFilePath=output_path)
from bimvee.importIitYarp import importIitYarpBinaryDataLog
from bimvee.exportIitYarp import exportIitYarp
from os import listdir
from os.path import join
import argparse



# # Macarena (VOJEXT)
# input_path = "/home/ggoyal/data/vojext_exp/binary/macarena/giu/exp1/binaryevents.log"
# # input_path = "/home/ggoyal/data/vojext_exp/binary/fatigue/"
# output_path = "/home/ggoyal/data/vojext_exp/yarp/macarena/giu/exp1"
# # output_path = "/home/ggoyal/data/vojext_exp/yarp/fatigue/"
# # subjects = listdir(input_path)
# # for subject in subjects:
# #     experiments = listdir(join(input_path,subject))
# #     for experiment in experiments:
# #         input_path_i = join(input_path, subject, experiment,'binaryevents.log')
# #         output_path_i = join(output_path, subject, experiment)
# #         print(input_path_i, output_path_i)
# #         if 'franco' in input_path_i:
# #             continue
# data = importIitYarpBinaryDataLog(filePathOrName=input_path)
#
#         # exportIitYarp(data, exportFilePath=output_path_i)
#         # print(data['info'][])
#
# print(type(data))
# vals = len(data['data']['left']['dvs']['ts'])
# print(vals)
# for i in range(vals//100000 +1):
#     print(i)
#     writedict = {}
#     writedict['info'] = data['info']
#     writedict['data'] = {}
#     writedict['data']['ch0'] = {}
#     writedict['data']['ch0']['dvs'] = {}
#     writedict['data']['ch0']['dvs']['tsOffset'] = data['data']['left']['dvs']['tsOffset']
#     writedict['data']['ch0']['dvs']['ts'] = data['data']['left']['dvs']['ts'][i*100000:((i+1)*100000)-1]
#     writedict['data']['ch0']['dvs']['x'] = data['data']['left']['dvs']['x'][i*100000:((i+1)*100000)-1]
#     writedict['data']['ch0']['dvs']['y'] = data['data']['left']['dvs']['y'][i*100000:((i+1)*100000)-1]
#     writedict['data']['ch0']['dvs']['pol'] = data['data']['left']['dvs']['pol'][i*100000:((i+1)*100000)-1]
#     exportIitYarp(writedict, exportFilePath=output_path, writeMode='a', protectedWrite=False)



# button experiment (APRIL)
input_path = "/home/ggoyal/data/vojext_exp/binary/osai/"
output_path = "/home/ggoyal/data/vojext_exp/yarp/osai/"

types = listdir(input_path)
types = ['fault_button']

for exptype in types:
    experiments = listdir(join(input_path,exptype))
    for experiment in experiments:
        input_path_i = join(input_path, exptype, experiment,'binaryevents.log')
        output_path_i = join(output_path, exptype, experiment)
        print(input_path_i, output_path_i)
        data = importIitYarpBinaryDataLog(filePathOrName=input_path_i)
        writedict = {}
        writedict['info'] = data['info']
        writedict['data'] = {}
        writedict['data']['ch0'] = {}
        writedict['data']['ch0']['dvs'] = {}
        writedict['data']['ch0']['dvs']['tsOffset'] = data['data']['left']['dvs']['tsOffset']
        writedict['data']['ch0']['dvs']['ts'] = data['data']['left']['dvs']['ts']
        writedict['data']['ch0']['dvs']['x'] = data['data']['left']['dvs']['x']
        writedict['data']['ch0']['dvs']['y'] = data['data']['left']['dvs']['y']
        writedict['data']['ch0']['dvs']['pol'] = data['data']['left']['dvs']['pol']
        exportIitYarp(writedict, exportFilePath=output_path_i)


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

#/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
#-ckpt /home/ggoyal/code/hpe-core/example/movenet/models/e97_valacc0.81209.pth \
#-dev False -input /home/ggoyal/data/vojext_exp/fatigue/Part_001/atis/ \
#-output /home/ggoyal/data/vojext_exp/fatigue/Part_001/atis/ \
#-write_csv /home/ggoyal/data/vojext_exp/fatigue/Part_001/atis.csv
#
#
#/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
#-ckpt /home/ggoyal/code/hpe-core/example/movenet/models/e97_valacc0.81209.pth \
#-dev True -input /home/ggoyal/data/DHP19/yarp/S2_2_1/ch3dvs/ -frame_width 350 -frame_height 280
#


#/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
#-ckpt /home/ggoyal/code/hpe-core/example/movenet/models/e97_valacc0.81209.pth \
#-dev False -input /home/ggoyal/data/h36m/EV2/cam2_S1_Directions/ch0dvs/ \
#-output /home/ggoyal/data/h36m/ \
#-write_csv /home/ggoyal/data/h36m/cam2_S1_Directions/atis.csv


#
#/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
#-ckpt /home/ggoyal/code/hpe-core/example/movenet/models/e97_valacc0.81209.pth \
#-dev False -input /home/ggoyal/data/autism/G1_BPG209/atis \
#-output /home/ggoyal/data/autism/ \
#-write_csv /home/ggoyal/data/autism/G1_BPG209/atis.csv





### EROS
#/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
#-ckpt /home/ggoyal/code/hpe-core/example/movenet/models/e97_valacc0.81209.pth \
#-dev False -input /home/ggoyal/data/cvpr_experiments/yarp/cam4_S9_Directions/ch0dvs \
#-output /home/ggoyal/data/cvpr_experiments/moveenet_h36m/cam4_S9_Directions/ \
#-save_input /home/ggoyal/data/cvpr_experiments/eros/cam4_S9_Directions/ \
#-write_image True -stop 2000 -ts_scaler 12.5

/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
-ckpt /home/ggoyal/code/hpe-core/example/movenet/models/e97_valacc0.81209.pth \
-dev False -input /home/ggoyal/data/cvpr_experiments/yarp/cam4_S9_Sitting/ch0dvs \
-output /home/ggoyal/data/cvpr_experiments/moveenet_h36m/cam4_S9_Sitting/ \
-save_input /home/ggoyal/data/cvpr_experiments/eros/cam4_S9_Sitting/ \
-write_image True -stop 2000 -ts_scaler 12.5
#
/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
-ckpt /home/ggoyal/code/hpe-core/example/movenet/models/e97_valacc0.81209.pth \
-dev False -input /home/ggoyal/data/cvpr_experiments/yarp/cam4_S11_Phoning_3/ch0dvs \
-output /home/ggoyal/data/cvpr_experiments/moveenet_h36m/cam4_S11_Phoning_3/ \
-save_input /home/ggoyal/data/cvpr_experiments/eros/cam4_S11_Phoning_3/ \
-write_image True -stop 2000 -ts_scaler 12.5

#
#/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
#-ckpt /home/ggoyal/code/hpe-core/example/movenet/models/dhp19_allcams_e33_valacc0.87996.pth \
#-dev False -input /home/ggoyal/data/cvpr_experiments/yarp/S13_1_1/ch2dvs \
#-output /home/ggoyal/data/cvpr_experiments/moveenet_dhp19/S13_1_1/ \
#-save_input /home/ggoyal/data/cvpr_experiments/eros/S13_1_1/ \
#-write_image True -stop 2000 -frame_width 346 -frame_height 280
#
#/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
#-ckpt /home/ggoyal/code/hpe-core/example/movenet/models/dhp19_allcams_e33_valacc0.87996.pth \
#-dev False -input /home/ggoyal/data/cvpr_experiments/yarp/S14_1_8/ch3dvs \
#-output /home/ggoyal/data/cvpr_experiments/moveenet_dhp19/S14_1_8/ \
#-save_input /home/ggoyal/data/cvpr_experiments/eros/S14_1_8/ \
#-write_image True -stop 2000 -frame_width 346 -frame_height 280
#
#/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
#-ckpt /home/ggoyal/code/hpe-core/example/movenet/models/dhp19_allcams_e33_valacc0.87996.pth \
#-dev False -input /home/ggoyal/data/cvpr_experiments/yarp/S16_1_5/ch1dvs \
#-output /home/ggoyal/data/cvpr_experiments/moveenet_dhp19/S16_1_5/ \
#-save_input /home/ggoyal/data/cvpr_experiments/eros/S16_1_5/ \
#-write_image True -stop 2000 -frame_width 346 -frame_height 280
#

#/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
#-ckpt /home/ggoyal/code/hpe-core/example/movenet/models/e97_valacc0.81209.pth \
#-dev False -input /home/ggoyal/data/cvpr_experiments/yarp/macarena/ch0dvs \
#-output /home/ggoyal/data/cvpr_experiments/moveenet_h36m/macarena/ \
#-save_input /home/ggoyal/data/cvpr_experiments/eros/macarena/ \
#-write_image True -stop 1000
#
#/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
#-ckpt /home/ggoyal/code/hpe-core/example/movenet/models/e97_valacc0.81209.pth \
#-dev False -input /home/ggoyal/data/cvpr_experiments/yarp/buckets/ \
#-output /home/ggoyal/data/cvpr_experiments/moveenet_h36m/buckets/ \
#-save_input /home/ggoyal/data/cvpr_experiments/eros/buckets/ \
#-write_image True -stop 1000
#
#/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
#-ckpt /home/ggoyal/code/hpe-core/example/movenet/models/dhp19_allcams_e33_valacc0.87996.pth \
#-dev False -input /home/ggoyal/data/cvpr_experiments/yarp/buckets/ \
#-output /home/ggoyal/data/cvpr_experiments/moveenet_h36m/buckets/ \
#-save_input /home/ggoyal/data/cvpr_experiments/eros/buckets/ \
#-write_image True -stop 1000

############## EventFrames
#/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
#-ckpt /home/ggoyal/data/models/eF.pth -input /home/ggoyal/data/cvpr_experiments/yarp/cam4_S9_Directions/ch0dvs \
#-output /home/ggoyal/data/cvpr_experiments/movenet_eF/cam4_S9_Directions/ \
#-save_input /home/ggoyal/data/cvpr_experiments/eF/cam4_S9_Directions/ \
#-dev False -write_images True -stop 2000 -rep ef -ts_scaler 12.5
##
#/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
#-ckpt /home/ggoyal/data/models/eF.pth \
#-dev False -input /home/ggoyal/data/cvpr_experiments/yarp/cam4_S9_Sitting/ch0dvs \
#-output /home/ggoyal/data/cvpr_experiments/movenet_eF/cam4_S9_Sitting/ \
#-save_input /home/ggoyal/data/cvpr_experiments/eF/cam4_S9_Sitting/ \
#-write_image True -stop 2000 -rep ef -ts_scaler 12.5

#/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
#-ckpt /home/ggoyal/data/models/eF.pth \
#-dev False -input /home/ggoyal/data/cvpr_experiments/yarp/cam4_S11_Phoning_3/ch0dvs \
#-output /home/ggoyal/data/cvpr_experiments/movenet_eF/cam4_S11_Phoning_3/ \
#-save_input /home/ggoyal/data/cvpr_experiments/eF/cam4_S11_Phoning_3/ \
#-write_image True -stop 2000 -rep ef -ts_scaler 12.5
#
#/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
#-ckpt /home/ggoyal/data/models/eF.pth \
#-dev False -input /home/ggoyal/data/cvpr_experiments/yarp/macarena/ch0dvs \
#-output /home/ggoyal/data/cvpr_experiments/movenet_eF/macarena/ \
#-save_input /home/ggoyal/data/cvpr_experiments/eF/macarena/ \
#-write_image True -stop 1000 -rep ef
#
#
#/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
#-ckpt /home/ggoyal/data/models/eF.pth \
#-dev False -input /home/ggoyal/data/cvpr_experiments/yarp/buckets/ \
#-output /home/ggoyal/data/cvpr_experiments/movenet_eF/buckets/ \
#-save_input /home/ggoyal/data/cvpr_experiments/eF/buckets/ \
#-write_image True -stop 1000 -rep ef



#/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
#-ckpt /home/ggoyal/data/models/eF.pth -input /home/ggoyal/data/cvpr_experiments/yarp/office/ch0dvs \
#-output /home/ggoyal/data/cvpr_experiments/movenet_eF/office/ \
#-save_input /home/ggoyal/data/cvpr_experiments/eF/office/ \
#-dev True -write_images True -rep ef
#
#/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
#-ckpt /home/ggoyal/code/hpe-core/example/movenet/models/dhp19_allcams_e33_valacc0.87996.pth \
#-dev True -input /home/ggoyal/data/cvpr_experiments/yarp/office/ch0dvs \
#-output /home/ggoyal/data/cvpr_experiments/moveenet_dhp19/office/ \
#-write_image True
#
#/home/ggoyal/code/hpe-core/venv/bin/python event-to-skeleton.py \
#-ckpt /home/ggoyal/code/hpe-core/example/movenet/models/e97_valacc0.81209.pth \
#-dev False -input /home/ggoyal/data/cvpr_experiments/yarp/office/ \
#-output /home/ggoyal/data/cvpr_experiments/moveenet_h36m/office/ \
#-save_input /home/ggoyal/data/cvpr_experiments/eros/office/ \
#-write_image True
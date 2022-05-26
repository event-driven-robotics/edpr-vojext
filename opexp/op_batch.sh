filenames=`ls /usr/local/HPE/data/H36m/_cam2_S9_videos/*.mp4`
outpath='/usr/local/HPE/experiments/openpose_comparison/'
for eachfile in $filenames
do
   outfile=${eachfile/.mp4/.csv}
   outfile="$outpath${outfile##*/}"
   if ! [ -f $outfile ]; then
       openpose-evaluation $eachfile $outfile
   fi
done

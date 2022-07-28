filenames=`ls /usr/local/HPE/data/H36m/_cam4_S9_videos/*.mp4`
outpath='/usr/local/HPE/experiments/openpose_comparison/'
algname='openpose_rgb.csv'
for eachfile in $filenames
do
    #this currently creates /outpath/datasetname.csv but we want
    #                       /output/datasetname/algname.csv
    #outfile=${eachfile/.mp4/.csv}

    outfile=${eachfile%.*}
    outdir="$outpath${outfile##*/}"
    outfile="$outdir/$algname"
    #echo ${outdir}

    if ! [ -d $outdir ]; then
	    mkdir $outdir
    fi
    chown aglover:aglover $outdir

    if ! [ -f $outfile ]; then
	echo "processing ${outfile}"
        openpose-evaluation $eachfile $outfile 2>1 1>/dev/null
    else
	#file_size=`du -k ${outfile} | cut -f1`
	video_frames=`ffprobe -v error -select_streams v:0 -count_packets -show_entries stream=nb_read_packets -of csv=p=0 ${eachfile}`
	file_frames=`wc -l ${outfile} | awk '{print $1}'`
	if ! [ "$video_frames" -eq "$file_frames" ]; then
            echo "reprocessing ${outfile}: [${video_frames} ${file_frames}]"
            openpose-evaluation $eachfile $outfile 2>1 1>/dev/null
    	else
	    echo "skipping ${outfile}: already exists [${video_frames} ${file_frames}]"
	fi
    fi

    chown aglover:aglover $outfile
done

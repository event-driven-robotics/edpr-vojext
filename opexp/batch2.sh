datasetroot='/usr/local/HPE/data/H36m'
alldatasets=`ls ${datasetroot}`
algfile='pim.mp4'
outpath='/usr/local/HPE/experiments/openpose_comparison'
outfilename='openpose_pim.csv'

for thisdataset in $alldatasets
do

    #check that the pimfile exists
    vidfile="${datasetroot}/${thisdataset}/${algfile}"
    if ! [ -f $vidfile ]; then
	    echo "skipping $thisdataset: no $algfile"
	    continue
    fi

    outdir="${outpath}/${thisdataset}"
    outfile="${outdir}/${outfilename}"
    if ! [ -d $outdir ]; then
	mkdir $outdir
    	chown aglover:aglover $outdir
    fi

    if [ -d $outfile ]; then
	echo "removing outdirectory $outfile"
	rm -r $outfile
    fi

    if ! [ -f $outfile ]; then
	echo "processing ${outfile}"
        #openpose-evaluation $vidfile $outfile 2>1 1>/dev/null
        chown aglover:aglover $outfile
    else
	video_frames=`ffprobe -v error -select_streams v:0 -count_packets -show_entries stream=nb_read_packets -of csv=p=0 ${vidfile}`
	file_frames=`wc -l ${outfile} | awk '{print $1}'`
	if ! [ "$video_frames" -eq "$file_frames" ]; then
            echo "reprocessing ${outfile}: [${video_frames} ${file_frames}]"
            #openpose-evaluation $vidfile $outfile 2>1 1>/dev/null
    	    chown aglover:aglover $outfile
    	else
	    echo "skipping ${outfile}: already exists [${video_frames} ${file_frames}]"
	fi
    fi

done

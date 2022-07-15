filenames=`ls /usr/local/HPE/data/H36m/_cam2_S11_videos/*.mp4`
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
    echo ${outdir}

    if ! [ -d $outdir ]; then
	    mkdir $outdir
    fi
    chown aglover:aglover $outdir
    if ! [ -f $outfile ]; then
        openpose-evaluation $eachfile $outfile
    fi
    chown aglover:aglover $outfile
done

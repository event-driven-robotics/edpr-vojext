rootfolder='/usr/local/HPE/data/H36m'
filenames=`ls ${rootfolder}/_cam2_S11_videos/*.mp4`
algname='rgb.mp4'
for eachfile in $filenames
do

    outfile=${eachfile%.*}
    outdir="${rootfolder}/${outfile##*/}"
    outfile="$outdir/$algname"

    if ! [ -f $outfile ]; then
        echo "$eachfile -> $outfile"
	    cp $eachfile $outfile 
    else
        echo "$outfile already exists"
    fi

    chown aglover:aglover $outfile
done

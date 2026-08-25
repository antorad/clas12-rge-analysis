#!/bin/bash

TARGET=$1
BANKS="dc_fmt"

WORKFLOW_NAME="rge_data_tuples_${TARGET}_${BANKS}"

# Create workflow
swif2 create $WORKFLOW_NAME

# Loop over run numbers from file and add jobs to workflow
while read -r RUN_NUMBER; do
    INPUT_CMD=""
    for mss_path in $(ls /mss/clas12/rg-e/production/spring2024/pass1/torus-1/${TARGET}_D2/dst/recon/$RUN_NUMBER/*); do
        FILENAME=$(basename "$mss_path")
        # Append an -input flag for EVERY file to the command string
        INPUT_CMD="$INPUT_CMD -input run_${RUN_NUMBER}/${FILENAME} mss:${mss_path}"
    done
    swif2 add-job $WORKFLOW_NAME \
        -name run_$RUN_NUMBER \
        -partition production \
        -time 24h \
        -disk-scratch 200g \
        -ram 6g \
        -shell /bin/bash \
        $INPUT_CMD \
        "/work/clas12/rg-e/antorad/clas12-rge-analysis/make_root_files_wf.sh -a -b $BANKS -r $RUN_NUMBER -t $TARGET"

done < runs/runs_inb_${TARGET}_D2_1.txt
#done < runs/runs_inb_test.txt

# Run workflow
swif2 run $WORKFLOW_NAME
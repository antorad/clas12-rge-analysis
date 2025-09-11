#!/bin/bash

WORKFLOW_NAME="rge_data_tuples_0.8"

# Create workflow
swif2 create $WORKFLOW_NAME

# Loop over run numbers from file and add jobs to workflow
while read -r RUN_NUMBER; do
    swif2 add-job $WORKFLOW_NAME \
        -name run_$RUN_NUMBER \
        -partition production \
        -time 2h \
        -ram 1g \
        -disk 1g \
        -shell /bin/bash \
        "cd /work/clas12/rg-e/antorad/clas12-rge-analysis && ./job_wrapper.sh $RUN_NUMBER"
done < run_list_all.txt

# Run workflow
swif2 run $WORKFLOW_NAME
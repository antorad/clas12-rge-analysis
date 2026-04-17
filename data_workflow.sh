#!/bin/bash

WORKFLOW_NAME="rge_data_tuples_C_1"
BANKS="dc"

# Create workflow
swif2 create $WORKFLOW_NAME

# Loop over run numbers from file and add jobs to workflow
while read -r RUN_NUMBER; do
    swif2 add-job $WORKFLOW_NAME \
        -name run_$RUN_NUMBER \
        -partition production \
        -time 5h \
        -ram 2g \
        -disk 100g \
        -shell /bin/bash \
        "cd /work/clas12/rg-e/antorad/clas12-rge-analysis && ./job_wrapper.sh $BANKS $RUN_NUMBER"
done < runs/run_list.txt

# Run workflow
swif2 run $WORKFLOW_NAME
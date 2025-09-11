#!/bin/bash

WORKFLOW_NAME="rge_simul_tuples"

# Create workflow
swif2 create $WORKFLOW_NAME

# Loop over job numbers from file and add jobs to workflow
while read -r JOB_NUMBER; do
    swif2 add-job $WORKFLOW_NAME \
        -name run_$JOB_NUMBER \
        -partition production \
        -time 2h \
        -ram 1g \
        -disk 1g \
        -shell /bin/bash \
        "cd /work/clas12/rg-e/antorad/clas12-rge-analysis && ./job_wrapper.sh $JOB_NUMBER"
done < run_list_simul.txt

# Run workflow
swif2 run $WORKFLOW_NAME
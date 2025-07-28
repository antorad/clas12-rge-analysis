#!/bin/bash

MAIN_DIR=/work/clas12/rg-e/antorad/clas12-rge-analysis

# Load Environment Commands
MODULE_USE_CMD="module use /scigroup/cvmfs/hallb/clas12/sw/modulefiles"
MODULE_LOAD_CMD="module load clas12"
EXPORT_ROOT_CMD="export ROOT=/u/scigroup/cvmfs/hallb/clas12/sw/almalinux9-gcc11/local/root/6.30.04/"
CD_TO_MAIN_DIR_CMD="cd $MAIN_DIR"

# Workflow name
WORKFLOW_NAME="test_data"

# Create workflow
swif2 create $WORKFLOW_NAME

# Loop over run numbers from file and add jobs to workflow
while read -r RUN_NUMBER; do
    JOB_SCRIPT="./make_root_files_wf.sh -r $RUN_NUMBER"

    # Compose full command by concatenating parts
    FULL_COMMAND="$MODULE_USE_CMD; $MODULE_LOAD_CMD; $EXPORT_ROOT_CMD; $CD_TO_MAIN_DIR_CMD; $JOB_SCRIPT"

    # Add job to workflow
    swif2 add-job $WORKFLOW_NAME -name run_$RUN_NUMBER -shell /bin/bash -command "$FULL_COMMAND"

done < run_list.txt

#!/bin/bash

MAIN_DIR=/work/clas12/rg-e/antorad/clas12-rge-analysis

#Load clas12 modules
module use /scigroup/cvmfs/hallb/clas12/sw/modulefiles
module load clas12

#Set ROOT location
#HIPO location is set by clas12 module
export ROOT=/u/scigroup/cvmfs/hallb/clas12/sw/almalinux9-gcc11/local/root/6.30.04/

#Create workflow
swif2 create test_data

#Loop over run numbers from file adn add jobs to workflow
while read -r RUN_NUMBER; do
    COMMAND="./make_root_files_wf.sh -r $RUN_NUMBER"
    swif2 add-job test_data -shell /bin/bash "cd $MAIN_DIR; $COMMAND"
    #echo 'testing command: $COMMAND'
done < run_list.txt
#!/bin/bash

echo "--- Running: job_wrapper ---"

module use /scigroup/cvmfs/hallb/clas12/sw/modulefiles
module load clas12
export ROOT=/u/scigroup/cvmfs/hallb/clas12/sw/almalinux9-gcc11/local/root/6.36.04/
cd /work/clas12/rg-e/antorad/clas12-rge-analysis
echo "About to run make_root_files_wf.sh with RUN_NUMBER=$2 and $1 banks"

if [ $1 == "dc" ]; then
    ./make_root_files_wf.sh -a -r $2
elif [ $1 == "fmt" ]; then
    ./make_root_files_wf.sh -a -f -r $2
else
    echo "Wrong bank name"
fi
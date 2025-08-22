  GNU nano 5.6.1                              job_wrapper.sh                                         
#!/bin/bash

echo "--- Running: job_wrapper ---"

module use /scigroup/cvmfs/hallb/clas12/sw/modulefiles
module load clas12
export ROOT=/u/scigroup/cvmfs/hallb/clas12/sw/almalinux9-gcc11/local/root/6.30.04/
cd /work/clas12/rg-e/antorad/clas12-rge-analysis
echo "About to run make_root_files_wf.sh with RUN_NUMBER=$1"
./make_root_files_wf.sh -a -r $1
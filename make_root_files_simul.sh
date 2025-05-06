#!/bin/bash

# Programs directories
# HIPO2ROOT="/home/antorad/hipo_tools/bin/hipo2root" #hipo2root original
HIPO2ROOT="./bin/hipo2root" #hipo2root bruno
# DST2ROOT="/home/antorad/hipo_tools/bin/dst2root" #dst2root
MAKENTUPLES="./bin/make_ntuples" #makentuples

# Directories necessary
HIPO_DIR="/volatile/clas12/osg/antorad/job_9075/output/"
WORK_DIR="root_io/simul_D2"
OUT_DIR="ntuple_files/simul_D2"

#Energy in sumualtions in 0.1*GeV
ENERGY=106

# Define the number of files to process in each subdirectory
NUM_FILES_TO_PROCESS=3

# Define the flag to process all files (set to true to process all, false to process a fixed number)
PROCESS_ALL_FILES=false

# Define the file extension to look for
FILE_EXTENSION=".hipo"

mkdir -p $WORK_DIR
mkdir -p $OUT_DIR
#cd $WORK_DIR

#Get job number
JOB_NUMBER=$(echo "$HIPO_DIR" | grep -oP 'job_\K[0-9]+')
echo "Working of files from job $JOB_NUMBER"

# Parse command-line options
while getopts ":a" opt; do
  case $opt in
    a)
      PROCESS_ALL_FILES=true
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
  esac
done

# Iterate over each subdirectory in the run list
if [ -d "$HIPO_DIR" ]; then
    echo "Checking directory: $HIPO_DIR"
    # Find all files with the specified extension in the current subdirectory
    FILES=($(find "$HIPO_DIR" -maxdepth 1 -type f -name "*$FILE_EXTENSION"))
    FILE_COUNT=0
    # Process files based on the flag
    for FILE in "${FILES[@]}"; do
        if [ "$PROCESS_ALL_FILES" = false ] && [ "$FILE_COUNT" -ge "$NUM_FILES_TO_PROCESS" ]; then
            break
        fi
        echo "Processing file: $FILE"
        # Run hipo2root and rename the output
        BANKS_OUT=$(printf "banks_999%03d_%05d.root" "$ENERGY" "$FILE_COUNT")
        echo "Generated BANKS_OUT: $BANKS_OUT"
        $HIPO2ROOT -w "$WORK_DIR" "$FILE"
        mv $(printf "$WORK_DIR/banks_%06d.root" "$JOB_NUMBER") "$WORK_DIR/$BANKS_OUT"
        FILE_COUNT=$((FILE_COUNT + 1))
    done
    echo "Merging root banks"
    BANKS_RUN_OUT=$(printf "banks_999%03d.root" "$ENERGY")
    echo "Generated BANKS_RUN_OUT: $BANKS_RUN_OUT"
    hadd -f "$WORK_DIR/$BANKS_RUN_OUT" $(printf "%s/banks_999%03d_*.root" "$WORK_DIR" "$ENERGY")
    echo "Making ntuples"
    $MAKENTUPLES -w "$WORK_DIR" "$WORK_DIR/$BANKS_RUN_OUT"
    mv $(printf "%s/ntuples_dc_999%03d.root" "$WORK_DIR" "$ENERGY") $(printf "%s/ntuples_dc_999%03d.root" "$OUT_DIR" "$ENERGY")
else
    echo "Directory $HIPO_DIR does not exist."
fi

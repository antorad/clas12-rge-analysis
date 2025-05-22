#!/bin/bash

# Programs directories
HIPO2ROOT="./bin/hipo2root" #hipo2root
MAKENTUPLES="./bin/make_ntuples" #makentuples

# Define the number of files to process in each subdirectory
NUM_FILES_TO_PROCESS=3
# Define the flag to process all files (set to true to process all, false to process a fixed number)
PROCESS_ALL_FILES=false

# Parse command-line options
while getopts "aj:t:" opt; do
  case $opt in
    a) PROCESS_ALL_FILES=true ;;
    j) JOB_NUMBER=$OPTARG ;;
    t) TARGET=$OPTARG ;;
    \?) echo "Invalid option: -$OPTARG" >&2 ;;
  esac
done

# Directories necessary
HIPO_DIR="/volatile/clas12/osg/antorad/job_$JOB_NUMBER/output/"
WORK_DIR="root_io/simul/$JOB_NUMBER"
OUT_DIR="ntuple_files/simul/$TARGET"

mkdir -p $OUT_DIR
mkdir -p $WORK_DIR

# Iterate over each subdirectory in the run list
if [ -d "$HIPO_DIR" ]; then
    echo "Checking directory: $HIPO_DIR"
    # Find all files with the specified extension in the current subdirectory
    FILES=($(find "$HIPO_DIR" -maxdepth 1 -type f -name "*"))
    FILE_COUNT=0

    # Process files based on the flag
    for FILE in "${FILES[@]}"; do
        if [ "$PROCESS_ALL_FILES" = false ] && [ "$FILE_COUNT" -ge "$NUM_FILES_TO_PROCESS" ]; then
            break
        fi

        echo "Processing file: $FILE"
        # Run hipo2root
        $HIPO2ROOT -s -w $WORK_DIR $FILE
        # Run make_ntuples and change name of the output
        $MAKENTUPLES -s -w $WORK_DIR $WORK_DIR/banks_*.root
        mv $WORK_DIR/ntuples_dc_*.root $WORK_DIR/${FILE_COUNT}_ntuples_dc.root
        rm $WORK_DIR/banks_*.root
        FILE_COUNT=$((FILE_COUNT + 1))
    done

    # Merge all root files into one and move them to output directory
    echo "Merging root banks"
    hadd -f $WORK_DIR/ntuples_dc_${JOB_NUMBER}.root $WORK_DIR/*_ntuples_dc.root
    mv $WORK_DIR/ntuples_dc_${JOB_NUMBER}.root $(printf $OUT_DIR/ntuples_dc_%06d.root $JOB_NUMBER)
    rm -rf $WORK_DIR
else
    echo "Directory $HIPO_DIR does not exist."
fi

#!/bin/bash

echo "--- Running: make_root_files_wf ---"

# Environment setup
module use /scigroup/cvmfs/hallb/clas12/sw/modulefiles
module load clas12
export ROOT=/u/scigroup/cvmfs/hallb/clas12/sw/almalinux9-gcc11/local/root/6.36.04/

# Programs directories
HIPO2ROOT="/work/clas12/rg-e/antorad/clas12-rge-analysis/bin/hipo2root" #hipo2root bruno
MAKENTUPLES="/work/clas12/rg-e/antorad/clas12-rge-analysis/bin/make_ntuples" #makentuples

# Define default flags (set to true to process all, false to process a fixed number)
NUM_FILES_TO_PROCESS=3
F_FLAG_H2R=""
F_FLAG_MNT=""
LABEL="UNK"
PROCESS_ALL_FILES=false

# Parse command-line options
while getopts "ab:r:t:" opt; do
  case $opt in
    a) PROCESS_ALL_FILES=true;;
    b)
       if [ "$OPTARG" == "dc" ]; then
           LABEL="dc"
       elif [ "$OPTARG" == "fmt" ]; then
           F_FLAG_H2R="-f"
           F_FLAG_MNT="-f 2"
           LABEL="fmt2"
       else
           echo "Wrong bank name"
           exit 1
       fi ;;
    r) RUN_NUMBER=$OPTARG;;
    t) TARGET=$OPTARG;;
    \?) echo "Invalid option: -$OPTARG" >&2;;
  esac
done

echo "--- Processing run: $RUN_NUMBER with target: $TARGET and $LABEL banks ---"

# Directories necessary
echo "Processing RUN_NUMBER: $RUN_NUMBER"
HIPO_DIR="run_${RUN_NUMBER}/"
OUT_DIR="/volatile/clas12/antorad/rge/data/pass1/${TARGET}_D2/"
mkdir -p $OUT_DIR

# Create a separate work dir for each run in the list
WORK_DIR=root_io/data/${LABEL}/$RUN_NUMBER
mkdir -p $WORK_DIR

if [ -d "$HIPO_DIR" ]; then
    echo "Checking directory: $HIPO_DIR"
    # Find all files with the subdirectory
    FILES=($(find "$HIPO_DIR" -maxdepth 1 -type f -name "*"))
    FILE_COUNT=0
    # Process files based on the flag
    for FILE in "${FILES[@]}"; do
        if [ "$PROCESS_ALL_FILES" = false ] && [ "$FILE_COUNT" -ge "$NUM_FILES_TO_PROCESS" ]; then
            break
        fi

        echo "Processing file: $FILE"
        # Extract the file number between 'evio' and 'hipo' using sed and remove leading zeros
        FILE_NUMBER=$(echo "$FILE" | sed -n 's/.*evio\.\([0-9]*\)\.hipo/\1/p' | sed 's/^0*//')
        echo "Extracted FILE_NUMBER: $FILE_NUMBER"
        # Run hipo2root and rename the output
        $HIPO2ROOT $F_FLAG_H2R -w $WORK_DIR "$FILE"
        # Run make_ntuples
        echo "Making ntuples"
        $MAKENTUPLES $F_FLAG_MNT -w $WORK_DIR $WORK_DIR/banks_*.root
        # Rename banks and tuples root files
        mv $WORK_DIR/ntuples_${LABEL}_*.root $WORK_DIR/${FILE_NUMBER}_ntuples_${LABEL}.root
        mv $WORK_DIR/banks_*.root $WORK_DIR/${FILE_NUMBER}_banks.root
        FILE_COUNT=$((FILE_COUNT + 1))
    done

    # Merge all root output files into one
    hadd -f $WORK_DIR/ntuples_${LABEL}_$RUN_NUMBER.root $WORK_DIR/*_ntuples_${LABEL}.root
    # Moving final files into volatile
    mkdir -p $OUT_DIR/$LABEL/ntuple_files
    mkdir -p $OUT_DIR/$LABEL/banks_root_files/$RUN_NUMBER
    mv $WORK_DIR/ntuples_${LABEL}_$RUN_NUMBER.root $OUT_DIR/$LABEL/ntuple_files
    mv $WORK_DIR/*_banks.root $OUT_DIR/$LABEL/banks_root_files/$RUN_NUMBER
    rm -rf $WORK_DIR
else
    echo "Directory $HIPO_DIR does not exist."
fi

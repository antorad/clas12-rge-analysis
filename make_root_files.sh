#!/bin/bash

# Programs directories
HIPO2ROOT="./bin/hipo2root" #hipo2root bruno
MAKENTUPLES="./bin/make_ntuples" #makentuples

# Directories necessary
HIPO_DIR="/volatile/clas12/rg-e/production/pass0.10/mon/recon/"
OUT_DIR="ntuple_files/data/"

mkdir -p $OUT_DIR

# Define the file that contains the list of runs to process
RUN_LIST_FILE="run_list.txt"

# Define the number of files to process in each subdirectory
NUM_FILES_TO_PROCESS=3

# Define the flag to process all files (set to true to process all, false to process a fixed number)
F_FLAG_H2R=""
F_FLAG_MNT=""
LABEL="dc"
PROCESS_ALL_FILES=false

# Parse command-line options
while getopts ":af" opt; do
  case $opt in
    a) PROCESS_ALL_FILES=true;;
    f)
       F_FLAG_H2R="-f"
       F_FLAG_MNT="-f 2"
       LABEL="fmt2"
       ;;
    \?) echo "Invalid option: -$OPTARG" >&2;;
  esac
done

# Read the list of runs from the text file
RUNS_TO_PROCESS=()
while IFS= read -r line || [ -n "$line" ]; do
  RUNS_TO_PROCESS+=("$line")
done < "$RUN_LIST_FILE"


# Iterate over each subdirectory in the run list
for RUN_NUMBER in "${RUNS_TO_PROCESS[@]}"; do
    echo "Processing RUN_NUMBER: $RUN_NUMBER"
    SUBDIR=$HIPO_DIR/$RUN_NUMBER

    # Create a separate work dir for each run in the list
    WORK_DIR=root_io/data/${LABEL}/$RUN_NUMBER
    mkdir -p $WORK_DIR

    if [ -d "$SUBDIR" ]; then
        echo "Checking directory: $SUBDIR"
        # Find all files with the subdirectory
        FILES=($(find "$SUBDIR" -maxdepth 1 -type f -name "*"))
        FILE_COUNT=0
        # Process files based on the flag
        for FILE in "${FILES[@]}"; do
            if [ "$PROCESS_ALL_FILES" = false ] && [ "$FILE_COUNT" -ge "$NUM_FILES_TO_PROCESS" ]; th>
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
        mv $WORK_DIR/ntuples_${LABEL}_$RUN_NUMBER.root $OUT_DIR
    else
        echo "Directory $SUBDIR does not exist."
    fi
done

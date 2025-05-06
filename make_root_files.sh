#!/bin/bash

# Programs directories
# HIPO2ROOT="/home/antorad/hipo_tools/bin/hipo2root" #hipo2root original
HIPO2ROOT="./bin/hipo2root" #hipo2root bruno
# DST2ROOT="/home/antorad/hipo_tools/bin/dst2root" #dst2root
MAKENTUPLES="./bin/make_ntuples" #makentuples

# Directories necessary
HIPO_DIR="/volatile/clas12/rg-e/production/pass0.7/mon/recon/"
WORK_DIR="root_io"
OUT_DIR="ntuple_files/"

# Define the file that contains the list of runs to process
RUN_LIST_FILE="run_list.txt"

# Define the number of files to process in each subdirectory
NUM_FILES_TO_PROCESS=3

# Define the flag to process all files (set to true to process all, false to process a fixed number)
PROCESS_ALL_FILES=false

# Define the file extension to look for
FILE_EXTENSION=".hipo"

mkdir -p $WORK_DIR
#cd $WORK_DIR

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

# Read the list of runs from the file
RUNS_TO_PROCESS=()
while IFS= read -r line || [ -n "$line" ]; do
  RUNS_TO_PROCESS+=("$line")
done < "$RUN_LIST_FILE"

# Iterate over each subdirectory in the run list
for RUN_NUMBER in "${RUNS_TO_PROCESS[@]}"; do
    echo "Processing RUN_NUMBER: $RUN_NUMBER"
    SUBDIR=$(printf "%s/%06d" "$HIPO_DIR" "$RUN_NUMBER")
    echo "Generated SUBDIR: $SUBDIR"
    if [ -d "$SUBDIR" ]; then
        echo "Checking directory: $SUBDIR"
        # Find all files with the specified extension in the current subdirectory
        FILES=($(find "$SUBDIR" -maxdepth 1 -type f -name "*$FILE_EXTENSION"))
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
            BANKS_OUT=$(printf "banks_%06d_%05d.root" "$RUN_NUMBER" "$FILE_NUMBER")
            echo "Generated BANKS_OUT: $BANKS_OUT"
            $HIPO2ROOT "$FILE"
            mv "$WORK_DIR/banks_000000.root" "$WORK_DIR/$BANKS_OUT"
            FILE_COUNT=$((FILE_COUNT + 1))
        done
        echo "Merging root banks"
        BANKS_RUN_OUT=$(printf "banks_%06d.root" "$RUN_NUMBER")
        echo "Generated BANKS_RUN_OUT: $BANKS_RUN_OUT"
        hadd "$WORK_DIR/$BANKS_RUN_OUT" $(printf "%s/banks_%06d_*.root" "$WORK_DIR" "$RUN_NUMBER")
        echo "Making ntuples"
        $MAKENTUPLES "$WORK_DIR/$BANKS_RUN_OUT"
        mv $(printf "%s/ntuples_dc_%06d.root" "$WORK_DIR" "$RUN_NUMBER") $(printf "ntuple_files/ntuples_dc_%06d.root" "$RUN_NUMBER")
    else
        echo "Directory $SUBDIR does not exist."
    fi
done

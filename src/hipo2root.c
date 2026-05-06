// CLAS12 RG-E Analyser.
// Copyright (C) 2022-2023 Bruno Benkel
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option) any
// later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
// details.
//
// You can see a copy of the GNU Lesser Public License under the LICENSE file.

// C.
#include <libgen.h>

// ROOT.
#include "TFile.h"
#include "TTree.h"

// HIPO.
#include "bank.h"
#include "dictionary.h"
#include "event.h"
#include "reader.h"

// rge-analysis.
#include "../lib/rge_constants.h"
#include "../lib/rge_err_handler.h"
#include "../lib/rge_filename_handler.h"
#include "../lib/rge_hipo_bank.h"
#include "../lib/rge_io_handler.h"
#include "../lib/rge_progress.h"

#include <iostream>
#include <vector>
#include <string>

static const char *USAGE_MESSAGE =
"Usage: hipo2root [-hfn:w:] infile\n"
" * -h         : show this message and exit.\n"
" * -f         : set this to true to process FMT::Tracks bank. If this is set\n"
"                and FMT::Tracks bank is not present in the HIPO file, the\n"
"                program will crash.\n"
" * -s         : set this to true to save MC banks.\n"
"                Should only be used for simulation files. If this is set\n"
"                and MC banks are not present in the HIPO file, the\n"
"                program will crash.\n"
" * -n nevents : number of events.\n"
" * -w workdir : location where output root files are to be stored. Default\n"
"                is root_io.\n"
" * infile     : input HIPO file. Expected format is <text>run_no.hipo.\n\n"
"    Convert a file from hipo to root format. This program only conserves the\n"
"    banks that are useful for RG-E analysis, as specified in the\n"
"    lib/rge_hipo_bank.h file.\n";

/** Number of banks in BANKLIST. */
static const uint NBANKS       = 10;
static const uint NBANKS_NOFMT_NOMC = 7;
static const uint NBANKS_NOFMT_WITHMC = 9;
static const uint NBANKS_WITHFMT_NOMC = 8;
/** List of banks hipo2root is capable of processing. */
std::vector<std::string> BANKVECTOR = {
    RGE_EVENT, RGE_RECPARTICLE, RGE_RECTRACK, RGE_RECCALORIMETER, RGE_RECCHERENKOV,
    RGE_RECSCINTILLATOR, RGE_RECTRAJ
};
/** run() function of the program. Check USAGE_MESSAGE for details. */
static int run(
        char *in_filename, char *work_dir, bool use_fmt, bool is_MC, int run_no,
        lint nevents
) {
    // Number of banks to read/write depends on type of analysis.
    if (use_fmt) BANKVECTOR.push_back(RGE_RECFTRACK);
    if (is_MC)
    {
        BANKVECTOR.push_back(RGE_MCPARTICLE);
        BANKVECTOR.push_back(RGE_MCEVENT);
    }
    const int nbanks = BANKVECTOR.size();

    // Access input sources.
    hipo::reader reader;
    hipo::dictionary factory;
    hipo::event event;
    
    reader.open(in_filename);
    reader.readDictionary(factory);

    // Create output tree and file.
    TTree *out_tree = new TTree(RGE_TREENAMEDATA, RGE_TREENAMEDATA);

    char out_filename[PATH_MAX];
    sprintf(out_filename, "%s/banks_%06d.root", work_dir, run_no);
    TFile *out_file = TFile::Open(out_filename, "RECREATE");

    // Open input file and get hipo schemas.
    hipo::bank   hbanks[nbanks];
    rge_hipobank rbanks[nbanks];

    for ( int i = 0; i < nbanks; i++ )
    {
        hbanks[i] = hipo::bank(factory.getSchema(BANKVECTOR[i].c_str()));
        // Initialize rge banks.
        rbanks[i] = rge_hipobank_init(BANKVECTOR[i].c_str());
        if (rge_errno != RGEERR_UNDEFINED) return 1;
        rge_link_branches(&(rbanks[i]), out_tree);
    }

    // Get event count.
    if (nevents == -1 || nevents > reader.getEntries())
        nevents = reader.getEntries();
    printf("Reading %ld events from %s.\n", nevents, in_filename);

    // Prepare fancy progress bar.
    rge_pbar_set_nentries(nevents);

    for (int event_no = 0; event_no < nevents; ++event_no) {
        // Print fancy progress bar.
        rge_pbar_update(event_no);

        // Read next event.
        reader.next();
        reader.read(event);

        // Fill banks from hipo event.
        luint total_nrows = 0;
        for (uint i = 0; i < nbanks; ++i) {
            event.getStructure(hbanks[i]);
            if (rge_fill(&(rbanks[i]), hbanks[i])) return 1;
            total_nrows += rbanks[i].nrows;
        }

        // Write to tree *if* event is not empty.
        if (total_nrows > 0) out_tree->Fill();
    }

    // Write to root tree and clean up after ourselves.
    out_tree->Write();
    out_file->Close();

    rge_errno = RGEERR_NOERR;
    return 0;
}

/**
 * Handle arguments for hipo2root using optarg. Error codes used are explained
 *     in the handle_err() function.
 */
static int handle_args(
        int argc, char **argv, char **in_filename, char **work_dir,
        bool *use_fmt, bool *is_MC, int *run_no, lint *nevents
) {
    // Handle arguments.
    int opt;
    while ((opt = getopt(argc, argv, "-hfsn:w:")) != -1) {
        switch (opt) {
            case 'h':
                rge_errno = RGEERR_USAGE;
                return 1;
            case 'f':
                *use_fmt = true;
                break;
            case 's':
                *is_MC = true;
                break;
            case 'n':
                if (rge_process_nentries(nevents, optarg)) return 1;
                break;
            case 'w':
                *work_dir = static_cast<char *>(malloc(strlen(optarg) + 1));
                strcpy(*work_dir, optarg);
                break;
            case 1:
                *in_filename = static_cast<char *>(malloc(strlen(optarg) + 1));
                strcpy(*in_filename, optarg);
                break;
            default:
                rge_errno = RGEERR_BADOPTARGS;
                return 1;
        }
    }

    // Define workdir if undefined.
    if (*work_dir == NULL) {
        *work_dir = static_cast<char *>(malloc(PATH_MAX));
        sprintf(*work_dir, "%s/../root_io", dirname(argv[0]));
    }

    // Check that a positional argument was given.
    if (*in_filename == NULL) {
        rge_errno = RGEERR_NOINPUTFILE;
        return 1;
    }

    if (rge_handle_hipo_filename(*in_filename, run_no)) return 1;

    return 0;
}

/** Entry point of hipo2root. Check usage() for details. */
int main(int argc, char **argv) {
    // Handle arguments.
    char *in_filename  = NULL;
    char *work_dir     = NULL;
    bool use_fmt       = false;
    bool is_MC         = false;
    int  run_no        = -1;
    lint nevents       = -1;

    handle_args(
            argc, argv, &in_filename, &work_dir, &use_fmt, &is_MC, &run_no, &nevents
    );

    // Run.
    if (rge_errno == RGEERR_UNDEFINED) {
        run(in_filename, work_dir, use_fmt, is_MC, run_no, nevents);
    }

    // Free up memory.
    if (in_filename != NULL) free(in_filename);
    if (work_dir    != NULL) free(work_dir);

    // Return errcode.
    return rge_print_usage(USAGE_MESSAGE);
}

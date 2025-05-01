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
#include <limits.h>

// ROOT.
#include <TFile.h>
#include <TNtuple.h>
#include <TROOT.h>
#include <TObjArray.h>

// rge-analysis.
#include "../lib/rge_constants.h"
#include "../lib/rge_err_handler.h"
#include "../lib/rge_extract_sf.h"
#include "../lib/rge_file_handler.h"
#include "../lib/rge_filename_handler.h"
#include "../lib/rge_hipo_bank.h"
#include "../lib/rge_io_handler.h"
#include "../lib/rge_particle.h"
#include "../lib/rge_progress.h"

static const char *USAGE_MESSAGE =
"Usage: make_ntuples [-hDf:cn:w:d:] infile\n"
" * -h         : show this message and exit.\n"
" * -D         : activate debug mode.\n"
" * -f fmtlyrs : define how many FMT layers should the track have hit.\n"
"                Options are 0 (tracked only by DC), 2, and 3. If set to\n"
"                something other than 0 and there is no FMT::Tracks bank in\n"
"                the input file, the program will crash. Default is 0.\n"
" * -c         : apply FMT geometry cut on data.\n"
" * -n nevents : number of events.\n"
" * -w workdir : location where output root files are to be stored. Default\n"
"                is root_io.\n"
" * -d datadir : location where sampling fraction files are. Default is data.\n"
" * infile     : input ROOT file. Expected file format: <text>run_no.root`.\n\n"
"    Generate ntuples relevant to SIDIS analysis based on the reconstructed\n"
"    variables from CLAS12 data.\n";

/** Detector IDs from CLAS12 reconstruction. */
static const uint FTOF_ID = 12;
static const uint HTCC_ID = 15;
static const uint LTCC_ID = 16;

/** FTOF layer IDs from CLAS12 reconstruction. */
static const uint FTOF1A_LYR = 1;
static const uint FTOF1B_LYR = 2;
static const uint FTOF2_LYR  = 3;

/** FMT geometry cut constants. */
static const double FMTCUT_RMIN  =  4.2575;
static const double FMTCUT_RMAX  = 18.4800;
static const double FMTCUT_Z0    = 26.1197;
static const double FMTCUT_ANGLE = 57.29;

/**
 * Find and return the most precise time of flight (TOF). Both the Forward Time
 *     Of Flight (FTOF) detectors and the Electronic Calorimeter (EC) can
 *     measure TOF, but they have different precisions. So, in order to get the
 *     most accurate measurement possible, this function returns the TOF
 *     measured by the most accurate detector for a given particle.
 *
 * In order of decreasing precision, the list of detectors are:
 *     FTOF1B > FTOF1A > FTOF2 > PCAL > ECIN > ECOU.
 *
 * @param scintillator : pointer to rge_hipobank containing scintillator data.
 * @param calorimeter  : pointer to rge_hipobank containing calorimeter data.
 * @param pindex       : particle index of the particle we're studying.
 * @return             : the most accurate TOF available in the scintillator and
 *                       calorimeter banks.
 */
static double get_tof(
        rge_hipobank *scintillator, rge_hipobank *calorimeter, uint pindex
) {
    int    most_precise_lyr = 0;
    double tof              = INFINITY;

    // Find TOF from scintillator.
    for (uint i = 0; i < scintillator->nrows; ++i) {
        // Filter out incorrect pindex and hits not from FTOF.
        if (
                rge_get_uint(scintillator, "pindex", i)   != pindex ||
                rge_get_uint(scintillator, "detector", i) != FTOF_ID
        ) {
            continue;
        }

        uint layer  = rge_get_uint(scintillator, "layer", i);
        double time = rge_get_double(scintillator, "time", i);

        // Check FTOF 1B (most precise FTOF layer).
        if (layer == FTOF1B_LYR) {
            most_precise_lyr = FTOF1B_LYR;
            tof = time;
            break; // Things won't get better than this.
        }

        // Check FTOF 1A.
        else if (layer == FTOF1A_LYR) {
            if (most_precise_lyr == FTOF1A_LYR) continue;
            most_precise_lyr = FTOF1A_LYR;
            tof = time;
        }

        // Check FTOF 2.
        else if (layer == FTOF2_LYR) {
            // We already have a similar or better hit.
            if (most_precise_lyr != 0) continue;
            most_precise_lyr = FTOF2_LYR;
            tof = time;
        }
    }
    if (most_precise_lyr != 0) return tof;

    // If no hits from FTOF were found, try to find TOF from calorimeters.
    for (uint i = 0; i < calorimeter->nrows; ++i) {
        // Filter out incorrect pindex.
        if (rge_get_uint(calorimeter, "pindex", i) != pindex) continue;

        // Check PCAL (Calorimeter with the most precise TOF).
        uint layer  = rge_get_uint(calorimeter, "layer", i);
        double time = rge_get_double(calorimeter, "time", i);

        if (layer == PCAL_LYR) {
            most_precise_lyr = 10 + PCAL_LYR;
            tof = time;
            break; // Things won't get better than this.
        }

        // Check ECIN.
        else if (layer == ECIN_LYR) {
            if (most_precise_lyr == 10 + ECIN_LYR) continue;
            most_precise_lyr = 10 + ECIN_LYR;
            tof = time;
        }

        // Check ECOU.
        else if (layer == ECOU_LYR) {
            if (most_precise_lyr != 0) continue;
            most_precise_lyr = 10 + ECOU_LYR;
            tof = time;
        }
    }

    return tof;
}

/**
 * Get deposited energy for particle with pindex from PCAL, ECIN, and ECOU.
 *
 * @param calorimeter : pointer to the calorimeter rge_hipobank.
 * @param pindex      : particle index of the particle we're studying
 * @param energy_PCAL : pointer to double to which we'll write the PCAL energy.
 * @param energy_ECIN : pointer to double to which we'll write the ECIN energy.
 * @param energy_ECOU : pointer to double to which we'll write the ECOU energy.
 * @return            : error code. 0 if successful, 1 otherwise. The function
 *                      only returns 1 if there's an invalid layer in the
 *                      calorimeter bank, suggesting corruption or a change in
 *                      the REC::Calorimeter bank structure.
 */
static int get_deposited_energy(
        rge_hipobank *calorimeter, uint pindex, double *energy_PCAL,
        double *energy_ECIN, double *energy_ECOU
) {
    *energy_PCAL = 0;
    *energy_ECIN = 0;
    *energy_ECOU = 0;

    for (uint i = 0; i < calorimeter->nrows; ++i) {
        if (rge_get_uint(calorimeter,"pindex",i) != pindex) continue;

        int layer     = rge_get_int   (calorimeter, "layer",  i);
        double energy = rge_get_double(calorimeter, "energy", i);

        if      (layer == PCAL_LYR) *energy_PCAL += energy;
        else if (layer == ECIN_LYR) *energy_ECIN += energy;
        else if (layer == ECOU_LYR) *energy_ECOU += energy;
        else {
            rge_errno = RGEERR_INVALIDCALLAYER;
            return 1;
        }
    }

    return 0;
}

/**
 * Count number of photoelectrons deposited on HTCC and LTCC detectors.
 *
 * @param cherenkov : pointer to rge_hipobank struct with Cherenkov's data.
 * @param pindex    :    particle index of the particle we're studying.
 * @param nphe_HTCC : pointer to int where we'll write the number of
 *                    photoelectrons deposited on HTCC.
 * @param nphe_LTCC : pointer to int where we'll write the number of
 *                    photoelectrons deposited on LTCC.
 * @return          : error code. 0 if successful, 1 otherwise. The function
 *                    only returns 1 if there's an invalid detector ID in the
 *                    cherenkov bank, suggesting data corruption or a change
 *                    in the REC::Cherenkov bank structure.
 */
static int count_photoelectrons(
        rge_hipobank *cherenkov, uint pindex, int *nphe_HTCC, int *nphe_LTCC
) {
    *nphe_HTCC = 0;
    *nphe_LTCC = 0;

    for (uint i = 0; i < cherenkov->nrows; ++i) {
        if (rge_get_uint(cherenkov, "pindex", i) != pindex) continue;

        int detector = rge_get_int(cherenkov, "detector", i);
        int nphe     = rge_get_int(cherenkov, "nphe",     i);
        if      (detector == HTCC_ID) *nphe_HTCC += nphe;
        else if (detector == LTCC_ID) *nphe_LTCC += nphe;
        else {
            rge_errno = RGEERR_INVALIDCHERENKOVID;
            return 1;
        }
    }

    return 0;
}

/**
 * Apply FMT geometry cut on a particle. This cut is defined by the particle's
 *     vz and its theta angle. theta_min and theta_max are given by:
 *     theta_min = 57.29 * atan(FMTCUT_RMIN / (FMTCUT_Z0 - vz)),
 *     theta_max = 57.29 * atan(FMTCUT_RMAX / (FMTCUT_Z0 - vz)),
 *     where FMTCUT_RMIN and FMTCUT_RMAX are the radii of the inner and outer
 *     circles of FMT, and FMTCUT_Z0 is the z position of the first FMT layer.
 *
 * @param p : particle for which we're applying the cut.
 * @return  : 0 if particle passes the cut, 1 otherwise, 2 if there's an angle
 *            conversion error.
 */
static int apply_fmtgeomtry_cut(rge_particle *p) {
    // Get minimum theta allowed for particle.
    double theta_min;
    if (rge_to_rad(
            FMTCUT_ANGLE * atan(FMTCUT_RMIN / (FMTCUT_Z0 - p->vz)),
            &theta_min
    )) return 2;

    // Get maximum theta allowed for particle.
    double theta_max;
    if (rge_to_rad(
            FMTCUT_ANGLE * atan(FMTCUT_RMAX / (FMTCUT_Z0 - p->vz)),
            &theta_max
    )) return 2;

    // Get particle's theta.
    double theta = atan2(sqrt(p->px*p->px + p->py*p->py), p->pz);

    // Return 1 if particle fails.
    if (theta_min > theta || theta > theta_max) return 1;

    // Return 0 otherwise.
    return 0;
}

/** run() function of the program. Check USAGE_MESSAGE for details. */
static int run(
        char *filename_in, char *work_dir, char *data_dir, bool debug,
        lint fmt_nlayers, bool fmt_cut, bool save_MC, lint n_events, int run_no,
        double energy_beam
) {
    // Get sampling fraction.
    char sampling_fraction_file[PATH_MAX];
    if (run_no / 1000 != 999) {
        // Input file is data.
        sprintf(
                sampling_fraction_file, "%s/sf_params_%06d.txt",
                data_dir, run_no
        );
    }
    else {
        // Input file is simulation.
        sprintf(sampling_fraction_file, "%s/sf_params_mc.txt", data_dir);
    }
    double sampling_fraction_params[RGE_NSECTORS][RGE_NSFPARAMS][2];
    if (access(sampling_fraction_file, F_OK) != 0) {
        // No sampling fraction file for this run, we need to extract it.
        printf(
                "No sampling fraction data found for run %d. Running "
                "extract_sf().\n", run_no
        );
        if (rge_extract_sf(filename_in, work_dir, data_dir, n_events, run_no)) {
            return 1;
        }
        printf("Done!\n\n");
        rge_errno = RGEERR_UNDEFINED;
    }
    if (rge_get_sf_params(sampling_fraction_file, sampling_fraction_params)) {
        return 1;
    }

    // Access input file.
    TFile *file_in  = TFile::Open(filename_in, "READ");
    if (!file_in || file_in->IsZombie()) {
        rge_errno = RGEERR_BADINPUTFILE;
        return 1;
    }
    // Create TTree and TNTuples.
    TTree *tree_in = file_in->Get<TTree>(RGE_TREENAMEDATA);
    if (tree_in == NULL) {
        rge_errno = RGEERR_BADROOTFILE;
        return 1;
    }

    // Checking whether the MC and FMT branches exist if we want to save them
    TObjArray *branch_list = tree_in->GetListOfBranches();
    bool found_fmt = false, found_mc = false;
    if ( fmt_nlayers != 0 || save_MC)
    {
        for( int i = 0; i < branch_list->GetEntries(); i++)
        {
            std::string branch_name = branch_list->At(i)->GetName();
            if ( fmt_nlayers != 0 && branch_name.find(RGE_FMTTRACKS) != std::string::npos)
                found_fmt = true;
            else if ( save_MC && branch_name.find(RGE_MCPARTICLE) != std::string::npos)
                found_mc = true;
            // Ending search early conditions
            if ( (found_fmt && found_mc) || (found_fmt && !save_MC) || (found_mc && fmt_nlayers == 0) )
                break;
        }
    }

    // If fmt_nlayers != 0, check that FMT::Tracks bank exists.
    if (fmt_nlayers != 0 && !found_fmt) {
        rge_errno = RGEERR_NOFMTBANK;
        return 1;
    }
    // If we want to save MC but the MC::Particle banks doesn't exit, throw error
    if (save_MC && !found_mc)
    {
        rge_errno = RGEERR_NOMC;
        return 1;
    }
    // Return to top directory (weird root stuff).
    gROOT->cd();

    // Generate lists of variables.
    TString vars_string("");
    for (int var_i = 0; var_i < RGE_VARS_SIZE; ++var_i) {
        vars_string.Append(Form("%s", RGE_VARS[var_i]));
        if (var_i != RGE_VARS_SIZE-1) vars_string.Append(":");
    }
    TString MC_vars_string("");
    if (save_MC)
    {   
        for (int var_i = 0; var_i < RGE_MC_VARS_SIZE; ++var_i) {
            MC_vars_string.Append(Form("%s", RGE_MC_VARS[var_i]));
            if (var_i != RGE_MC_VARS_SIZE-1) MC_vars_string.Append(":");
        }
    }

    TNtuple *tree_out;
    tree_out = new TNtuple(RGE_TREENAMEDATA, RGE_TREENAMEDATA, vars_string);

    TNtuple *MC_tree_out;
    MC_tree_out = new TNtuple(RGE_MCTREENAME, RGE_MCTREENAME, MC_vars_string);

    // Change n_events to number of entries if it is equal to -1 or invalid.
    if (n_events == -1 || n_events > tree_in->GetEntries()) {
        n_events = tree_in->GetEntries();
    }

    // Associate banks to TTree.
    rge_hipobank bpart = rge_hipobank_init(RGE_RECPARTICLE,     tree_in);
    rge_hipobank btrk  = rge_hipobank_init(RGE_RECTRACK,        tree_in);
    rge_hipobank bcal  = rge_hipobank_init(RGE_RECCALORIMETER,  tree_in);
    rge_hipobank bchkv = rge_hipobank_init(RGE_RECCHERENKOV,    tree_in);
    rge_hipobank bsci  = rge_hipobank_init(RGE_RECSCINTILLATOR, tree_in);
    // Optional hipo banks
    rge_hipobank bfmt, bmcpart, bmcevent;
    if (fmt_nlayers != 0)
        bfmt  = rge_hipobank_init(RGE_FMTTRACKS, tree_in);
    if (save_MC)
    {
        bmcpart  = rge_hipobank_init(RGE_MCPARTICLE, tree_in);
        bmcevent  = rge_hipobank_init(RGE_MCEVENT, tree_in);
    }
    
    // Iterate through input file. Each TTree entry is one event.
    printf("Processing %ld events from %s.\n", n_events, filename_in);

    // Prepare fancy progress bar.
    rge_pbar_reset();
    rge_pbar_set_nentries(n_events);

    // Particle counters.
    int trigger_counter = 0;
    int pionp_counter   = 0;
    int pionm_counter   = 0;

    // Loop through events in input file.
    for (lint event = 0; event < n_events; ++event) {
        // Print fancy progress bar.
        if (!debug) rge_pbar_update(event);

        // Get entries from input file.
        rge_get_entries(&bpart, tree_in, event);
        rge_get_entries(&btrk,  tree_in, event);
        rge_get_entries(&bcal,  tree_in, event);
        rge_get_entries(&bchkv, tree_in, event);
        rge_get_entries(&bsci,  tree_in, event);
        if (fmt_nlayers != 0) rge_get_entries(&bfmt, tree_in, event);
        if (save_MC) 
        {
            rge_get_entries(&bmcpart, tree_in, event);
            rge_get_entries(&bmcevent, tree_in, event);
        }

        // Save generated info from MC
        if (save_MC){
            int npart      = rge_get_double(&bmcevent, "npart", 0);
            int atarget    = rge_get_double(&bmcevent, "atarget", 0);
            int ztarget    = rge_get_double(&bmcevent, "ztarget", 0);
            double ptarget = rge_get_double(&bmcevent, "ptarget", 0);
            double pbeam   = rge_get_double(&bmcevent, "pbeam", 0);
            int btype      = rge_get_double(&bmcevent, "btype", 0);
            double ebeam   = rge_get_double(&bmcevent, "ebeam", 0);
            int targetid   = rge_get_double(&bmcevent, "targetid", 0);
            int processid  = rge_get_double(&bmcevent, "processid", 0);
            double weight  = rge_get_double(&bmcevent, "weight", 0);
            for (uint pos = 0; pos < bmcpart.nrows; ++pos)
            {
                Float_t arr[RGE_MC_VARS_SIZE];
                int    pid = rge_get_double(&bmcpart, "pid", pos);
                double px  = rge_get_double(&bmcpart, "px",  pos);
                double py  = rge_get_double(&bmcpart, "py",  pos);
                double pz  = rge_get_double(&bmcpart, "pz",  pos);
                double vx  = rge_get_double(&bmcpart, "vx",  pos);
                double vy  = rge_get_double(&bmcpart, "vy",  pos);
                double vz  = rge_get_double(&bmcpart, "vz",  pos);
                double vt  = rge_get_double(&bmcpart, "vt",  pos);
                
                arr[RGE_MC_RUNNO.addr] = static_cast<Float_t>(run_no);
                arr[RGE_MC_EVENTNO.addr] = static_cast<Float_t>(event);
                arr[RGE_MC_PID.addr] = static_cast<Float_t>(pid);
                arr[RGE_MC_PX.addr] = px;
                arr[RGE_MC_PY.addr] = py;
                arr[RGE_MC_PZ.addr] = pz;
                arr[RGE_MC_VX.addr] = vx;
                arr[RGE_MC_VY.addr] = vy;
                arr[RGE_MC_VZ.addr] = vz;
                arr[RGE_MC_VT.addr] = vt;
                arr[RGE_MC_NPART.addr] = static_cast<Float_t>(npart);
                arr[RGE_MC_ATARGET.addr] = static_cast<Float_t>(atarget);
                arr[RGE_MC_ZTARGET.addr] = static_cast<Float_t>(ztarget);
                arr[RGE_MC_PTARGET.addr] = ptarget;
                arr[RGE_MC_PBEAM.addr] = pbeam;
                arr[RGE_MC_BTYPE.addr] = static_cast<Float_t>(btype);
                arr[RGE_MC_EBEAM.addr] = ebeam;
                arr[RGE_MC_TARGETID.addr] = static_cast<Float_t>(targetid);
                arr[RGE_MC_PROCESSID.addr] = static_cast<Float_t>(processid);
                arr[RGE_MC_WEIGHT.addr] = weight;
                MC_tree_out->Fill(arr);
            }
        }

        // Filter events without the necessary banks.
        if (bpart.nrows == 0 || btrk.nrows == 0) continue;
        // Check existence of trigger electron
        rge_particle part_trigger;
        bool trigger_exist  = false;
        uint trigger_pos    = UINT_MAX;
        uint trigger_pindex = UINT_MAX;
        double trigger_tof  = -1.;
        for (uint pos = 0; pos < btrk.nrows; ++pos) {
            uint pindex = rge_get_uint(&btrk, "pindex", pos);

            // Get reconstructed particle from DC and from FMT.
            part_trigger = rge_particle_init(
                &bpart, &btrk, &bfmt, pos, fmt_nlayers
            );

            // Skip particle if it doesn't fit requirements.
            if (!part_trigger.is_valid) continue;

            // Cut triggers outside of FMT's active region.
            if (fmt_cut) {
                int result = apply_fmtgeomtry_cut(&part_trigger);
                if (result == 1) continue;
                if (result == 2) return 1;
            }

            // Get energy deposited in calorimeters.
            double energy_PCAL, energy_ECIN, energy_ECOU;
            if (get_deposited_energy(
                    &bcal, pindex, &energy_PCAL, &energy_ECIN, &energy_ECOU
            )) return 1;

            // Get number of photoelectrons from Cherenkov counters.
            int nphe_HTCC, nphe_LTCC;
            if (count_photoelectrons(&bchkv, pindex, &nphe_HTCC, &nphe_LTCC))
                return 1;

            // Get time of flight from scintillators or calorimeters.
            double tof = get_tof(&bsci, &bcal, pindex);

            // Get miscellaneous data.
            int status  = rge_get_double(&bpart, "status", pindex);
            double chi2 = rge_get_double(&btrk,  "chi2",   pos);
            double ndf  = rge_get_double(&btrk,  "NDF",    pos);

            // Assign PID.
            if (rge_set_pid(
                    &part_trigger, rge_get_double(&bpart, "pid", pindex),
                    status, energy_PCAL+energy_ECIN+energy_ECOU, energy_PCAL,
                    nphe_HTCC, nphe_LTCC,
                    sampling_fraction_params[rge_get_uint(&btrk, "sector", pos)]
            )) return 1;

            // Skip particle if its not the trigger electron.
            if (!part_trigger.is_trigger) continue;

            // Fill TNtuple with trigger electron information.
            Float_t arr[RGE_VARS_SIZE];
            if (rge_fill_ntuples_arr(
                    arr, part_trigger, part_trigger, run_no, event, status,
                    energy_beam, chi2, ndf, energy_PCAL, energy_ECIN,
                    energy_ECOU, tof, tof, nphe_LTCC, nphe_HTCC
            )) return 1;

            tree_out->Fill(arr);

            // Fill out trigger electron data and end loop.
            trigger_exist  = true;
            trigger_pindex = pindex;
            trigger_pos    = pos;
            trigger_tof    = tof;
            break;
        }

        // Skip events without a trigger electron.
        if (!trigger_exist) continue;
        ++trigger_counter;

        // Processing particles.
        for (uint pos = 0; pos < btrk.nrows; ++pos) {
            uint pindex = rge_get_uint(&btrk, "pindex", pos);

            // Avoid double-counting the trigger electron.
            if (trigger_pindex == pindex && trigger_pos == pos) {
                continue;
            }

            // Get reconstructed particle from DC and from FMT.
            rge_particle part = rge_particle_init(
                &bpart, &btrk, &bfmt, pos, fmt_nlayers
            );

            // Skip particle if it doesn't fit requirements.
            if (!part.is_valid) continue;

            // Cut particles outside of FMT's active region.
            if (fmt_cut) {
                int result = apply_fmtgeomtry_cut(&part);
                if (result == 1) continue;
                if (result == 2) return 1;
            }

            // Get energy deposited in calorimeters.
            double energy_PCAL, energy_ECIN, energy_ECOU;
            if (get_deposited_energy(
                    &bcal, pindex, &energy_PCAL, &energy_ECIN, &energy_ECOU
            )) return 1;

            // Get Cherenkov counters data.
            int nphe_HTCC, nphe_LTCC;
            if (count_photoelectrons(&bchkv, pindex, &nphe_HTCC, &nphe_LTCC))
                return 1;

            // Get time-of-flight (tof).
            double tof = get_tof(&bsci, &bcal, pindex);

            // Get miscellaneous data.
            int status  = rge_get_double(&bpart, "status", pindex);
            double chi2 = rge_get_double(&btrk,  "chi2",   pos);
            double ndf  = rge_get_double(&btrk,  "NDF",    pos);

            // Assign PID.
            if (rge_set_pid(
                    &part, rge_get_double(&bpart, "pid", pindex), status,
                    energy_PCAL + energy_ECIN + energy_ECOU, energy_PCAL,
                    nphe_HTCC, nphe_LTCC,
                    sampling_fraction_params[rge_get_uint(&btrk, "sector", pos)]
            )) return 1;

            // Fill TNtuples. If adding new variables, check their order in
            //     RGE_VARS.
            Float_t arr[RGE_VARS_SIZE];
            if (rge_fill_ntuples_arr(
                    arr, part, part_trigger, run_no, event, status, energy_beam,
                    chi2, ndf, energy_PCAL, energy_ECIN, energy_ECOU, tof,
                    trigger_tof, nphe_LTCC, nphe_HTCC
            )) return 1;

            tree_out->Fill(arr);

            if (part.pid ==  211) ++pionp_counter;
            if (part.pid == -211) ++pionm_counter;
        }

    }

    // Print number of particles found to detect errors early.
    printf("e-  found: %d\n",   trigger_counter);
    printf("pi+ found: %d\n",   pionp_counter);
    printf("pi- found: %d\n\n", pionm_counter);

    // Create output file.
    char filename_out[PATH_MAX];
    if (fmt_nlayers == 0) {
        sprintf(filename_out, "%s/ntuples_dc_%06d.root", work_dir, run_no);
    }
    else {
        sprintf(
                filename_out, "%s/ntuples_fmt%1ld_%06d.root", work_dir,
                fmt_nlayers, run_no
        );
    }
    TFile *file_out = TFile::Open(filename_out, "RECREATE");

    // Write to output file.
    file_out->cd();
    tree_out->Write();
    if (save_MC)
        MC_tree_out->Write();

    // Clean up after ourselves.
    file_in ->Close();
    file_out->Close();

    rge_errno = RGEERR_NOERR;
    return 0;
}

/** Handle arguments for make_ntuples using optarg. */
static int handle_args(
        int argc, char **argv, char **filename_in, char **work_dir,
        char **data_dir, bool *debug, lint *fmt_nlayers, bool *fmt_cut,
        bool *save_MC, lint *n_events, int *run_no, double *energy_beam
) {
    // Handle arguments.
    int opt;
    while ((opt = getopt(argc, argv, "-hDf:csn:w:d:")) != -1) {
        switch (opt) {
            case 'h':
                rge_errno = RGEERR_USAGE;
                return 1;
            case 'D':
                *debug = true;
                break;
            case 'f':
                if (rge_process_fmtnlayers(fmt_nlayers, optarg)) return 1;
                break;
            case 'c':
                *fmt_cut = true;
                break;
            case 's':
                *save_MC = true;
                break;
            case 'n':
                if (rge_process_nentries(n_events, optarg)) return 1;
                break;
            case 'w':
                *work_dir = static_cast<char *>(malloc(strlen(optarg) + 1));
                strcpy(*work_dir, optarg);
                break;
            case 'd':
                *data_dir = static_cast<char *>(malloc(strlen(optarg) + 1));
                strcpy(*data_dir, optarg);
                break;
            case 1:
                *filename_in = static_cast<char *>(malloc(strlen(optarg) + 1));
                strcpy(*filename_in, optarg);
                break;
            default:
                rge_errno = RGEERR_BADOPTARGS;
                return 1;
        }
    }

    // Define workdir if undefined.
    char tmpfilename[PATH_MAX];
    sprintf(tmpfilename, "%s", argv[0]);
    if (*work_dir == NULL) {
        *work_dir = static_cast<char *>(malloc(PATH_MAX));
        sprintf(*work_dir, "%s/../root_io", dirname(argv[0]));
    }

    // Define datadir if undefined.
    if (*data_dir == NULL) {
        *data_dir = static_cast<char *>(malloc(PATH_MAX));
        sprintf(*data_dir, "%s/../data", dirname(tmpfilename));
    }

    // Check positional argument.
    if (*filename_in == NULL) {
        rge_errno = RGEERR_NOINPUTFILE;
        return 1;
    }
    if (rge_handle_root_filename(*filename_in, run_no, energy_beam)) return 1;

    return 0;
}

/** Entry point of the program. */
int main(int argc, char **argv) {
    // Handle arguments.
    char *filename_in  = NULL;
    char *work_dir     = NULL;
    char *data_dir     = NULL;
    bool debug         = false;
    lint fmt_nlayers   = 0;
    bool fmt_cut       = false;
    bool save_MC        = false;
    lint n_events      = -1;
    int run_no         = -1;
    double energy_beam = -1;

    int err = handle_args(
            argc, argv, &filename_in, &work_dir, &data_dir, &debug,
            &fmt_nlayers, &fmt_cut, &save_MC, &n_events, &run_no, &energy_beam
    );

    // Run.
    if (rge_errno == RGEERR_UNDEFINED && err == 0) {
        run(
                filename_in, work_dir, data_dir, debug, fmt_nlayers, fmt_cut,
                save_MC, n_events, run_no, energy_beam
        );
    }

    // Free up memory.
    if (filename_in != NULL) free(filename_in);
    if (work_dir    != NULL) free(work_dir);
    if (data_dir    != NULL) free(data_dir);

    // Return errcode.
    return rge_print_usage(USAGE_MESSAGE);
}

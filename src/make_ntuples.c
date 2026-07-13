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
static const uint DC_ID   = 6;

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
static void get_time_path(
        rge_hipobank *scintillator, rge_hipobank *calorimeter, uint pindex,
        double *time_tof, double *path_tof, double *time_cal, double *path_cal
) {
    int    most_precise_lyr = 0;
    *time_tof         = INFINITY;
    *path_tof               = -1.;
    *time_cal         = INFINITY;
    *path_cal               = -1.;

    // Find TOF and path from scintillator.
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
        double path = rge_get_double(scintillator, "path", i);

        // Check FTOF 1B (most precise FTOF layer).
        if (layer == FTOF1B_LYR) {
            most_precise_lyr = FTOF1B_LYR;
            *time_tof = time;
            *path_tof = path;
            break; // Things won't get better than this.
        }

        // Check FTOF 1A.
        else if (layer == FTOF1A_LYR) {
            if (most_precise_lyr == FTOF1A_LYR) continue;
            most_precise_lyr = FTOF1A_LYR;
            *time_tof = time;
            *path_tof = path;
        }

        // Check FTOF 2.
        else if (layer == FTOF2_LYR) {
            // We already have a similar or better hit.
            if (most_precise_lyr != 0) continue;
            most_precise_lyr = FTOF2_LYR;
            *time_tof = time;
            *path_tof = path;
        }
    }

    // Find TOF and path from calorimeters.
    for (uint i = 0; i < calorimeter->nrows; ++i) {
        // Filter out incorrect pindex.
        if (rge_get_uint(calorimeter, "pindex", i) != pindex) continue;

        uint layer  = rge_get_uint(calorimeter, "layer", i);
        double time = rge_get_double(calorimeter, "time", i);
        double path = rge_get_double(calorimeter, "path", i);
        
        // Check PCAL (Calorimeter with the most precise time).
        if (layer == PCAL_LYR) {
            most_precise_lyr = 10 + PCAL_LYR;
            *time_cal = time;
            *path_cal = path;
            break; // Things won't get better than this.
        }

        // Check ECIN.
        else if (layer == ECIN_LYR) {
            if (most_precise_lyr == 10 + ECIN_LYR) continue;
            most_precise_lyr = 10 + ECIN_LYR;
            *time_cal = time;
            *path_cal = path;
        }

        // Check ECOU.
        else if (layer == ECOU_LYR) {
            if (most_precise_lyr != 0) continue;
            most_precise_lyr = 10 + ECOU_LYR;
            *time_cal = time;
            *path_cal = path;
        }
    }
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
 * Get distance on each PCAL side for particle with pindex from PCAL.
 *
 * @param calorimeter : pointer to the calorimeter rge_hipobank.
 * @param pindex      : particle index of the particle we're studying
 * @param PCAL_U      : pointer to double to which we'll write the PCAL_U distance.
 * @param PCAL_V      : pointer to double to which we'll write the PCAL_V distance.
 * @param PCAL_W      : pointer to double to which we'll write the PCAL_W distance.
 * @return            : error code. 0 if successful, 1 otherwise. The function
 *                      only returns 1 if there's an invalid layer in the
 *                      calorimeter bank, suggesting corruption or a change in
 *                      the REC::Calorimeter bank structure.
 */
static int get_pcal_distance(
        rge_hipobank *calorimeter, uint pindex, double *PCAL_U,
        double *PCAL_V, double *PCAL_W
) {
    *PCAL_U = -999;
    *PCAL_V = -999;
    *PCAL_W = -999;

    for (uint i = 0; i < calorimeter->nrows; ++i) {
        if (rge_get_uint(calorimeter,"pindex",i) != pindex) continue;

        int layer = rge_get_int   (calorimeter, "layer",  i);

        if (layer == PCAL_LYR){
            *PCAL_U = rge_get_double(calorimeter, "lu", i);
            *PCAL_V = rge_get_double(calorimeter, "lv", i);
            *PCAL_W = rge_get_double(calorimeter, "lw", i);
        }
        else if (layer == ECOU_LYR || layer == ECIN_LYR) continue;
        else {
            rge_errno = RGEERR_INVALIDCALLAYER;
            return 1;
        }
    }

    return 0;
}

/**
 * Get distance desitance to edge for each DC region for particle with pindex from Traj.
 *
 * @param trajectory  : pointer to the trajectory rge_hipobank.
 * @param pindex      : particle index of the particle we're studying
 * @param DC_R1_edge  : pointer to double to which we'll write the DC_R1_edge distance.
 * @param DC_R1_edge  : pointer to double to which we'll write the DC_R2_edge distance.
 * @param DC_R1_edge  : pointer to double to which we'll write the DC_R3_edge distance.
 * @return            : error code. 0 if successful, 1 otherwise.
 */
static int get_dc_edge(
        rge_hipobank *trajectory, uint pindex, double *DC_R1_edge,
        double *DC_R2_edge, double *DC_R3_edge
) {
    *DC_R1_edge = -999;
    *DC_R2_edge = -999;
    *DC_R3_edge = -999;

    for (uint i = 0; i < trajectory->nrows; ++i) {
        if (rge_get_uint(trajectory,"pindex",i) != pindex ||
            rge_get_int(trajectory,"detector",i) != DC_ID) continue;

        int layer = rge_get_int(trajectory, "layer",  i);

        if      (layer == DC_R1_LYR) *DC_R1_edge = rge_get_double(trajectory, "edge", i);
        else if (layer == DC_R2_LYR) *DC_R2_edge = rge_get_double(trajectory, "edge", i);
        else if (layer == DC_R3_LYR) *DC_R3_edge = rge_get_double(trajectory, "edge", i);
        else {
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
        bool fmt_switch, bool fmt_cut, bool save_MC, lint n_events, int run_no,
        double energy_beam
) {
    //// Get sampling fraction.
    //char sampling_fraction_file[PATH_MAX];
    //if (run_no / 1000 != 999) {
    //    // Input file is data.
    //    sprintf(
    //            sampling_fraction_file, "%s/sf_params_%06d.txt",
    //            data_dir, run_no
    //    );
    //}
    //else {
    //    // Input file is simulation.
    //    sprintf(sampling_fraction_file, "%s/sf_params_mc.txt", data_dir);
    //}
    //double sampling_fraction_params[RGE_NSECTORS][RGE_NSFPARAMS][2];
    //if (access(sampling_fraction_file, F_OK) != 0) {
    //    // No sampling fraction file for this run, we need to extract it.
    //    printf(
    //            "No sampling fraction data found for run %d. Running "
    //            "extract_sf().\n", run_no
    //    );
    //    if (rge_extract_sf(filename_in, work_dir, data_dir, n_events, run_no)) {
    //        return 1;
    //    }
    //    printf("Done!\n\n");
    //    rge_errno = RGEERR_UNDEFINED;
    //}
    //if (rge_get_sf_params(sampling_fraction_file, sampling_fraction_params)) {
    //    return 1;
    //}

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
    if ( fmt_switch || save_MC)
    {
        for( int i = 0; i < branch_list->GetEntries(); i++)
        {
            std::string branch_name = branch_list->At(i)->GetName();
            if ( fmt_switch && branch_name.find(RGE_RECFTRACK) != std::string::npos)
                found_fmt = true;
            else if ( save_MC && branch_name.find(RGE_MCPARTICLE) != std::string::npos)
                found_mc = true;
            // Ending search early conditions
            if ( (found_fmt && found_mc) || (found_fmt && !save_MC) || (found_mc && !fmt_switch) )
                break;
        }
    }

    // If fmt_switch is true, check that REC::FTrack bank exists.
    if (fmt_switch && !found_fmt) {
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
    std::cout<<vars_string<<std::endl;
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
    rge_hipobank bevent = rge_hipobank_init(RGE_EVENT,           tree_in);
    rge_hipobank bpart  = rge_hipobank_init(RGE_RECPARTICLE,     tree_in);
    rge_hipobank btrk   = rge_hipobank_init(RGE_RECTRACK,        tree_in);
    rge_hipobank btraj  = rge_hipobank_init(RGE_RECTRAJ,         tree_in);
    rge_hipobank bcal   = rge_hipobank_init(RGE_RECCALORIMETER,  tree_in);
    rge_hipobank bchkv  = rge_hipobank_init(RGE_RECCHERENKOV,    tree_in);
    rge_hipobank bsci   = rge_hipobank_init(RGE_RECSCINTILLATOR, tree_in);
    // Optional hipo banks
    rge_hipobank bfmt, bmcpart, bmcevent;
    if (fmt_switch)
        bfmt  = rge_hipobank_init(RGE_RECFTRACK, tree_in);
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
        rge_get_entries(&bevent, tree_in, event);
        rge_get_entries(&bpart,  tree_in, event);
        rge_get_entries(&btrk,   tree_in, event);
        rge_get_entries(&btraj,  tree_in, event);
        rge_get_entries(&bcal,   tree_in, event);
        rge_get_entries(&bchkv,  tree_in, event);
        rge_get_entries(&bsci,   tree_in, event);
        if (fmt_switch) rge_get_entries(&bfmt, tree_in, event);
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

            bool trigger_exist  = false;
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
                
                rge_particle first_elec, mc_p;
                mc_p = mc_particle_init(pid, vx, vy, vz, px, py, pz);

                if (pos==0 && pid==11){
                    mc_p.is_trigger = true;
                    first_elec = mc_p;
                    trigger_exist = true;
                }
                if (mc_rge_fill_ntuples_arr(
                    arr, mc_p, first_elec, run_no, event, vt, npart, atarget, ztarget,
                    ptarget, pbeam, btype, ebeam, targetid, processid, weight
                )) return 1;

                if (trigger_exist) MC_tree_out->Fill(arr);
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
        uint entry_counter = btrk.nrows;
        // if (fmt_switch)
        //     entry_counter = bfmt.nrows; 
        for (uint pos = 0; pos < entry_counter; ++pos) {
            int fmt_nlayers = 0;
            if (fmt_switch) {
                int fmt_nlayers = rge_get_uint(&bfmt, "NDF", pos);
            }
            
            uint pindex = rge_get_uint(&btrk, "pindex", pos);
            if (fmt_switch)
                pindex = rge_get_uint(&bfmt, "pindex", pos);

            // Get reconstructed particle from DC and from FMT.
            part_trigger = rge_particle_init(
                &bpart, &btrk, &bfmt, pos, fmt_nlayers
            );

            // Skip particle if it doesn't fit requirements.
            if (!part_trigger.is_valid) continue;

            // Cut triggers outside of FMT's active region.
            //if (fmt_cut) {
            //    int result = apply_fmtgeomtry_cut(&part_trigger);
            //    if (result == 1) continue;
            //    if (result == 2) return 1;
            //}

            // Get energy deposited in calorimeters.
            double energy_PCAL, energy_ECIN, energy_ECOU;
            if (get_deposited_energy(
                    &bcal, pindex, &energy_PCAL, &energy_ECIN, &energy_ECOU
            )) return 1;

            //Get PCAL distances to each side
            double PCAL_U, PCAL_V, PCAL_W;
            if (get_pcal_distance(
                    &bcal, pindex, &PCAL_U, &PCAL_V, &PCAL_W
            )) return 1;

            //Get DC edge distances for each region
            double DC_R1_edge, DC_R2_edge, DC_R3_edge;
            if (get_dc_edge(
                    &btraj, pindex, &DC_R1_edge, &DC_R2_edge, &DC_R3_edge
            )) return 1;

            // Get number of photoelectrons from Cherenkov counters.
            int nphe_HTCC, nphe_LTCC;
            if (count_photoelectrons(&bchkv, pindex, &nphe_HTCC, &nphe_LTCC))
                return 1;

            // Get time of flight from scintillators or calorimeters.
            double time_tof, path_tof, time_cal, path_cal;
            get_time_path(&bsci, &bcal, pindex, &time_tof, &path_tof, &time_cal, &path_cal);

            // Get miscellaneous data.
            int status  = rge_get_double(&bpart, "status", pindex);
            double chi2 = rge_get_double(&btrk,  "chi2",   pos);
            double ndf  = rge_get_double(&btrk,  "NDF",    pos);

            //Get start time of the event
            double start_time = rge_get_double(&bevent, "startTime", 0);

            // Assign PID.
            if (rge_set_pid(
                    &part_trigger, rge_get_double(&bpart, "pid", pindex), status
            )) return 1;

            // Skip particle if its not the trigger electron.
            if (!part_trigger.is_trigger) continue;

            // Fill TNtuple with trigger electron information.
            Float_t arr[RGE_VARS_SIZE];
            if (rge_fill_ntuples_arr(
                    arr, part_trigger, part_trigger, run_no, event, start_time,
                    status, energy_beam, chi2, ndf, fmt_nlayers, energy_PCAL, energy_ECIN,
                    energy_ECOU, time_tof, path_tof, time_cal, path_cal,
                    nphe_LTCC, nphe_HTCC, PCAL_U, PCAL_V, PCAL_W, DC_R1_edge, DC_R2_edge, DC_R3_edge
            )) return 1;

            tree_out->Fill(arr);

            // Fill out trigger electron data and end loop.
            trigger_exist  = true;
            trigger_pindex = pindex;
            trigger_pos    = pos;
            trigger_tof    = time_tof;
            break;
        }

        // Skip events without a trigger electron.
        if (!trigger_exist) continue;
        ++trigger_counter;

    // ==========================================================
        // NUEVO BLOQUE: Recuperacion de Fotones (Neutros)
        // Insertar esto justo despues del bucle "for (uint pos = 0; pos < btrk.nrows...)"
        // y antes de cerrar el bucle de eventos "for (lint event = 0...)"
        // ==========================================================

        for (uint pindex = 0; pindex < bpart.nrows; ++pindex) {
            
            int pid = rge_get_double(&bpart, "pid", pindex);
            int charge = rge_get_double(&bpart, "charge", pindex); // Opcional si confías en el PID
            int fmt_nlayers = 0;

            if (fmt_switch) {
                int fmt_nlayers = rge_get_uint(&bfmt, "NDF", pindex);
            }

            // 1. FILTRO DE FOTONES
            // Solo procesar si es PID 22 (Foton)
            if (pid != 22) continue;

            // 2. FILTRO DE ENERGIA (Mineeva sugiere > 0.2 o 0.3 GeV)
            // Obtenemos momento px, py, pz del banco de partículas
            double px = rge_get_double(&bpart, "px", pindex);
            double py = rge_get_double(&bpart, "py", pindex);
            double pz = rge_get_double(&bpart, "pz", pindex);
            double p_mom = sqrt(px*px + py*py + pz*pz);
            
            //if (p_mom < 0.1) continue; // Descartar ruido de baja energía

            // 3. OBTENER ENERGIA DEL CALORIMETRO
            // Necesitamos esto para las correcciones de Mineeva
            double energy_PCAL, energy_ECIN, energy_ECOU;
            // Nota: get_deposited_energy usa pindex, asi que funciona para neutros
            if (get_deposited_energy(&bcal, pindex, &energy_PCAL, &energy_ECIN, &energy_ECOU)) 
                continue; 

            //Get PCAL distances to each side
            double PCAL_U, PCAL_V, PCAL_W;
            if (get_pcal_distance(
                    &bcal, pindex, &PCAL_U, &PCAL_V, &PCAL_W
            )) return 1;


            // 4. CONSTRUIR LA PARTICULA 'MANUALMENTE'
            // Como no hay 'pos' de track, no podemos usar rge_particle_init.
            // Creamos una estructura dummy y la llenamos manualmente.
            rge_particle photon_part;
            photon_part.pid = pid;
            photon_part.px = px;
            photon_part.py = py;
            photon_part.pz = pz;
            photon_part.vx = rge_get_double(&bpart, "vx", pindex);
            photon_part.vy = rge_get_double(&bpart, "vy", pindex);
            photon_part.vz = rge_get_double(&bpart, "vz", pindex);
            photon_part.vt = rge_get_double(&bpart, "vt", pindex);

            photon_part.charge = 0;
            photon_part.beta = rge_get_double(&bpart, "beta", pindex);
 
            // Importante: marcar como valida
            photon_part.is_valid = true; 

            // 5. IDENTIFICACION DE FOTONES (Cortes de tiempo/beta)
            // Mineeva usa cortes de beta. Obtenemos el TOF.
            double time_tof, path_tof, time_cal, path_cal;
            get_time_path(&bsci, &bcal, pindex, &time_tof, &path_tof, &time_cal, &path_cal);
            
            // 6. GUARDAR EN EL TREE
            // Usamos valores 'dummy' (0 o -1) para variables que solo tienen las trazas (chi2, ndf, etc)
            Float_t arr[RGE_VARS_SIZE];
            
            // status del banco de particulas
            int status = rge_get_double(&bpart, "status", pindex);

            //Get start time of the event
            double start_time = rge_get_double(&bevent, "startTime", 0);

            // Llamamos a la funcion de llenado.
            // Nota: Pasamos photon_part como la particula, y part_trigger (el electron) como referencia
            if (rge_fill_ntuples_arr(
                    arr, photon_part, part_trigger, run_no, event, start_time, status,
                    energy_beam, -100.0, -100.0, fmt_nlayers, energy_PCAL, energy_ECIN, energy_ECOU, time_tof,
                    path_tof, time_cal, path_cal, 0, 0, PCAL_U, PCAL_V, PCAL_W, -999, -999, -999 // No Cherenkov para fotones
            )) continue;

            tree_out->Fill(arr);
        }

    //-----------------------------------------------------
    //ACA TERMINA EL NUEVO BLOQUE
    //-----------------------------------------------------

        // Processing particles.
        for (uint pos = 0; pos < entry_counter; ++pos) {
            uint pindex = rge_get_uint(&btrk, "pindex", pos);
            int fmt_nlayers = 0;

            if (fmt_switch){
                // pindex = rge_get_uint(&bfmt, "pindex", pos);
                fmt_nlayers = rge_get_uint(&bfmt, "NDF", pos);
            }
            

            // Avoid double-counting the trigger electron.
            if (trigger_pindex == pindex && trigger_pos == pos) {
                continue;
            }

            // Omit particles with pid=0
            if (rge_get_double(&bpart, "pid", pindex)==0){
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

            //Get PCAL distances to each side
            double PCAL_U, PCAL_V, PCAL_W;
            if (get_pcal_distance(
                    &bcal, pindex, &PCAL_U, &PCAL_V, &PCAL_W
            )) return 1;

            //Get DC edge distances for each region
            double DC_R1_edge, DC_R2_edge, DC_R3_edge;
            if (get_dc_edge(
                    &btraj, pindex, &DC_R1_edge, &DC_R2_edge, &DC_R3_edge
            )) return 1;

            // Get Cherenkov counters data.
            int nphe_HTCC, nphe_LTCC;
            if (count_photoelectrons(&bchkv, pindex, &nphe_HTCC, &nphe_LTCC))
                return 1;

            // Get time-of-flight (tof).
            double time_tof, path_tof, time_cal, path_cal;
            get_time_path(&bsci, &bcal, pindex, &time_tof, &path_tof, &time_cal, &path_cal);

            // Get miscellaneous data.
            int status  = rge_get_double(&bpart, "status", pindex);
            double chi2 = rge_get_double(&btrk,  "chi2",   pos);
            double ndf  = rge_get_double(&btrk,  "NDF",    pos);

            //Get start time of the event
            double start_time = rge_get_double(&bevent, "startTime", 0);

            // Assign PID.
            if (rge_set_pid(
                    &part, rge_get_double(&bpart, "pid", pindex), status
            )) return 1;

            // Fill TNtuples. If adding new variables, check their order in
            //     RGE_VARS.
            Float_t arr[RGE_VARS_SIZE];
            if (rge_fill_ntuples_arr(
                    arr, part, part_trigger, run_no, event, start_time, status,
                    energy_beam, chi2, ndf, fmt_nlayers, energy_PCAL, energy_ECIN, energy_ECOU,
                    time_tof, path_tof, time_cal, path_cal, nphe_LTCC, nphe_HTCC, PCAL_U,
                    PCAL_V, PCAL_W, DC_R1_edge, DC_R2_edge, DC_R3_edge
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
    if (!fmt_switch) {
        sprintf(filename_out, "%s/ntuples_dc_%06d.root", work_dir, run_no);
    }
    else {
        sprintf(
                filename_out, "%s/ntuples_fmt_%06d.root", work_dir, run_no
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
        char **data_dir, bool *debug, bool *fmt_switch, bool *fmt_cut,
        bool *save_MC, lint *n_events, int *run_no, double *energy_beam
) {
    // Handle arguments.
    int opt;
    while ((opt = getopt(argc, argv, "-hDfcsn:w:d:")) != -1) {
        switch (opt) {
            case 'h':
                rge_errno = RGEERR_USAGE;
                return 1;
            case 'D':
                *debug = true;
                break;
            case 'f':
                // if (rge_process_fmtnlayers(fmt_nlayers, optarg)) return 1; // Just checks that fmt_nlayers is valid, doesn't set fmt_cut
                // break;
            // case 'l':
                *fmt_switch = true;
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
    bool fmt_switch    = false;
    bool fmt_cut       = false;
    bool save_MC        = false;
    lint n_events      = -1;
    int run_no         = -1;
    double energy_beam = -1;

    int err = handle_args(
            argc, argv, &filename_in, &work_dir, &data_dir, &debug, &fmt_switch,
            &fmt_cut, &save_MC, &n_events, &run_no, &energy_beam
    );

    // Run.
    if (rge_errno == RGEERR_UNDEFINED && err == 0) {
        run(
                filename_in, work_dir, data_dir, debug, fmt_switch, fmt_cut,
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

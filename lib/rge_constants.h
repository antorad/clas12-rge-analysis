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

#ifndef RGE_CONSTANTS
#define RGE_CONSTANTS

// --+ preamble +---------------------------------------------------------------
// typedefs.
typedef unsigned int uint;
typedef long unsigned int luint;
typedef long int lint;

// --+ structs +----------------------------------------------------------------
/**
 * Struct containing the address in RGE_VARS and name of a variable.
 * NOTE. This approach is error-prone and hard to work with -- I would prefer to
 *     use an std::map<const char *, double, cmp_str>, but ROOT's dumb TNTuple
 *     object constructor rules make this approach uglier in comparison.
 *         -Bruno.
 *
 * @param addr : address of the variable in the RGE_VARS array.
 * @param name : name of the variable in ROOT's string format.
 */
typedef struct {
    int addr;
    const char *name;
} RGE_VAR;

// --+ library +----------------------------------------------------------------
/** Data tree name used by various programs. */
#define RGE_TREENAMEDATA "DT"
#define RGE_MCTREENAME "MC"
/** Detector constants. */
#define RGE_NSECTORS     6 /** # of CLAS12 sectors. */
#define RGE_NSFPARAMS    4 /** # of sampling fraction parameters. */

/** Cuts (geometric, fiducial, SIDIS, etc.). */
#define RGE_Q2CUT        1.   /** Q2 of trigger must be over this value. */
#define RGE_W2CUT        4.   /** W2 of trigger must be over this value. */
#define RGE_YBCUT        0.85 /** Yb of trigger must be under this value. */
#define RGE_CHI2NDFCUT  15    /** Chi2/NDF must be below this value. */
#define RGE_VXVYCUT      4    /** sqrt(vx^2 + vy^2) must be below this value. */
#define RGE_VZLOWCUT   -40    /** vz must be above this value. */
#define RGE_VZHIGHCUT   26.1197 /** vz must be below this. */

/** Variable array data. */
#define RGE_VARS_SIZE 49
extern const char *RGE_VARS[RGE_VARS_SIZE];
#define RGE_MC_VARS_SIZE 34
extern const char *RGE_MC_VARS[RGE_MC_VARS_SIZE];

/** Metadata variables. */
const RGE_VAR RGE_RUNNO   = {.addr = 0, .name = "run_num"};
const RGE_VAR RGE_EVENTNO = {.addr = 1, .name = "event_num"};
const RGE_VAR RGE_BEAME   = {.addr = 2, .name = "E_beam"};
const RGE_VAR RGE_START   = {.addr = 3, .name = "start_time"};

/** Particle variables. */
const RGE_VAR RGE_PID    = {.addr =  4, .name = "pid"};
const RGE_VAR RGE_CHARGE = {.addr =  5, .name = "charge"}; // Unit: e
const RGE_VAR RGE_STATUS = {.addr =  6, .name = "status"};
const RGE_VAR RGE_MASS   = {.addr =  7, .name = "mass"}; // Unit: GeV
const RGE_VAR RGE_VX     = {.addr =  8, .name = "vx"}; // Unit: cm
const RGE_VAR RGE_VY     = {.addr =  9, .name = "vy"}; // Unit: cm
const RGE_VAR RGE_VZ     = {.addr = 10, .name = "vz"}; // Unit: cm
const RGE_VAR RGE_VT     = {.addr = 11, .name = "vt"}; // Unit: ns
const RGE_VAR RGE_PX     = {.addr = 12, .name = "px"}; // Unit: GeV
const RGE_VAR RGE_PY     = {.addr = 13, .name = "py"}; // Unit: GeV
const RGE_VAR RGE_PZ     = {.addr = 14, .name = "pz"}; // Unit: GeV
const RGE_VAR RGE_P      = {.addr = 15, .name = "p"}; // Unit: GeV
const RGE_VAR RGE_THETA  = {.addr = 16, .name = "theta"}; // Unit: rad
const RGE_VAR RGE_PHI    = {.addr = 17, .name = "phi"};  // Unit: rad
const RGE_VAR RGE_BETA   = {.addr = 18, .name = "beta"};
const RGE_VAR RGE_TRIGGERSTATUS   = {.addr = 19, .name = "trigger_status"}; // 0 if particle is not the trigger electron, 1 if particle is the trigger electron
const RGE_VAR RGE_SECTOR = {.addr = 20, .name = "sector"}; // Sector name

/** Tracking variables. */
const RGE_VAR RGE_CHI2 = {.addr = 21, .name = "chi2"};
const RGE_VAR RGE_NDF  = {.addr = 22, .name = "NDF"};

/** Trajectory variables. */
const RGE_VAR RGE_DCR1EDGE = {.addr = 23, .name = "DC_R1_edge"}; // Unit: cm
const RGE_VAR RGE_DCR2EDGE = {.addr = 24, .name = "DC_R2_edge"}; // Unit: cm
const RGE_VAR RGE_DCR3EDGE = {.addr = 25, .name = "DC_R3_edge"}; // Unit: cm

/** Calorimeter variables. */
const RGE_VAR RGE_PCALE = {.addr = 26, .name = "E_PCAL"}; // Unit: GeV 
const RGE_VAR RGE_ECINE = {.addr = 27, .name = "E_ECIN"}; // Unit: GeV
const RGE_VAR RGE_ECOUE = {.addr = 28, .name = "E_ECOU"}; // Unit: GeV
const RGE_VAR RGE_TOTE  = {.addr = 29, .name = "E_total"}; // Unit: GeV
const RGE_VAR RGE_PCALU    = {.addr = 30, .name = "PCAL_U"}; // Unit: GeV
const RGE_VAR RGE_PCALV    = {.addr = 31, .name = "PCAL_V"}; // Unit: cm
const RGE_VAR RGE_PCALW    = {.addr = 32, .name = "PCAL_W"}; // Unit: cm
const RGE_VAR RGE_TIMECAL = {.addr = 33, .name = "time_cal"}; // Unit: ns
const RGE_VAR RGE_PATHCAL = {.addr = 34, .name = "path_cal"}; // Unit: cm

/** Scintillator variables. */
const RGE_VAR RGE_TIMETOF = {.addr = 35, .name = "time_tof"}; // Unit: ns
const RGE_VAR RGE_PATHTOF = {.addr = 36, .name = "path_tof"}; // Unit: cm

/** Cherenkov counters variables. */
const RGE_VAR RGE_NPHELTCC = {.addr = 37, .name = "Nphe_LTCC"};
const RGE_VAR RGE_NPHEHTCC = {.addr = 38, .name = "Nphe_HTCC"};

/** DIS variables. */
const RGE_VAR RGE_Q2 = {.addr = 39, .name = "Q2"}; // Unit: GeV^2
const RGE_VAR RGE_NU = {.addr = 40, .name = "nu"}; // Unit: GeV
const RGE_VAR RGE_XB = {.addr = 41, .name = "x_bjorken"};
const RGE_VAR RGE_YB = {.addr = 42, .name = "y_bjorken"};
const RGE_VAR RGE_W2 = {.addr = 43, .name = "W2"}; // Unit: GeV

/** SIDIS variables. */
const RGE_VAR RGE_ZH      = {.addr = 44, .name = "z_h"};
const RGE_VAR RGE_PT2     = {.addr = 45, .name = "p_T2"}; // Unit: GeV^2
const RGE_VAR RGE_PL2     = {.addr = 46, .name = "p_L2"}; // Unit: GeV^2
const RGE_VAR RGE_PHIPQ   = {.addr = 47, .name = "phi_PQ"}; // Unit: rad
const RGE_VAR RGE_THETAPQ = {.addr = 48, .name = "theta_PQ"}; // Unit: rad

/** MC variables */
const RGE_VAR RGE_MC_RUNNO   = {.addr = 0, .name = "run_num"};
const RGE_VAR RGE_MC_EVENTNO = {.addr = 1, .name = "event_num"};
const RGE_VAR RGE_MC_PID = {.addr = 2, .name = "pid"};
const RGE_VAR RGE_MC_PX  = {.addr = 3, .name = "px"};
const RGE_VAR RGE_MC_PY  = {.addr = 4, .name = "py"};
const RGE_VAR RGE_MC_PZ  = {.addr = 5, .name = "pz"};
const RGE_VAR RGE_MC_VX  = {.addr = 6, .name = "vx"};
const RGE_VAR RGE_MC_VY  = {.addr = 7, .name = "vy"};
const RGE_VAR RGE_MC_VZ  = {.addr = 8, .name = "vz"};
const RGE_VAR RGE_MC_VT  = {.addr = 9, .name = "vt"};
const RGE_VAR RGE_MC_NPART = {.addr = 10, .name = "vt"};
const RGE_VAR RGE_MC_ATARGET = {.addr = 11, .name = "atarget"};
const RGE_VAR RGE_MC_ZTARGET = {.addr = 12, .name = "ztarget"};
const RGE_VAR RGE_MC_PTARGET = {.addr = 13, .name = "ptarget"};
const RGE_VAR RGE_MC_PBEAM = {.addr = 14, .name = "pbeam"};
const RGE_VAR RGE_MC_BTYPE = {.addr = 15, .name = "btype"};
const RGE_VAR RGE_MC_EBEAM = {.addr = 16, .name = "ebeam"};
const RGE_VAR RGE_MC_TARGETID = {.addr = 17, .name = "targetid"};
const RGE_VAR RGE_MC_PROCESSID = {.addr = 18, .name = "processid"};
const RGE_VAR RGE_MC_WEIGHT = {.addr = 19, .name = "weight"};
/** MC CALCULATED variables */
const RGE_VAR RGE_MC_P      = {.addr = 20, .name = "p"}; // Unit: GeV
const RGE_VAR RGE_MC_THETA  = {.addr = 21, .name = "theta"}; // Unit: rad
const RGE_VAR RGE_MC_PHI    = {.addr = 22, .name = "phi"};  // Unit: rad
const RGE_VAR RGE_MC_BETA   = {.addr = 23, .name = "beta"};
/** DIS_MC variables. */
const RGE_VAR RGE_MC_Q2 = {.addr = 24, .name = "Q2"}; // Unit: GeV^2
const RGE_VAR RGE_MC_NU = {.addr = 25, .name = "nu"}; // Unit: GeV
const RGE_VAR RGE_MC_XB = {.addr = 26, .name = "x_bjorken"};
const RGE_VAR RGE_MC_YB = {.addr = 27, .name = "y_bjorken"};
const RGE_VAR RGE_MC_W2 = {.addr = 28, .name = "W2"}; // Unit: GeV
/** SIDIS_MC variables. */
const RGE_VAR RGE_MC_ZH      = {.addr = 29, .name = "z_h"};
const RGE_VAR RGE_MC_PT2     = {.addr = 30, .name = "p_T2"}; // Unit: GeV^2
const RGE_VAR RGE_MC_PL2     = {.addr = 31, .name = "p_L2"}; // Unit: GeV^2
const RGE_VAR RGE_MC_PHIPQ   = {.addr = 32, .name = "phi_PQ"}; // Unit: rad
const RGE_VAR RGE_MC_THETAPQ = {.addr = 33, .name = "theta_PQ"}; // Unit: rad

#endif

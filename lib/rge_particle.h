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

#ifndef RGE_PARTICLE
#define RGE_PARTICLE

// --+ preamble +---------------------------------------------------------------
// C.
#include <math.h>
#include <stdlib.h>

// rge-analysis.
#include "rge_constants.h"
#include "rge_hipo_bank.h"
#include "rge_math_utils.h"
#include "rge_pid_utils.h"

// typedefs.
typedef unsigned int uint;
typedef long unsigned int luint;
typedef long int lint;

// --+ structs +----------------------------------------------------------------
/**
 * Data associated to a detected particle in CLAS12.
 *
 *     IDENTIFIER BOOLEANS.
 * @param is_valid   : boolean identifying if the particle is valid.
 * @param is_trigger : boolean identifying if the particle is the trigger
 *                     electron.
 * @param is_hadron  : boolean identifying if the particle is a hadron.
 *
 *     EVENT BUILDER VARIABLES.
 * @param pid        : PID of the particle.
 * @param charge     : charge of the particle.
 * @param sector     : CLAS12 sector where the particle was detected.
 * @param beta       : beta (v/c) of the particle.
 *
 *     TRACKING VARIABLES.
 * @param vx         : vertex x position coordinate of the particle.
 * @param vy         : vertex y position coordinate of the particle.
 * @param vz         : vertex z position coordinate of the particle.
 * @param vt         : vertex t time of the particle.
 * @param px         : vertex x momentum coordinate of the particle.
 * @param py         : vertex y momentum coordinate of the particle.
 * @param pz         : vertex z momentum coordinate of the particle.
 *
 *     VARIABLES DERIVED FROM PID.
 * @param mass       : mass of the particle.
 */
typedef struct {
    bool is_valid, is_trigger, is_hadron;
    int pid, charge, sector;
    double beta, vx, vy, vz, vt, px, py, pz, mass;
} rge_particle;

// --+ internal +---------------------------------------------------------------
/** Maximum beta allowed to assign PID 2212 (neutron). */
static const double NEUTRON_MAXBETA     = .9;
/** Minimum energy allowed to assign PID 22 (photon). */
static const double PHOTON_MINENERGY    = 1e-9;
/** Number of photoelectrons required to assume that particle passed HTCC. */
static const int    HTCC_NPHE_CUT       = 2;
/** Number of photoelectrons required to assume that particle passed LTCC. */
static const int    LTCC_NPHE_CUT       = 2;
/** Minimum energy deposited in PCAL to assign PID 11 or -11 (electron). */
static const double MIN_PCAL_ENERGY     = .060;
/** Maximum ECAL sampling fraction sigma to assign PID 11 or -11 (electron). */
static const double E_SF_NSIGMA         = 5.0;
/** Momentum (GeV) required to consider particle crossing HTCC to be a pion. */
static const double HTCC_PION_THRESHOLD = 4.9 ;

/**
 * Initialize a dummy particle. All booleans are defined to false, and all other
 *     attributes are left undefined.
 */
static rge_particle particle_init();

/**
 * Initialize a new particle using data from the input parameters. Particle is
 *     declared as valid, but PID and mass remain undetermined -- and thus it is
 *     not yet defined if particle is a trigger electron or a hadron.
 */
static rge_particle particle_init(
        int charge, double beta, int sector, double vx, double vy, double vz,
        double vt, double px, double py, double pz
);

/**
 * Assign PID to a neutral particle.
 *   * PID 2112 (neutron) is assigned if particle's beta is below
 *     NEUTRON_MAXBETA.
 *   * PID 22 (photon) is assigned if particle's total energy is above
 *     PHOTON_MINENERGY.
 *   * PID 0 (undefined) is assigned if none of this criteria are met.
*/
static int assign_neutral_pid(double energy, double beta);

/**
 * Check if a particle satisfies all requirements to be considered an electron
 *     or positron. Requirements are taken from the Event Builder (EB), and are:
 *   * Total deposited energy (total_energy) must be above 0.
 *   * Total particle vertex momentum (p) must be above 0.
 *   * Number of HTCC photoelectrons (htcc_nphe) must be greater than
 *     HTCC_NPHE_CUT.
 *   * Energy deposited in PCAL (pcal_energy) must be greater than
 *     MIN_PCAL_ENERGY.
 *   * ECAL sampling fraction should be below threshold (E_SF_NSIGMA).
 *
 * If all requirements are met, the function returns true. Otherwise, it returns
 *     false.
 */
static bool is_electron(
        double total_energy, double pcal_energy, double htcc_nphe, double p,
        double pars[RGE_NSFPARAMS][2]
);

/**
 * Based on criteria defined in rge_set_pid, match PID hypothesis with available
 * checks.
 *
 * @param pid                 : pointer to int where to write found PID.
 * @param hypothesis          : PID hypothesis to check.
 * @param recon_match         : True if hypothesis matches reconstruction PID.
 * @param electron_check      : True if particle passed is_electron() check.
 * @param htcc_signal_check   : True if number of photoelectrons is above
 *                              HTCC_NPHE_CUT.
 * @param htcc_pion_threshold : True if momentum is above HTCC_PION_THRESHOLD.
 * @return                    : PID if hypothesis matches criteria, 0 otherwise.
 */
static int match_pid(
        int *pid, int hypothesis, bool recon_match, bool electron_check,
        bool htcc_signal_check, bool htcc_pion_threshold
);

/** Compute theta angle in lab frame from the vertex momentum of a particle. */
static double theta_lab(rge_particle particle);

/** Compute phi angle in lab frame from the vertex momentum of a particle. */
static double phi_lab(rge_particle p);

/** Compute momentum magnitude from its components. */
static double momentum(rge_particle p);

/** Compute Q^2 from beam energy, particle momentum, and theta angle. */
static double Q2(rge_particle p, double beam_E);

/** Compute nu from beam energy and total momentum. */
static double nu(rge_particle p, double beam_E);

/** Compute x_bjorken from beam energy, particle momentum, and theta angle. */
static double Xb(rge_particle p, double beam_E);

/** Calculate y_bjorken from beam energy and nu. */
static double Yb(rge_particle p, double beam_E);

/** Compute the invariant mass of the electron-nucleon interaction. */
static double W(rge_particle p, double beam_E);

/** Compute the squared invariant mass of the electron-nucleon interaction. */
static double W2(rge_particle p, double bE);

/** Compute the polar angle of a particle p wrt the virtual photon direction. */
static double theta_pq(rge_particle p, rge_particle e, double bE);

/** Compute the azimuthal angle of a particle p wrt the virtual photon. */
static double phi_pq(rge_particle p, rge_particle e, double bE);

/** Compute the cosine of theta_PQ. */
static double cos_theta_pq(rge_particle p, rge_particle e, double bE);

/** Compute the squared momentum transverse to the virtual photon. */
static double Pt2(rge_particle p, rge_particle e, double bE);

/** Compute the squared momentum longitudinal to the virtual photon. */
static double Pl2(rge_particle p, rge_particle e, double bE);

/**
 * Compute the fraction of the virtual photon's energy taken by the produced
 *     particle in the lab frame.
 */
static double zh(rge_particle p, rge_particle e, double bE);

/**
 * Compute beta for the MC particle. In case the aprticle is not
 *     found in the PID_MAP. It gives beta=-1.
 */
static double beta(rge_particle p);

// --+ library +----------------------------------------------------------------
/**
  * Initialize a new particle from the REC::Particle, REC::TRACK, and
  *     FMT::Tracks banks. If fmt_nlayers == 0, will only use DC tracking data.
  *     If it is 2 or 3, it will use DC+FMT tracking data, restricting the
  *     number of FMT layers the particle passed to a number equal or larger
  *     than fmt_nlayers.
  *
  * @param particle    : pointer to the particle rge_hipobank.
  * @param track       : pointer to the track rge_hipobank to get DC tracks.
  * @param fmttrack    : pointer to the fmt tracks hipobank to get FMT tracks.
  * @param pos         : position of the particle at the track class.
  * @param fmt_nlayers : number of FMT layers required to make a particle.
  */
rge_particle rge_particle_init(
        rge_hipobank *particle, rge_hipobank *track, rge_hipobank *fmttrack,
        uint pos, lint fmt_nlayers
);

/**
 * Set PID from all available information. This functions mimics PIDMatch from
 *     the EB (Event Builder) CLAS12 reconstruction engine.
 *
 * @param particle     : instance of rge_particle for which to find the PID.
 * @param recon_pid    : PID defined by the EB engine.
 * @param status       : status variable defined in the REC::Particle bank. Used
 *                       to check for the trigger electron.
 * @param total_energy : Total deposited energy in ECIN, ECOU, and PCAL.
 * @param pcal_energy  : Deposited energy in PCAL.
 * @param htcc_nphe    : Number of photoelectrons generated in HTCC.
 * @param sf_params    : Array containing the sampling fraction parameters for
 *                       the particle's sector.
 * @return             : error code.
 */
int rge_set_pid(
        rge_particle *particle, int recon_pid, int status, double total_energy,
        double pcal_energy, int htcc_nphe, int ltcc_nphe,
        double sf_params[RGE_NSFPARAMS][2]
);

/**
 * Fill array to be stored in ntuples_%06d.root file. Array is of constant size
 *     RGE_VARS_SIZE, and the order of variables can be seen in constants.h.
 */
int rge_fill_ntuples_arr(
        Float_t *arr, rge_particle p, rge_particle e, int run_no, int evn,
        int status, double beam_E, float chi2, float ndf, double pcal_energy,
        double ecin_E, double ecou_E, double tof, double path, int nphe_ltcc,
        int nphe_htcc, double PCAL_U, double PCAL_V, double PCAL_W,
        double DC_R1_edge, double DC_R2_edge, double DC_R3_edge
);

/**
 * Initialize a new MC particle using data from the input parameters. Particle is
 *     declared as valid.
 */
rge_particle mc_particle_init(
        int pid, double vx, double vy, double vz,
        double px, double py, double pz
);

/**
 * Fill MC array to be stored in ntuples_%06d.root file. Array is of constant size
 *     RGE_MC_VARS_SIZE, and the order of variables can be seen in constants.h.
 */
int mc_rge_fill_ntuples_arr(
        Float_t *arr, rge_particle p, rge_particle e, int run_no, int evn, double vt,
        int npart, int atarget, int ztarget, double ptarget, double pbeam, int btype, 
        double ebeam, int targetid, int processid, double weight
);

#endif

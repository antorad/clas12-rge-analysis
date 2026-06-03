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

#include "../lib/rge_particle.h"

// --+ internal +---------------------------------------------------------------
rge_particle particle_init() {
    rge_particle p;
    p.is_valid   = false;
    p.is_hadron  = false;
    p.is_trigger = false;

    return p;
}

rge_particle particle_init(
        int charge, double beta, int sector, double vx, double vy, double vz,
        double vt, double px, double py, double pz
) {
    rge_particle p;

    p.is_valid   = true;
    p.is_trigger = false;
    p.is_hadron  = false;

    p.pid    = 0;
    p.charge = charge;
    p.sector = sector;
    p.beta   = beta;

    p.vx = vx;
    p.vy = vy;
    p.vz = vz;
    p.vt = vt;
    p.px = px;
    p.py = py;
    p.pz = pz;

    return p;
}

//int assign_neutral_pid(double energy, double beta) {
//    return beta < NEUTRON_MAXBETA ? 2112 : (energy > PHOTON_MINENERGY ? 22 : 0);
//}

//bool is_electron(
//        double total_energy, double pcal_energy, double htcc_nphe, double p,
//        double pars[RGE_NSFPARAMS][2]
//) {
//    // Require ECAL.
//    if (total_energy < 1e-9) return false;
//    // Momentum must be greater than 0.
//    if (p < 1e-9) return false;
//    // Require HTCC photoelectrons.
//    if (htcc_nphe < HTCC_NPHE_CUT) return false;
//    // Require PCAL.
//    if (pcal_energy < MIN_PCAL_ENERGY) return false;
//
//    // Require ECAL sampling fraction to be below threshold.
//    double mean = pars[0][0] * (
//            pars[1][0] +
//            pars[2][0] / total_energy +
//            pars[3][0] / (total_energy*total_energy)
//    );
//    double sigma = pars[0][1] * (
//            pars[1][1] +
//            pars[2][1] / total_energy +
//            pars[3][1] / (total_energy*total_energy)
//    );
//
//    if (fabs((total_energy/p - mean)/sigma) > E_SF_NSIGMA) return false;
//
//    return true;
//}

//int match_pid(
//        int *pid, int hypothesis, bool recon_match, bool electron_check,
//        bool htcc_signal_check, bool htcc_pion_threshold
//) {
//    switch(abs(hypothesis)) {
//        case 11:
//            if (recon_match || electron_check) *pid = hypothesis;
//            break;
//        case 211:
//            if (recon_match || (
//                    !electron_check &&
//                    htcc_signal_check &&
//                    htcc_pion_threshold
//            )) {
//                *pid = hypothesis;
//            }
//            break;
//        case 321: case 2212: case 45: case 2112: case 22: case 13: case -13:
//            if (recon_match) *pid = hypothesis;
//            break;
//        default:
//            rge_errno = RGEERR_UNSUPPORTEDPID;
//            return 1;
//    }
//    return 0;
//}

double theta_lab(rge_particle p) {
    if (abs(p.px) + abs(p.py) + abs(p.pz) < 1e-9) return 0;
    return atan2(sqrt(p.px*p.px + p.py*p.py), p.pz);
}

double phi_lab(rge_particle p) {
    return atan2(p.py, p.px);
}

double momentum(rge_particle p) {
    return rge_calc_magnitude(p.px, p.py, p.pz);
}

double Q2(rge_particle p, double bE) {
    if (!p.is_trigger) return 0;
    return 4 * bE * momentum(p) * pow(sin(theta_lab(p)/2), 2);
}

double nu(rge_particle p, double bE) {
    if (!p.is_trigger) return 0;
    return bE - momentum(p);
}

double Xb(rge_particle p, double bE) {
    if (!p.is_trigger) return 0;
    double proton_mass;
    if (rge_get_mass(2212, &proton_mass)) return 0;

    return Q2(p, bE) / (2*proton_mass*nu(p, bE));
}

double Yb(rge_particle p, double beam_E) {
    if (!p.is_trigger) return 0;
    return nu(p, beam_E) / beam_E;
}

double W(rge_particle p, double bE) {
    if (!p.is_trigger) return 0;
    return sqrt(fabs(W2(p, bE)));
}

double W2(rge_particle p, double bE) {
    if (!p.is_trigger) return 0;
    double proton_mass;
    if (rge_get_mass(2212, &proton_mass)) return 0;

    return proton_mass*proton_mass + 2*proton_mass*nu(p, bE)-Q2(p, bE);
}

double theta_pq(rge_particle p, rge_particle e, double bE) {
    if (!(p.is_hadron && e.is_trigger)) return 0;
    return rge_calc_angle(-e.px, -e.py, bE-e.pz, p.px, p.py, p.pz);
}

double phi_pq(rge_particle p, rge_particle e, double bE) {
    if (!(p.is_hadron && e.is_trigger)) return 0;

    double gpx = -e.px, gpy = -e.py, gpz = bE-e.pz;
    double ppx = p.px,  ppy = p.py,  ppz = p.pz;

    // Analyser uses gp isntead of p, i.e. the momentum of virtual photon.
    double phi_z = M_PI - atan2(gpy, gpx);

    rge_rotate_z(&gpx, &gpy, phi_z);
    rge_rotate_z(&ppx, &ppy, phi_z);

    double phi_y = rge_calc_angle(gpx, gpy, gpz, 0, 0, 1);

    rge_rotate_y(&ppx, &ppz, phi_y);

    return atan2(ppy, ppx);
}

double cos_theta_pq(rge_particle p, rge_particle e, double bE) {
    if (!(p.is_hadron && e.is_trigger)) return 0;
    return (p.pz*(bE-e.pz) - p.px*e.px - p.py*e.py) /
            (sqrt(nu(e,bE)*nu(e,bE) + Q2(e,bE)) * momentum(p));
}

double Pt2(rge_particle p, rge_particle e, double bE) {
    if (!(p.is_hadron && e.is_trigger)) return 0;
    return pow(momentum(p),2) * (1 - pow(cos_theta_pq(p,e,bE),2));
}

double Pl2(rge_particle p, rge_particle e, double bE) {
    if (!(p.is_hadron && e.is_trigger)) return 0;
    return pow(momentum(p),2) * pow(cos_theta_pq(p,e,bE),2);
}

double zh(rge_particle p, rge_particle e, double bE) {
    if (!(p.is_hadron && e.is_trigger)) return 0;
    if (p.mass == -1) return -1;
    return sqrt(p.mass*p.mass + pow(momentum(p), 2)) / nu(e,bE);
}

double beta(rge_particle p) {
    double mass = rge_get_mc_mass(p.pid);
    if (mass==-1) return -1;
    double p_p = momentum(p);
    return p_p/(sqrt(p_p*p_p + mass*mass));
}

// --+ library +----------------------------------------------------------------
rge_particle rge_particle_init(
        rge_hipobank *particle, rge_hipobank *track, rge_hipobank *fmttrack,
        uint pos, lint fmt_nlayers
) {
    uint pindex = rge_get_uint(track, "pindex", pos);

    // Use only DC tracking data.
    if (fmt_nlayers == 0) {
        return particle_init(
                rge_get_double(particle, "charge", pindex),
                rge_get_double(particle, "beta",   pindex),
                rge_get_double(track,    "sector", pos),
                rge_get_double(particle, "vx", pindex),
                rge_get_double(particle, "vy", pindex),
                rge_get_double(particle, "vz", pindex),
                rge_get_double(particle, "vt", pindex),
                rge_get_double(particle, "px", pindex),
                rge_get_double(particle, "py", pindex),
                rge_get_double(particle, "pz", pindex)
        );
    }

    // Use DC+FMT tracking data.
    pindex = rge_get_uint(fmttrack, "pindex", pos);
    // Apply FMT cuts.
    // Track reconstructed by FMT.
    if (fmttrack->nrows < 1) return particle_init();
    // Track crossed enough FMT layers.
    if (rge_get_uint(fmttrack, "NDF", pos) < fmt_nlayers)
        return particle_init();

    return particle_init(
            rge_get_double(particle, "charge", pindex),
            rge_get_double(particle, "beta",   pindex),
            rge_get_double(fmttrack, "sector", pos),
            rge_get_double(fmttrack, "vx", pos),
            rge_get_double(fmttrack, "vy", pos),
            rge_get_double(fmttrack, "vz", pos),
            rge_get_double(fmttrack, "vt", pos),
            rge_get_double(fmttrack, "px",   pos),
            rge_get_double(fmttrack, "py",   pos),
            rge_get_double(fmttrack, "pz",   pos)
    );
}

int rge_set_pid(
        rge_particle *particle, int recon_pid, int status
) {
    // Assign PID for neutrals and store PID from reconstruction for charged
    //         particles.
    //if (particle->charge == 0) {
    //    recon_pid = assign_neutral_pid(total_energy, particle->beta);
    //}

/*
    // Create PID list.
    uint hypotheses_size = 0;
    rge_get_pidlist_size_by_charge(particle->charge, &hypotheses_size);

    int hypotheses[hypotheses_size];
    rge_get_pidlist_by_charge(particle->charge, hypotheses);

    // Perform checks.
    bool e_check = is_electron(
            total_energy, pcal_energy, htcc_nphe, momentum(*particle), sf_params
    );

    bool htcc_signal_check = htcc_nphe > HTCC_NPHE_CUT;
    bool htcc_pion_threshold = momentum(*particle) > HTCC_PION_THRESHOLD;

    // Match PID.
    for (uint pi = 0; particle->pid==0 && pi < hypotheses_size; ++pi) {
        if (match_pid(
                &(particle->pid), hypotheses[pi], hypotheses[pi] == recon_pid,
                e_check, htcc_signal_check, htcc_pion_threshold
        )) return 1;
    }
*/
    //set pid comming from reconstruction
    particle->pid = recon_pid;

    // Check if particle is trigger electron and define mass from PID.
    particle->is_trigger = (particle->pid == 11 && status < 0);
    if (rge_get_mass(particle->pid, &(particle->mass))) return 1;

    // If particle is not a lepton, check if it is a valid hadron.
    if (particle->pid >= 100 || particle->pid <= -100) {
        particle->is_hadron = true;
    }

    return 0;
}

int rge_fill_ntuples_arr(
        Float_t *arr, rge_particle p, rge_particle e, int run_no, int evn,
        double start_time, int status, double beam_E, float chi2, float ndf,
        double pcal_energy, double ecin_E, double ecou_E, double tof, double path,
        int nphe_ltcc, int nphe_htcc, double PCAL_U, double PCAL_V, double PCAL_W,
        double DC_R1_edge, double DC_R2_edge, double DC_R3_edge
) {
    // Metadata.
    arr[RGE_RUNNO.addr]     = static_cast<Float_t>(run_no);
    arr[RGE_EVENTNO.addr]   = static_cast<Float_t>(evn);
    arr[RGE_BEAME.addr]     = beam_E;
    arr[RGE_START.addr]     = start_time;

    // Particle.
    arr[RGE_PID.addr]    = static_cast<Float_t>(p.pid);
    arr[RGE_CHARGE.addr] = p.charge;
    arr[RGE_STATUS.addr] = static_cast<Float_t>(status);
    arr[RGE_MASS.addr]   = p.mass;
    arr[RGE_VX.addr]     = p.vx;
    arr[RGE_VY.addr]     = p.vy;
    arr[RGE_VZ.addr]     = p.vz;
    arr[RGE_VT.addr]     = p.vt;
    arr[RGE_PX.addr]     = p.px;
    arr[RGE_PY.addr]     = p.py;
    arr[RGE_PZ.addr]     = p.pz;
    arr[RGE_P.addr]      = momentum(p);
    arr[RGE_THETA.addr]  = theta_lab(p);
    arr[RGE_PHI.addr]    = phi_lab(p);
    arr[RGE_BETA.addr]   = p.beta;
    arr[RGE_TRIGGERSTATUS.addr] = p.is_trigger;
    arr[RGE_SECTOR.addr] = p.sector;

    // Tracking.
    arr[RGE_CHI2.addr] = chi2;
    arr[RGE_NDF.addr]  = ndf;

    //Trajectory
    arr[RGE_DCR1EDGE.addr] = DC_R1_edge;
    arr[RGE_DCR2EDGE.addr] = DC_R2_edge;
    arr[RGE_DCR3EDGE.addr] = DC_R3_edge;

    // Calorimeter.
    arr[RGE_PCALE.addr] = pcal_energy;
    arr[RGE_ECINE.addr] = ecin_E;
    arr[RGE_ECOUE.addr] = ecou_E;
    arr[RGE_TOTE.addr]  = pcal_energy + ecin_E + ecou_E;
    arr[RGE_PCALU.addr] = PCAL_U;
    arr[RGE_PCALV.addr] = PCAL_V;
    arr[RGE_PCALW.addr] = PCAL_W;

    // Scintillator.
    arr[RGE_TOF.addr] = tof;
    arr[RGE_PATH.addr] = path;

    // Cherenkov.
    arr[RGE_NPHELTCC.addr] = nphe_ltcc;
    arr[RGE_NPHEHTCC.addr] = nphe_htcc;

    // DIS -- For hadrons, just use e- data.
    arr[RGE_Q2.addr] = Q2(e, beam_E);
    arr[RGE_NU.addr] = nu(e, beam_E);
    arr[RGE_XB.addr] = Xb(e, beam_E);
    arr[RGE_YB.addr] = Yb(e, beam_E);
    arr[RGE_W2.addr] = W2(e, beam_E);
    if (rge_errno == RGEERR_PIDNOTFOUND) return 1;

    // SIDIS -- if p is trigger electron, all will be 0 by default.
    arr[RGE_ZH.addr]      = zh(p, e, beam_E);
    arr[RGE_PT2.addr]     = Pt2(p, e, beam_E);
    arr[RGE_PL2.addr]     = Pl2(p, e, beam_E);
    arr[RGE_PHIPQ.addr]   = phi_pq(p, e, beam_E);
    arr[RGE_THETAPQ.addr] = theta_pq(p, e, beam_E);
    return 0;
}

//for MC particles
rge_particle mc_particle_init(
        int pid, double vx, double vy, double vz,
        double px, double py, double pz
) {
    rge_particle p;

    p.is_valid   = true;
    p.is_trigger = false;

    //check hadron status
    if (pid >= 100 || pid <= -100) {
        p.is_hadron = true;
    }
    else p.is_hadron  = false;

    p.pid    = pid;
    p.mass = rge_get_mc_mass(pid);
    p.vx = vx;
    p.vy = vy;
    p.vz = vz;
    p.px = px;
    p.py = py;
    p.pz = pz;

    return p;
}

//for filling array of MC particles
int mc_rge_fill_ntuples_arr(
        Float_t *arr, rge_particle p, rge_particle e, int run_no, int evn, double vt,
        int npart, int atarget, int ztarget, double ptarget, double pbeam, int btype, 
        double ebeam, int targetid, int processid, double weight
) {
    arr[RGE_MC_RUNNO.addr] = static_cast<Float_t>(run_no);
    arr[RGE_MC_EVENTNO.addr] = static_cast<Float_t>(evn);
    arr[RGE_MC_PID.addr] = p.pid;
    arr[RGE_MC_PX.addr] = p.px;
    arr[RGE_MC_PY.addr] = p.py;
    arr[RGE_MC_PZ.addr] = p.pz;
    arr[RGE_MC_VX.addr] = p.vx;
    arr[RGE_MC_VY.addr] = p.vy;
    arr[RGE_MC_VZ.addr] = p.vz;
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
    arr[RGE_MC_P.addr] = momentum(p);
    arr[RGE_MC_THETA.addr] = theta_lab(p);
    arr[RGE_MC_PHI.addr] = phi_lab(p);
    arr[RGE_MC_BETA.addr] = beta(p);
    arr[RGE_MC_Q2.addr] = Q2(e, ebeam);
    arr[RGE_MC_NU.addr] = nu(e, ebeam);
    arr[RGE_MC_XB.addr] = Xb(e, ebeam);
    arr[RGE_MC_YB.addr] = Yb(e, ebeam);
    arr[RGE_MC_W2.addr] = W2(e, ebeam);
    arr[RGE_MC_ZH.addr] = zh(p, e, ebeam);
    arr[RGE_MC_PT2.addr] = Pt2(p, e, ebeam);
    arr[RGE_MC_PL2.addr] = Pl2(p, e, ebeam);
    arr[RGE_MC_PHIPQ.addr] = phi_pq(p, e, ebeam);
    arr[RGE_MC_THETAPQ.addr] = theta_pq(p, e, ebeam);
    return 0;
}
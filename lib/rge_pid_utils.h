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

#ifndef RGE_PIDUTILS
#define RGE_PIDUTILS

// --+ preamble +---------------------------------------------------------------
// C.
#include <float.h>

// C++.
#include <map>

// rge-analysis.
#include "rge_err_handler.h"

// typedefs.
typedef unsigned int uint;
typedef long unsigned int luint;
typedef long int lint;

// --+ structs +----------------------------------------------------------------
/**
 * Data associated to a particule associated to a particular PID. The PIDs are
 *     defined from the LUND convention.
 *
 * @param charge : charge of the particle.
 * @param mass   : mass of the particle.
 * @param name   : name under which the particle is known.
 */
typedef struct {
    int charge;
    double mass;
    const char *name;
} rge_pidconstants;

// --+ internal +---------------------------------------------------------------
/**
 * Initialize one rge_pidconstants struct with the given input data.
 *
 * @param q : PID charge (electron charge).
 * @param m : PID mass (GeV).
 * @param n : PID name.
 * @return  : rge_pidconstants instance with input data.
 */
static rge_pidconstants pid_constants_init(int q, double m, const char *n);

/** PID_MAP global iterator. */
static std::map<int, rge_pidconstants>::const_iterator pid_it;

/** Counters for negative, neutral, and positive PIDs in list. */
static uint negative_size = 0;
static uint neutral_size  = 0;
static uint positive_size = 0;

// --+ library +----------------------------------------------------------------
/** Return 0 if PID_MAP contains pid, 1 otherwise. */
int rge_pid_invalid(int pid);

/**
 * Get charge of particle associated to PID. If PID is not found in PID_MAP,
 *     sets rge_errno to RGEERR_PIDNOTFOUND and returns 1.
 *
 * @param pid    : pid value of the charge to look for.
 * @param charge : pointer to integer where to write charge.
 * @return       : error code.
 */
int rge_get_charge(int pid, int *charge);

/**
 * Get mass of particle associated to pid. If PID is not found in PID_MAP, sets
 *     rge_errno to RGEERR_PIDNOTFOUND and returns 1.
 *
 * @param pid  : pid value of the mass to look for.
 * @param mass : pointer to double where to write mass.
 * @return     : error code.
 */
int rge_get_mass(int pid, double *mass);

/**
 * Get number of PIDs in PID_MAP that match the given charge.
 *
 * @param charge : charge value of the PIDs to look for.
 * @param size   : pointer to int where to write size of pidlist of charge.
 * @param return : error code.
 */
int rge_get_pidlist_size_by_charge(int charge, uint *size);

/**
 * Fill an array of PIDs that match the given charge from PID_MAP. Function
 *     assumes that pidlist is of enough size to receive all data -- a segfault
 *     will happens if this is not the case.
 *
 * @param charge  : charge value of the PIDs to look for.
 * @param pidlist : array of PIDs where to write list of PIDs.
 * @return        : error code.
 */
int rge_get_pidlist_by_charge(int charge, int pidlist[]);

/** Print all PIDs in PID_MAP and their corresponding names to stdout. */
int rge_print_pid_names();

//get mass from mc particles and return if they are not in the map
double rge_get_mc_mass(int pid);

#endif

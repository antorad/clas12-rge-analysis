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

#include "../lib/rge_pid_utils.h"

// --+ internal +---------------------------------------------------------------
/** Map linking PIDs to rge_pidconstants. */
static const std::map<int, rge_pidconstants> PID_MAP = {
    {     -3322, pid_constants_init( 0, 1.314860, "antixi"               )},
    {     -3312, pid_constants_init( 1, 1.321710, "antinegative xi"      )},
    {     -3222, pid_constants_init(-1, 1.189370, "antipositive sigma"   )},
    {     -3122, pid_constants_init( 0, 1.115683, "antilambda"           )},
    {     -3112, pid_constants_init( 1, 1.197449, "antinegative sigma"   )},
    {     -2212, pid_constants_init( 1, 0.938272, "antiproton"           )},
    {     -2112, pid_constants_init( 0, 0.939565, "antineutron"          )},
    {      -321, pid_constants_init(-1, 0.493677, "negative kaon"        )},
    {      -211, pid_constants_init(-1, 0.139570, "negative pion"        )},
    {       -13, pid_constants_init( 1, 0.10566,  "positive muon"        )},
    {       -11, pid_constants_init( 1, 0.000051, "positron"             )},
    {         0, pid_constants_init( 0, DBL_MAX,  "unidentified particle")},
    {        11, pid_constants_init(-1, 0.000051, "electron"             )},
    {        13, pid_constants_init(-1, 0.10566,  "negative muon"        )},
    {        22, pid_constants_init( 0, 0.,       "photon"               )},
    {        45, pid_constants_init( 0, DBL_MAX,  "unidentified particle")},
    {       111, pid_constants_init( 0, 0.134977, "neutral pion"         )},
    {       130, pid_constants_init( 0, 0.497611, "neutral kaon"         )},
    {       211, pid_constants_init( 1, 0.139570, "positive pion"        )},
    {       310, pid_constants_init( 0, 0.497614, "short kaon"           )},
    {       321, pid_constants_init( 1, 0.493677, "positive kaon"        )},
    {      2112, pid_constants_init( 0, 0.939565, "neutron"              )},
    {      2212, pid_constants_init( 1, 0.938272, "proton"               )},
    {      3112, pid_constants_init(-1, 1.197449, "negative sigma"       )},
    {      3122, pid_constants_init( 0, 1.115683, "lambda"               )},
    {      3222, pid_constants_init( 1, 1.189370, "positive sigma"       )},
    {      3312, pid_constants_init(-1, 1.321710, "negative xi"          )},
    {      3322, pid_constants_init( 0, 1.314860, "xi"                   )}
};

rge_pidconstants pid_constants_init(int q, double m, const char *n) {
    // Update relevant charge counter.
    if (q  < 0) ++negative_size;
    if (q == 0) ++neutral_size;
    if (q  > 0) ++positive_size;

    // Return instance.
    return (rge_pidconstants) {.charge = q, .mass = m, .name = n};
}

// --+ library +----------------------------------------------------------------
int rge_pid_invalid(int pid) {
    try {
        PID_MAP.at(pid);
        return 0;
    }
    catch (...) {}

    rge_errno = RGEERR_PIDNOTFOUND;
    return 1;
}

int rge_get_charge(int pid, int *charge) {
    if (rge_pid_invalid(pid)) return 1;

    *charge = PID_MAP.at(pid).charge;
    return 0;
}

int rge_get_mass(int pid, double *mass) {
    if (rge_pid_invalid(pid)) return 1;

    *mass = PID_MAP.at(pid).mass;
    return 0;
}

int rge_get_pidlist_size_by_charge(int charge, uint *size) {
    if (charge  < 0) *size = negative_size;
    if (charge == 0) *size = neutral_size;
    if (charge  > 0) *size = positive_size;

    return 0;
}

int rge_get_pidlist_by_charge(int charge, int pidlist[]) {
    uint counter = 0;
    for (pid_it = PID_MAP.begin(); pid_it != PID_MAP.end(); ++pid_it) {
        if (
                (charge == 0 && pid_it->second.charge == 0) || // both neutral.
                (charge * pid_it->second.charge > 0)           // equal signs.
        ) {
            pidlist[counter] = pid_it->first;
            ++counter;
        }
    }

    return 0;
}

int rge_print_pid_names() {
    for (pid_it = PID_MAP.begin(); pid_it != PID_MAP.end(); ++pid_it) {
        printf("  * %5d (%s).\n", pid_it->first, pid_it->second.name);
    }

    return 0;
}

double rge_get_mc_mass(int pid) {
    auto it = PID_MAP.find(pid);
    if (it != PID_MAP.end()) {
        return it->second.mass;
    } else {
        return -1;
    }
}
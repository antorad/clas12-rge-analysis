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

#ifndef RGE_HIPOBANK
#define RGE_HIPOBANK

// --+ preamble +---------------------------------------------------------------
// C.
#include "string.h"

// C++.
#include <map>
#include <vector>

// ROOT.
#include <TTree.h>

// HIPO.
#include "bank.h"

// rge-analysis.
#include "rge_err_handler.h"

#include <string>
// typedefs.
typedef unsigned int uint;
typedef long unsigned int luint;
typedef long int lint;

/** Definitions to refer to different hipo banks using strings. */
#define RGE_RECPARTICLE     "REC::Particle"
#define RGE_RECTRACK        "REC::Track"
#define RGE_RECCALORIMETER  "REC::Calorimeter"
#define RGE_RECCHERENKOV    "REC::Cherenkov"
#define RGE_RECSCINTILLATOR "REC::Scintillator"
#define RGE_RECTRAJ         "REC::Traj"
#define RGE_RECFTRACK       "REC::FTrack"
#define RGE_MCPARTICLE      "MC::Particle"
#define RGE_MCEVENT         "MC::Event"

/** ECAL layer IDs in CLAS12 banks. */
#define PCAL_LYR 1
#define ECIN_LYR 4
#define ECOU_LYR 7
#define DC_R1_LYR 6
#define DC_R2_LYR 18
#define DC_R3_LYR 36

// --+ structs +----------------------------------------------------------------
/** String comparer to use const char * as std::map keys. */
struct cmp_str {
    bool operator() (char const *a, char const *b) const {
        return strcmp(a, b) < 0;
    }
};

/**
 * Struct containing one entry of a particular hipo bank.
 *
 * @param addr   : address of the entry in the hipo bank, as in BANK::NAME::VAR.
 * @param data   : vector with the data of the entry.
 * @param branch : pointer to TBranch where to write the data.
 * @param type   : integer containing primitive type in hipo bank, as defined in
 *                 the internal variables.
 */
typedef struct {
    const char *addr;
    std::vector<double> *data;
    TBranch *branch;
    uint type;
} rge_hipoentry;

/** Struct containing a map of all entries associated to a hipo bank. */
typedef struct {
    luint nrows;
    std::map<const char *, rge_hipoentry, cmp_str> entries;
} rge_hipobank;

// --+ internal +---------------------------------------------------------------
/** internal variables to refer to different primitive types. */
static const uint BYTE  = 0;
static const uint SHORT = 1;
static const uint INT   = 2;
static const uint FLOAT = 3;

/** Internal iterator used to loop through entries. */
static std::map<const char *, rge_hipoentry, cmp_str>::const_iterator it;

/**
 * Initialize and return one rge_hipoentry. Parameters addr and type are
 *     initialized to input, data is initialized to an empty vector to be read
 *     from hipo, and branch is initialized to a nullptr to be handled by root.
 */
static rge_hipoentry entry_init(const char *in_addr, uint in_type);

/** Set b.nrows to in_rows. */
static int set_nrows(rge_hipobank *b, luint in_nrows);

/** Get entry number idx with name var from bank b. */
static double get_entry(rge_hipobank *b, const char *var, luint idx);

// --+ library +----------------------------------------------------------------
/** Initialize rge_hipobank based on static map related to bank_version. */
rge_hipobank rge_hipobank_init(const char *bank_version);

/** Initialize rge_hipobank and set branch addresses to t's branches. */
rge_hipobank rge_hipobank_init(const char *bank_version, TTree *t);

/** Link branches of t to entries of b. */
int rge_link_branches(rge_hipobank *b, TTree *t);

/** Fill entries in rb with data from hb. */
int rge_fill(rge_hipobank *rb, hipo::bank hb);

/** Read entries from t into b. */
int rge_get_entries(rge_hipobank *b, TTree *t, int idx);

/** Get entry number idx with name var from bank b as a double. */
double rge_get_double(rge_hipobank *b, const char *var, luint idx);

/** Get entry number idx with name var from bank b as an int. */
int rge_get_int(rge_hipobank *b, const char *var, luint idx);

/** Get entry number idx with name var from bank b as an unsigned int. */
uint rge_get_uint(rge_hipobank *b, const char *var, luint idx);

#endif

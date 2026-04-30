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

#include "../lib/rge_constants.h"

// --+ library +----------------------------------------------------------------
const char *RGE_VARS[RGE_VARS_SIZE] = {
        RGE_RUNNO.name, RGE_EVENTNO.name, RGE_BEAME.name,
        RGE_PID.name, RGE_CHARGE.name, RGE_STATUS.name, RGE_MASS.name,
                RGE_VX.name, RGE_VY.name, RGE_VZ.name, RGE_VT.name, RGE_PX.name, RGE_PY.name,
                RGE_PZ.name, RGE_P.name, RGE_THETA.name, RGE_PHI.name,
                RGE_BETA.name, RGE_TRIGGERSTATUS.name, RGE_SECTOR.name,
        RGE_CHI2.name, RGE_NDF.name,
        RGE_DCR1EDGE.name, RGE_DCR2EDGE.name, RGE_DCR3EDGE.name,
        RGE_PCALE.name, RGE_ECINE.name, RGE_ECOUE.name, RGE_TOTE.name,
        RGE_PCALU.name, RGE_PCALV.name, RGE_PCALW.name,
        RGE_TOF.name, RGE_PATH.name,
        RGE_NPHELTCC.name, RGE_NPHEHTCC.name,
        RGE_Q2.name, RGE_NU.name, RGE_XB.name, RGE_YB.name, RGE_W2.name,
        RGE_ZH.name, RGE_PT2.name, RGE_PL2.name, RGE_PHIPQ.name,
                RGE_THETAPQ.name
};
const char *RGE_MC_VARS[RGE_MC_VARS_SIZE] = {
        RGE_MC_RUNNO.name,
        RGE_MC_EVENTNO.name,
        RGE_MC_PID.name,
        RGE_MC_PX.name,
        RGE_MC_PY.name,
        RGE_MC_PZ.name,
        RGE_MC_VX.name,
        RGE_MC_VY.name,
        RGE_MC_VZ.name,
        RGE_MC_VT.name,
        RGE_MC_NPART.name,
        RGE_MC_ATARGET.name,
        RGE_MC_ZTARGET.name,
        RGE_MC_PTARGET.name,
        RGE_MC_PBEAM.name,
        RGE_MC_BTYPE.name,
        RGE_MC_EBEAM.name,
        RGE_MC_TARGETID.name,
        RGE_MC_PROCESSID.name,
        RGE_MC_WEIGHT.name,
        RGE_P.name,
        RGE_THETA.name,
        RGE_PHI.name,
        RGE_BETA.name,
        RGE_MC_Q2.name, RGE_MC_NU.name, RGE_MC_XB.name, RGE_MC_YB.name, RGE_MC_W2.name,
        RGE_MC_ZH.name, RGE_MC_PT2.name, RGE_MC_PL2.name, RGE_MC_PHIPQ.name,
                RGE_MC_THETAPQ.name
};

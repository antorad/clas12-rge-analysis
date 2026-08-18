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

#include "../lib/rge_io_handler.h"

// --+ internal +---------------------------------------------------------------
int is_number(char c) {
    if (c >= '0' && c <= '9') return 1;
    return 0;
}

int is_number(char *s) {
    if (is_number(s[0]) || (s[0] == '-' && is_number(s[1]))) return 1;
    return 0;
}

int run_strtol(lint *n, char *t) {
    char *eptr;
    errno = 0;
    *n = strtol(t, &eptr, 10);
    if (errno == EINVAL) return 1;
    if (errno == ERANGE) return 2;
    return 0;
}

// --+ library +----------------------------------------------------------------
int rge_grab_string(char *arg, char **str) {
    *str = static_cast<char *>(malloc(strlen(arg) + 1));
    strcpy(*str, arg);
    return 0;
}

int rge_grab_multiarg(int argc, char **argv, int *opt_idx, lint **arr) {
    int idx   = *opt_idx - 1;
    int start = idx;
    int size = 0;
    char *next;

    // Get size.
    while (idx < argc) {
        next = strdup(argv[idx++]);
        if (is_number(next)) ++size;
        else break;
    }

    // Raise error if size != 4.
    if (size != 4) {
        rge_errno = RGEERR_TOOMANYNUMBERS;
        return 1;
    }

    // Restart counter.
    idx = start;

    // Fill array.
    int i = 0;
    while (idx < argc) {
        next = strdup(argv[idx++]);
        if (is_number(next)) (*arr)[i++] = atof(next);
        else break;
    }

    // Continue with getopts.
    *opt_idx = idx - 1;
    return 0;
}

int rge_grab_multiarg(
        int argc, char **argv, int *opt_idx, luint *size, double **arr
) {
    int idx   = *opt_idx - 1;
    int start = idx;
    *size     = 0;
    char *next;

    // Get size.
    while (idx < argc) {
        next = strdup(argv[idx++]);
        if (is_number(next)) ++(*size);
        else break;
    }

    // Restart counter and initialize binning.
    idx = start;
    (*arr) = static_cast<double *>(malloc((*size) * sizeof(**arr)));

    // Fill binning.
    int i = 0;
    while (idx < argc) {
        next = strdup(argv[idx++]);
        if (is_number(next)) (*arr)[i++] = atof(next);
        else break;
    }

    // Continue with getopts.
    *opt_idx = idx - 1;
    return 0;
}

int rge_process_nentries(lint *nentries, char *arg) {
    int err = run_strtol(nentries, arg);
    if (err == 1) {
        rge_errno = RGEERR_INVALIDENTRIES;
        return 1;
    }
    if (err == 2) {
        rge_errno = RGEERR_NENTRIESLARGE;
        return 1;
    }
    if (*nentries <= 0) {
        rge_errno = RGEERR_NENTRIESNEGATIVE;
        return 1;
    }

    return 0;
}

int rge_process_pid(lint *pid, char *arg) {
    int err = run_strtol(pid, arg);
    if (err == 1 || err == 2) {
        rge_errno = RGEERR_INVALIDPID;
        return 1;
    }

    return 0;
}

int rge_process_fmtnlayers(lint *nlayers, char *arg) {
    int err = run_strtol(nlayers, arg);
    if (err == 1 || (
            *nlayers != 0 && *nlayers != -1 &&
            (FMTMINLAYERS > *nlayers || *nlayers > FMTNLAYERS)
    )) {
        rge_errno = RGEERR_INVALIDFMTNLAYERS;
        return 1;
    }
    return 0;
}

bool rge_catch_yn() {
    while (true) {
        char str[32];
        printf(">>> ");
        scanf_dump = scanf("%31s", str);

        if (!strcmp(str, "y") || !strcmp(str, "Y")) return true;
        if (!strcmp(str, "n") || !strcmp(str, "N")) return false;
    }
}

long rge_catch_long() {
    long r;
    while (true) {
        char str[32];
        char *endptr;
        printf(">>> ");
        scanf_dump = scanf("%31s", str);
        r = strtol(str, &endptr, 10);

        if (endptr != str) break;
    }

    return r;
}

double rge_catch_double() {
    double r;
    while (true) {
        char str[32];
        char *endptr;
        printf(">>> ");
        scanf_dump = scanf("%31s", str);
        r = strtod(str, &endptr);

        if (endptr != str) break;
    }

    return r;
}

int rge_catch_string(const char *arr[], int size) {
    int x;
    while (true) {
        char str[32];
        printf(">>> ");
        scanf_dump = scanf("%31s", str);

        for (int i = 0; i < size; ++i) if (!strcmp(str, arr[i])) x = i;
        if (x != -1) break;
    }

    return x;
}

int rge_catch_var(const char *arr[], int size) {
    long x;
    while (true) {
        char str[32];
        char *endptr;
        printf(">>> ");
        scanf_dump = scanf("%31s", str);
        x = strtol(str, &endptr, 10);

        if (endptr != str && 0 <= x && x < size) break;
    }

    return x;
}

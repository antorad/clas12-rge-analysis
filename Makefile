# CLAS12 RG-E Analyser.
# Copyright (C) 2022-2023 Bruno Benkel
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You can see a copy of the GNU Lesser Public License under the LICENSE file.

# C++ version.
CXX_STD = -std=c++17

# Locations.
BIN := ./bin
BLD := ./build
SRC := ./src
LIB := ./lib

# C++ compiler + flags.
CXX         := g++
# NOTE. I'm leaving -pedantic and -Wextra out because they include mostly
#		C++ I don't really care about :). If you want to add them back, you'll
#       need to add `__extension__` behind every variable-length array.
#          -Bruno.
CFLAGS_DBG  := -g -Wall -Wcast-align -Wcast-qual \
			   -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 \
			   -Winit-self -Wlogical-op -Wmissing-declarations \
			   -Wmissing-include-dirs -Wnoexcept -Wold-style-cast \
			   -Woverloaded-virtual -Wredundant-decls -Wshadow \
			   -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel \
			   -Wstrict-overflow=4 -Wswitch-default -Wundef -Werror -Wno-unused
CFLAGS_PROD := -O3
CXX         := $(CXX) $(CFLAGS_PROD)

# ROOT.
ROOTCFLAGS  := -pthread $(CXX_STD) -m64 -isystem$(ROOT)/include
RLIBS       := $(shell root-config --libs) -lEG
RXX         := $(CXX) $(ROOTCFLAGS)

# HIPO.
HIPOCFLAGS  := -isystem$(HIPO)/include/hipo4
HLIBS       := $(RLIBS) -L$(HIPO)/lib -lhipo4
HXX         := $(RXX) $(HIPOCFLAGS)

# Objects.
OBJS := $(BLD)/constants.o \
		$(BLD)/err_handler.o \
		$(BLD)/file_handler.o \
		$(BLD)/filename_handler.o \
		$(BLD)/hipo_bank.o \
		$(BLD)/io_handler.o \
		$(BLD)/math_utils.o \
		$(BLD)/particle.o \
		$(BLD)/pid_utils.o \
		$(BLD)/progress.o

# Executables.
BINS := $(BIN)/hipo2root \
		$(BIN)/make_ntuples

# Targets.
all: $(BINS)

$(OBJS): $(BLD)/%.o: $(SRC)/rge_%.c $(LIB)/rge_%.h
	$(HXX) -c $< -o $@

$(BINS): $(BIN)/%: $(SRC)/%.c $(OBJS)
	$(HXX) $(OBJS) $< -o $@ $(HLIBS)

clean:
	@echo "Removing all build files and binaries."
	@rm $(BLD)/*.o
	@rm $(BIN)/*

# RG-E Analysis Tools
The purpose of this repository is to contain various tools used in the analysis of RG-E data from the CLAS12 experiment. We use [root](https://root.cern.ch/) for analysis, and [hipo](https://github.com/gavalian/hipo) for the handling of hipo files. If you run into any problems compiling or using the tools, please raise an [issue](https://github.com/bleaktwig/clas12-rge-analysis/issues/new).

## Compilation
First, set the environment variables `ROOT` and `HIPO` to the location where root and hipo are respectively installed. Then, compile everything by running `make` in the root directory (where `Makefile` is). The repository has been tested and works fine with the following root versions:
* 6.20/04.
* 6.24/02.
* 6.26/08.
* 6.28/00.

We specifically avoid using features associated to specific versions of C++, so that the program can be run with a version of ROOT compiled  against any version of C++. Note that the first variable set in `Makefile` is `CXX_STD`. Set that to the C++ version your ROOT is compiled against.

## Usage
### hipo2root
```
Usage: hipo2root [-hfn:w:] infile
 * -h         : show this message and exit.
 * -f         : set this to true to process FMT::Tracks bank. If this is set
                and FMT::Tracks bank is not present in the HIPO file, the
                program will crash.
 * -n nevents : number of events.
 * -w workdir : location where output root files are to be stored. Default
                is root_io.
 * infile     : input HIPO file. Expected format is <text>run_no.hipo.
```
Convert a file from hipo to root format. This program only conserves the banks that are useful for RG-E analysis, as specified in the `lib/rge_hipo_bank.h` file. It's important for the input hipo file to specify the run number at the end of the filename (`<text>run_no.hipo`), so that `hipo2root` can get the beam energy from the run number.

Since simulation files don't have a run number, we use a convention for specifying the beam energy. For this files, the filename should be `<text>999XXX.hipo`, where `XXX` is the beam energy used in the simulation in [0.1*GeV].

### extract_sf
```
Usage: extract_sf [-hn:w:d:] infile
 * -h         : show this message and exit.
 * -n nevents : number of events
 * -w workdir : location where output root files are to be stored. Default
                is root_io.
 * -d datadir : location where sampling fraction files are stored. Default
                is data.
 * infile     : input ROOT file. Expected file format: <text>run_no.root.
```
Obtain the EC sampling fraction from an input file. An alternative to using this program is to fill the output file corresponding to the studied run (by default stored in the `data` directory) with the data obtained from [CCDB](https://clasweb.jlab.org/cgi-bin/ccdb/versions?table=/calibration/eb/electron_sf). The function used to fit the data is

```
[0]*Gaus(x,[1],[2]) + [3]*x*x + [4]*x + [5]
```
where *[0]* is the amplitude of the Gaussian, *[1]* and *[2]* its mean and sigma, and *[3]*, *[4]*, and *[5]* the *p0*, *p1*, and *p2* used to fit the background.

The output of the program is the `sf_params_<run_no>.txt`, which contains a table with the sampling fractions and their errors. The table is formatted like the one at CCDB, as in

```
         | sf0     sf1     sf2     sf3     sfs1    sfs2    sfs3    sfs4
---------+-----------------------------------------------------------------
sector 1 | %011.8f %011.8f %011.8f %011.8f %011.8f %011.8f %011.8f %011.8f
sector 2 | %011.8f %011.8f %011.8f %011.8f %011.8f %011.8f %011.8f %011.8f
sector 3 | %011.8f %011.8f %011.8f %011.8f %011.8f %011.8f %011.8f %011.8f
sector 4 | %011.8f %011.8f %011.8f %011.8f %011.8f %011.8f %011.8f %011.8f
sector 5 | %011.8f %011.8f %011.8f %011.8f %011.8f %011.8f %011.8f %011.8f
sector 6 | %011.8f %011.8f %011.8f %011.8f %011.8f %011.8f %011.8f %011.8f
```

### acc_corr
```
Usage: acc_corr [-hq:n:z:p:f:g:s:d:FD]
 * -h         : show this message and exit.
 * -q ...     : Q2 bins.
 * -n ...     : nu bins.
 * -z ...     : z_h bins.
 * -p ...     : Pt2 bins.
 * -f ...     : phi_PQ bins (in degrees).
 * -g genfile : generated events ROOT file.
 * -s simfile : simulated events ROOT file.
 * -d datadir : location where sampling fraction files are found. Default is
                data.
 * -F         : flag to tell program to use FMT data instead of DC data from
                the simulation file.
 * -D         : flag to tell program that generated events are in degrees
                instead of radians.
```
Get the 5-dimensional acceptance correction factors for *Q2*, *nu*, *z_h*, *Pt2*, and *phi_PQ*. For each optional argument, an array of doubles is expected. The first double will be the lower limit of the leftmost bin, the final double will be the upper limit of the rightmost bin, and all doubles between them will be the separators between each bin.

The output will be written to the `acc_corr.txt` file, by default in the `data` directory, which is formatted to make it easy to read by the `draw_plots` program:
* First line contains five integers; the size of each of the five binnings.
* The next five lines are each of the binning schemes, in order *Q2*, *nu*, *z_h*, *Pt2*, and *phi_PQ*.
* The following line contains one integer which is the size of the list of PIDs, followed by a line containing each of these PIDs.
* Finally, a number of lines equal to the number of PIDs follows. Each line contains a list of the bins, ordered as `[Q2][nu][z_h][Pt2][phi_PQ]`.

### make_ntuples
```
Usage: make_ntuples [-hDf:cn:w:d:] infile
 * -h         : show this message and exit.
 * -D         : activate debug mode.
 * -f fmtswtch: select which bank to use for reconstruction.
                Options are 0 (tracked only by REC::Track), 1 (tracked only
                by FMT::Tracks bank), 2 (tracked by FMT::Tracks and
                REC::Track when the first is not available), and 3 (runs the
                3 previous options sequentially). Default is 2.
 * -c         : apply FMT geometry cut on data.
 * -n nevents : number of events.
 * -w workdir : location where output root files are to be stored. Default
                is root_io.
 * -d datadir : location where sampling fraction files are. Default is data.
 * infile     : input ROOT file. Expected file format: <text>run_no.root`.
```
Generate ntuples relevant to SIDIS analysis based on the reconstructed variables from CLAS12 data. The output of the program is the `ntuples_<run_no>.root` file, which contains all relevant ntuples for RG-E analysis. This file can be studied directly in root or through the `draw_plots` program.

### draw_plots
```
Usage: draw_plots [-hp:cn:o:a:w:] infile
 * -h          : show this message and exit.
 * -p pid      : skip particle selection and draw plots for pid.
 * -c          : apply all cuts (general, geometry, and DIS) instead of
                 asking which ones to apply while running.
 * -n nentries : number of entries to process.
 * -o outfile  : output file name. Default is plots_<run_no>.root.
 * -a accfile  : apply acceptance correction using acc_filename.
 * -A          : get acceptance correction plots without applying acceptance
                 correction. Requires -a to be set.
 * -w workdir  : location where output root files are to be stored. Default
                 is root_io.
 * infile      : input file produced by make_ntuples.
```
Draw plots from a ROOT file built from `make_ntuples`. File should be named `<text>run_no.root`. This tool is built for those who don't enjoy using root too much, and should be able to get most basic plots needed in SIDIS analysis.

## Debugging
As always, debugging ROOT code is terrible. If you want to use Valgrind, run it as follows to hide (some of) of ROOT's terrible memory management practices:

```
valgrind --num-callers=30 --suppressions=$ROOTSYS/etc/valgrind-root.supp
```

Keep in mind that opening any `TFile` will give you about 140 lines of memory management errors.

## Contributing
Pull requests are welcome. For major changes, open an issue first to discuss the changes and figure out a work plan before putting serious work into them.

**Pending tasks are**:
- [ ] Implement variable bin sizes for every variable and binning.
- [ ] Include GitHub tests -- I've no clue on how to do this with ROOT + HIPO.
- [ ] Apply radiative correction.
- [ ] Apply Feynman cuts.
- [ ] Get beam energy from `RCDB` instead of hardcoding it -- check `get_beam_energy()` function from `file_handler.c`.

## License
[GNU LGPLv3](https://www.gnu.org/licenses/lgpl-3.0.en.html). Details in [LICENSE](/LICENSE) file.

# Yet Another Pulsar Processor (YAPP) 3.7-beta
## README

Yet Another Pulsar Processor (YAPP) is a suite of radio pulsar data analysis software.

The YAPP tools available with this release are:

* `yapp_viewmetadata` : Prints metadata to standard output.
* `yapp_viewdata` : Plots data to PGPLOT device.
* `yapp_ft` : Performs PFB/FFT on 8-bit, complex, dual-pol. baseband data.
* `yapp_dedisperse` : Dedisperses filterbank format data.
* `yapp_decimate` : Decimates filterbank and time series data.
* `yapp_smooth` : Boxcar-smoothes dedispersed time series data.
* `yapp_filter` : Processes dedispersed time series data with a custom frequency-domain filter.
* `yapp_add` : Coherently add dedispersed time series data from multiple frequency bands.
* `yapp_fold` : Folds filterbank and dedispersed time series data.
* `yapp_subtract` : Subtracts two dedispersed time series files.
* `yapp_siftpulses` : Sifts multiple dedispersed time series files for bright pulses.
* `yapp_stacktim` : Stacks time series data to form filterbank data.
* `yapp_split` : Splits time series data into time sections.
* `yapp_showinfo` : Displays supported file types and colourmaps.

YAPP also comes with the following utilities:

* `yapp_fits2fil` : Converts PSRFITS to SIGPROC `.fil`.
* `yapp_fil2h5` : Converts SIGPROC `.fil` to HDF5.
* `yapp_dat2tim` : Converts PRESTO '.dat' to SIGPROC `.tim`.
* `yapp_tim2dat` : Converts SIGPROC '.tim' to PRESTO `.dat`.
* `yapp_ym2fil` : Converts YAPP Metadata to SIGPROC filterbank header format.

YAPP includes the following scripts:

* `yapp_genpfbcoeff.py` : Generate PFB pre-filter co-efficients for `yapp_ft`.
* `yapp_genfiltermask.py` : Generate filter response for `yapp_filter`.
* `yapp_calcspecidx.py` : Calculate spectral index from a sequence of time series files corresponding to multiple bands.
* `yapp_stackprof.py` : Stacks folded profiles from multiple bands to show a plot of phase versus frequency.
* `yapp_addprof.py` : Add [calibrated] profiles from two polarisations.
* `yapp_viewcand.rb` : Converts prepfold candidate plots in PS format to PNG, and generates a set of HTML pages displaying a tiled set of plots.
* `yapp_subbanddedisperse.rb` : Creates a sub-band-dedispersed filterbank file from a raw filterbank file and optionally does smoothing.
* `yapp_replacemetadata.rb` : Replaces header in a SIGPROC `.fil` file with user-supplied header.

The supported file formats are SIGPROC `.fil` and SIGPROC `.tim`, with limited support for DAS `.spec` and `.dds`, PSRFITS, PRESTO `.dat`, and HDF5. Not all programs support all file formats.

For detailed usage instructions, refer the man pages or online documentation.

### System requirements

Linux/OS X, a C compiler, PGPLOT with C binding, FFTW3, CFITSIO, and optionally, HDF5, Python with NumPy and Matplotlib, Ruby, and ImageMagick.

### Installation instructions

#### Standard

Firstly, clone this repository.

```
git clone https://github.com/jayanthc/yapp.git
```

On a typical Ubuntu-based machine in which PGPLOT, FFTW3, and CFITSIO were installed via APT, and the optional HDF5 was installed in its default location, running `make` (or `make HDF5=yes`), optionally followed by `sudo make install`, should work, with the binaries being copied to `/usr/local/bin`. For different operating systems and/or different PGPLOT/FFTW3/CFITSIO/HDF5 installation directories and/or a different choice of YAPP installation directory, the makefile may need to be modified by hand. YAPP scripts require YAPP binaries to be in the search path.

```
cd yapp
make # 'make HDF5=yes' if you would like HDF5 support
sudo make install
```

#### Using Docker

YAPP can be installed and run using Docker, as shown below. To install:

```
docker build -t yapp https://raw.githubusercontent.com/jayanthc/yapp/release/Dockerfile
```

To run YAPP with graphics, the X server on the host needs to be made available to the container. The easiest way to use YAPP with graphics is to run the script `runYAPPOnDocker.sh`, which will run the container, mounting a data volume of your choice, and provide a terminal to work on that data.

```
./runYAPPOnDocker.sh
root@sdfasdf:/#

... run YAPP commands ...

root@asdafsad:/#exit # exit when done
```

You will need to modify `runYAPPOnDocker.sh` to specify your data directory, and if your Ethernet interface is not `en0`. Note that if your host operating system is Linux, replace `en0` with `eth0` (or whatever interface is appropriate). `runYAPPOnDocker.sh` has not been tested on Linux hosts.

Created by Jayanth Chennamangalam  
[http://jayanthc.github.io/yapp/](http://jayanthc.github.io/yapp/)

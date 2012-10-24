#
# Makefile for Yet Another Pulsar Processor
#
# Created by Jayanth Chennamangalam on 2008.11.17
#

# C compiler and flags
CC = gcc
CFLAGS = -std=gnu99 -pedantic -Wall
CFLAGS_C_DEBUG = $(CFLAGS) -g -c
CFLAGS_C_RELEASE = $(CFLAGS) -O3 -c
ifeq ($(OPT_DEBUG), yes)
CFLAGS_C = $(CFLAGS_C_DEBUG)
else
CFLAGS_C = $(CFLAGS_C_RELEASE)
endif
CFLAGS_L_DEBUG = $(CFLAGS) -g
CFLAGS_L_RELEASE = $(CFLAGS) -O3
ifeq ($(OPT_DEBUG), yes)
CFLAGS_L = $(CFLAGS_L_DEBUG)
else
CFLAGS_L = $(CFLAGS_L_RELEASE)
endif

# enable/disable the debug flag
ifeq ($(OPT_DEBUG), yes)
DDEBUG = -DDEBUG
endif

# linker flags
LFLAGS_PGPLOT_DIR = # define if not in $PATH
# in some cases, linking needs to be done with the X11 library, in which case
# append '-lX11' (and possibly the path to the library) to the line below
LFLAGS_PGPLOT = $(LFLAGS_PGPLOT_DIR) -lpgplot -lcpgplot
LFLAGS_MATH = -lm
LFLAGS_SNDFILE = -lsndfile

# directories
SRCDIR = src
MANDIR = man
IDIR = src
BINDIR = bin
# binary installation directory - modify if needed
BININSTALLDIR = /usr/local/bin
# man page installation directory - modify if needed
MANINSTALLDIR = /usr/local/share/man/man1

# command definitions
DELCMD = rm

all: yapp_makever \
	 yapp_version.o \
	 yapp_erflookup.o \
	 yapp_common.o \
	 yapp_viewmetadata.o \
	 yapp_viewmetadata \
	 colourmap.o \
	 yapp_viewdata.o \
	 yapp_viewdata \
	 yapp_ft.o \
	 yapp_ft \
	 yapp_dedisp.o \
	 yapp_dedisp \
	 yapp_fold.o \
	 yapp_fold \
	 tags
#	 yapp_dedisperse.o \
	 set_colours.o \
	 yapp_dedisperse \
	 reorderdds.o \
	 reorderdds \
	 yapp_pulsarsnd.o \
	 yapp_pulsarsnd \

yapp_makever: $(SRCDIR)/yapp_makever.c
	$(CC) $(CFLAGS_L) $< -o $(IDIR)/$@
	$(IDIR)/$@
	$(DELCMD) $(IDIR)/$@

yapp_version.o: $(SRCDIR)/yapp_version.c
	$(CC) $(CFLAGS_C) $< -o $(IDIR)/$@

yapp_erflookup.o: $(SRCDIR)/yapp_erflookup.c $(SRCDIR)/yapp.h
	$(CC) $(CFLAGS_C) $< -o $(IDIR)/$@

yapp_common.o: $(SRCDIR)/yapp_common.c $(SRCDIR)/yapp.h
	$(CC) $(CFLAGS_C) $(DDEBUG) $< -o $(IDIR)/$@

yapp_viewmetadata.o: $(SRCDIR)/yapp_viewmetadata.c $(SRCDIR)/yapp.h
	$(CC) $(CFLAGS_C) $< -o $(IDIR)/$@

# even though yapp_viewmetadata does not use PGPLOT, yapp_common does
yapp_viewmetadata: $(IDIR)/yapp_viewmetadata.o $(IDIR)/yapp_version.o \
	$(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o
	$(CC) $^ $(LFLAGS_PGPLOT) $(LFLAGS_MATH) -o $(BINDIR)/$@

colourmap.o: $(SRCDIR)/colourmap.c $(SRCDIR)/colourmap.h
	$(CC) $(CFLAGS_C) $< -o $(IDIR)/$@

yapp_viewdata.o: $(SRCDIR)/yapp_viewdata.c $(SRCDIR)/yapp.h
	$(CC) $(CFLAGS_C) $(DDEBUG) $< -o $(IDIR)/$@

yapp_viewdata: $(IDIR)/yapp_viewdata.o $(IDIR)/yapp_version.o \
	$(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o $(IDIR)/colourmap.o
	$(CC) $^ $(LFLAGS_PGPLOT) $(LFLAGS_MATH) -o $(BINDIR)/$@

yapp_ft.o: $(SRCDIR)/yapp_ft.c $(SRCDIR)/yapp.h
	$(CC) $(CFLAGS_C) $(DDEBUG) $< -o $(IDIR)/$@

yapp_ft: $(IDIR)/yapp_ft.o $(IDIR)/yapp_version.o \
	$(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o
	$(CC) $^ $(LFLAGS_PGPLOT) $(LFLAGS_MATH) -lfftw3f -o $(BINDIR)/$@

yapp_dedisp.o: $(SRCDIR)/yapp_dedisp.c $(SRCDIR)/yapp.h
	$(CC) $(CFLAGS_C) $(DDEBUG) $(DFC) $(SRCDIR)/yapp_dedisp.c -o $(IDIR)/$@

yapp_dedisp: $(IDIR)/yapp_dedisp.o
	$(CC) $(IDIR)/yapp_dedisp.o $(IDIR)/yapp_version.o $(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o \
		$(IDIR)/colourmap.o $(LFLAGS_PGPLOT) $(LFLAGS_MATH) -o $(BINDIR)/$@

yapp_fold.o: $(SRCDIR)/yapp_fold.c $(SRCDIR)/yapp.h $(SRCDIR)/yapp_sigproc.h
	$(CC) $(CFLAGS_C) $(DDEBUG) $< -o $(IDIR)/$@

yapp_fold: $(IDIR)/yapp_fold.o $(IDIR)/yapp_version.o \
	$(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o
	$(CC) $^ $(LFLAGS_PGPLOT) $(LFLAGS_MATH) -o $(BINDIR)/$@

ifeq ($(FC), g77)
DFC = -D_FC_F77_
set_colours.o: $(SRCDIR)/set_colours.f
	$(FC) $(FFLAGS) $(SRCDIR)/set_colours.f -o $(IDIR)/$@
else
DFC = -D_FC_F95_
set_colours.o: $(SRCDIR)/set_colours_f95.f
	$(FC) $(FFLAGS) $(SRCDIR)/set_colours_f95.f -o $(IDIR)/$@
endif

yapp_dedisperse.o: $(SRCDIR)/yapp_dedisperse.c $(SRCDIR)/yapp.h
	$(CC) $(CFLAGS_C) $(DFC) $(DDEBUG) $(SRCDIR)/yapp_dedisperse.c -o $(IDIR)/$@

yapp_dedisperse: $(IDIR)/yapp_dedisperse.o
	$(FC) $(IDIR)/yapp_dedisperse.o $(IDIR)/yapp_version.o $(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o \
		$(IDIR)/set_colours.o $(LFLAGS_PGPLOT) $(LFLAGS_MATH) -o $(BINDIR)/$@

#killrfi.o: $(SRCDIR)/killrfi.c
#	$(CC) $(CFLAGS_C) $(DDEBUG) $(SRCDIR)/killrfi.c -o $(IDIR)/$@
#
#killrfi: $(IDIR)/killrfi.o
#	$(FC) $(IDIR)/killrfi.o $(IDIR)/yapp_version.o $(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o \
#		#$(IDIR)/set_colours.o
#		$(LFLAGS_PGPLOT) $(LFLAGS_MATH) -o $(BINDIR)/$@

reorderdds.o: $(SRCDIR)/reorderdds.c
	$(CC) $(CFLAGS_C) $(DFC) $(SRCDIR)/reorderdds.c -o $(IDIR)/$@

reorderdds: $(IDIR)/reorderdds.o
	$(FC) $(IDIR)/reorderdds.o $(IDIR)/yapp_version.o $(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o \
        $(IDIR)/set_colours.o $(LFLAGS_PGPLOT) $(LFLAGS_MATH) -o $(BINDIR)/$@

yapp_pulsarsnd.o: $(SRCDIR)/yapp_pulsarsnd.c $(SRCDIR)/yapp.h
	$(CC) $(CFLAGS_C) $(SRCDIR)/yapp_pulsarsnd.c -o $(IDIR)/$@

yapp_pulsarsnd: $(IDIR)/yapp_pulsarsnd.o
	$(CC) $(SRCDIR)/yapp_pulsarsnd.o $(IDIR)/yapp_version.o $(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o \
		$(LFLAGS_MATH) $(LFLAGS_SNDFILE) -o $(BINDIR)/$@

#gendispdata.o: $(SRCDIR)/gendispdata.c
#	$(CC) $(CFLAGS_C) $(SRCDIR)/gendispdata.c -o $(IDIR)/$@
#
#gendispdata: $(IDIR)/gendispdata.o
#	$(CC) $(SRCDIR)/gendispdata.c $(IDIR)/yapp_version.o $(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o \
#		-o $(BINDIR)/$@

# create the tags file
tags: $(SRCDIR)/yapp*.* $(SRCDIR)/colourmap*
	ctags $^

# install the man pages
install:
	@echo Copying binaries...
	cp $(BINDIR)/* $(BININSTALLDIR)
	@echo DONE
	@echo Copying man pages...
	cp $(MANDIR)/*.1 $(MANINSTALLDIR)
	@echo DONE

clean:
	$(DELCMD) $(SRCDIR)/yapp_version.c
	$(DELCMD) $(IDIR)/yapp_version.o
	$(DELCMD) $(IDIR)/yapp_erflookup.o
	$(DELCMD) $(IDIR)/yapp_common.o
	$(DELCMD) $(IDIR)/yapp_viewmetadata.o
	$(DELCMD) $(IDIR)/colourmap.o
	$(DELCMD) $(IDIR)/yapp_viewdata.o
	$(DELCMD) $(IDIR)/yapp_ft.o
	$(DELCMD) $(IDIR)/yapp_dedisp.o
	$(DELCMD) $(IDIR)/yapp_fold.o
#	$(DELCMD) $(IDIR)/set_colours.o
#	$(DELCMD) $(IDIR)/yapp_dedisperse.o
#	$(DELCMD) $(IDIR)/reorderdds.o
#	$(DELCMD) $(IDIR)/yapp_pulsarsnd.o
#	$(DELCMD) $(IDIR)/killrfi.o
#	$(DELCMD) $(IDIR)/gendispdata.o


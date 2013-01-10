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
	 yapp_dedisperse.o \
	 yapp_dedisperse \
	 yapp_smooth.o \
	 yapp_smooth

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

yapp_dedisperse.o: $(SRCDIR)/yapp_dedisperse.c $(SRCDIR)/yapp.h
	$(CC) $(CFLAGS_C) $(DDEBUG) $(DFC) $(SRCDIR)/yapp_dedisperse.c -o $(IDIR)/$@

yapp_dedisperse: $(IDIR)/yapp_dedisperse.o
	$(CC) $(IDIR)/yapp_dedisperse.o $(IDIR)/yapp_version.o $(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o \
		$(IDIR)/colourmap.o $(LFLAGS_PGPLOT) $(LFLAGS_MATH) -o $(BINDIR)/$@

yapp_smooth.o: $(SRCDIR)/yapp_smooth.c $(SRCDIR)/yapp.h $(SRCDIR)/yapp_sigproc.h
	$(CC) $(CFLAGS_C) $(DDEBUG) $< -o $(IDIR)/$@

yapp_smooth: $(IDIR)/yapp_smooth.o $(IDIR)/yapp_version.o \
	$(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o
	$(CC) $^ $(LFLAGS_PGPLOT) $(LFLAGS_MATH) -o $(BINDIR)/$@

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
	$(DELCMD) $(IDIR)/yapp_dedisperse.o
	$(DELCMD) $(IDIR)/yapp_smooth.o


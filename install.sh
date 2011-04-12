#
# Yet Another Pulsar Processor (YAPP) Installation Script
#
# Created by Jayanth Chennamangalam on 2011.04.14
#

CC=gcc
FC=gfortran
PGPLOT_DIR=/usr/lib
INSTALL_DIR=/usr/bin

# print title, description, etc.
echo "Yet Another Pulsar Processor (YAPP)"
echo -n "---------------------------------------------------------------------"
echo "----------"
echo "Created by Jayanth Chennamangalam"
echo ""

echo "You are about to commence installation of YAPP. Please enter the queried"
echo "values, or press enter to choose the default."
echo ""

# query user about compilers, paths, etc.
echo -n "C compiler [gcc]: "
read INPUT
if [ -n "$INPUT" ]
then
    CC=$INPUT
fi
echo -n "Fortran compiler [gfortran]: "
read INPUT
if [ -n "$INPUT" ]
then
    FC=$INPUT
fi
echo -n "Path to PGPLOT libraries [/usr/lib]: "
read INPUT
if [ -n "$INPUT" ]
then
    PGPLOT_DIR=$INPUT
fi
echo -n "Path to install directory [/usr/bin]: "
read INPUT
if [ -n "$INPUT" ]
then
    INSTALL_DIR=$INPUT
fi

# create Makefile
echo $CC
echo $FC
echo $PGPLOT_DIR
echo $INSTALL_DIR

# run make and make clean
make
make clean


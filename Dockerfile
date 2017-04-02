# use latest LTS version of Ubuntu
FROM ubuntu:latest

# set up shell stuff
#ENV PS1="\u@\h:\w\$ "
RUN echo "alias rm='rm -i'\nalias cp='cp -i'\nalias mv='mv -i'" >> /root/.bashrc

# update APT sources list to use multiverse (needed for pgplot5)
RUN echo "deb http://us.archive.ubuntu.com/ubuntu/ $(cat /etc/lsb-release | grep DISTRIB_CODENAME | cut -d '=' -f 2) multiverse" >> /etc/apt/sources.list
RUN echo "deb-src http://us.archive.ubuntu.com/ubuntu/ $(cat /etc/lsb-release | grep DISTRIB_CODENAME | cut -d '=' -f 2) multiverse" >> /etc/apt/sources.list
RUN echo "deb http://us.archive.ubuntu.com/ubuntu/ $(cat /etc/lsb-release | grep DISTRIB_CODENAME | cut -d '=' -f 2)-updates multiverse" >> /etc/apt/sources.list
RUN echo "deb-src http://us.archive.ubuntu.com/ubuntu/ $(cat /etc/lsb-release | grep DISTRIB_CODENAME | cut -d '=' -f 2)-updates multiverse" >> /etc/apt/sources.list

# update package index and install dependencies available on APT
RUN apt-get -y update && \
    apt-get install -y \
        gcc \
        git \
        g++ \
        libcfitsio-dev \
        libfftw3-dev \
        make \
        pgplot5 \
        wget

# get latest version of HDF5 (not available on APT) and install it
RUN wget https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.0-patch1/src/hdf5-1.10.0-patch1.tar.gz && \
    tar xzvf hdf5-1.10.0-patch1.tar.gz && \
    cd hdf5-1.10.0-patch1 && \
    ./configure --prefix=/usr/local/hdf5 && \
    make && \
    make install && \
    cd .. && \
    rm -f hdf5-1.10.0-patch1.tar.gz && \
ENV LD_LIBRARY_PATH=/usr/local/hdf5/lib

# fetch and install YAPP
RUN git clone https://github.com/jayanthc/yapp.git && \
    cd yapp && \
    make HDF5=yes && \
    mkdir /usr/local/share/man/man1 && \
    make install


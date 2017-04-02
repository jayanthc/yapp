FROM ubuntu:latest
RUN echo "deb http://us.archive.ubuntu.com/ubuntu/ $(cat /etc/lsb-release | grep DISTRIB_CODENAME | cut -d '=' -f 2) multiverse" >> /etc/apt/sources.list
RUN echo "deb-src http://us.archive.ubuntu.com/ubuntu/ $(cat /etc/lsb-release | grep DISTRIB_CODENAME | cut -d '=' -f 2) multiverse" >> /etc/apt/sources.list
RUN echo "deb http://us.archive.ubuntu.com/ubuntu/ $(cat /etc/lsb-release | grep DISTRIB_CODENAME | cut -d '=' -f 2)-updates multiverse" >> /etc/apt/sources.list
RUN echo "deb-src http://us.archive.ubuntu.com/ubuntu/ $(cat /etc/lsb-release | grep DISTRIB_CODENAME | cut -d '=' -f 2)-updates multiverse" >> /etc/apt/sources.list
RUN apt-get -y update && apt-get install -y \
        gcc \
        git \
        libcfitsio-dev \
        libfftw3-dev \
        make \
        pgplot5
RUN mkdir /usr/local/share/man/man1
RUN git clone https://github.com/jayanthc/yapp.git && cd yapp && make && make install

FROM ubuntu:latest
MAINTAINER "Antoine COZZI" <antoine.cozzi@gmail.com>

RUN apt-get update
RUN apt-get install -y --no-install-recommends build-essential libssl-dev openssl sox wget git unzip autoconf libtool automake
RUN wget --no-check-certificate https://github.com/cinecert/asdcplib/archive/master.zip && unzip master.zip && rm master.zip
RUN cd /asdcplib-master && autoreconf -if && ./configure --enable-freedist --enable-as-02 && make -j3 && make install && make clean
RUN rm -rf /asdcplib-master
RUN ldconfig
RUN mkdir -p /mnt

## Clean up
RUN apt-get remove -y wget build-essential
RUN rm -rf /var/lib/apt/lists/*

CMD ["bash"]

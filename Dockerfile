FROM ubuntu:latest
LABEL author="Antoine COZZI <antoine.cozzi@gmail.com>"
ARG version=rel_2_10_32

RUN apt-get update
RUN apt-get install -y --no-install-recommends build-essential libssl-dev openssl sox wget git unzip autoconf libtool automake
RUN wget --no-check-certificate https://github.com/cinecert/asdcplib/archive/refs/tags/$version.zip && unzip $version.zip && rm $version.zip
RUN cd /asdcplib-$version && autoreconf -if && ./configure --enable-freedist --enable-as-02 && make -j3 && make install && make clean
RUN rm -rf /asdcplib-$version
RUN ldconfig
RUN mkdir -p /mnt

## Clean up
RUN apt-get remove -y wget build-essential
RUN rm -rf /var/lib/apt/lists/*

CMD ["bash"]

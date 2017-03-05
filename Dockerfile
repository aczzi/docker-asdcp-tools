FROM ubuntu:latest
MAINTAINER "Antoine COZZI" <antoine.cozzi@gmail.com>

RUN apt-get update
RUN apt-get install -y build-essential
RUN apt-get install -y fakeroot devscripts
RUN apt-get install -y libssl-dev
RUN apt-get install -y debhelper
RUN apt-get install -y openssl
RUN apt-get install -y sox
RUN apt-get install -y wget

RUN wget http://download.cinecert.com/asdcplib/asdcplib-2.7.19.tar.gz && tar xvzf asdcplib-2.7.19.tar.gz && rm asdcplib-2.7.19.tar.gz
RUN cd /asdcplib-2.7.19 && ./configure --prefix=/asdcplib-2.7.19/build/ && make -j3 && make install && make clean
RUN cp -r /asdcplib-2.7.19/build/bin/* /usr/local/bin 
RUN mkdir -p /usr/local/include/asdcplib && cp -r /asdcplib-2.7.19/build/include/* /usr/local/include/asdcplib 
RUN cp -r /asdcplib-2.7.19/build/lib/* /usr/lib
RUN cd / && rm -rf /asdcplib-2.7.19

RUN mkdir -p /mnt
WORKDIR /mnt
CMD ["bash"]
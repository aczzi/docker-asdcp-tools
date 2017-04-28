FROM ubuntu:latest
MAINTAINER "Antoine COZZI" <antoine.cozzi@gmail.com>

RUN apt-get update
RUN apt-get install -y --no-install-recommends build-essential libssl-dev openssl sox wget

RUN wget http://download.cinecert.com/asdcplib/asdcplib-2.7.19.tar.gz && tar xvzf asdcplib-2.7.19.tar.gz && rm asdcplib-2.7.19.tar.gz
RUN cd /asdcplib-2.7.19 && ./configure --prefix=/asdcplib-2.7.19/build/ --enable-as-02 && make -j3 && make install && make clean
RUN cp -r /asdcplib-2.7.19/build/bin/* /usr/local/bin 
RUN mkdir -p /usr/local/include/asdcplib && cp -r /asdcplib-2.7.19/build/include/* /usr/local/include/asdcplib 
RUN cp -r /asdcplib-2.7.19/build/lib/* /usr/lib
RUN rm -rf /asdcplib-2.7.19
RUN mkdir -p /mnt

## Clean up
RUN apt-get remove -y wget build-essential
RUN rm -rf /var/lib/apt/lists/*

CMD ["bash"]
FROM gcc

RUN mkdir /src
WORKDIR /src

RUN apt-get update
RUN apt-get -y libboost-all-dev
RUN apt-get -y libgoogle-glog-dev

ADD ./src /src
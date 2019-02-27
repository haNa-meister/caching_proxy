FROM gcc 

RUN mkdir /src
WORKDIR /src

RUN apt-get update
RUN apt-get install -y libboost-all-dev
RUN apt-get install -y libgoogle-glog-dev

ADD ./src /src

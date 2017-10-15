FROM ubuntu:14.04

RUN apt-get update -y && \
  apt-get upgrade -y && \
  apt-get install -y \
    build-essential

ADD clang-llvm /usr/local/

COPY . /app
# ADD https://github.com/MarinShalamanov/competitive-linter/archive/master.zip .  
# RUN unzip master.zip && rm master.zip 
RUN cd app && ./build_vs_released_binary.sh /usr/local/ 
RUN mkdir /usr/local/lib/comp-linter/
RUN mv app/build/*.so /usr/local/lib/comp-linter/ 
RUN rm -R app 

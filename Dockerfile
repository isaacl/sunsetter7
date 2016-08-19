FROM debian:oldstable
RUN apt-get update && apt-get install -y make g++
VOLUME /home/builder/Sunsetter
WORKDIR /home/builder/Sunsetter
RUN groupadd -r builder && useradd -r -g builder builder
USER builder
CMD make EXE=sunsetter-x86_64 CC=gcc CXX=g++

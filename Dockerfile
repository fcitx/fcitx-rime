FROM ubuntu:18.04

RUN apt-get update && apt-get install -y \
  git cmake g++ pkg-config \
  qt5-default libfcitx-qt5-dev \
  librime-dev

WORKDIR /app
RUN git clone https://github.com/fcitx/fcitx-rime.git
WORKDIR /app/fcitx-rime
RUN mkdir -p /usr/share/rime/data
RUN cmake .
RUN make
RUN make install


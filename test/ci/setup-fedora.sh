#! /bin/bash

dnf -y update
dnf -y groupinstall "C Development Tools and Libraries"
dnf -y install binutils boost-devel cmake ca-certificates coreutils diffutils \
  findutils fmt-devel gcc-c++ git hostname ninja-build patch procps python \
  redhat-lsb-core sed tar wget which
dnf -y clean all

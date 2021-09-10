#! /bin/bash

dnf -y update
dnf -y install https://alfa-ci.gsi.de/packages/rpm/fedora-$1-x86_64/fairsoft-release-dev.rpm
dnf -y install ninja-build 'dnf-command(builddep)' libasan liblsan libtsan libubsan clang-tools-extra
dnf -y builddep fairlogger
dnf -y clean all

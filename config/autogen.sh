#!/bin/bash

set -xe

aclocal
autoconf
automake
./configure

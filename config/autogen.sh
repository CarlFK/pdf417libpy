#!/bin/bash

set -xe

aclocal
autoconf
automake --add-missing
./configure

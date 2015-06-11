#!/bin/sh

mkdir -p build && cd build && cmake .. -DTEST=on -DCMAKE_BUILD_TYPE=Debug $*

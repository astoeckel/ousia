#!/bin/sh

cd build && cmake .. -DTEST=on -DCMAKE_BUILD_TYPE=Debug && make && make test


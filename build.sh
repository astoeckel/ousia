#!/bin/sh

cd build && cmake .. -Dtest=on && make && make test


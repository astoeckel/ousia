#!/bin/sh

# Download and unzip googletest
if [ ! -e gtest-1.7.0.zip ]; then
	wget https://googletest.googlecode.com/files/gtest-1.7.0.zip
	unzip gtest-1.7.0.zip
fi

# Download utf8-cpp (header only library)
if [ ! -e utf8cpp ]; then
	svn checkout svn://svn.code.sf.net/p/utfcpp/code/v2_0/source utf8cpp
fi

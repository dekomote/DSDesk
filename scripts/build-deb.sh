#!/bin/bash

cmake -S . -B build
cmake --build build
cpack --config build/CPackConfig.cmake -G DEB

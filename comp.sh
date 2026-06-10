#!/usr/bin/env bash
set -e

rm -rf build

cmake -B build \
  -DCMAKE_C_COMPILER="$(which mpicc)" \
  -DCMAKE_CXX_COMPILER="$(which mpic++)" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -Dxios_DIR="/home/melt/sync/cambridge/projects/current/sasip/xios"

cmake --build build -j 8

#!/usr/bin/env bash

source /home/melt/.envs/nextsim/bin/activate
spack env activate -p nextsim-cali
# export LD_LIBRARY_PATH="/lib/x86_64-linux-gnu/:$LD_LIBRARY_PATH"
export PATH="/home/melt/sync/cambridge/projects/current/sasip/nextsimdg/domain_decomp/build:$PATH"

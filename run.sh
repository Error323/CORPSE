#!/bin/bash

BIN="./release/CORPSE"
ARG="./data/params.lua"

export LD_LIBRARY_PATH="./release"
$BIN $ARG

#!/bin/bash

BIN="./bin/PathEngine.bin"
ARG="./data/params.lua"

export LD_LIBRARY_PATH="./bin/"
# ./bin/PathEngine.bin ./data/params.lua
$BIN $ARG

#!/bin/bash

BIN="./qtcreator/CORPSE"
ARG="./data/params.lua"

export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:"./qtcreator"
$BIN $ARG

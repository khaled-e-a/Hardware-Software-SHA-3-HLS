#! /bin/bash

# Get the scripts path
DIR="$( cd "$( dirname "$0" )" && pwd )"
DIR="$( cd "$DIR"/../ && pwd )"
cd $DIR/system_virtex

vivado -mode gui -source $DIR/scripts_virtex/build_project.tcl -tclargs $DIR
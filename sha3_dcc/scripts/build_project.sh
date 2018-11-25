#! /bin/bash

# Get the scripts path
DIR="$( cd "$( dirname "$0" )" && pwd )"
DIR="$( cd "$DIR"/../ && pwd )"
cd $DIR/system

vivado -mode gui -source $DIR/scripts/build_project.tcl -tclargs $DIR
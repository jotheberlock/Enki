#!/bin/bash

COMPILER="../enki linux_amd64_target.ini"
OUTPUT="./a.out"

function compile()
{
    $COMPILER $1 &> "$0_compileroutput.txt"
    if [[ $? -ne 0 ]]; then
	echo -n "$0: "
	tput setf 1
	echo "NOT COMPILED"
	tput op
	exit 2
    fi
}

function skip()
{
    echo -n "$0: "
    tput setf 5
    echo "SKIP"
    tput op
    exit 0
}

function success()
{
    echo -n "$0: "
    tput setf 2
    echo "SUCCESS"
    tput op
    exit 0
}

function failure()
{
    echo -n "$0: "
    tput setf 4
    echo "FAILURE"
    tput op
    exit 1
}
    
function expectExit()
{
    if [[ $? -eq $1 ]]; then
	success
    else
	failure
    fi
}

function expectString()
{
    if [[ $1 == $2 ]]; then
	success
    else
	failure
    fi
}

function linuxonly()
{
    if [[ `uname` != "Linux" ]]; then
	skip
    fi
}
    

#!/bin/bash

if [[ -f "../enki" ]]; then
	COMPILER="../enki"
elif [[ -f "../parsey/x64/Debug/enki.exe" ]]; then
	COMPILER="../parsey/x64/Debug/enki.exe"
else
	echo "Can't find compiler!"
	exit 1
fi

if [[ -f "../parsey/x64/Debug/enki.exe" ]]; then
    OUTPUT="./a.exe"
elif [[ `uname` == "Linux" ]]; then
    OUTPUT="./a.out"
elif [[ `uname` == "Darwin" ]]; then
    OUTPUT="./a.out"
elif [[ `uname` = CYGWIN* ]]; then
    OUTPUT="./a.exe"
else
    echo "Unknown platform!"
fi

function compile()
{
    $COMPILER $1 &> "$0_compileroutput.txt"
	RESULT=$?
    if [[ $RESULT -ne 0 ]]; then
	echo -n "$0: "
	tput setf 1
	echo "NOT COMPILED - $RESULT"
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

function expectResult()
{
    if [[ $? -eq $1 ]]; then
	if [[ $2 == $3 ]]; then
	    success
	else
	    failure
	fi
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

function amd64only()
{
    if [[ `uname -m` != "x86_64" ]]; then
	skip
    fi
}

function windowsonly()
{
    if [[ ! -f "../parsey/x64/Debug/enki.exe" ]]; then
		skip
	fi
}

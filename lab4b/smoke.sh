#!/bin/bash

#NAME:Ryan Wakefield
#EMAIL: ryanwakefield@g.ucla.edu
#ID: 904975422

logfile=$(mktemp tmp.XXX)

make

echo "...checking arguments"
./lab4b --bogus < /dev/tty > /dev/null 2>STDERR
if [ $? -ne 1 ]
then
	echo "no detection of invalid option"
else
	echo "Correctly detected invalid option, exitted properly"
fi

echo "Testing Arguments to be passed in"
./lab4b --period=2 --scale=F --log=$logfile <<-EOF
SCALE=F
SCALE=C
PERIOD=3
STOP
START
LOG THIS IS A TEST
OFF
EOF


if [ $? -ne 0 ]
then
	echo "Error, program didn't exit properly"
fi

if [ ! -s $logfile ]
then
	echo "No logfile created"
else
	for input in SCALE=F SCALE=C PERIOD=3 STOP START "LOG THIS IS A TEST" OFF
	do
		grep "$input" $logfile
		if [ $? -ne 0 ]
		then
			echo "$input was not logged..."
		else
			echo "$input found..."
		fi
	done
fi

echo "Exitting smoke test" 

rm -f $logfile




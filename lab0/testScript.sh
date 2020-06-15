#!/bin/bash

TEMP_INP=$(mktemp tmp.XXX)
TEMP_OUT=$(mktemp tmp.XXX)

tests_passed=0
tests_failed=0

echo "This is my input file" > $TEMP_INP
function failed_test
{
        echo "Test case $1 failed."
	tests_failed=$((tests_failed+1)) 
}

function passed_test
{
        echo "Test case $1 passed."
	tests_passed=$((tests_passed+1))
}

function check_result
{
        if [ $2 -eq $3 ]
        then
        passed_test $1
        else
        failed_test $1
        fi
}

function check_result_string {
    
    if [ $2 -eq $3 ] && cmp -s "$TEMP_INP" "$TEMP_OUT" 
        then
        passed_test $1
        else
        failed_test $1
    fi

    echo " " > $TEMP_OUT
}

function check_output {

    if [ $2 -eq $3 ] && ! cmp -s "$TEMP_INP" "$TEMP_OUT"
        then
        passed_test $1
        else
        failed_test $1
    fi
    
    echo " " > $TEMP_OUT
}

function start_case {
    printf "==============Beginning test case: $1, checking $2===============\n\n"

}

function end_case {
    printf "\n==============End of test case $1===============\n\n"
}

start_case 1 "for input option with no given argument"
printf "Running: ./lab0 --input \n"
./lab0 --input
check_result 1 $? 1
end_case 1

start_case 2 "for bogus argument '--NotAnArg'"
printf "Running: ./lab0 --NotAnArg \n"
./lab0 --NotAnArg
check_result 2 $? 1
end_case 2

start_case 3 "segfault option casuses function to exit immediately"
printf "Running: ./lab0 --segfault \n"
./lab0 --segfault
check_result 3 $? 139
end_case 3

start_case 4 "segfault option causes function to exit immediately, even with invalid input file."
printf "Running: ./lab0 --segfault --input=h3ll0.txt \n"
./lab0 --segfault --input=h3ll0.txt
check_result 4 $? 2
end_case 4

start_case 5 "that '--catch' will catch segfault if '--segfault' option is inputted'"
printf "Running: ./lab0 --segfault --catch \n"
./lab0 --segfault --catch
check_result 5 $? 4
end_case 5

start_case 6 "to see if files transfer properly"
printf "Running: ./lab0 --input=SomeRealFile \n"
./lab0 --input=$TEMP_INP 
check_result 6 $? 0
end_case 6

start_case 7 "that files transfer properly with input and output given"
printf "Running: ./lab0 --catch --input=SomeRealFile --output=SomeFile \n"
./lab0 --catch --input=$TEMP_INP --output=$TEMP_OUT
check_result_string 7 $? 0
end_case 7

start_case 8 "that the program will fail and exit when given input file that doesn't exist" 
printf "Running: ./lab0 --catch --input=NOTAFILE \n"
./lab0 --catch --input=NOTAFILE
check_result 8 $? 2
end_case 8

start_case 9 "that the program will run normally if '--check' argument is given and no '--segfault'"
printf "Running: ./lab0 --catch --input=$TEMP_INP --output=$TEMP_OUT \n"
./lab0 --catch --input=$TEMP_INP --output=$TEMP_OUT
check_result_string 9 $? 0
end_case 9

start_case 10 "to see that '--catch' and '--segfault' will exit before copying the input file to output"
printf "Running: ./lab0 --segafult --catch --input=$TEMP_INP --output=$TEMP_OUT \n" 
./lab0 --segfault --catch --input=$TEMP_INP --output=$TEMP_OUT
check_output 10 $? 4 
end_case 10

printf "Results: Cases Passed: $tests_passed, Cases Failed: $tests_failed \n"


printf "...Removing temporary files \n"

rm -f $TEMP_INP
rm -f $TEMP_OUT

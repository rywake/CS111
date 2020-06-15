#! /usr/local/cs/bin/gnuplot
#
# purpose:
#        generate data reduction graphs for the multi-threaded list project
#
# input: lab2_list.csv
#       1. test name
#       2. # threads
#       3. # iterations per thread
#       4. # lists
#       5. # operations performed (threads x iterations x (ins + lookup + delete))
#       6. run time (ns)
#       7. run time per operation (ns)
#
# output:
#       lab2_list-1.png ... cost per operation vs threads and iterations
#       lab2_list-2.png ... threads and iterations that run (un-protected) w/o failure
#       lab2_list-3.png ... threads and iterations that run (protected) w/o failure
#       lab2_list-4.png ... cost per operation vs number of threads
#
# Note:
#       Managing data is simplified by keeping all of the results in a single
#       file.  But this means that the individual graphing commands have to
#       grep to select only the data they want.
#
#       Early in your implementation, you will not have data for all of the
#       tests, and the later sections may generate errors for missing data.
#

#variable
billion = 1000000000

# general plot parameters
set terminal png
set datafile separator ","

set xtics

set title "List2b-1: Aggregate Throughput at 1000 Iterations"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Number of operations/second (Ops/sec)"
set logscale y 10
set output 'lab2b_1.png'
set key right top
plot \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(billion)/($7) \
        title 'List w/mutex' with linespoints lc rgb 'blue', \
     "< grep -e 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(billion)/($7) \
        title 'List w/spin-lock' with linespoints lc rgb 'green'

unset xtics
set xtics

set title "List2b-2: Avg Wait Time and Avg Time per Op at 1000 Iterations"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Average Time (ns)"
set logscale y 10
set output 'lab2b_2.png'
set key right top
plot \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($8) \
        title 'Average Wait Time for Lock' with linespoints lc rgb 'blue', \
     "< grep -e 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
        title 'Average time per operation' with linespoints lc rgb 'green'

set title "List2b-3: Successful Runs w/ Varying Threads, and Iterations"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Number of Iterations"
set logscale y 10
set output 'lab2b_3.png'
# note that unsuccessful runs should have produced no output
plot \
     "< grep list-id-none lab2b_list.csv" using ($2):($3) \
        title 'yield=id w/o lock' with points lc rgb 'red', \
	"< grep list-id-m lab2b_list.csv" using ($2):($3) \
        title 'yield=id w/ mutex' with points lc rgb 'green', \
	"< grep list-id-s lab2b_list.csv" using ($2):($3) \
        title 'yield=id w/ spin-lock' with points lc rgb 'blue'

set title "List2b-4: Throughput vs. Number of Threads with Mutex"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Operations per Second (Throughput) (Ops/sec)"
set logscale y 10
set output 'lab2b_4.png'
set key right top
plot \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(billion)/($7) \
        title 'List=1 w/mutex' with linespoints lc rgb 'blue', \
     "< grep -e 'list-none-m,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(billion)/($7) \
        title 'List=4 w/mutex' with linespoints lc rgb 'red', \
     "< grep -e 'list-none-m,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(billion)/($7) \
        title 'List=8 w/mutex' with linespoints lc rgb 'green', \
     "< grep -e 'list-none-m,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(billion)/($7) \
        title 'List=16 w/mutex' with linespoints lc rgb 'orange', \

set title "List2b-5: Throughput vs. Number of Threads with Spin-Lock"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Operations Per Second (Throughput) (Ops/sec)"
set logscale y 10
set output 'lab2b_5.png'
set key right top
plot \
     "< grep -e 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(billion)/($7) \
        title 'List=1 w/spin-lock' with linespoints lc rgb 'blue',\
     "< grep -e 'list-none-s,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(billion)/($7) \
        title 'List=4 w/spin-lock' with linespoints lc rgb 'green',\
     "< grep -e 'list-none-s,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(billion)/($7) \
        title 'List=8 w/spin-lock' with linespoints lc rgb 'red',\
     "< grep -e 'list-none-s,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(billion)/($7) \
        title 'List=16 w/spin-lock' with linespoints lc rgb 'orange'

#!/bin/sh

set -e

# Test and plot script for Radix Tree Implementation tests.

# Step through GitHub branches and obtain the executable test files
# Step through the executable test files
# Run each of them with different parameter values.
# Obtain the running time for each execution
# Store this data in an output file
# Plot a graph for each output file

# Graphs to be generated (each graph type will correspond
# to one test function):
#	1) Number of threads x Running Time

GRAPH_EXT="png"
GNUPLOT_TERM="png"
GRAPH_SIZE="1024,768"
GRAPH_STYLE="lines"
print_help()
{
	echo "Test and plot script for Radix Tree Implementation tests.\n"
	echo "Graph Type 1:\tNumber of Threads x Running time."
	echo "\t$0 1 <range> <keys> <tests> <max_threads>"
	exit
}

get_test_files()
{
	rm -rf test_files
	mkdir test_files

	git branch --list > branch_list.txt

	echo "Generating executable files...\n"

	while read -r line
	do
		name=${line##* }
		git checkout $name > /dev/null
		make > /dev/null && echo "Generated test file $name"
		mv radix_test ./test_files/$name	
	done < branch_list.txt

	echo ""

	rm branch_list.txt
	make clean > /dev/null
}

test1()
{
	cd test_files

	echo "Testing files...\n"

	for file in ./*
	do
		filename=${file##./}
		rm -rf $filename.data
		touch $filename.data

		echo "Testing $filename..."

		counter=1
		while [ $counter -le $4 ]
		do
			echo -n "$counter\t" >> $filename.data
			echo "Number of threads: $counter"

			taskset -c 0-$((counter-1)) $file $1 $2 $3 $counter >> $filename.data

			counter=$((counter+1))
		done

		echo "\nFinished testing $filename\n"
	done

	echo ""
}

plot_all()
{
	echo "Plotting graphs...\n"

	rm -rf plot_commands.gp
	touch plot_commands.gp

	echo -n "set terminal $GNUPLOT_TERM " >> plot_commands.gp
	echo "size $GRAPH_SIZE" >> plot_commands.gp
	echo "set output 'graph.$GRAPH_EXT'" >> plot_commands.gp
	echo -n "set title \"Numer of Threads x Running Time\\\n" >> plot_commands.gp
	echo "RANGE: $1 KEYS: $2 TESTS: $3\"">> plot_commands.gp

	echo "set xlabel 'Number of Threads'" >> plot_commands.gp
	echo "set ylabel 'Running Time (s)'" >> plot_commands.gp
	echo "set xtics 1" >> plot_commands.gp

	echo -n "plot [1:] " >> plot_commands.gp

	counter=1
	number_of_files="$(ls *.data | wc -l)"

	for file in ./*.data
	do
		branch=${file##./}
		branch=${branch%.data}
		echo -n "'$file' using 1:2 title \"$branch\" with $GRAPH_STYLE" >> plot_commands.gp

		counter=$((counter+1))
		if [ $counter -gt $number_of_files ]; then
			break
		fi

		echo -n ", " >> plot_commands.gp
	done

	gnuplot plot_commands.gp
	mv graph.$GRAPH_EXT ../graph.$GRAPH_EXT
	echo "Plotted graph.$GRAPH_EXT"

	cd ..
	rm -rf test_files

	echo ""
}

# Main #

if [ "$#" -ge 1 ] && [ "$1" = "help" ]; then
	print_help
fi

if [ "$#" -lt 1 ]; then
	echo "Error. Please specify the type of graph to generate"
	echo "Usage:\t$0 <type> <parameters>"
	echo "<type>: Graph type [1]"
	echo "<parameters>: Necessary parameters to generate the specified graph."
	echo "\nFor help, use $0 help"
	exit
fi

if [ "$1" -eq 1 ] && [ "$#" -lt 5 ]; then
	echo "Error. Insufficient number of parameters."
	echo "For graph type $1, please use: $0 $1 <range> <keys> <tests> <max_threads>"
	echo "\nFor help, use $0 help"
	exit
fi

get_test_files && echo "Done.\n"

case "$1" in
"1") 
	test1 $2 $3 $4 $5 && echo "Done.\n"
	;;
*)
	echo "Error. Unknown graph type."
	echo "Graph types:"
	echo "[1]\tNumber of threads x Running Time"
	echo "\nFor help, use $0 help"
	exit
esac

plot_all $2 $3 $4 && echo "Done."

git checkout master
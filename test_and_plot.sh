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

		counter=1
		while [ $counter -le $4 ]
		do
			echo -n "$counter\t" >> $filename.data

			START=$(date +%s.%N)
			eval "taskset -c 0-$counter $file $1 $2 $3 $counter > /dev/null"
			END=$(date +%s.%N)
			DIFF=$(echo "$END - $START" | bc)
			echo "$DIFF" >> $filename.data

			counter=$((counter+2))
		done

		echo "Finished testing $filename"
	done

	echo ""
}

plot_all()
{
	echo "Plotting graphs...\n"

	rm -rf ../graphs/
	mkdir ../graphs/

	for file in ./*.data
	do
		rm -rf plot_commands.gp
		touch plot_commands.gp

		filename=${file##./}
		output=${filename%.data}

		echo -n "set terminal $GNUPLOT_TERM " >> plot_commands.gp
		echo "size $GRAPH_SIZE" >> plot_commands.gp
		echo "set output \"$output.$GRAPH_EXT\"" >> plot_commands.gp
		echo "set title 'Branch $output'" >> plot_commands.gp
		echo "set xlabel 'Number of Threads'" >> plot_commands.gp
		echo "set ylabel 'Running Time'" >> plot_commands.gp
		echo "plot [1:] '$filename' using 1:2 with linespoints" >> plot_commands.gp

		gnuplot plot_commands.gp
		mv $output.$GRAPH_EXT ../graphs/$output.$GRAPH_EXT

		echo "Plotted $output.$GRAPH_EXT"
	done

	cd ..
	rm -rf test_files

	echo ""
}

# Main #

if [ "$#" -lt 1 ]; then
	echo "Error. Please specify the type of graph to generate"
	echo "Usage:\t$0 <type> <parameters>"
	echo "<type>: Graph type [1]"
	echo "<parameters>: Necessary parameters to generate the specified graph."
	exit
fi

if [ "$1" -eq 1 ] && [ "$#" -lt 5 ]; then
	echo "Error. Insufficient number of parameters."
	echo "For graph type $1, please use: $0 $1 <range> <keys> <tests> <max_threads>"
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
	exit
esac

plot_all && echo "Done."

#!/usr/bin/env python

# Test and plot script for Radix Tree Implementation tests.

# Obtain the executable test files
# Step through the executable test files
# Run each of them with different parameter values.
# Obtain the running time for each execution
# Store this data in an output file
# Plot a graph for each output file

# Graphs to be generated (each graph type will correspond
# to one test function):
#	1) Number of Threads x Running Time
#	2) Number of Threads x Throughput (Lookups/Execution Time)

import glob
import os
import shutil
import subprocess as subp
import sys

# Error checking

MILLION = 1E6

args = sys.argv

filename = args[0]
test_file = "radix_test"
num_args = len(args)

def print_help(args):
	print("Test and plot script for Radix Tree Implementation tests.\n")
	print("Graph Type 1:\tNumber of Threads x Running time.")
	print("\t{} 1 <tree_range> <keys> <lookups> <tests> "
		"<max_threads>".format(filename))
	print("\nGraph Type 2:\tNumber of threads x Throughput")
	print("\t{} 2 <tree_range> <keys> <lookups> <tests> "
		"<max_threads>".format(filename))

	sys.exit()

if (num_args >= 2 and args[1] == "help"):
	print_help(args)

if (num_args < 2):
	print("Error. Please specify the type of graph to generate")
	print("\nFor help, use {} help".format(filename))
	sys.exit()

graph_type = int(args[1])

if (graph_type <= 0):
	print("Error. Invalid type of graph.")
	print("\nFor help, use {} help".format(filename))
	sys.exit()

if (graph_type <= 2 and num_args < 7):
	print("Error. Insufficient number of parameters.")
	print("\nFor help, use {} help".format(filename))
	sys.exit()

# Parameters

graph_ext = "png"
gnuplot_term = "png"
graph_size = "1024,768"
graph_style = "lines"
time_unit = "s"

tree_range = int(args[2])
keys = int(args[3])
lookups = int(args[4])
tests = int(args[5])
threads = int(args[6])

implementations = [
	"lock_level",
	"sequential",
	"lock_node",
	"lock_subtree",
	"lockless"
]

# Functions

def greetings():
	print("Test and plot script for Radix Tree Implementations\n".upper())


def get_test_files():
	print("Generating executable files...\n")

	shutil.rmtree("{}/test_files".format(os.getcwd()), ignore_errors = True)
	subp.call(["mkdir", "test_files"])
	error = subp.call(["make", "all", "clear"])

	if (error != 0):
		print("Compilation error. Aborting.")
		sys.exit()

	subp.call(["mv", test_file, "test_files/{}".format(test_file)])

	print("\nDone.\n")


def test1():
	print("Testing files...\n")

	os.chdir("{}/test_files".format(os.getcwd()))

	for f in implementations:
		try:
			os.remove("{}.dat".format(f))
		except OSError:
			pass

		datafile = open("{}.dat".format(f), 'w')

		print("Testing {}...".format(f))

		counter = 1
		while (counter <= threads):
			datafile.write("{}\t".format(counter))
			print("Number of threads: {}".format(counter))

			output = subp.check_output(["taskset", "-c",
				"0-{}".format(counter - 1),
				"valgrind",
				"./{}".format(test_file),
				"-r {}".format(str(tree_range)),
				"-k {}".format(str(keys)),
				"-l {}".format(str(lookups)),
				"-t {}".format(str(tests)),
				"-p {}".format(str(counter)),
				"-i {}".format(f)])

			datafile.write("{}".format(output))

			if (counter == 1):
				counter = counter * 4
			else:
				counter = counter + 4

		print("\nFinished testing {}.\n".format(f))
		datafile.close()

	print("Done.\n")


def test2():
	print("Testing files...\n")

	os.chdir("{}/test_files".format(os.getcwd()))

	for f in implementations:
		try:
			os.remove("{}.data".format(f))
		except OSError:
			pass

		datafile = open("{}.data".format(f), 'w')

		print("Testing {}...".format(f))

		counter = 1
		while (counter <= threads):
			datafile.write("{}\t".format(counter))
			print("Number of threads: {}".format(counter))

			run_time = subp.check_output(["taskset", "-c",
				"valgrind",
				"0-{}".format(counter - 1),
				"./{}".format(test_file),
				"-r {}".format(str(tree_range)),
				"-k {}".format(str(keys)),
				"-l {}".format(str(lookups)),
				"-t {}".format(str(tests)),
				"-p {}".format(str(counter)),
				"-i {}".format(f)])

			num_lookups = lookups * tests * counter

			throughput = (num_lookups / MILLION) / float(run_time)

			datafile.write("{}\n".format(throughput))

			if (counter == 1):
				counter = counter * 4
			else:
				counter = counter + 4

		print("\nFinished testing {}.\n".format(f))
		datafile.close()

	print("Done.\n")


def plot_all():
	print("Plotting graphs...\n")

	try:
		os.remove("plot_commands.gp")
	except OSError:
		pass

	plot_cmds = open("plot_commands.gp", 'w')

	plot_cmds.write("set terminal {} ".format(gnuplot_term))
	plot_cmds.write("size {}\n".format(graph_size))
	plot_cmds.write("set output '{}/../graph.{}'\n".format(os.getcwd(),
		graph_ext))

	if (graph_type == 1):
		plot_cmds.write("set title \"Number of Threads x Running Time\\n")
		plot_cmds.write("RANGE: {} KEYS: {} LOOKUPS: {} TESTS: {}\"\n"
		.format(tree_range, keys, lookups, tests))
		plot_cmds.write("set xlabel 'Number of Threads'\n")
		plot_cmds.write("set ylabel 'Running Time ({})'\n".format(time_unit))

	if (graph_type == 2):
		plot_cmds.write("set title \"Number of Threads x Throughput\\n")
		plot_cmds.write("RANGE: {} KEYS: {} LOOKUPS: {} TESTS: {}\"\n"
		.format(tree_range, keys, lookups, tests))
		plot_cmds.write("set xlabel 'Number of Threads'\n")
		plot_cmds.write("set ylabel 'Throughput (M lookups/{})'\n"
			.format(time_unit))

	plot_cmds.write("plot [1:] ")

	counter = 1
	num_files = len(glob.glob1(os.getcwd(), "*.data"))

	for f in os.listdir(os.getcwd()):
		if f.endswith(".data"):
			f_name = f.replace(".data", "")

			plot_cmds.write("'{}' using 1:2 title \"{}\" with {}"
				.format(f, f_name, graph_style))

			counter = counter + 1
			if (counter > num_files):
				break;

			plot_cmds.write(", ")

	plot_cmds.close()
	subp.call(["gnuplot", "plot_commands.gp"])

	print("Plotted graph.{}".format(graph_ext))
	print("Done\n")


def main():
	greetings()

	get_test_files()

	if (graph_type == 1):
		test1()

	elif (graph_type == 2):
		test2()

	else:
		print(graph_type)
		print("Error. Unknown graph type.")
		print("\nFor help, use {} help".format(filename))
		sys.exit()

	plot_all()


main()

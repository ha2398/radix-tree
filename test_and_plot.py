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

args = sys.argv

filename = args[0]
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

GRAPH_EXT = "png"
GNUPLOT_TERM = "png"
GRAPH_SIZE = "1024,768"
GRAPH_STYLE = "lines"
TIME_UNIT = "ns"

tree_range = int(args[2])
keys = int(args[3])
lookups = int(args[4])
tests = int(args[5])
threads = int(args[6])

# Functions

def greetings():
	print("Test and plot script for Radix Tree Implementations\n".upper())


def get_test_files():
	print("Generating executable files...\n")

	shutil.rmtree("{}/test_files".format(os.getcwd()), ignore_errors = True)
	subp.call(["mkdir", "test_files"])
	subp.call(["make", "all", "clear"])

	execs = ["master", "p_lock_level", "p_lock_node", "p_no_lock",
		 "p_lock_subtree"]

	for f in execs:
		subp.call(["mv", f, "test_files/{}".format(f)])

	print("\nDone.\n")


def test1():
	print("Testing files...\n")

	os.chdir("{}/test_files".format(os.getcwd()))

	for f in os.listdir(os.getcwd()):
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

			output = subp.check_output(["taskset", "-c",
				"0-{}".format(counter - 1), "./{}".format(f),
				str(tree_range), str(keys), str(lookups),
				str(tests), str(counter)])

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

	for f in os.listdir(os.getcwd()):
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
				"0-{}".format(counter - 1), "./{}".format(f),
				str(tree_range), str(keys), str(lookups),
				str(tests), str(counter)])

			num_lookups = lookups * tests * threads
			throughput = num_lookups / float(run_time)

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

	plot_cmds.write("set terminal {} ".format(GNUPLOT_TERM))
	plot_cmds.write("size {}\n".format(GRAPH_SIZE))
	plot_cmds.write("set output '{}/../graph.{}'\n".format(os.getcwd(),
		GRAPH_EXT))

	if (graph_type == 1):
		plot_cmds.write("set title \"Number of Threads x Running Time\\n")
		plot_cmds.write("RANGE: {} KEYS: {} TESTS: {}\"\n"
		.format(tree_range, keys, tests))
		plot_cmds.write("set xlabel 'Number of Threads'\n")
		plot_cmds.write("set ylabel 'Running Time ({})'\n".format(TIME_UNIT))
	
	if (graph_type == 2):
		plot_cmds.write("set title \"Number of Threads x Throughput\\n")
		plot_cmds.write("RANGE: {} KEYS: {} TESTS: {}\"\n"
		.format(tree_range, keys, tests))
		plot_cmds.write("set xlabel 'Number of Threads'\n")
		plot_cmds.write("set ylabel 'Throughput (lookups/{})'\n"
			.format(TIME_UNIT))

	plot_cmds.write("plot [1:] ")

	counter = 1
	num_files = len(glob.glob1(os.getcwd(), "*.data"))

	for f in os.listdir(os.getcwd()):
		if f.endswith(".data"):
			f_name = f.replace(".data", "")

			plot_cmds.write("'{}' using 1:2 title \"{}\" with {}"
				.format(f, f_name, GRAPH_STYLE))

			counter = counter + 1
			if (counter > num_files):
				break;

			plot_cmds.write(", ")

	plot_cmds.close()
	subp.call(["gnuplot", "plot_commands.gp"])

	print("Plotted graph.{}".format(GRAPH_EXT))
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
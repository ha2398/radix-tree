#!/usr/bin/env python

# Test and plot script for Radix Tree Implementation tests.

# 1) Compile the source code and obtain the executable test file.
# 2) Run it for each of the implementations with different parameter values.
# 3) Obtain the running time for each execution.
# 4) Store this data in an output file.
# 5) Plot a graph for each output file.

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
proc_list_policy = "--policy=compact-smt"
num_args = len(args)

def mean(sample):
        return (sum(sample) / len(sample))


def corrected_stdev(sample):
	size = len(sample)
	if (size == 1):
		return 0

        m = mean(sample)
        diff = [((x - m) ** 2) for x in sample]

        return ((1./(size - 1)) * sum(diff)) ** (0.5)


def print_help(args):
	print("Test and plot script for Radix Tree Implementation tests.\n")
	print("Graph Type 1:\tNumber of Threads x Running time.")
	print("\t{} 1 <bits> <radix> <keys> <lookups> <tests> "
		"<max_threads>".format(filename))
	print("\nGraph Type 2:\tNumber of threads x Throughput")
	print("\t{} 2 <bits> <radix> <keys> <lookups> <tests> "
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

if (num_args < 8):
	print("Error. Insufficient number of parameters.")
	print("\nFor help, use {} help".format(filename))
	sys.exit()

# Parameters

graph_ext = "png"
gnuplot_term = "png"
graph_size = "1200,900"
graph_style = "lines"
time_unit = "s"

bits = int(args[2])
radix = int(args[3])
keys = int(args[4])
lookups = int(args[5])
tests = int(args[6])
threads = int(args[7])

implementations = [
	"sequential",
	"lock_level",
	"lock_node",
	"lock_subtree",
	"lockless"
]

graph_colors = {
	"sequential": "#FF0000",
	"lock_level": "#0011FF",
	"lock_node": "#007517",
	"lock_subtree": "#D9D500",
	"lockless": "#FF00FF"
}

error_color = '#E69F00';

# Functions

def greetings():
	print("Test and plot script for Radix Tree Implementations\n"
		.upper())


def get_test_files():
	print("Generating executable files...\n")

	shutil.rmtree("{}/test_files".format(os.getcwd()),
		ignore_errors = True)

	subp.call(["mkdir", "test_files"])
	error = subp.call(["make", "all", "clear"])

	if (error != 0):
		print("Compilation error. Aborting.")
		sys.exit()

	print("\nDone.\n")


def test1():
	print("Testing files...\n")

	for f in implementations:
		try:
			os.remove("{}/test_files/{}.dat".format(os.getcwd(),
				f))
		except OSError:
			pass

		datafile = open("{}/test_files/{}.dat".format(os.getcwd(), f),
			'w')

		print("Testing {}...".format(f))

		counter = 1
		while (counter <= threads):
			datafile.write("{}\t".format(counter))
			print("Number of threads: {}".format(counter))

			test = 1
			outputs = []
			while (test <= tests):
				proc_list = subp.check_output(["./list.pl",
					proc_list_policy, str(counter)])\
					.rstrip()

				output = subp.check_output(["taskset", "-c",
					proc_list,
					"./{}".format(test_file),
					"-b{}".format(str(bits)),
					"-r{}".format(str(radix)),
					"-k{}".format(str(keys)),
					"-l{}".format(str(lookups)),
					"-p{}".format(str(counter)),
					"-i{}".format(f)])

				outputs.append(float(output))
				test = test + 1

			run_time = mean(outputs)
			datafile.write("{}\t".format(run_time))

			stdev = corrected_stdev(outputs)
			rel_error = (stdev / run_time)
			error = rel_error * run_time

			datafile.write("{}\n".format(error))

			if (counter < 8):
				counter = counter * 2
			else:
				counter = counter + 8

		print("\nFinished testing {}.\n".format(f))
		datafile.close()

	print("Done.\n")


def test2():
	print("Testing files...\n")		

	for f in implementations:
		try:
			os.remove("{}/test_files/{}.dat".format(os.getcwd(),
				f))
		except OSError:
			pass

		datafile = open("{}/test_files/{}.dat".format(os.getcwd(), f),
			'w')

		print("Testing {}...".format(f))

		counter = 1
		while (counter <= threads):
			datafile.write("{}\t".format(counter))
			print("Number of threads: {}".format(counter))

			proc_list = subp.check_output(["./list.pl",
				proc_list_policy, str(counter)])\
				.rstrip()

			test = 1
			outputs = []
			while (test <= tests):
				proc_list = subp.check_output(["./list.pl",
					proc_list_policy, str(counter)])

				output = subp.check_output(["taskset", "-c",
					proc_list,
					"./{}".format(test_file),
					"-b{}".format(str(bits)),
					"-r{}".format(str(radix)),
					"-k{}".format(str(keys)),
					"-l{}".format(str(lookups)),
					"-p{}".format(str(counter)),
					"-i{}".format(f)])

				outputs.append(float(output))
				test = test + 1

			run_time = mean(outputs)
			run_time_error = (corrected_stdev(outputs) / run_time)

			num_lookups = lookups * counter
			throughput = (num_lookups / MILLION) / float(run_time)

			datafile.write("{}\t".format(throughput))

			throughput_error = ((run_time_error / run_time) ** 2)\
				** (0.5)
			datafile.write("{}\n".format(throughput_error))

			if (counter < 8):
				counter = counter * 2
			else:
				counter = counter + 8

		print("\nFinished testing {}.\n".format(f))
		datafile.close()

	print("Done.\n")


def plot_all():
	os.chdir("test_files")

	print("Plotting graphs...\n")

	try:
		os.remove("plot_commands.gp")
	except OSError:
		pass

	plot_cmds = open("plot_commands.gp", 'w')

	plot_cmds.write("set terminal {} ".format(gnuplot_term))
	plot_cmds.write("size {}\n".format(graph_size))
	plot_cmds.write("set output 'graph.{}'\n".format(graph_ext))

	if (graph_type == 1):
		plot_cmds.write("set title \"Number of Threads x "
			"Running Time\\n")
		plot_cmds.write("BITS: {} RADIX: {} KEYS: {} LOOKUPS: {} "
			"TESTS: {}\"\n".format(bits, radix, keys, lookups,
			tests))
		plot_cmds.write("set xlabel 'Number of Threads'\n")
		plot_cmds.write("set ylabel 'Running Time ({})'\n"
			.format(time_unit))

	if (graph_type == 2):
		plot_cmds.write("set title \"Number of Threads x "
			"Throughput\\n")
		plot_cmds.write("BITS: {} RADIX: {} KEYS: {} LOOKUPS: {} "
			"TESTS: {}\"\n".format(bits, radix, keys, lookups,
			tests))
		plot_cmds.write("set xlabel 'Number of Threads'\n")
		plot_cmds.write("set ylabel 'Throughput (M lookups/{})'\n"
			.format(time_unit))

	plot_cmds.write("set xrange [1:]\n")
	plot_cmds.write("set yrange [0:]\n")

	plot_cmds.write("plot ")

	counter = 1
	num_files = len(glob.glob1("{}".format(os.getcwd()), "*.dat"))

	for f in os.listdir("{}".format(os.getcwd())):
		if f.endswith(".dat"):
			f_name = f.replace(".dat", "")

			plot_cmds.write("'{}' u 1:2 t \"{}\" with"
				" {} ".format(f, f_name, graph_style))

			plot_cmds.write("lc rgb"
				" '{}'".format(graph_colors[f_name]))

			if (tests > 1):
				plot_cmds.write(", ")
				plot_cmds.write("'{}' u 1:2:3 with errorbars "
					"notitle lc rgb '{}'"
					"\\\n".format(f, error_color))

			counter = counter + 1
			if (counter > num_files):
				break;

			plot_cmds.write(", ")

	plot_cmds.close()
	subp.call(["gnuplot", "plot_commands.gp"])
	shutil.move("graph.{}".format(graph_ext),
		"../graph.{}".format(graph_ext))

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

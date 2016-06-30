# CpuTopology.pm
# query sysfs to read the system's cpu topology.
#
# Assumptions:
# - The system is a regular hierarchy of sockets->cores->threads. That is:
#   + All sockets have the same amount of cores
#   + All cores have the same amount of threads
# - socket[0]->core[0] always exists
# - sysfs exists
#   + sysfs is mounted on /sys

package CpuTopology;

use strict;
use warnings;
use Exporter qw(import);

our @EXPORT_OK = qw(
	cpus
	init
	n_cores_per_socket
	n_sockets
	n_threads_per_core
	threads_in_socket
	sockets
);

my $sysfs_path = '/sys/devices/system/cpu';

my %sockets;
my @cpus;

sub read_param {
    my ($id, $param) = @_;
    my $val;

    my $filename = "$sysfs_path/cpu$id/topology/$param";
    open(INPUT, "<$filename") or die("Cannot open $filename: $!\n");
    while (<INPUT>) {
	chomp;
	if ($_ =~ /^([0-9]+)$/) {
	    $val = $1;
	} else {
	    die ("Cannot interpret '$_' in $filename\n");
	}
    }
    close(INPUT) or die("Cannot close $filename: $!\n");
    return $val;
}

sub get_cpus {
    opendir(my $dh, $sysfs_path) || die "can't opendir $sysfs_path: $!";
    my @array = grep { /^cpu[0-9]+$/ } readdir($dh);
    closedir $dh;

    foreach my $cpu (@array) {
	$cpu =~ s/cpu//;
    }
    @cpus = sort { $a <=> $b } @array;

    if (@cpus == 0) {
	die ("0 cpus seen -- is sysfs mounted on /sys? Are you on Linux?");
    }
}

# No need to call this function unless you have CPU hotplug, in which
# case by calling it you'll force a read of the topology.
sub init {
    get_cpus();
    foreach my $id (@cpus) {
	my $socket = read_param($id, 'physical_package_id');
	my $core = read_param($id, 'core_id');
	push @{ $sockets{$socket}->{$core} }, $id;
    }
}

sub _init {
    if (@cpus and %sockets) {
	return;
    }
    init();
}

sub cpus {
    _init();
    return @cpus;
}

sub n_cores_per_socket {
    _init();
    return scalar(keys %{ $sockets{0} });
}

sub n_sockets {
    _init();
    return scalar(keys %sockets);
}

sub n_threads_per_core {
    _init();
    return scalar(@{ $sockets{0}->{0} });
}

# Returns a list of threads in the same socket.
# $max_threads_per_core threads from each core in the socket are returned.
sub threads_in_socket {
    my ($socket, $max_threads_per_core) = @_;

    _init();
    die ("Socket $socket does not exist\n") if !$sockets{$socket};

    my $n = $max_threads_per_core;
    my $max_n = n_threads_per_core();
    if ($n > $max_n) {
	warn "requested max_threads_per_core=$max_threads_per_core > n_threads_per_core=$max_n: using $max_n\n";
	$n = $max_n;
    }

    my @ret = ();
    foreach my $core (sort { $a <=> $b } keys %{ $sockets{$socket} }) {
	for (my $i = 0; $i < $n; $i++) {
	    my $thread_list = $sockets{$socket}->{$core};
	    push @ret, $thread_list->[$i];
	}
    }
    return @ret;
}

sub sockets {
    _init();
    return %sockets;
}

return 1;

#!/usr/bin/perl
#
# list.pl
# List groups of CPUs based on socket placement policies.

use warnings;
use strict;
use Getopt::Long;
use File::Basename;
use lib dirname (__FILE__); # find CpuTopology regardless of what cwd is
use CpuTopology;

my %policies = (
    'compact' => {
	'description' => 'Place threads on the same socket first, avoiding SMT if possible',
	'func' => \&compact,
    },
    'compact-smt' => {
	'description' => 'Place threads on the same socket first, favouring same-socket SMT over out-of-socket placement',
	'func' => \&compact_smt,
    },
    'scatter' => {
	'description' => 'Place threads on separate sockets first, avoiding SMT if possible',
	'func' => \&scatter,
    },
    );

my $default_policy = 'scatter';
my $policy = $default_policy;

GetOptions(
    'policy=s' => \$policy,
    );

if (!@ARGV) {
    usage_die();
}

if (!$policies{$policy}) {
    print STDERR "fatal: policy $policy is not a valid policy\n";
    usage_die();
}

my $n_threads;
if (is_natural_number($ARGV[0])) {
    $n_threads = $ARGV[0];
} else {
    print STDERR "fatal: $ARGV[0] does not look like a natural number\n";
    usage_die();
}

my @list = $policies{$policy}->{func}->($n_threads);
print join(",", @list), "\n";
exit 0;

sub usage_die {
    my $usage = "";
    $usage .= "usage: ./list.pl [options] n\n";
    $usage .= "  Lists n cores as a comma-separated list.\n";
    $usage .= "options:\n";
    $usage .= "  --policy: One of:\n";
    foreach my $pol (sort keys %policies) {
	my $desc = $policies{$pol}->{description};

	$usage .= "    $pol: $desc.";
	if ($pol eq $default_policy) {
	    $usage .= " (Default)";
	}
	$usage .= "\n";
    }

    die($usage);
}

sub compact {
    my ($n) = @_;

    my $sockets = { CpuTopology::sockets };

    my @list = ();
    LOOP:
    for (my $i = 0; $i < CpuTopology::n_threads_per_core; $i++) {
	for (my $j = 0; $j < CpuTopology::n_sockets; $j++) {
	    for (my $k = 0; $k < CpuTopology::n_cores_per_socket; $k++) {
		push @list, $sockets->{$j}->{$k}->[$i];
		if (scalar(@list) == $n) {
		    last LOOP;
		}
	    }
	}
    }
    return @list;
}

sub compact_smt {
    my ($n) = @_;

    my $sockets = { CpuTopology::sockets };

    my @list = ();
    LOOP:
    for (my $i = 0; $i < CpuTopology::n_sockets; $i++) {
	for (my $j = 0; $j < CpuTopology::n_threads_per_core; $j++) {
	    for (my $k = 0; $k < CpuTopology::n_cores_per_socket; $k++) {
		push @list, $sockets->{$i}->{$k}->[$j];
		if (scalar(@list) == $n) {
		    last LOOP;
		}
	    }
	}
    }
    return @list;
}

sub scatter {
    my ($n) = @_;

    my $sockets = { CpuTopology::sockets };

    my @list = ();
    LOOP:
    for (my $i = 0; $i < CpuTopology::n_threads_per_core; $i++) {
	for (my $j = 0; $j < CpuTopology::n_cores_per_socket; $j++) {
	    for (my $k = 0; $k < CpuTopology::n_sockets; $k++) {
		push @list, $sockets->{$k}->{$j}->[$i];
		if (scalar(@list) == $n) {
		    last LOOP;
		}
	    }
	}
    }
    return @list;
}

sub is_natural_number {
    my ($v) = @_;

    if ($v =~ /^\d+$/ and $v > 0) {
	return 1;
    }
    return 0;
}

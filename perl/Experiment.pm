package Experiment;

use Exporter;
use Carp;

our @ISA = qw(Exporter);
our @EXPORT_OK = qw(
    parse_tarball_name
);
our @EXPORT = @EXPORT_OK;

sub parse_tarball_name {
    @_ == 1 || @_ == 2 or croak "Usage: parse_tarball_name(FILENAME [, PRINT_FILENAME])";
    my ($tb, $prn) = @_ == 1 ? (@_, @_) : @_;
    $tb =~ /
	(S\d+(?:R\d+)L\d+)              # $1 topology
	([A-Z]*)-                       # $2 suffix to make unique
	(\w+)-                          # $3 testbed e.g. vwall1, vwall2, grid5000
	(\d{14})-                       # $4 boot timestamp YYYYMMDDHHMMSS
	(multicast|scp|tcp|udp)-        # $5 update method
	(immediate|random|random2)-     # $6 schedule
	(\d+)-                          # $7 file size in MB
	(\d{14})                        # $8 run timestamp YYYYMMDDHHMMSS
	\.tar\.gz
    /x or die "Invalid tarball name $prn\n";
    my %d = (
	topology   => $1,
	suffix     => $2,
	testbed    => $3,
	boot_time  => $4,
	update     => $5,
	schedule   => $6,
	file_size  => $7,
	run_time   => $8,
    );
    $d{is_multicast} = $d{update_method} eq 'multicast';
    if ($d{topology} =~ /^S(\d+)L(\d+)$/) {
	$d{n_servers} = $1;
	$d{n_router_levels} = 0;
	$d{n_lans} = 1;
	$d{n_clients} = $2;
    } elsif ($d{topology} =~ /^S(\d+)R(\d+)L(\d+)$/) {
	$d{n_servers} = $1;
	$d{n_router_levels} = $2;
	$d{n_lans} = 2 ** ($2 - 1);
	$d{n_clients} = $3 * $d{lans};
    } else {
	die "Internal error, topology($d{topology}) has wrong format\n";
    }
    wantarray ? %d : \%d;
}


Preparation:

Put the login.pem file in ~/data (this location is currently hardcoded)

To create the "LAN" experiment with $C clients:

C=...

./bin/mknet L=$C [other options, see below]

To create the "Two LANs" experiment with $C clients:

C=...

./bin/mknet L=$C R=1 [other options, see below]

To create the "Generic experiment with $L clients per LAN and $R router levels
($R >= 2):

L=...
R=...

./bin/mknet L=$L R=$R [other options, see below]

other options (case is actually unimportant, so "time=180" is the same as
("TIME=180"):
    TIME=180                 time in minutes before the testbed expires,
                             default is 120 (jfed's default)
    AUTH=vwall1              change autority (default: wvall2)
    AUTH=grid5000            Change authority to GRID5000 (Separate testbed)
    SUFFIX=A                 Add a suffix to the experiment name, which is
                             generated from the number of routers and clients,
                             if needed to make the name unique in the testbed
    EXP=S1R3L5               overrides the generated experiment name
    PWD=~/data/login.pwd     file containing password for login.pem
    K=~/.ssh/id_rsa.pub      add another allowed ssh key to the testbed
                             (the one from login.pem and the one in the
                             "objects" directory are added automatically)
    Klibrecas=~/data/id_rsa.pub
                             Add key for non-default user (here "librecas")

If no other keys are specified with "K=", the default key from login.pem
as well as the private key in the "objects" directory can be used to log
in to the testbed: the latter key is also used by the nodes to talk to
each other.

xvfb-run java -Djdk.gtk.version=2 -jar ~/jfed_cli/experimenter-cli2.jar \
    -a /var/tmp/$exp/action.yaml

This takes about 10 minutes, then it stops without saying anything;
to check progress:

tail /var/tmp/$exp/debug.txt

To prepare the testbed for the experiment using $SIZES data sizes
(a comma-separate list, in megabytes)

SIZES=...

./bin/setup-experiment $SIZES $exp

This currently copies all the information to the testbed but does not set
things up and run the experiment.  Log in to the "director":

./bin/ssh-experiment $exp director0

On the director itself, start by setting up all the other nodes:

/tmp/experiment/setup-all

And then run the experiment:

/tmp/experiment/run-experiment

Alternatively to leave the experiment running until the testbed expires:

while true; do /tmp/experiment/run-experiment; done

To log in to any other node:

./bin/ssh-experiment $exp NODE_NAME

To run a command using ssh:

./bin/ssh-experiment $exp NODE_NAME PROGRAM [ARGUMENTS]

If the scripts have been edited locally, they can be updated on the testbed
by re-running setup-experiment locally, then asking director0 to copy
them to all other nodes:

./bin/setup-experiment SIZES $exp
./bin/ssh-experiment $exp director0 /tmp/experiment/copy-experiment

To copy data to/from a node in the experiment, there are two scripts,
rsync-to-experiment and rsync-from-experiment, called as:

./bin/rsync-to-experiment EXP_NAME NODE RSYNC_OPTIONS LOCAL_SOURCE [LOCAL_SOURCE]... REMOTE_DEST
./bin/rsync-from-experiment EXP_NAME NODE RSYNC_OPTIONS REMOTE_SOURCE [REMOTE_SOURCE]... LOCAL_DEST

for example after building new binaries in /tmp/binaries on director0 one
could copy them back to the objects directory with:

./bin/rsync-from-experiment $exp director0 -avHP /tmp/binaries/ objects/

or for more complicated rsync options:

./bin/rsync-from-experiment $exp director0 '-avHP --delete' /tmp/binaries/ objects/

==============================================

To quickly average some data from the experiment, the "averages" script
may help.  It reads the result tarballs and extracts the required data
into a sqlite database, then calculates averages, minimum, maximum and
standard deviation.  The database is considered a cache, so if the same
tarball is read back in the script will not read the tarball at all.
Currently, it can determine the total time taken by clients to download
the update, but the script can be extended to collect other data.

./bin/averages [OPTIONS] DATA_NAME CACHE_DATABASE TARBALL [TARBALL]

DATA_NAME is the data item to extract: currently it can only be "run-time"

OPTIONS can be one or more of:

    -r   show raw data (e.g. 123.456 seconds instead of 2:03.456)
    -a   sort by average value (default)
    -m   sort by maximum value
    -n   sort by experiment name ("UPDATE SCHEDULE SIZE")
    -c   do not display data, only update cache database
    -p   interpret TARBALL as a "prefix" (like adding a "*" to the end
         but expanded by the script and not the shell, and also excluding
         incomplete and test runs)

for example, to compare the running time of all "2GB" experiments sorting
by maximum update time (assuming the tarballs are in ~/results and the
file names are as produced by the experiment):

./bin/average -m run-time /tmp/cache.sqlite ~/results/*-2048-*[0-9].tar.gz

(the "[0-9]" is a quick way to exclude names which don't end in a
numeric timestamp, for example incomplete and test runs)

and to summarise all runs from experiment S1L49F:

./bin/average -m -p run-time /tmp/cache.sqlite ~/results/S1L49F-

in this case, using "-p" simplifies the command line

==============================================

To extract the data produced by the experiments to a set of files in a
directory, if the data is in $SRC_DIR and the results will go into
$DST_DIR:

./bin/extract-data "$SRC_DIR" "$DST_DIR" '*'

Or to extract only some files matching PATTERNS:

./bin/extract-data "$SRC_DIR" "$DST_DIR" PATTERNS

The extracted files in $DST_DIR will have names like

NODE.EXPERIMENT.SCHEDULING.SIZE.TOPOLOGY.TIMESTAMP

for example:

server0.multicast.immediate.128.S1R2L2.20211219135844

These are lwmon binary files; to display the contants (install lwmon and) type:

lwmon -P- -R "$DST_DIR"/server0.multicast.immediate.128.S1R2L2.20211219135844

See lwmon(1) for other options


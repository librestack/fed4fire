package Lwmon;

use Carp;

use constant header_length => 10;

sub new {
    @_ == 2 or croak "Usage: Lwmon->new(FILE)";
    my ($class, $filename) = @_;
    open(my $filehandle, '<', $filename) or die "$filename: $!\n";
    bless [$filehandle, $filename], $class;
}

sub _checksum {
    my ($data) = @_;
    my ($chk) = unpack('@2 %32C*', $data);
    # max value of $chk at this point is 1280 * 255 = 326400, make it 16 bits...
    $chk = ($chk & 0xffff) + ($chk >> 16);
    # and now it could be 65535 + 4 == 65539 (326400 >> 16 is 4)
    $chk = ($chk & 0xffff) + ($chk >> 16);
    # now it's definitely going to fit in 16 bits
    return $chk;
}

sub _uint {
    my ($data, $pos) = @_;
    # XXX is there a way to know how much of $data is consumed by this unpack?
    my ($res) = unpack("\@$$pos w", $data);
    $$pos += length(pack('w', $res));
#    my $res = 0;
#    my $p = $$pos;
#    while ($p < length($data)) {
#	my $C = unpack("\@$p C", $data);
#	$p++;
#	$res = ($res << 7) | ($C & 0x7f);
#	$C & 0x80 or last;
#    }
#    $$pos = $p;
    $res;
}

sub _int {
    my ($data, $pos) = @_;
    my $res = _uint($data, $pos);
    my $sign = $res & 1;
    $res >>= 1;
    $sign and $res = -$res;
    $res;
}

sub _name {
    my ($filename, $data, $ipos, $space, $namelen) = @_;
    my $opos = $$ipos;
    $opos + $namelen > length($data)
	and die "$filename: invalid data item ($opos, $namelen, " . length($data) . ")\n";
    $$ipos += $namelen;
    return substr($data, $opos, $namelen);
}

sub nextrecord {
    @_ == 1 or croak "Usage: \$lwmon->nextrecord";
    my ($self) = @_;
    my ($filehandle, $filename) = @$self;
    my $packet;
    my $nr = read $filehandle, $packet, header_length;
    defined $nr or die "$filename: $!\n";
    $nr == 0 and return undef; # end of file
    $nr < header_length and die "$filename: unexpected end of file\n";
    my ($checksum, $length, $data_start, $host, $hostlen, $packetid, $seqno) =
	unpack('nnnCCCC', $packet);
    $length > header_length or die "$filename: invalid packet\n";
    $data_start > $length || $host + $hostlen > $data_start || $host <= header_length
	and die "$filename: invalid packet header\n";
    $nr = read $filehandle, $packet, $length - header_length, header_length;
    defined $nr or die "$filename: $!\n";
    $nr < $length - header_length and die "$filename: unexpected end of file\n";
    my $calc = _checksum($packet);
    $checksum == $calc
	or die "$filename: packet has invalid checksun ($calc != $checksum)\n";
    my $pos = header_length;
    my $timestamp = _uint($packet, \$pos);
    $pos < $host and $timestamp += _uint($packet, \$pos) / 1000;
    my %packet = (
	host      => substr($packet, $host, $hostlen),
	packetid  => $packetid,
	seqno     => $seqno,
	timestamp => $timestamp,
    );
    $pos = $data_start;
    my @data = ();
    while ($pos < $length) {
	my ($data_length, $type, $namelen, $parmlen, $n_values) =
	    unpack("\@$pos nnCCC", $packet);
	$pos + $data_length > $length and die "$filename: data after end of packet\n";
	my %item = (
	    type => $type,
	);
	my @values;
	if ($type != 65535) {
	    my $dpos = $pos + 7;
	    my $end = $pos + $data_length;
	    $item{name} = _name($filename, $packet, \$dpos, $end, $namelen);
	    $item{parm} = _name($filename, $packet, \$dpos, $end, $parmlen);
	    for (my $v = 0; $v < $n_values; $v++) {
		push @values, _int($packet, \$dpos);
	    }
	    $dpos > $end and die "$filename: Invalid data in packet\n";
	}
	$item{values} = \@values;
	push @data, \%item;
	$pos += $data_length;
    }
    $packet{data} = \@data;
    \%packet;
}

sub DESTROY {
    my ($self) = @_;
    my $filehandle = $self->[0];
    close $filehandle;
}

1

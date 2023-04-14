#!/usr/bin/perl
my $iterations = 100;
$iterations = $ARGV[0] if ($#ARGV == 0);
open my $handle, '<', "./list";
chomp(my @lines = <$handle>);
close $handle;
$base = "https://www.halsoft.com:4433/";
while($iterations-- > 0) {
	$i = rand @lines;
	$url = $base . $lines[$i];
	`curl --cacert cacert.pem $url`;
}

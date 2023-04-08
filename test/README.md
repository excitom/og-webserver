# og-webserver tests
The directory contains various tests.

The `pound.pl` script takes a list of URLs from the file `list` in
the current directory and requests them in random order. The input parameter
determines how many times to request a URL.

The `run.sh` script launches copies of `pound.pl` to run in parallel and 
increase the pounding.

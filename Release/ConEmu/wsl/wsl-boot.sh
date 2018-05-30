#/bin/sh
uname -a
./256colors2.pl
cd ~
`getent passwd $USER | cut -d: -f7` -l -i

#!/bin/sh

host=`hostname`
cf_base="-DRG=21 -DRG_MASTER=4 -UDEBUG -DDBG_LVL=3 -O6"
ld_base=""
cc="gcc"

echo " -- $host -- "

case $host in
  alpha??*|aladin*)
          cf="$cf_base -Wall -pedantic";;
  nostromo*|onouga*)
          cf="$cf_base -Wall -pedantic -m486";;
  beorn*)
          cf="$cf_base -Wall -pedantic";;
  darou*|moloch*|kremlin*)
          cf="$cf_base -Wall -pedantic";;
  holiday*|zep*)
          cf="$cf_base -Wall -pedantic -mpentium";;
  clipper*|pirogue*|sequoia*)
          cf="$cf_base -Wall -pedantic"
          ld="-lsocket -lnsl";;
  *)
          cf="$cf_base -Wall -pedantic";;
esac


rm -f Makefile

echo "#### Makefile automagique ####" > Makefile
echo >> Makefile
echo "CC = gcc" >> Makefile
echo "CFLAGS = " $cf >> Makefile
echo "LDFLAGS = " $ld >> Makefile
echo >> Makefile

cat depend >> Makefile

make clean
make slave

case $host in
  alpha??*|aladin*) 
          mv slave slave.alpha;;
  beorn*)
          mv slave slave.alphapc;;
  onouga*|nostromo*)
          mv slave slave.486;;
  darou*|moloch*|kremlin*)
          mv slave slave.pentium;;
  holiday*|zep*)
          mv slave slave.pentium_opt;;
  clipper*|pirogue*|sequoia*)
          mv slave slave.solaris;;
  *)
          mv slave slave.sun4;;
esac

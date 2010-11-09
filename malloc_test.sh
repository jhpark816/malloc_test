#!/bin/sh
/usr/bin/time ./ptmalloc_test 64   $1
/usr/bin/time ./tcmalloc_test 64   $1
/usr/bin/time ./ptmalloc_test 128  $1
/usr/bin/time ./tcmalloc_test 128  $1
/usr/bin/time ./ptmalloc_test 256  $1
/usr/bin/time ./tcmalloc_test 256  $1
/usr/bin/time ./ptmalloc_test 512  $1
/usr/bin/time ./tcmalloc_test 512  $1
/usr/bin/time ./ptmalloc_test 1024 $1
/usr/bin/time ./tcmalloc_test 1024 $1
/usr/bin/time ./ptmalloc_test 2048 $1
/usr/bin/time ./tcmalloc_test 2048 $1

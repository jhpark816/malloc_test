all:
	gcc -o ptmalloc_test malloc_test.c
	gcc -o tcmalloc_test malloc_test.c -DUSE_TCMALLOC -ltcmalloc

clean:
	\rm ptmalloc_test tcmalloc_test
	 

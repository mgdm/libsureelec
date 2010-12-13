all:
	gcc -c -fPIC libsureelec.c -o libsureelec.o 
	gcc -shared -Wl,-soname,libsureelec.so.1 -o libsureelec.so.1.0.1 libsureelec.o

debug:
	gcc -g -c -fPIC libsureelec.c -o libsureelec.o 
	gcc -g -shared -Wl,-soname,libsureelec.so.1 -o libsureelec.so.1.0.1 libsureelec.o

test_prog:
	gcc -g -o test test.c -L. -lsureelec

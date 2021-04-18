 linux: cruce.c cruce.h libcruce_linux.a
	gcc -g cruce.c libcruce_linux.a -o p -lm -m32

solaris: cruce.c cruce.h libcruce_solaris.a
	gcc -g cruce.c libcruce_solaris.a -o p -lm

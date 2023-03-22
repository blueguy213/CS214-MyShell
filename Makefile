# Make mysh.c into the executable mysh using the gcc compiler and -Wall flag
# to produce warnings

mysh: src/mysh.c
	gcc -Wall -o mysh src/mysh.c
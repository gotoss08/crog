OPTIONS = -Wall -Wunused -Wextra -Wno-pointer-sign -Wno-unused-parameter
INCLUDE = -I"include"
LINK = -L"lib"
LIBS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image

main: utils.o list.o generation.o
	gcc ${OPTIONS} src/utils.o src/list.o src/generation.o src/main.c ${INCLUDE} ${LINK} ${LIBS} -o bin/crog.exe
	cd bin; ./crog.exe

utils.o:
	gcc -c src/utils.c -o src/utils.o

list.o:
	gcc -c src/list.c -o src/list.o

generation.o:
	gcc -c src/generation.c -o src/generation.o

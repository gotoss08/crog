OPTIONS = -Wall -Wunused -Wextra -Wno-pointer-sign -Wno-unused-parameter
INCLUDE = -I"include"
LINK = -L"lib"
LIBS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image

main: list.o
	gcc ${OPTIONS} src/list.o src/main.c ${INCLUDE} ${LINK} ${LIBS} -o bin/crog.exe
	cd bin; ./crog.exe

list.o:
	gcc -c src/list.c -o src/list.o
EXE 		= renderer

SRC 		= $(wildcard src/*.cpp)
OBJ 		= $(subst src, build, $(patsubst %.cpp, %.o, $(SRC)))

DBG 		= 	# debug flags

INCLUDE 	= -I include
# LIB 		= -L lib/SFML -lsfml-graphics -lsfml-window -lsfml-system -lopengl32 	# this is for linking SFML with shared libraries
LIB 		= -L lib/SFML_static -L lib/Other -lsfml-graphics-s -lsfml-window-s -lsfml-system-s -lopengl32 -lfreetype -lwinmm -lgdi32 -lcomdlg32	# this is for static linking SFML
EXTRA		= -Werror
STATIC  	= -static-libgcc -static-libstdc++ -static 	# for static linking with libgcc and libstdc++ and more

all: check-build link

check-build:
	if not exist build (mkdir build)

remake: clean all

image/%:	# if you ever need to link an image
	objcopy -I binary -O elf64-x86-64 obj/$*.obj build/$*.o

run:
	$(EXE)

clean:
	erase $(subst build/, build\, $(OBJ))

build/%.o: src/%.cpp
	g++ $(INCLUDE) -c src/$*.cpp -o build/$*.o $(DBG) $(EXTRA)

link: $(OBJ)
	g++ $(OBJ) -o $(EXE) $(LIB) $(STATIC) $(DBG) build/cow.o $(EXTRA)

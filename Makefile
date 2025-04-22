# ASSIGNMENT BINARIES
assignment1_09: build/dungeon.o build/pathfinding.o build/character.o build/game.o build/game_loop.o build/parser.o build/item.o build/message_queue.o build/assignment1_09.o
	g++ build/dungeon.o build/pathfinding.o build/character.o build/game.o build/game_loop.o build/parser.o build/item.o build/message_queue.o build/assignment1_09.o -o assignment1_09 -lm -lncurses

assignment1_08: build/dungeon.o build/pathfinding.o build/character.o build/game.o build/game_loop.o build/parser.o build/item.o build/assignment1_08.o
	g++ build/dungeon.o build/pathfinding.o build/character.o build/game.o build/game_loop.o build/parser.o build/item.o build/assignment1_08.o -o assignment1_08 -lm -lncurses

assignment1_07: dungeon.o heap.o pathfinding.o character.o game.o game_loop.o parser.o assignment1_07.o
	g++ dungeon.o heap.o pathfinding.o character.o game.o game_loop.o parser.o assignment1_07.o -o assignment1_07 -lm -lncurses

assignment1_06: dungeon.o heap.o pathfinding.o character.o game.o game_loop.o assignment1_06.o
	g++ dungeon.o heap.o pathfinding.o character.o game.o game_loop.o assignment1_06.o -o assignment1_06 -lm -lncurses

assignment1_05: dungeon.o files.o heap.o pathfinding.o character.o game.o assignment1_05.o
	gcc dungeon.o files.o heap.o pathfinding.o character.o game.o assignment1_05.o -o assignment1_05 -lm -lncurses

assignment1_04: dungeon.o files.o heap.o pathfinding.o character.o assignment1_04.o
	gcc dungeon.o files.o heap.o pathfinding.o character.o assignment1_04.o -o assignment1_04 -lm

assignment1_03: dungeon.o files.o heap.o pathfinding.o assignment1_03.o
	gcc dungeon.o files.o assignment1_03.o heap.o pathfinding.o -o assignment1_03 -lm

assignment1_02: dungeon.o files.o assignment1_02.o
	gcc dungeon.o files.o assignment1_02.o -o assignment1_02 -lm

assignment1_01: dungeon.o assignment1_01.o
	gcc dungeon.o assignment1_01.o -o assignment1_01 -lm

# OBJECT FILES
assignment1_01.o: src/assignments/1_01.c
	gcc src/assignments/1_01.c -o assignment1_01.o -Wall -Werror -c -g

assignment1_02.o: src/assignments/1_02.c
	gcc src/assignments/1_02.c -o assignment1_02.o -Wall -Werror -c -g

assignment1_03.o: src/assignments/1_03.c
	gcc src/assignments/1_03.c -o assignment1_03.o -Wall -Werror -c -g

assignment1_04.o: src/assignments/1_04.c
	gcc src/assignments/1_04.c -o assignment1_04.o -Wall -Werror -c -g

assignment1_05.o: src/assignments/1_05.c
	gcc src/assignments/1_05.c -o assignment1_05.o -Wall -Werror -c -g

assignment1_06.o: src/assignments/1_06.cpp
	g++ src/assignments/1_06.cpp -o assignment1_06.o -Wall -Werror -c -g

assignment1_07.o: src/assignments/1_07.cpp
	g++ src/assignments/1_07.cpp -o assignment1_07.o -Wall -Werror -c -g

build/assignment1_08.o: src/assignments/1_08.cpp src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ src/assignments/1_08.cpp -o build/assignment1_08.o -Wall -Werror -c -g

build/assignment1_09.o: src/assignments/1_09.cpp src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ src/assignments/1_09.cpp -o build/assignment1_09.o -Wall -Werror -c -g

build/game.o: src/game.cpp src/game.h src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ src/game.cpp -o build/game.o -Wall -Werror -c -g

build/game_loop.o: src/game_loop.cpp src/game.h src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ src/game_loop.cpp -o build/game_loop.o -Wall -Werror -c -g

build/dungeon.o: src/dungeon.cpp src/dungeon.h src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ src/dungeon.cpp -o build/dungeon.o -Wall -Werror -c -g

build/pathfinding.o: src/pathfinding.cpp src/heap.h src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ src/pathfinding.cpp -o build/pathfinding.o -Wall -Werror -c -g

build/character.o: src/character.cpp src/character.h src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ src/character.cpp -o build/character.o -Wall -Werror -c -g

build/parser.o: src/parser.cpp src/parser.h src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ src/parser.cpp -o build/parser.o -Wall -Werror -c -g

build/item.o: src/item.cpp src/item.h src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ src/item.cpp -o build/item.o -Wall -Werror -c -g

build/message_queue.o: src/message_queue.cpp src/message_queue.h src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ src/message_queue.cpp -o build/message_queue.o -Wall -Werror -c -g

# PHONY TARGETS
clean:
	rm -f assignment1_* *.o *.tar.gz *.pgm; \
	rm -rf build

# This target creates a tarball ready to submit to Canvas for a particular assignment.
# If this is a submitted file, the script will not exist and it will not work.
submit:
	@if [ -f "sh/submit.sh" ]; then\
		./sh/submit.sh $(assignment);\
	else\
		echo "err: this target is only available in the original repository, not a submitted tarball";\
	fi

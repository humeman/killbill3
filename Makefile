# ASSIGNMENT BINARIES 
assignment1_05: dungeon.o files.o heap.o pathfinding.o character.o game.o assignment1_05.o
	gcc dungeon.o files.o heap.o pathfinding.o character.o game.o assignment1_04.o -o assignment1_05 -lm -lncurses

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

game.o: src/game.c src/game.h
	gcc src/game.c -o game.o -Wall -Werror -c -g

dungeon.o: src/dungeon.c src/dungeon.h
	gcc src/dungeon.c -o dungeon.o -Wall -Werror -c -g

files.o: src/files.c src/files.h
	gcc src/files.c -o files.o -Wall -Werror -c -g

heap.o: src/heap.c src/heap.h
	gcc src/heap.c -o heap.o -Wall -Werror -c -g

pathfinding.o: src/pathfinding.c src/heap.h
	gcc src/pathfinding.c -o pathfinding.o -Wall -Werror -c -g

character.o: src/character.c src/character.h
	gcc src/character.c -o character.o -Wall -Werror -c -g

# PHONY TARGETS
clean:
	rm -f assignment1_* *.o *.tar.gz *.pgm

# This target creates a tarball ready to submit to Canvas for a particular assignment.
# If this is a submitted file, the script will not exist and it will not work.
submit:
	@if [ -f "sh/submit.sh" ]; then\
		./sh/submit.sh $(assignment);\
	else\
		echo "err: this target is only available in the original repository, not a submitted tarball";\
	fi

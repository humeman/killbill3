# ASSIGNMENT BINARIES 
assignment1_02: dungeon.o files.o assignment1_02.o
	gcc dungeon.o files.o assignment1_02.o -o assignment1_02 -lm

assignment1_01: dungeon.o assignment1_01.o
	gcc dungeon.o assignment1_01.o -o assignment1_01 -lm

# OBJECT FILES
assignment1_01.o: src/assignments/1_01.c
	gcc src/assignments/1_01.c -o assignment1_01.o -Wall -Werror -c -g

assignment1_02.o: src/assignments/1_02.c
	gcc src/assignments/1_02.c -o assignment1_02.o -Wall -Werror -c -g

dungeon.o: src/dungeon.c src/dungeon.h
	gcc src/dungeon.c -o dungeon.o -Wall -Werror -c -g

files.o: src/files.c src/files.h
	gcc src/files.c -o files.o -Wall -Werror -c -g

# PHONY TARGETS
clean:
	rm -f assignment1_* *.o *.tar.gz

# This target creates a tarball ready to submit to Canvas for a particular assignment.
# If this is a submitted file, the script will not exist and it will not work.
submit:
	@if [ -f "sh/submit.sh" ]; then\
		./sh/submit.sh $(assignment);\
	else\
		echo "err: this target is only available in the original repository, not a submitted tarball";\
	fi

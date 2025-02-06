
assignment1_1: dungeon.o assignment1_1.o
	gcc dungeon.o assignment1_1.o -o assignment1_1 -lm

assignment1_1.o: src/assignments/1_1.c
	gcc src/assignments/1_1.c -o assignment1_1.o -Wall -Werror -c -g

dungeon.o: src/dungeon.c src/dungeon.h
	gcc src/dungeon.c -o dungeon.o -Wall -Werror -c -g

clean:
	rm -f assignment1_1 *.o

# This target creates a tarball ready to submit to Canvas for a particular assignment.
# If this is a submitted file, the script will not exist and it will not work.
submit:
	@if [ -f "sh/submit.sh" ]; then\
		./sh/submit.sh $(assignment);\
	else\
		echo "err: this target is only available in the original repository, not a submitted tarball";\
	fi
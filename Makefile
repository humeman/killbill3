# ASSIGNMENT BINARIES
killbill3: build/dungeon.o build/pathfinding.o build/character.o build/game.o build/game_loop.o build/game_controls.o build/game_menu.o build/parser.o build/item.o build/message_queue.o build/logger.o build/resource_manager.o build/plane_manager.o build/decorations.o build/killbill3.o
	g++ -std=c++17 \
		build/dungeon.o \
		build/pathfinding.o \
		build/character.o \
		build/game.o \
		build/game_loop.o \
		build/game_controls.o \
		build/game_menu.o \
		build/parser.o \
		build/item.o \
		build/message_queue.o \
		build/logger.o \
		build/resource_manager.o \
		build/plane_manager.o \
		build/decorations.o \
		build/killbill3.o \
		-o killbill3 \
		-lm -lnotcurses-core -lnotcurses -lnotcurses++ -lsfml-audio -lsfml-system

# OBJECT FILES
build/killbill3.o: src/assignments/killbill3.cpp src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ -std=c++17 src/assignments/killbill3.cpp -o build/killbill3.o -Wall -Werror -c -g

build/game.o: src/game.cpp src/game.h src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ -std=c++17 src/game.cpp -o build/game.o -Wall -Werror -c -g

build/game_loop.o: src/game_loop.cpp src/game.h src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ -std=c++17 src/game_loop.cpp -o build/game_loop.o -Wall -Werror -c -g

build/game_controls.o: src/game_controls.cpp src/game.h src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ -std=c++17 src/game_controls.cpp -o build/game_controls.o -Wall -Werror -c -g

build/game_menu.o: src/game_menu.cpp src/game.h src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ -std=c++17 src/game_menu.cpp -o build/game_menu.o -Wall -Werror -c -g

build/dungeon.o: src/dungeon.cpp src/dungeon.h src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ -std=c++17 src/dungeon.cpp -o build/dungeon.o -Wall -Werror -c -g

build/pathfinding.o: src/pathfinding.cpp src/heap.h src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ -std=c++17 src/pathfinding.cpp -o build/pathfinding.o -Wall -Werror -c -g

build/character.o: src/character.cpp src/character.h src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ -std=c++17 src/character.cpp -o build/character.o -Wall -Werror -c -g

build/parser.o: src/parser.cpp src/parser.h src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ -std=c++17 src/parser.cpp -o build/parser.o -Wall -Werror -c -g

build/item.o: src/item.cpp src/item.h src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ -std=c++17 src/item.cpp -o build/item.o -Wall -Werror -c -g

build/message_queue.o: src/message_queue.cpp src/message_queue.h src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ -std=c++17 src/message_queue.cpp -o build/message_queue.o -Wall -Werror -c -g

build/logger.o: src/logger.cpp src/logger.h src/macros.h
	@ mkdir -p build
	g++ -std=c++17 src/logger.cpp -o build/logger.o -Wall -Werror -c -g

build/resource_manager.o: src/resource_manager.cpp src/resource_manager.h src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ -std=c++17 src/resource_manager.cpp -o build/resource_manager.o -Wall -Werror -c -g

build/plane_manager.o: src/plane_manager.cpp src/plane_manager.h src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ -std=c++17 src/plane_manager.cpp -o build/plane_manager.o -Wall -Werror -c -g

build/decorations.o: src/decorations.cpp src/decorations.h src/macros.h src/random.h src/ascii.h src/heap.h
	@ mkdir -p build
	g++ -std=c++17 src/decorations.cpp -o build/decorations.o -Wall -Werror -c -g

# PHONY TARGETS
clean:
	rm -f assignment1_* *.o *.tar.gz *.pgm killbill3; \
	rm -rf build

# This target creates a tarball ready to submit to Canvas for a particular assignment.
# If this is a submitted file, the script will not exist and it will not work.
submit:
	@if [ -f "sh/submit.sh" ]; then\
		./sh/submit.sh $(assignment);\
	else\
		echo "err: this target is only available in the original repository, not a submitted tarball";\
	fi

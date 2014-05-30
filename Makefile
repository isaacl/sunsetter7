# Makefile to build sunsetter for linux.
#
# -Wall turns on full compiler warnings, only of interest to developers
# -O1 turns on optimization at a moderate level.
# -O3 turns on optimization at the highest level. (confuses the debugger though)
# -g turns on debugging data in the executable.
# -DNDEBUG turns off assert() debugging in the code
#
# uncomment this for a "release" build with no debugging and highly optimized.
# CFLAGS = -O3 -DNDEBUG
#
# uncomment this line to build a full debug version(slow, more likely to crash).
# need to type "make clean;make" to get the full effect
#CFLAGS = -Wall -g -DDEBUG
#
# or this one for a "light debug" version, works with gdb
# but is otherwise like the release version.
CFLAGS = -Wall -g -O1 -DNDEBUG
#

OBJECTS = aimoves.o bitboard.o board.o book.o bughouse.o evaluate.o moves.o search.o capture_moves.o check_moves.o interface.o notation.o order_moves.o partner.o quiescense.o tests.o transposition.o validate.o

# sunsetter is the default target, so either "make" or "make sunsetter" will do
sunsetter: $(OBJECTS) Makefile
	g++ $(CFLAGS) $(OBJECTS) -o sunsetter

# so "make clean" will wipe out the files created by a make.
clean:
	rm $(OBJECTS)


aimoves.o: aimoves.cpp definitions.h variables.h board.h brain.h interface.h
	g++ $(CFLAGS) -c aimoves.cpp -o $@

bitboard.o: bitboard.cpp board.h bughouse.h interface.h brain.h
	g++ $(CFLAGS) -c bitboard.cpp -o $@

board.o: board.cpp board.h brain.h interface.h definitions.h bughouse.h \
	variables.h notation.h
	g++ $(CFLAGS) -c board.cpp -o $@

book.o: book.cpp variables.h definitions.h board.h
	g++ $(CFLAGS) -c book.cpp -o $@

bughouse.o: bughouse.cpp variables.h definitions.h board.h interface.h bughouse.h brain.h
	g++ $(CFLAGS) -c bughouse.cpp -o $@

capture_moves.o: capture_moves.cpp board.h brain.h
	g++ $(CFLAGS) -c capture_moves.cpp -o $@

check_moves.o: check_moves.cpp board.h
	g++ $(CFLAGS) -c check_moves.cpp -o $@

evaluate.o: evaluate.cpp brain.h board.h evaluate.h
	g++ $(CFLAGS) -c evaluate.cpp -o $@

interface.o: interface.cpp interface.h variables.h notation.h bughouse.h brain.h board.h
	g++ $(CFLAGS) -c interface.cpp -o $@

moves.o: moves.cpp interface.h variables.h board.h
	g++ $(CFLAGS) -c moves.cpp -o $@

notation.o: notation.cpp bughouse.h notation.h interface.h board.h brain.h variables.h
	g++ $(CFLAGS) -c notation.cpp -o $@

order_moves.o: order_moves.cpp board.h brain.h interface.h notation.h
	g++ $(CFLAGS) -c order_moves.cpp -o $@

partner.o: partner.cpp board.h brain.h interface.h notation.h
	g++ $(CFLAGS) -c partner.cpp -o $@

quiescense.o: quiescense.cpp board.h brain.h interface.h variables.h notation.h
	g++ $(CFLAGS) -c quiescense.cpp -o $@

search.o: search.cpp board.h brain.h bughouse.h notation.h interface.h
	g++ $(CFLAGS) -c search.cpp -o $@

tests.o: tests.cpp board.h brain.h notation.h interface.h
	g++ $(CFLAGS) -c tests.cpp -o $@

transposition.o: transposition.cpp interface.h definitions.h board.h notation.h brain.h
	g++ $(CFLAGS) -c transposition.cpp -o $@

validate.o: validate.cpp interface.h board.h bughouse.h
	g++ $(CFLAGS) -c validate.cpp -o $@



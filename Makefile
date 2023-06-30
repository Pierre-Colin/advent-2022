.POSIX:
CPP=g++ -std=c++20
CPPFLAGS=-Wall -Wextra -Wpedantic -Weffc++ -Wshadow -Wconversion -Og -g -pg
LDFLAGS=
OBJ=advent.o common.o interval_union.o read.o 01.o 02.o 03.o 04.o 05.o 06.o\
07.o 08.o 09.o 10.o 11.o 12.o 13.o 14.o 15.o 16.o 17.o 18.o 19.o 20.o 21.o\
22.o 23.o 24.o 25.o

advent: $(OBJ)
	$(CPP) $(CPPFLAGS) $(LDFLAGS) -o $@ $(OBJ)

advent.o: advent.cpp common.h
interval_union.o: interval_union.cpp interval.h interval_union.h
read.o: read.cpp read.h
01.o: 01.cpp common.h
02.o: 02.cpp common.h
03.o: 03.cpp common.h
04.o: 04.cpp common.h
05.o: 05.cpp common.h read.h
06.o: 06.cpp common.h
07.o: 07.cpp common.h
08.o: 08.cpp common.h
09.o: 09.cpp common.h
10.o: 10.cpp common.h
11.o: 11.cpp common.h read.h
12.o: 12.cpp common.h
13.o: 13.cpp common.h
14.o: 14.cpp common.h interval.h interval_union.h
15.o: 15.cpp common.h interval.h interval_union.h read.h
16.o: 16.cpp common.h read.h
17.o: 17.cpp common.h
18.o: 18.cpp common.h
19.o: 19.cpp common.h read.h
20.o: 20.cpp common.h checked.h
21.o: 21.cpp common.h
22.o: 22.cpp common.h
23.o: 23.cpp common.h
24.o: 24.cpp common.h
25.o: 25.cpp common.h

.cpp.o:
	$(CPP) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -f advent $(OBJ) 

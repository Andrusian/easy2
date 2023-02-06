TARGET=easy2
CFLAGS=-O0 -g3 -ggdb -w
CPPFLAGS=-O0 -g3 -ggdb -w
CLIBS=-ll
HEADERS=easy_wav.hpp easy_code.h easy.hpp easy_node.hpp

easy2: easy_debug.o easy_sound.o easy_wav.o easy_node.o lex.yy.o easy_code.o 
	g++ -o $@ $^ $(CFLAGS) $(CLIBS)

easy_code.o: $(HEADERS) easy_code.cpp
easy_sound.o: $(HEADERS) easy_sound.cpp
easy_wav.o: $(HEADERS) easy_wav.cpp
easy_node.o: $(HEADERS) easy_node.cpp
easy_debug.o: $(HEADERS) easy_debug.cpp

lex.yy.o: lex.yy.c

lex.yy.c: easy2.l
	lex easy2.l

clean:
	rm *.o
	rm easy2


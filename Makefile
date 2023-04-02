TARGET=easy2
CFLAGS=-O0 -g3 -ggdb -w -flto
CPPFLAGS=$(CFLAGS) -std=c++11
CLIBS=
LDFLAGS=$(CPPFLAGS) $(CLIBS)
HEADERS=easy_wav.hpp easy_code.hpp easy.hpp easy_node.hpp

easy2: easy_debug.o easy_sound.o easy_wav.o easy_node.o lex.yy.o easy_code.o
	g++ -o $@ $^ $(LDFLAGS)

easy.yy.o: $(HEADERS) easy.yy.cpp
easy_code.o: $(HEADERS) easy_code.cpp
easy_sound.o: $(HEADERS) easy_sound.cpp
easy_wav.o: $(HEADERS) easy_wav.cpp
easy_node.o: $(HEADERS) easy_node.cpp
easy_debug.o: $(HEADERS) easy_debug.cpp

lex.yy.cpp: easy2.l
	lex -o lex.yy.cpp easy2.l
lex.yy.o: lex.yy.cpp


clean:
	rm *.o
	rm easy2


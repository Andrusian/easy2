#ifdef _WIN32
#define M_PI 3.1415926535
#define fileno(x) _fileno(x)  

#endif


#include <stdio.h>

#ifndef EASY_CODE
#define EASY_CODE 1

extern int linecount;
extern int lineNumber;   // have two competing ways to count, trying to figure out which is better

struct node * pop(void);
struct node * unshift(void);
//void shift(fpos_t pos, int dtype, float value, char * str);
void push(int dtype, float value, char * str);
void checkEndOfInclude(void);
void displayBackward();
void displayForward();
void emptyList();
int listLength();
int isEmpty();
void init (void);
int isCommand(struct node * n);
int countArgs(struct node * n);
void processCommands (void);
void numberhz (char * str);
void numberPeriod (char * str);
void numberpct (char * str);
void numbers (char * str);
void number (char * str);
void sound (double length);
void finish(void);
const char * doFilename (char *);
void doComma(void);
void doNegative(void);
char * assignvar(char *);    
char * loopvar(char *); 

// these are found in easy_node.cpp:

void displayBackward();
void displayForward();
//void push(int dtype, float value, char * str);
//void push(int dtype, float value, char * str);
//void shift(int dtype, float value, char * str);

// opcodes for parsing the input file:


#define WF_SINE 1000
#define WF_SQUARE 1001
#define WF_SAW 1002
#define WF_TRI 1003
#define WF_TENS 1004
#define WF_NOISE 1005

#define PLUS -1
#define MINUS -2
#define MULT -3
#define DIV -4
#define MODULUS -5
#define STRING 100
#define NUMBER 0
#define ASSIGNMENT 101
#define FILENAME 102
#define AUTOMIX 1
#define OUTPUT 2
#define FADEIN 3
#define FADEOUT 4
#define FORM 5
#define RIGHT 6
#define LEFT 7
#define FREQ 8
#define PHASE 9
#define VOL 10
#define BAL 11
#define DUTY 12
#define RANDOM 13
#define RAMP 14
#define SHAPE 15
#define OSC 16
#define SOUND 17
#define MIX 18
#define SILENCE 19
#define TIME 20
#define REWIND 21
#define AMOD 22
#define BOOST 24
#define REPEAT 25
#define BOTH 26
#define RANDSEQ 27
#define NOOP 999
#define ADDTIME 29
#define SEQ 30
#define MANUALMIX 31
#define LOOP 32
#define REVERB 33
#define CIRCUIT 34
#define NOCIRCUIT 35
#define CIRP 36
#define CIRI 37
#define TO 38
#define FREQ2 39
#define FREQ3 40
#define VOL2 41
#define VOL3 42
#define RAMPS 43
#define EXIT 44
#define TIMESTAMP 45
#define ASSIGNLHS 46
#define INCLUDE 47

#define COMMA 99

#define SH_TEASE1 2000
#define SH_TEASE2 2001
#define SH_TEASE3 2002
#define SH_PULSE1 2004
#define SH_PULSE2 2005
#define SH_PULSE3 2006
#define SH_KICK1 2007
#define SH_KICK2 2008
#define SH_KICK3 2009
#define SH_NOTCH1 2010
#define SH_NOTCH2 2011
#define SH_NOTCH3 2012
#define SH_ADSR1 2013
#define SH_ADSR2 2014
#define SH_ADSR3 2015
#define SH_REV1 2016
#define SH_REV2 2017
#define SH_REV3 2018
#define SH_WEDGE1 2019
#define SH_WEDGE2 2020
#define SH_GAP1 2021
#define SH_GAP2 2022
#define LASTSHAPE 2022

#define RED "\e[0;31m"
#define GRN "\e[0;32m"
#define YEL "\e[0;33m"
#define BLU "\e[0;34m"
#define MAG "\e[0;35m"
#define CYN "\e[0;36m"
#define WHT "\e[0;37m"
#define BLINK "\033[5m"
#define RESET "\033[0m"


#define NO_NUMBER -9999999

#endif
    

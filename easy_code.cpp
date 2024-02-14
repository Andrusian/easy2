//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Congratulations, you have found the central file for:
//
//  EEE    A     SS   Y   Y    22                                            
//  E     A A   S     Y   Y   2  2                                           
//  EE   A   A   SS    Y Y      2                                           
//  E    AAAAA     S    Y      2                                             
//  EEE  A   A  SSS     Y     2222                                            
//                                                                      
// Easy 2 is a script-based sound generator to make audio files.
// It is focused on certain types of sound effects for electronic
// purposes. The idea is to make a versatile system that is
// as easy as possible to use. This mainly focuses on keeping the language
// as simple as possible.
//
// Inspiration is drawn from analog synths, LISP, and many audio files
// that happened to come to my attention.
//
// This is the second attempt at a system like this Easy V1 was written
// in Python and was severely limited by that language. Aside from
// complaining about Python, some lessons learned were:
//
// - Audio generation is about numbers and numbers about numbers. As such
//   the language to describe certain concepts quickly becomes abstract and
//   lacking. It goes with the territory, so, sorry. I've tried.
//
// - The code that does actual audio wave generation is vulnerable to
//   becoming an mess that is difficult to debug. There are
//   some tricks to doing it right and simpler implemented here.
//
// - Implementing a language is hard especially when the subject
//   is abstract. Implementing rigorous syntax checking to cover
//   everything the user might type is harder. In Easy V1, even as the 
//   author, I would be mystified as to why the sound produced was
//   not right (often missing!). Easy 2 attempts to help this
//   by providing reasonable defaults and more carefree syntax.
//
// - WAV files are easy to write. MPEG4 files are a pain in the ass due
//   to proprietary reasons. For this reason 48k WAV files are supported
//   rather than 48k MPEG4. Your choice of sample rate matters as
//   only SoX accurately transcodes 44.1 to 48k.
//
// - the external MP3 LAME encoder is used to convert the WAV file to MP3.
//   This code is written for Linux and will need minor adaptation to
//   Windows. 
//
// The file extension ".e2" is suggested as the source script.
//
// The minimum content of a .e2 file is something like:
//
//    output "filename.wav"
//    sound 10   
//
// ... or merely tell easy2 what filename to call it and
//    make at least one sound.
//
//
// Here is the road map of the source files:
//
// easy2.l        ... the input to lex for it to write the lex.yy.c file.
//                  I use this in C mode because the internet told me to.
//                  Indeed, when tried in C++ mode it got pissy.
//
//                  Note: while Lex is used, YACC is not. The Easy2 language is
//                  simple enough that the work normally done by YACC is handled
//                  in easy_code.cpp. The different language objects did not
//                  become clear until well into the development.
//
// easy_code.h    ... any headers needed in C format
// easy.hpp       ... any general C++ format headers
// easy_code.cpp  ... this handles most the general work and is called
//                    in response to Lex parsing the input file. The routine
//                    processCommands() is the heart of the system.
//
// easy_node.cpp  ... The node linked list class. This is really a C-style
//                    double linked list as it was written early on. Each
//                    item in the input text becomes a node in the linked list.
//
// easy_sound.cpp ... deals with sound generation and higher-level audio
//                    manipulations of the output buffer.
//
// easy_wav.cpp   ... deals with lower-level writing into the output buffer.
//                    It handles the details of both 48k and 44.1k WAV files.
//                    This code actually writes the output.
//
// easy_debug.cpp ... extra code to help with debugging
//
// easy_mp3.cpp   ... conversion routine for wav->mp3
//                                                                      
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


extern "C" {
  // C called from C++ must be in this block for dumb legacy reasons
  
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "easy_code.h"
#include "math.h"
  extern void restart(void);   // over in lex.yy.c
}

#include <ctime>
#include <iostream>
#include <map>
#include <iostream>
#include <algorithm>
#include <string>
#include <stack>

#include "easy.hpp"
#include "easy_wav.hpp"
#include "easy_node.hpp"


extern "C" {
  extern int flag48;
  FILE * copyyyin;
  const char * copyinfile;
  int linecount;      // two different ways
  int lineNumber=0;
}

void doMp3(const char * infile, const char * songname);


using namespace std;

void doMath0 (void);
void doMath1 (void);
void doMath2 (void);
node * isAssignment(void);
void doAssignment ();
bool isNumerical(void);
const char * GetVarname(void);
void listVariables(void);
bool swapVariables(void);
void replaceMacro(node * spot, node *nodes);
void doStuff(void);
double NumberRight (node * n);
node * ToRight (node * n);
char * StringRight (node * n);
void lookForRepeat(void);
bool loadSeq(node * n, Seq * , Ramps *);
node * GetLeft (node * center);
node * GetRight (node * center);
const char * debug_type (int dtype);

// these are over in easy_sound...

void doSound(double, bool);
void doMix(double);
void doSilence(double);
void doBoost(double, NumberDriver *);
void doReverb (double length, NumberDriver *amt, NumberDriver *del);

// GLOBALS...

char * outputFile=NULL;
node * copyNodes (node * n);
double masterTime=0;
WaveWriter *wavout;
long defaultFormat = 44100;             // 16bit, 44kHz
const char * defaultFileout = "output.wav";   // because people will forget
uint32_t SR=999999;                      // sample rate
uint32_t soundLengthX;                  // length of current sound or boost in samples

Ramp *unusedRamp=NULL;                  // temp, until I figure out what to do with these
Osc *unusedOsc=NULL;                    // temp, until I figure out what to do with these
Ramps *unusedRamps=NULL;
RandSeq *unusedRand=NULL;

stack<double> rewindHistory;            // remembers previous times of sounds
bool updateDefaults=true;               // flag for updating default settings


//------------------------------------------

NumberDriver::~NumberDriver() {}


//======================================================================


struct settings_struct defaults;


struct settings_struct_stacked settings;

//--------------------------------
  
float tt=0;                      // current time

extern node *elist;
extern node *begn;
extern node *current;

//===========================================================================
// We provide some auto conversion from different units.
// What this doesn't do is check that the units are used in appropriate
// situations. And this might be needed if we want oscillators
// to work in Hz and Periods (s) interchangably.
//
// % divides by 100
//

void numberhz (char * str) {
  double val;
  sscanf(str,"%lf",&val);
  push(NUMBER,val,NULL);
}
void numberpct (char * str) {  // convert % to fraction
  double val;
  sscanf(str,"%lf",&val);
  val=val/100.;
  push(NUMBER,val,NULL);
}
void numbers (char * str) {
  double val;
  sscanf(str,"%lf",&val);
  push(NUMBER,val,NULL);
}
void number (char * str) {
  double val;
  sscanf(str,"%lf",&val);
  push(NUMBER,val,NULL);
}
void numberPeriod (char * str) {
  double val,inv;
  sscanf(str,"%lf",&val);
  if (val!=0) {
    inv=1./val;
  }
  else {
    inv=1;
  }
  push(NUMBER,inv,NULL);
}

//----------------------------------------------------------------------
// shape presets
//
// class definition over in easy.hpp but initialization
// doesn't belong there
//
// Important considerations here:
// - values and times are scaled 0-.99
// - there have to be the same number of values and times in each pair of tables
// - don't use the end time of 1., use something like .95 instead, as length of
//   signal is used in the slope calculation

std::vector<double> notch1_entT {0,    .1   ,.15    ,.2    ,.3   ,.9};
std::vector<double> notch1_entV {.99,  .6   ,.3     ,.6    ,.9  ,.99};

std::vector<double> notch2_entT {0     ,.1   ,.2    ,.3   ,.5};
std::vector<double> notch2_entV {.99   ,.1   ,.4    ,.8  ,.99};

std::vector<double> notch3_entT {0,    .1    ,.13   ,.2  ,.3};
std::vector<double> notch3_entV {.99,  .6    ,.3    ,.6  ,.99};

std::vector<double> tease1_entT {0   ,.1   ,.2  ,.3  ,.4  ,.5   ,.6  ,.7  ,.8   ,.9    ,.95};
std::vector<double> tease1_entV {.8  ,.9   ,1   ,.75 ,.8  ,.85  ,.8  ,.75 ,.84  ,.95   ,1.};

std::vector<double> tease2_entT {0   ,.1   ,.2  ,.3  ,.4  ,.5   ,.6  ,.7  ,.8   ,.9    ,.95};
std::vector<double> tease2_entV {.8  ,.7   ,.75 ,.85 ,.9  ,.95  ,.99   ,.99   ,.99    ,.75   ,.85};

std::vector<double> tease3_entT {0     ,.1   ,.2  ,.3   ,.4  ,.5     ,.6  ,.7   ,.8   ,.9  ,.95};
std::vector<double> tease3_entV {.99   ,.95   ,9  ,.95  ,.85  ,.95   ,.9  ,.99  ,.99   ,.9 ,.85};

std::vector<double> pulse1_entT {0   ,.1   ,.2   ,.3   ,.4  ,.5    ,.6  ,.7  ,.8   ,.9  ,.95};
std::vector<double> pulse1_entV {0   ,.5   ,.6   ,.75  ,.8  ,.99   ,.8  ,.75 ,.6   ,.5  ,.2};

std::vector<double> pulse2_entT {0   ,.1   ,.2  ,.3   ,.4    ,.5    ,.6   ,.7  ,.8   ,.9  ,.95};
std::vector<double> pulse2_entV {.5  ,.7   ,.8   ,.9  ,.95   ,.99   ,.95   ,.9 ,.8   ,.7  ,.5};

std::vector<double> pulse3_entT {0   ,.1    ,.2  ,.3   ,.4   ,.5    ,.6    ,.7   ,.8  ,.9  ,.95};
std::vector<double> pulse3_entV {0   ,.4    ,.5  ,.65  ,.85  ,.95   ,.99   ,.95  ,.8  ,.6  ,.3};

std::vector<double> kick1_entT {0   ,.1   ,.2  ,.3  ,.4  ,.5   ,.6  ,.7  ,.8   ,.9    ,.95};
std::vector<double> kick1_entV {.5  ,.5   ,.5   ,.75 ,.9  ,.99   ,.99   ,.9  ,.75  ,.5    ,.5};

std::vector<double> kick2_entT {0   ,.1   ,.2  ,.3  ,.4    ,.5   ,.6    ,.7   ,.8   ,.9   ,.95};
std::vector<double> kick2_entV {.8  ,.8   ,.8   ,.8  ,.95   ,.99   ,.95   ,.8    ,.8   ,.8  ,.8};

std::vector<double> kick3_entT {0   ,.1   ,.2   ,.3   ,.4   ,.5    ,.6   ,.7   ,.8  ,.9  ,.95};
std::vector<double> kick3_entV {.3  ,.3   ,.3   ,.3   ,.8   ,.95   ,.99    ,.99    ,.8  ,.3  ,.3};

std::vector<double> adsr1_entT {0    ,.1   ,.2   ,.3   ,.4   ,.5   ,.6   ,.7   ,.8   ,.9   ,.95};
std::vector<double> adsr1_entV {.75  ,.99    ,.99    ,.75  ,.7   ,.7   ,.7   ,.7   ,.6   ,.4   ,.3};

std::vector<double> adsr2_entT {0    ,.1   ,.2   ,.3   ,.4   ,.5   ,.6   ,.7   ,.8   ,.9   ,.95};
std::vector<double> adsr2_entV {.5   ,.8   ,.99    ,.99    ,.99    ,.7   ,.6   ,.5   ,.4   ,.3   ,.1};
std::vector<double> adsr3_entT {0    ,.1   ,.2   ,.3   ,.4   ,.5   ,.6   ,.7   ,.8   ,.9   ,.95};
std::vector<double> adsr3_entV {.75  ,.99    ,.99    ,.99    ,.7   ,.6   ,.4   ,.4   ,.3   ,.2   ,.1};

std::vector<double> rev1_entT {0    ,.1   ,.2   ,.3   ,.4   ,.5   ,.6     ,.7    ,.8    ,.9  ,.95};
std::vector<double> rev1_entV {.4   ,.5   ,.6   ,.7   ,.8   ,.9   ,.99    ,.99    ,.99  ,.8  ,.6};

std::vector<double> rev2_entT {0    ,.1   ,.2   ,.3   ,.4   ,.5   ,.6   ,.7   ,.8   ,.9   ,.95};
std::vector<double> rev2_entV {.65  ,.7   ,85   ,.88  ,.9   ,.93  ,.96  ,.98  ,.99  ,.99  ,.99};

std::vector<double> rev3_entT {0    ,.1   ,.2    ,.3   ,.4   ,.5   ,.6     ,.7     ,.8     ,.9   ,.95};
std::vector<double> rev3_entV {.2   ,.4    ,.6   ,.7   ,.8   ,.9   ,.99    ,.99     ,.99   ,.99  ,.8};

std::vector<double> wedge1_entT {0      ,.4      ,.7    ,.95};
std::vector<double> wedge1_entV {.001   ,.001    ,.99   ,.99};

std::vector<double> wedge2_entT {0      ,.2      ,.5    ,.95};
std::vector<double> wedge2_entV {.001   ,.001    ,.99   ,.99};

std::vector<double> gap1_entT {0      ,.9      ,.91    ,.001};
std::vector<double> gap1_entV {.991   ,.99     ,.001   ,.99};

std::vector<double> gap2_entT {0     ,.15     ,.8      ,.81    ,.001};
std::vector<double> gap2_entV {.001  ,.99     ,.99     ,.001   ,.99};


void Shape::loadTable(void) {
    std::vector<double>::iterator it;
    
    // scale all values and times 0 to 1 full range

    if (preset==SH_NOTCH1) {
      entV=notch1_entV;
      entT=notch1_entT;
    }
    else if (preset==SH_NOTCH2) {
      entV=notch2_entV;
      entT=notch2_entT;
    }
    else if (preset==SH_NOTCH3) {
      entV=notch3_entV;
      entT=notch3_entT;
    }
    else if (preset==SH_TEASE1) {
      entV=tease1_entV;
      entT=tease1_entT;
    }
    else if (preset==SH_TEASE2) {
      entV=tease2_entV;
      entT=tease2_entT;
    }
    else if (preset==SH_TEASE3) {
      entV=tease3_entV;
      entT=tease3_entT;
    }
    else if (preset==SH_PULSE1) {
      entV=pulse1_entV;
      entT=pulse1_entT;
    }
    else if (preset==SH_PULSE2) {
      entV=pulse2_entV;
      entT=pulse2_entT;
    }
    else if (preset==SH_PULSE3) {
      entV=pulse3_entV;
      entT=pulse3_entT;
    }
    else if (preset==SH_KICK1) {
      entV=kick1_entV;
      entT=kick1_entT;
    }
    else if (preset==SH_KICK2) {
      entV=kick2_entV;
      entT=kick2_entT;
    }
    else if (preset==SH_KICK3) {
      entV=kick3_entV;
      entT=kick3_entT;
    }
    else if (preset==SH_ADSR1) {
      entV=adsr1_entV;
      entT=adsr1_entT;
    }
    else if (preset==SH_ADSR2) {
      entV=adsr2_entV;
      entT=adsr2_entT;
    }
    else if (preset==SH_ADSR3) {
      entV=adsr3_entV;
      entT=adsr3_entT;
    }
    else if (preset==SH_REV1) {
      entV=rev1_entV;
      entT=rev1_entT;
    }
    else if (preset==SH_REV2) {
      entV=rev2_entV;
      entT=rev2_entT;
    }
    else if (preset==SH_WEDGE1) {
      entV=wedge1_entV;
      entT=wedge1_entT;
    }
    else if (preset==SH_WEDGE2) {
      entV=wedge2_entV;
      entT=wedge2_entT;
    }
    else if (preset==SH_GAP1) {
      entV=gap1_entV;
      entT=gap1_entT;
    }
    else if (preset==SH_GAP2) {
      entV=gap2_entV;
      entT=gap2_entT;
    }

   // add one more target to every table... for loop around
    
    entV.push_back(entV[0]);   // same as start
    entT.push_back(1.0);       // will scale to length

    assert(entT.size()==entV.size());
    steps=entT.size();
    lenX=1.*length;  // convert times scaled to length
    
    for(int i=0; i<steps; i++) {
      double value=entT.at(i);  // annoying: C++ doesn't seem to do the right thing without this
      uint32_t entTXval=value*length;
      entTX.push_back(entTXval);  // convert times to samples and scaled to length
    }
    
    va=entV[1];
    vb=entV[0];
    ta=entTX[1];
    tb=entTX[0];
    
    step=0;

    // printf("initial shape (step %ld): %f -> %f   %d -> %d\n",step,vb,va,tb,ta);
}

//----------------------------------------------------------------------
// handle variable storage
// 
// a variable can be a float (if the result of a math equation)
// or a list of nodes (if the left hand side works out to anything else)
//
// Tried this as a structure but defeated by C++ complexities.

#define V_FLOAT 0
#define V_STRING 1

map<string,int> variables;
map<string,float> varValues;
map<string,node *> varNodes;
map<string,long> varFilepos;

//--------------------------------------------------

void listVariables(void) {
  // std::cout << "    List of all variables:\n";

  for(auto it=variables.begin(); it != variables.end(); ++it) {
    if (it->second==V_FLOAT) {
      std::cout << "      Key: " << it->first << "=" << varValues[it->first] << "\n";
    }
    else {
      std::cout << "      Key: " << it->first << "=" << "... is defined as something" << "\n";
    }
  }
}

//----------------------------------------------------------------------
// 4th or 5th time rewriting this
// It replaces the node at the spot given with the
// nodes listed in nodes.
//
// It does not do this by patching the linked list.
// It reconstructs the linked list from scratch.
//
// In general, patching out nodes has proven to be very fault-prone.
// Instead, our strategy is to mark them as NOOPs and skip over them
// when looking left or right.
//
//
void replaceMacro(node * spot, node *nodes) {
  node * ptr;
  
  // save old begn and elist in case they are needed

  node *oldBegn=begn;
  // node *oldElist=elist;

  emptyList();
  
  ptr=oldBegn;    // working from left --> right
  
  while (ptr!=spot) {
    push (ptr->dtype,ptr->value,ptr->str);
    ptr=ptr->rght; 
  }

  node *tailend=spot->rght;
  
  // we are skipping spot, which is the location of the macro name
  // now copy the replacement nodes

  ptr=nodes;
  while (ptr!=NULL) {
    push (ptr->dtype,ptr->value,ptr->str);
    ptr=ptr->rght; 
  }
  
  // okay - now we just have to copy any trailing nodes for the line
 
  ptr=tailend;
  while (ptr!=NULL) {
    push (ptr->dtype,ptr->value,ptr->str);
    ptr=ptr->rght; 
  }

  // and this should be good
  // unlike other methods, this should not require
  // handling of special edge cases
}

//--------------------------------------------------
// this routine does the substituion of either
// a value (simple) or a macro (a string of nodes inserted)
//
// it needs to pass "false" to indicate nothing found
// to handle variables-within-variables. Once it passes
// false, there should be no strings left on the line

bool swapVariables(void) {
  int count=0;
  bool found=false;
  bool loopFlag=false;   // swaps are disabled after loop commands
  
   //start from the begn
   struct node *ptr = begn;
   // displayBackward();
	
   while ((ptr != NULL)&&(loopFlag==false)) {
     count++;

     if (ptr->dtype==LOOP) {
       loopFlag=true;
     }
     else if (ptr->dtype==STRING) {
       string name=string(ptr->str);
       
       if (variables.find(name)!=variables.end()) {
         int type=variables[name];
         if (type==V_FLOAT) {
           if (ptr->filled==false) {
             // cout << name << " is a float\n";
             found=true;               // did a substitution
             // replace this node with the NUMBER
             // change: don't replace node, just populate
             // the value of the variable

             ptr->value=varValues[name];  // change datatype
             ptr->dtype=NUMBER;          // and fill in number

              // flag that this variable was populated with a number
             
             ptr->filled=true;
           }
         }
         else {
           //reconstruct line and start over
           //
           // monkeying around with patching over nodes was
           // very error prone. We don't care about memory
           // leakage.
           
           replaceMacro(ptr,varNodes[name]);

           return true;
         }
       }
       else {
         // note: will probably need to do something about the left-hand
         // side of assignments

       }
     }
     
     //move to lft item
     ptr = ptr ->rght;
   }
   return found;
}


//----------------------------------------------------------------------
// start is either the node that has the value
// or the start of the macro string
//

void doAssignment (node * start) {
  const char * varname;
  node * rightside;
  rightside=start->rght;

  varname=GetVarname();
  string varnameStr=string(varname);

  // printf("    debug: assignment varname is %s\n",varname);

  if (isNumerical()) {
    // printf("    debug: storing numerican variable %s\n",varname);
    if (variables.find(varnameStr) != variables.end()) {
      //found: modify existing
      variables[varnameStr]=V_FLOAT;
      varValues[varnameStr]=rightside->value;
    }
    else {
      // not found: add new entry
      variables.insert({varnameStr,V_FLOAT});
      varValues.insert({varnameStr,rightside->value});
    }
  }
  else {
    // printf("    debug: storing macro variable %s\n",varname);

    // otherwise, store the sequence of commands
    // here we copy from the present location to the end of the line

    variables.insert({varnameStr,V_STRING});
    rightside->lft=NULL;
    varNodes.insert({varnameStr,rightside});

    // this feels too simple: indeed, left pointer needed to be removed
  }
}

//----------------------------------------------------------------------
// exits on an error with a message
// TODO: add a line number to this

void syntaxError (node * cur,const char * str) {
  printf("%sERROR - line number %d - %s\n%s",RED,lineNumber,str,WHT);
  exit(1);
}

//----------------------------------------------------------------------
// This confirms that the node before and after the current command
// exist and are numbers. It's a syntax error if they aren't
//

bool confirmRLnum(node * right, node *left) {
  int t1,t2;

  t1=right->dtype;
  t2=left->dtype;

  if (((t1==NUMBER)||(t1==STRING)) &&( (t2==NUMBER)||t2==STRING)) {
    return true;
  }

  return false;
}

//----------------------------------------------------------------------
// confirm that Left node is anything but a number
// and the Right node IS a number

bool confirmRnumLop(node * right, node * left) {
  int tright,tleft;

  tright=right->dtype;
  tleft=left->dtype;

  if (((tleft!=NUMBER)&&(tleft!=STRING)) && ((tright==NUMBER)||(tright==NUMBER))) {
    return true;
  }

  return false;
}

//----------------------------------------------------------------------

void init (void) {
  defaults.freq=new Value(1000.);
  defaults.freq2=new Value(2000.);
  defaults.freq3=new Value(3000.);
  defaults.vol=new Value(0.70);       
  defaults.vol2=new Value(0);         // harmonics off by default
  defaults.vol3=new Value(0);

  defaults.automix=0.7;
  defaults.form=WF_SINE;
  defaults.fadein=0.5;
  defaults.fadeout=0;
  defaults.bal=new Value(0);
  defaults.left=true;
  defaults.right=true;
  defaults.phase=new Value(0.0);
  defaults.duty=new Value(0.5);
  defaults.shape=new Value(1.0);      // no shape by default
  defaults.circuit=false;             // OFF by default

  defaults.cirp=new Value(.4);
  defaults.ciri=new Value(.2);

  rewindHistory.push(masterTime);  // start history at time 0.

  
  srand(time(NULL));  // init random number generation


  if (flag48!=0) {
    wavout=new WaveWriter(48000*60*60*2.5,48000);
    SR=48000;
    printf("%sOutput format is 48kHz, 24 bit .wav\n%s",MAG,WHT);
  }
  else {
    wavout=new WaveWriter(44100*60*60*2.5,44100);
    SR=44100;
    printf("%sOutput format is 44.1kHz, 16 bit .wav\n%s",MAG,WHT);
  }
}

//--------------------------------------------------
// Copy current settings from defaults before overriding them
// in commands on current line.

void copySettings (void) {
  settings.freqStack.push(defaults.freq);
  settings.formStack.push(defaults.form);
  settings.phaseStack.push(defaults.phase);
  settings.dutyStack.push(defaults.duty);
  
  settings.freq2=defaults.freq2;
  settings.freq3=defaults.freq3;
  settings.vol=defaults.vol;
  settings.vol2=defaults.vol2;
  settings.vol3=defaults.vol3;
  settings.automix=defaults.automix;
  settings.fadein=defaults.fadein;
  settings.fadeout=defaults.fadeout;
  settings.bal=defaults.bal;
  settings.left=defaults.left;
  settings.right=defaults.right;
  settings.shape=defaults.shape;
  settings.circuit=defaults.circuit;
  settings.ciri=defaults.ciri;
  settings.cirp=defaults.cirp;
}

//---------------------------------------------------
// so, if the line ends without a command, this means
// we intended to change a default.
// so copy them back.

void copySettingsToDefaults (void) {
  defaults.freq=settings.freqStack.top();
  settings.freqStack.pop();                   // STL, uuugh. STL is crap.

  defaults.form=settings.formStack.top();
  settings.formStack.pop();

  defaults.phase=settings.phaseStack.top();
  settings.phaseStack.pop();

  defaults.duty=settings.dutyStack.top();
  settings.dutyStack.pop();
  
  defaults.freq2=settings.freq2;
  defaults.freq3=settings.freq3;
  defaults.vol=settings.vol;
  defaults.vol2=settings.vol2;
  defaults.vol3=settings.vol3;
  defaults.automix=settings.automix;
  defaults.fadein=settings.fadein;
  defaults.fadeout=settings.fadeout;
  defaults.bal=settings.bal;
  defaults.left=settings.left;
  defaults.right=settings.right;
  defaults.circuit=settings.circuit;
  defaults.ciri=settings.ciri;
  defaults.cirp=settings.cirp;

  // shape is never copied back
}

//--------------------------------------------------
// these pull one of the following settings off the stack
// making sure at least one residual setting remains

NumberDriver * getFreqStack (void) {
  NumberDriver * nd;

  nd=settings.freqStack.top();
  if (settings.freqStack.size()>1) {
    settings.freqStack.pop();
  }
  return nd;
}

NumberDriver * getDutyStack (void) {
  NumberDriver * nd;

  nd=settings.dutyStack.top();
  if (settings.dutyStack.size()>1) {
    settings.dutyStack.pop();
  }
  return nd;
}

NumberDriver * getPhaseStack (void) {
  NumberDriver * nd;

  nd=settings.phaseStack.top();
  if (settings.phaseStack.size()>1) {
    settings.phaseStack.pop();
  }
  return nd;
}

int getFormStack (void) {
  int form;

  form=settings.formStack.top();
  if (settings.formStack.size()>1) {
    settings.formStack.pop();
  }
  return form;
}

//----------------------------------------------------------------------
// CheckRight
//
// This checks if the node to the right of the current node has
// something that can be turned into a numerical setting
// (that is, a NumberDriver object).
//
// CheckRight:
//    - turns NUMBER into Value
//    - returns any other NumberDriver found at the node
//
// This will not skip over NOOP, TO, or COMMA

NumberDriver * CheckRight (node * n) {
  node * right;
  
  // first, check to make sure this is not the end of the line

  right=GetRight(n);
  if (right==NULL) {
    return NULL;
  }
  
  // see if it is a plain number...

  if (right->dtype==NUMBER) {
    float val=right->value;
    // printf("...debug value is %f\n",val);
    Value * nd=new Value(val);
    return nd;
  }

  // a string that has been replaced by a number is fine too...

  if (right->dtype==STRING) {
    float val=right->value;
    if (val==NO_NUMBER) {
      printf("%s ERROR - Line %d - The string '%s' wasn't defined so can't be used as a number.\n%s",RED,lineNumber,right->str,WHT);
      exit(2); 
    }
    
    Value * nd=new Value(val);
    return nd;
  }

  // lft, check for it it is an oscillator

  if (right->dtype==OSC) {
    // right->numberDriver Osc * nd=new Value(right->value);
    return right->nd;
  }

  // or maybe a ramp?

  if (right->dtype==RAMP) {
    return right->nd;
  }

  // or a random sequence?

  if (right->dtype==RANDSEQ) {
    return right->nd;
  }

  // or a seq?

  if (right->dtype==SEQ) {
    return right->nd;
  }

  // or a ramps?

  if (right->dtype==RAMPS) {
    return right->nd;
  }

  // or a shape?
  //
  // at present, SHAPE are set to drive volume only

  if (right->dtype==SHAPE) {
    return right->nd;
  }

  return NULL;
}


//----------------------------------------------------------------------
// StringRight
//
// This at the node on the right and expects it to be a string.
//
// This will not skip over NOOP, TO, or COMMA

char * StringRight (node * n) {
  node * right;
  
  // first, check to make sure this is not the end of the line

  right=GetRight(n);
  if (right==NULL) return NULL;

  // this better be a string...

  if (right->dtype==STRING) {
    return right->str;
  }

  return NULL;
}

//----------------------------------------------------------------------
// FilenameRight
//
// This at the node on the right and expects it to be a string.
//
// This will not skip over NOOP, TO, or COMMA

const char * FilenameRight (node * n) {
  node * right;
  
  // first, check to make sure this is not the end of the line

  right=GetRight(n);
  if (right==NULL) return "";

   // this better be a string...

  if (right->dtype==FILENAME) {
    return right->str;
  }

  return NULL;
}

// makes copy of filename without quotes

const char * doFilename (char * str) {
  char * newStr=(char *) malloc(strlen(str)+1);

  char *spot=newStr;
  for (int i=0;i<(int) strlen(str);i++) {
    if (str[i]!='"') {  // don't copy double quotes
      *spot=str[i];
      spot++;
    }
  }
  *spot='\0';
  return newStr;
}

//----------------------------------------------------------------------
// GetLeft  / GetRight
//
// Gets the Left or Right nodes on either side of the node
// given automatically skipping over NOOP (nodes that have
// been deleted) and COMMA
//

node * GetLeft (node * spot) {

  while (spot->lft!=NULL) {
    spot=spot->lft;
    if ((spot->dtype!=NOOP)&&(spot->dtype!=COMMA)) {
      return spot;
    }
  }
  return NULL;
}

node * GetRight (node * spot) {

  while (spot->rght!=NULL) {
    spot=spot->rght;
    if ((spot->dtype!=NOOP)&&(spot->dtype!=COMMA)) {
      return spot;
    }
  }
  return NULL;
}


//----------------------------------------------------------------------
// ToRight
//
// The 'to' command is necessary when there is more than one
// argument and the second argument might be negative. There is no
// other clean way to resolve this ambiguity.
//
// A benefit is that I think it makes the script easier to read.
//
// Examples:
// ramp 0 to -1
// osc -1 to 1
//
// If found, the 'to' node is returned to it can be skipped.
// If not found, the node passed in is returned.

node * ToRight (node * n) {
  node * right;
  
  // first, check to make sure this is not the end of the line

  right=GetRight(n);    // this will skip over NOOP and COMMA
  
  if (right==NULL) return NULL;

  if (right->dtype==TO) {
    return right;
  }

  return n;
}

//----------------------------------------------------------------------
// NumberRight
//
// This at the node on the right and expects it to be a NUMBER and
// not any form of number driver.

double NumberRight (node * n) {
  node * right;
  
  // first, check to make sure this is not the end of the line

  right=GetRight(n);    // skips over NOOP and COMMA
  if (right==NULL) return NO_NUMBER;

  if (right->dtype==NUMBER) {
    return right->value;
  }

  return NO_NUMBER;
}


//============================================================================
// This is essentially the main loop which is called once per line. The line
// is a linked list of nodes at this point.
//
// Macros and numerical variables are swapped in at the beginning.
// Math is done lft:
//    a) ambiguity between positive/plus and negative/minus
//    b) multiplication, division, and modulus
//    c) addition/subtraction
//
// Then lines that work out to macros or math assignments are handled.
//
// Then various types of settings and commands are handled, interactively,
// until there is nothing left to process.

void processCommands (void) {
  node * ass;
  updateDefaults=true;
  
  copySettings();

  printf("line %d\n",lineNumber);
  // printf("  before swapVariables... \n");
  
  // displayForward();
  // displayBackward();
  
  // swap variables into line

  while (swapVariables()) {}  // loop until no more to swap

  // printf("  after swapVariables... \n");

  // displayForward();
  // displayBackward();
  
  // printf("  before math0\n");

  // displayForward();
  // displayBackward();

  doMath0();           // handle positives and negatives

  // printf("  after math0, before math1 \n");

  // displayBackward();

  doMath1();           // multi,div,modulus

  // printf("  after math1, before math2 \n");

  // displayBackward();

  doMath2();           // adding and subtracting

  // printf("  after math2 \n");

  // at this point, we are down to two types of commands:
  // 1. a line that does something now like set
  //     a setting or make a sound
  // 2. a line that results in an assignment
  //
  // At this point, since the math is done, we need
  // do work out which this is.
  // Assigments will not result in further processing.
  // Whereas case (1) gets processed.
  //
  // This, in effect, defers procssing of various settings
  // until the sound is formed.
  
  if ((ass=isAssignment())!=NULL) {
    doAssignment(ass);
    // listVariables();
  }
  else {

    // do repeats and process remainder of line

    // TODO: experimennt with moving this up
    // so it surrounds the entire line's processing
    // that way, math commands will be repeatedly
    // processed. Writing this, I don't think
    // that will really work because assignments
    // and actions don't mix on one line.
    
    lookForRepeat();
  }

  if (updateDefaults) {
    // printf("updating defaults line %d\n",lineNumber);
    copySettingsToDefaults();
  }
  
  // displayBackward();
  emptyList();
}


//----------------------------------------------------------------------
// looks for the repeat command and runs
// commands after it xx times
//

void lookForRepeat() {
  bool repeatFound=false;
  //start from the begn
  struct node *ptr = begn;
	
  // navigate from the start
	
   while(ptr != NULL) {    
     if (ptr->dtype==REPEAT) {               // repeat!
       int repeats=int(NumberRight(ptr));
        if (repeats==NO_NUMBER) {
          repeats=1;
        }
        // printf("cmd: repeat %d times\n",repeats);
        for (int i=0; i<repeats; i++) {
          // printf("cmd: repeat %d / %d\n",i,repeats);
          
          // get rid of that repeat node and the one after otherwise we get
          // too many repeats

          ptr->rght->dtype=NOOP;
          ptr->dtype=NOOP;

          // now, do the repeats
          // note that numbers don't get recalculated
          
          doStuff();    // handle whatever commands are left multiple times
          repeatFound=true;
        }
     }
     ptr = ptr ->rght;
   }

   if (repeatFound==false) {
     doStuff();        // if no repeats call the line once.
   }
}

//----------------------------------------------------------------------
// do any tidy up work and write the output file

void finish(void) {

  if (outputFile==NULL) {

    // auto generate filename from infile if not specified
    
    printf("%sInfile was %s\n%s",CYN,copyinfile,CYN);
    outputFile=strdup(copyinfile);
    char * location=strcasestr(outputFile,".e2");
    if (location==NULL) {
      printf("Sorry - expecting .e2 input filename. Could not output.\n");
      exit(-1);
    }
    location++;  *location='w';
    location++;  *location='a';
    location++;  *location='v';
    location++;  *location='\0';

    printf("%s Output automatically set to %s %s\n",CYN,outputFile,WHT);
  }
  
  printf ("%s Writing file %s%s\n",CYN,outputFile,WHT);
  
  wavout->writeFile(outputFile);
  doMp3(outputFile,copyinfile);
}

//----------------------------------------------------------------------
// isAssignment looks for the first operator
// it finds on the line. If it is an assignment, then this line
// must involve some sort of assignment. It should normally be found
// at 2nd position after the label name.

node * isAssignment(void) {
  int count=0;
  node *cur;
  
  for (cur=begn; (cur!=NULL)&&(count<=2); cur=cur->rght) {
    count++;
    
    if (cur->dtype==ASSIGNMENT) {
      if (count!=2) {
        printf("info: unusual, assignment wasn't at position 2");
      }
      return cur;
    }
    else if ((cur->dtype>=0)&&(cur->dtype<100)) {
      // so, first thing found doesn't qualify this as an assignment
      return NULL;
    }
    // should be a string at position 0
  }

  return NULL;
}

//----------------------------------------------------------------------
// isNumerical looks for a purely
// mathematical calculation - this will result in a NUMBER at position
// 3

bool isNumerical(void) {
  int count=0;
  node * cur;
  
  for (cur=begn; (cur!=NULL)&&(count<=3); cur=cur->rght) {
    count++;
    
    if ((count==3)&&(cur->dtype==NUMBER)) {
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------
// Loads a table of values for either a Seq or a Ramps
//
// This code should work for either. Didn't really want to cut-paste it
// but it didn't seem right to make it a common NumberDriver method.

bool loadSeq(node * n, Seq * seq, Ramps *rmp) {
  double v1=0;
  double d1;
  bool found=false;
  long count=0;
  node * orig=n;
  vector<double> seqValues;
  vector<uint32_t> seqTimes;
  
  do {
    if (n->rght==NULL) {    // node on right exists?
      if (found==true) {
        if (seq!=NULL) {
          seq->load(seqValues,seqTimes);  // DONE!
          orig->nd=seq;
        }
        else if (rmp!=NULL){
          rmp->load(seqValues,seqTimes);  // DONE!
          orig->nd=rmp;
        }
        else {
          printf("huh? programming error\n");
          exit(-1);
        }
        return true;        // no- but we've found at least 1 pair
      }
      else {
        syntaxError(n,"No value table\n");   // otherwise error
        exit(0);
      }
    }
    n=GetRight(n);              // move right skipping NOOP and COMMA
    
    if (n->dtype==NUMBER) {    // number?   
      v1=n->value;             // get it
    }
    else {
      syntaxError(n,"Need number for value table\n");  //TODO: this is an error
      exit(0);
    }
    
    if (n->rght==NULL) {    // nother node on right exists?
      syntaxError(n,"Missing time for value in SEQ or RAMPS (always need pairs of numbers)\n");
      exit(0);
    }
    n=GetRight(n);               // move right
    
    if (n->dtype==NUMBER) {       // is this a number?
      if ((n->value==0)&&(found==false)) {
        syntaxError(n,"In table, don't make the first value 0.\n");
        exit(0);
      }
      
      d1=n->value*SR;             // get it but multiply by sample rate
    }
    else {
      syntaxError(n,"Need number for time in value table for SEQ or RAMPS (need pairs of numbers)\n");
      exit(0);
    }
    found=true;
    count++;

    // it would be better if vt,dt were in a single object but STL Containers in C++ are
    // a kludge. So after wasting a few hours debugging... fuck that.
    
    seqValues.push_back(v1);
    seqTimes.push_back(d1);
    
  } while(1);
}

//----------------------------------------------------------------------
// returns the variable name for an assignment
//
// this should always be found at the first part of the line
// which is the begn step

const char * GetVarname(void) {
  if (begn!=NULL) {
    if (begn->dtype==STRING) {
      return begn->str;
    }
  }
  return NULL;
}

//----------------------------------------------------------------------
// This handles an ambiguity between positive and plus and negative and minus.
//
// How?
//
// If a + sign has not a number to its left, it can be eliminated.
// If a - sign has not a number to its left, multi the number to the right by -1 and
//             eliminate it.
//
// This does rghtent us from detecting a +/- sign for other purposes.

void doMath0 (void) {
  node *cur;
  node *right;
  node *left;

  for (cur=begn; cur!=NULL; cur=cur->rght) {
    if (cur->dtype==PLUS) {
      left=cur->lft;
      right=cur->rght;
      
      if (confirmRnumLop(right,left)) {               // left must be not number and right must be numbers
        left->rght=right;                      // splice over current node
        right->lft=left;
        zeroNode(cur);
      }
    }
    else if (cur->dtype==MINUS) {
      left=cur->lft;
      right=cur->rght;
      
      if (confirmRnumLop(right,left)) {               // left must be not number and right must be numbers
        left->rght=right;                      // splice over current node
        right->lft=left;
        right->value=-right->value;            // switch sign of lft node
        zeroNode(cur);
      }
    }
  }
}

//----------------------------------------------------------------------
// This routine processes the math operations for multiply, divide, and modulus
// from left to right. These equiations are on our double-linked list. After
// doing the math, we replace the 3 steps on the linked list with the answer.
//
// so 2,*,3 gets replaced with a single NUMBER step of 6
//
// TODO: concept of direction in list doesn't match written direction
// DELETEs aren't happening 
//
// NOTE: stuff loaded left->right shows up as begn->elist 
//      
//

void doMath1 (void) {
  node *cur;
  node *right;
  node *left;

  for (cur=begn; cur!=NULL; cur=cur->rght) {
    double result;
    if (cur->dtype==MULT) {
      left=GetLeft(cur);
      right=GetRight(cur);

      if (left==NULL)  syntaxError(NULL,"expecting a number to left of MULT operator"); 
      if (right==NULL)  syntaxError(NULL,"expecting a number to right of MULT operator"); 
      
      if (!confirmRLnum(right,left)) {                          // left and right nodes must be numbers
        syntaxError(NULL,"error with MULT operator"); 
      }
      result=(left->value*right->value); // so math op with left and right operands
      replaceNode(left,NUMBER,result,NULL);  // put answer in LEFT node

      // zero out other two nodes with NOOPs
      
      zeroNode(right);                      // delete right operand
      zeroNode(cur);                        // delete right operand
      cur=left;                             // this will make for() work
    }
    else if (cur->dtype==DIV) {
      left=GetLeft(cur);
      right=GetRight(cur);

      if (left==NULL)  syntaxError(NULL,"expecting a number to left of DIV operator"); 
      if (right==NULL)  syntaxError(NULL,"expecting a number to right of DIV operator"); 
      
      if (!confirmRLnum(right,left)) {                          // left and right nodes must be numbers
        syntaxError(NULL,"error with DIV operator"); 
      }
      if (right->value==0) {
        syntaxError(NULL,"divide by zero");
      }

      result=(left->value/right->value); // so math op with left and right operands
      replaceNode(left,NUMBER,result,NULL);  // put answer in LEFT node

      // zero out other two nodes with NOOPs
      
      zeroNode(right);                      // delete right operand
      zeroNode(cur);                        // delete right operand
      cur=left;                             // this will make for() work
    }
    else if (cur->dtype==MODULUS) {
      left=GetLeft(cur);
      right=GetRight(cur);

      if (left==NULL)  syntaxError(NULL,"expecting a number to left of DIV operator"); 
      if (right==NULL)  syntaxError(NULL,"expecting a number to right of DIV operator"); 

      if (right->value==0) {
        syntaxError(NULL,"divide by zero");
      }
      result=(int(left->value)%int(right->value)); // so math op with left and right operands
      replaceNode(left,NUMBER,result,NULL);  // put answer in LEFT node

      // zero out other two nodes with NOOPs
      
      zeroNode(right);                      // delete right operand
      zeroNode(cur);                        // delete right operand
      cur=left;                             // this will make for() work

    }
  }
}

//----------------------------------------------------------------------
// This routine handles adding and subtracting on the linked list.
// We replace the three nodes on the list with one node which is the answer.
//
// so 2,+,2 gets replaced with 4


void doMath2 (void) { 
  node *cur;
  node *right;
  node *left;

  for (cur=begn; cur!=NULL; cur=cur->rght) {
    // displayNode(cur);
    double result;
    if (cur->dtype==PLUS) {
      left=GetLeft(cur);
      right=GetRight(cur);

      if (left==NULL)  syntaxError(NULL,"expecting a number to left of MULT operator"); 
      if (right==NULL)  syntaxError(NULL,"expecting a number to right of MULT operator"); 

      if (!confirmRLnum(right,left)) {
        syntaxError(NULL,"error with PLUS operator"); 
      }
      
      result=(left->value+right->value); // so math op with left and right operands
      // printf("      result: %lf\n",result);
      
      replaceNode(left,NUMBER,result,NULL);  // put answer in LEFT node

      // zero out other two nodes with NOOPs
      
      zeroNode(right);                      // NOOP in right operand
      zeroNode(cur);                        // NOOP in current operand
      cur=left;                             // this will make for() work
    }
    else if (cur->dtype==MINUS) {
      left=GetLeft(cur);
      right=GetRight(cur);

      if (left==NULL)  syntaxError(NULL,"expecting a number to left of MULT operator"); 
      if (right==NULL)  syntaxError(NULL,"expecting a number to right of MULT operator"); 


      if (!confirmRLnum(right,left)) {
        syntaxError(NULL,"error with MINUS operator"); 
      }

      result=(left->value-right->value); // so math op with left and right operands
      // printf("      result: %lf\n",result);
      
      replaceNode(left,NUMBER,result,NULL);  // put answer in LEFT node

      // zero out other two nodes with NOOPs
      
      zeroNode(right);                      // delete right operand
      zeroNode(cur);                        // delete right operand
      cur=left;                             // this will make for() work

    }
  }
}

//----------------------------------------------------------------------
// converts timestamps to numbers of seconds by converting node
//
// the return value is really just for debugging

int convertTimeStampToNumber(node * cur) {
  float value=0;
  char * colon1=NULL;
  char * colon2=NULL;
  char * timestamp=strdup(cur->str);
  char * spot;
  
  //printf("DEBUG: timestamp is %s\n", timestamp);

  // count the number of colons and their location
  // shit, there's probably a better C++ way to do this, but whatever
  // it's cold out and I feel like writing this from scratch

  for (spot=timestamp+strlen(timestamp); spot>=timestamp; spot--) {

    //printf ("char: %c\n",*spot);
    if (*spot==':') {
      if (colon1==NULL) {           // leftmost colon
        colon1=spot;
      }
      else if (colon2==NULL) {      // 2nd colon
        colon2=spot;
      }
      else {
        //printf("That's a colon too many in this timestamp.\n");
        exit(-1);
      }
    }
  }

  // okay, not let's iterpret what we have...

  if ((colon1==NULL)&&(colon2==NULL)) {  // no colons at all
    //printf("Timespec parsing error.\n");
    exit(-1);
  }

  // having covered fractional seconds here
  // I'll get around to it
  
  if ((colon1!=NULL)&&(colon1==timestamp)) {  // e.g. :45
    value=atof(colon1+1);
  }
  else if ((colon1!=NULL)&&(colon2==NULL)&&(colon1!=timestamp)) {  // e.g. 2:45
    value=atof(colon1+1);    // convert the seconds
    //printf("DEBUG: timestamp starts with %f\n", value);
    *colon1='\0';
    float value2=atof(timestamp);  // convert the minutes
    //printf("DEBUG: timestamp and adds %f\n", value2*60);
    value+=value2*60;
  }
  else if ((colon1!=NULL)&&(colon2!=NULL)) {  // e.g. 1:23:45
    value=atof(colon1+1);    // convert the seconds
    //printf("DEBUG: timestamp starts with %f\n", value);
    *colon1='\0';
    float value2=atof(colon2+1);   // convert the minutes
    //printf("DEBUG: timestamp adds %f\n", value2*60);
    value+=value2*60;
    *colon2='\0';
    float value3=atof(timestamp);
    //printf("DEBUG: timestamp adds another %f\n", value3*60*60);
    value+=value3*60*60;
  }

  //printf("DEBUG: timestamp works out to %f\n", value);

  // amend the current node to be a number

  cur->value=value;
  cur->dtype=NUMBER;
  
  return value;
}
  
//----------------------------------------------------------------------
// doStuff
//
// This handles whatever is left.
// Note that this scans right to left.
//

void doStuff(void) {
  node *cur;
  NumberDriver * item;

  //displayForward();
  //displayBackward();

  cur=elist;

  // navigate from end to beginning of list
  // in this way, all settings get processed
  // before they are used by actions
  	
   while(cur != NULL) {        

    //--------------------------------------------------
    // These are the Number Consumers
    // these are settings that take a NumberDriver as
    // am argument
    
    if (cur->dtype==EXIT) {
      printf("%scmd: exit --- COMMAND TO EXITING EARLY!\n%s",YEL,WHT);
      finish();
      exit(0);
    }
    if (cur->dtype==TIMESTAMP) {
      convertTimeStampToNumber(cur);
    }
    else if (cur->dtype==FREQ) {                
        printf("cmd: freq\n");
        item=CheckRight(cur);               
        if (item==NULL) syntaxError(cur,"No frequency specified.\n");
        settings.freqStack.push(item);
    }
    else if (cur->dtype==FREQ2) {               
        printf("cmd: freq2\n");
        item=CheckRight(cur);               
        if (item==NULL) syntaxError(cur,"No frequency specified.\n");
        settings.freq2=item;
    }
    else if (cur->dtype==FREQ3) {           
        printf("cmd: freq3\n");
        item=CheckRight(cur);               
        if (item==NULL) syntaxError(cur,"No frequency specified.\n");
        settings.freq3=item;
    }
    else if (cur->dtype==CIRP) {                
        printf("cmd: cirp\n");
        item=CheckRight(cur);
        if (item==NULL) syntaxError(cur,"Need parameter for circuit modeling proportional term.\n");
        settings.cirp=item;
    }
    else if (cur->dtype==CIRI) {             
        printf("cmd: ciri\n");
        item=CheckRight(cur);
        if (item==NULL) syntaxError(cur,"Need parameter for circuit modeling integral term.\n");
        settings.ciri=item;
    }
    else if (cur->dtype==VOL) {             
      printf("cmd: vol\n");
      item=CheckRight(cur);
      if (item==NULL) syntaxError(cur,"No volume specified.\n");
      settings.vol=item;
    }
    else if (cur->dtype==VOL2) {             
      printf("cmd: vol2\n");
      item=CheckRight(cur);
      if (item==NULL) syntaxError(cur,"No volume specified.\n");
      settings.vol2=item;
    }
    else if (cur->dtype==VOL3) {             
      printf("cmd: vol3\n");
      item=CheckRight(cur);
      if (item==NULL) syntaxError(cur,"No volume specified.\n");
      settings.vol3=item;
    }
    else if (cur->dtype==BAL) {        // range -1 to 1
      printf("cmd: bal\n");
      item=CheckRight(cur);
      if (item==NULL) syntaxError(cur,"No bal specified.\n");
      settings.bal=item;
    }
    else if (cur->dtype==PHASE) {     // range +.5 to -.5
      printf("cmd: phase\n");
      item=CheckRight(cur);
      if (item==NULL) syntaxError(cur,"No phase specified.\n");
      settings.phaseStack.push(item);
    }
    else if (cur->dtype==DUTY) {             
      printf("cmd: duty\n");
      
      item=CheckRight(cur);
      if (item==NULL) {
        syntaxError(cur,"No duty specified.\n");
      }
      
      settings.dutyStack.push(item);
    }

    //--------------------------------------------------
    // simple settings
    //
    // These each take a single plain number and do something.
    
    else if (cur->dtype==TIME) {                  
      printf("cmd: time\n");
      double targetTime=NumberRight(cur);
      if (targetTime==NO_NUMBER) syntaxError(cur,"Need a time in sec.\n");
      printf("%s Going to time %.1f %s\n",GRN,targetTime,WHT);
      masterTime=targetTime;
    }
    else if (cur->dtype==ADDTIME) {                
      printf("cmd: %saddtime%s\n",GRN,WHT);
      double targetTime=NumberRight(cur);
      if (targetTime==NO_NUMBER) syntaxError(cur,"Need a time to add in sec.\n");
      if ((masterTime+targetTime)>0.) {           // targetTime can be -ve
        masterTime+=targetTime;
      }
      else {
        masterTime=0.0;
      }
      printf("%sAdding %f to time = %f\n%s",GRN,targetTime,masterTime,WHT);
    }
    else if (cur->dtype==AUTOMIX) {                
      settings.automix=NumberRight(cur);
      if ((settings.automix>1.) || (settings.automix<0.)) {
        syntaxError(cur,"error - automix range is 0-1. manualmix turns it off.\n");
      }
    }
    else if (cur->dtype==REWIND) {                 // rewind time
      printf("cmd: %srewind%s\n",GRN,WHT);
      int steps=int(NumberRight(cur));
      if (steps==NO_NUMBER) {
        steps=1;
      }

      // printf("cmd debug: rewind %d %ld %f\n",steps,rewindHistory.size(),masterTime);

      if ((rewindHistory.size()-1)<1) {
        // syntaxError(NULL,"can't rewind before 0 time\n");
        masterTime=0.0;
      }
      else if (steps<1) {
        syntaxError(NULL,"rewind needs a number > 0 or nothing\n");
      }
      else {
        for (int i=1; i<steps; i++) {
          masterTime=rewindHistory.top();
          // printf("rewind debug skipping %f \n",masterTime);
          rewindHistory.pop();
        }
        masterTime=rewindHistory.top();
        printf("%srewinding to time %f \n%s",GRN,masterTime,WHT);
      }
    }


    //--------------------------------------------------
    // simple toggles
    //
    // These each take no arguments.

    else if (cur->dtype==FADEIN) {              
      settings.fadein=NumberRight(cur);
    }
    else if (cur->dtype==CIRCUIT) {             
      settings.circuit=true;
    }
    else if (cur->dtype==NOCIRCUIT) {           
      settings.circuit=false;
    }
    else if (cur->dtype==MANUALMIX) {           
      settings.automix=1000.;
    }
    else if (cur->dtype==FADEOUT) {             
      settings.fadeout=NumberRight(cur);
    }
    else if (cur->dtype==RIGHT) {               
      printf("cmd: right\n");
      settings.right=true;
      settings.left=false;
    }
    else if (cur->dtype==LEFT) {                
      printf("cmd: left\n");
      settings.right=false;
      settings.left=true;
    }
    else if (cur->dtype==BOTH) {             
      printf("cmd: both\n");
      settings.right=true;
      settings.left=true;
    }
    else if (cur->dtype==WF_SINE) {          
      printf("cmd: sine\n");
      settings.formStack.push(WF_SINE);
    }
    else if (cur->dtype==WF_SQUARE) {        
      printf("cmd: square\n");
      settings.formStack.push(WF_SQUARE);
    }
    else if (cur->dtype==WF_SAW) {           
      printf("cmd: saw\n");
      settings.formStack.push(WF_SAW);
    }
    else if (cur->dtype==WF_TRI) {           
      printf("cmd: tri\n");
      settings.formStack.push(WF_TRI);
    }
    else if (cur->dtype==WF_TENS) {          
      printf("cmd: tens\n");
      settings.formStack.push(WF_TENS);
    }
    else if (cur->dtype==WF_NOISE) {
      printf("cmd: noise\n");
      settings.formStack.push(WF_NOISE);
    }

    // handle all the sound shapes here. These are all simple
    // settings that optionally take a single optional parameter
    // for repeat period.
    // 
    // shapes are NOT retained in settings from line to line
      
    else if ((cur->dtype>=SH_TEASE1)&&(cur->dtype<=LASTSHAPE)) {              // shape 
      printf("cmd: shape\n");
      
      // handle the optional arg

      node * right = cur->rght;
      double shapeLen=-1;
      if (right!=NULL) {
        if (right->dtype==NUMBER) {
          shapeLen=(right->value)*SR;  // convert to samples
        }
      }
      
      Shape *unusedShape=new Shape(cur->dtype, shapeLen); // temporary
      cur->nd=unusedShape;                   // store this with the node
      settings.shape=unusedShape;            // no: copy used is passed through settings
    }
  
  
    //--------------------------------------------------
    // These are the NumberDrivers
    // NumberDrivers are objects that can drive
    // a sequence of numbers that vary with tim
     
    else if (cur->dtype==RAMP) {               // number driver
      printf("cmd: ramp\n");

      // ramp needs either two or three things
      // if time (3rd) is missing, the length
      // of the sound will be used
      //
      // Ramps are based on fixed or calcuated
      // values, not on other NumberDrivers
      //
      // Ramp settings don't overlap with anything else which
      // makes it simple.
      //
      // 'to' must be present to resolve the potential -ve
      // ambiguity

      double startV=NumberRight(cur);          // mandatory arg
      node * spot=ToRight(cur->rght);            // checks 2 over
      if (spot==NULL) {
        syntaxError(spot,"Need the 'to' keyword in ramp.\n");
      }
      else if (spot->dtype==TO) {
        // printf ("Found 'to' in ramp.\n");
      }
      else {
        syntaxError(spot,"Need the 'to' keyword in ramp.\n");
      }
      
      double endV=NumberRight(spot);            // mandatory arg
      long rampLen=-1;

      // handle the optional arg
      // current position will be the 'to'

      node * right3 = spot->rght->rght;   
      if (right3!=NULL) {
        if (right3->dtype==NUMBER) {
          rampLen=(right3->value)*SR;  // convert to samples
        }
      }

      printf("RAMP: start %f target %f len %ld\n",startV,endV,rampLen);
      
      Ramp *unusedRamp=new Ramp(startV, endV, rampLen); // temporary
      cur->nd=unusedRamp;     // store this with the node
      // so, passing NumberValues is awkward as we work
      // from right to left. The simplest thing is to
      // store them with the node then let the order
      // of node processing handle who-uses-what
      
    }

    // seq...
    
    else if (cur->dtype==SEQ) {                     // number driver
      printf("cmd: seq\n");

      Seq *unusedSeq=new Seq(); // temporary
      cur->nd=unusedSeq;     // store this with the node

      // seq drives number at various intervals
      // it needs at least 4 values to be meaningful:
      // 1) initial
      // 2) time interval
      // 3) 2nd value
      // 4) 2nd time interval

      node * sq=cur;

      loadSeq(sq, unusedSeq, NULL);
    }

    else if (cur->dtype==RAMPS) {                  // number driver
      printf("cmd: ramps\n");

      Ramps *unusedRamps=new Ramps(); // temporary

      // ramps is basically the same as Seq
      // it needs at least 4 values to be meaningful:
      // 1) initial
      // 2) time interval
      // 3) 2nd value
      // 4) 2nd time interval

      cur->nd=unusedRamps;     // store this with the node
      node * rmp=cur;

      loadSeq(rmp, NULL, unusedRamps);   // I think this can be reused
    }

    // randseq...
    
    else if (cur->dtype==RANDSEQ) {               // number driver
      printf("cmd: randseq\n");

      // randseq will generate a random number in a range
      // every time period

      double minV=NumberRight(cur);                 // mandetory arg
      double maxV=NumberRight(cur->rght);           // mandetory arg
      double interval=NumberRight(cur->rght->rght); // mandetory arg

      RandSeq *unusedRand=new RandSeq(minV, maxV, interval); // temporary
      cur->nd=unusedRand;     // store this with the node
    }
      
    // osc...
    
    else if (cur->dtype==OSC) {               // it's a number driver
      printf("cmd: osc\n");

      // OSC needs a min and max as parameters.
      //
      // Note that
      // while OSCs and SOUNDs share some characteristics only
      // SOUND writes into the audio file. SOUNDs never have a
      // DC component to them while OSC midpoint can be non-zero.
      //
      // OSC can (must?) consume a frequency
      // OSC can consume a wave form
      //
      // a TO node must be present, particularly as min and max value
      // can be in opposite order

      double minV=NumberRight(cur);        // mandatory arg
      node * spot=ToRight(cur->rght);              // checks 2 over
      if (cur==NULL) {
        syntaxError(spot,"Need the 'to' keyword in osc.\n");
      }
      double maxV=NumberRight(spot);  // mandatory arg

      // at minimum, we need to know the frequency. There will be
      // a default frequency but it is unlikely to be useful.

      Osc *unusedOsc=new Osc(minV, maxV, getFreqStack(),getFormStack(),getPhaseStack(),getDutyStack());
      cur->nd=unusedOsc;                           // store this with the node

      // note that each of the three overlapping settings from stack will get 'consumed'
      // leaving at least one setting left on their stack. 

    }

    //--------------------------------------------------
    // handle actions and after effects
    // these generally just take a time length
      
    else if (cur->dtype==SOUND) {              // action
      printf("cmd: sound\n");
             
      double soundLength=NumberRight(cur);
      soundLengthX=soundLength*SR;        // needed for shape and ramp

      if (soundLength==-1) {
        syntaxError(cur,"A sound must have a length.\n");
      }
      else {
        rewindHistory.push(masterTime);
        doSound(soundLength,false);
        updateDefaults=false;
        // printf("don't update defaults\n");
        masterTime+=soundLength;
      }
    }
      
    else if (cur->dtype==MIX) {              // action
      printf("cmd: mix\n");
             
      double soundLength=NumberRight(cur);
      soundLengthX=soundLength*SR;        // needed for shape and ramp
      
      if (soundLength==-1) {
        syntaxError(cur,"A sound must have a length.\n");
      }
      else {
        rewindHistory.push(masterTime);
        doSound(soundLength,true);           // write to scratch space first
        doMix(soundLength);
        updateDefaults=false;
        // printf("don't update defaults\n");
        masterTime+=soundLength;
      }
    }

    else if (cur->dtype==SILENCE) {              // action
      printf("cmd: silence\n");
             
      double silenceLength=NumberRight(cur);
      if (silenceLength<0) {
        syntaxError(cur,"Silence must have a length.\n");
      }
      else {
        rewindHistory.push(masterTime);
        doSilence(silenceLength);
        updateDefaults=false;
        printf("don't update defaults\n");
        masterTime+=silenceLength;
      }
    }

    else if (cur->dtype==BOOST) {                                       // after effect
      printf("cmd: boost\n");
             
      double boostLength=NumberRight(cur);
      soundLengthX=boostLength*SR;        // needed for shape and ramp
      rewindHistory.push(masterTime);

//      NumberDriver * boostAmount=CheckRight(cur->rght);
      NumberDriver * boostAmount=settings.vol;

      if (boostLength<0) {
        syntaxError(cur,"Boost must have a length.\n");
      }

      if (boostAmount==NULL) {
        syntaxError(cur,"Boost must have a multiplier: >1 amplify <1 quieten <0 invert\n");
      }
      else {
        doBoost(boostLength, boostAmount);
        updateDefaults=false;
        masterTime+=boostLength;
      }
    }

    else if (cur->dtype==REVERB) {                                       // after effect
      printf("cmd: reverb\n");
             
      rewindHistory.push(masterTime);
      double reverbLength=NumberRight(cur);
      //      NumberDriver * boostAmount=CheckRight(cur->rght);  // done through vol
      NumberDriver * reverbDelay=CheckRight(cur->rght);
      NumberDriver * reverbAmount=settings.vol;

      if (reverbLength<0) {
        syntaxError(cur,"Reverb must have a length (this is the source audio span).\n");
      }

      if (reverbDelay==NULL) {
        syntaxError(cur,"Reverb must have a time delay. Typically, .001s to .5s\n");
      }
      else {
        doReverb(reverbLength, reverbAmount, reverbDelay);
        updateDefaults=false;
        masterTime+=reverbLength;
      }
    }

    // --------------------------------------------------
    // everything left here is special...

    else if (cur->dtype==LOOP) {
      printf("cmd: loop\n");

      // a bit of a hack, and an excuse:
      //
      // lex seems to read in the entire file at once and then
      // parse it. As such, looking at position of yyin at any
      // point always show it at the end. An attempt was made
      // to store the file position at each opcode but this
      // turned out to be always the end of file... plus some.
      //
      // Easiest way is to rewind the file the file and scan
      // through for the loop label. It gets the job done simply
      // without exessively complicating things. Don't hate on me.
      //
      // This does have a functional implication. Using this
      // method will find the first label= string in the script.
      // If we were able to store position, that would find the
      // last label= usage in the script if there was somehow
      // more than one.
      
       char * destination=StringRight(cur);
       if (destination==NULL) {
          syntaxError(cur,"Looping to  what label?\n");
       }
       printf("cmd: loop to %s\n",destination);

       double counterValue=varValues[destination];

       counterValue=counterValue-1.0;  // decrement loop counter

       // test the loop condition >=1
       
       if (counterValue>=1.0) {

         // yes - we are doing the loop
         
         varValues[destination]=counterValue;

         // so we need to look for loop label
         
         rewind(copyyyin);  // start at beginning of input
         restart();
         lineNumber=0;      // and reset our counter too

         char buffer[4096];
         string search1=string(destination)+" =";
         string search2=string(destination)+"=";

         while (!feof(copyyyin)) {
           fgets(buffer,4093,copyyyin);
           lineNumber++;
           if ((strncmp(buffer,search1.c_str(),search1.length())==0)||
               (strncmp(buffer,search2.c_str(),search2.length())==0)) {
             printf("found! %d \n",lineNumber);

             // input file should be positioned properly to
             // continue in lex now (i hope)

             return;    // lets get out of here
           }
         }

         // what if the loop is not constructed right...
         
         printf("Error: loop counter '%s=' not found.\n",destination);
         fclose(copyyyin);
         exit(0);
       }

       // counter at zero... so do nothing continue on
    }

     // handle Output Filename because it is oddball
     
    else if (cur->dtype==OUTPUT) {                   // special: takes filename
      // printf("cmd: output\n");

      outputFile=(char *) FilenameRight(cur);
      if (outputFile==NULL) {
        syntaxError(cur,"Output filename is not specified\n");
      }
      
      // printf("cmd: output\n");
    }
    cur=cur->lft;
  }
   
}

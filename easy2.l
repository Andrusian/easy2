%option noyywrap

%{
#include "easy_code.hpp"

/* recognize the keywords */
%}

%%
"#".*                { /* DO NOTHING */  }
[\t ]+              /* ignore whitespace */

[_][\r\n]            { }
[\r\n]+              {processCommands(); }

 /* to is necessary to resolve the problem of negatives in arg lists */

to                  {push(TO,0,NULL); }
[,]                 {push(COMMA,0,NULL); }

 /* solo modifiers */

right                {push(RIGHT,0,NULL); }
left                 {push(LEFT,0,NULL); }
both                 {push(BOTH,0,NULL); }
 
 /* these modifiers can take any number driver */

phase                {push(PHASE,0,NULL); }
freq                 {push(FREQ,0,NULL); }
bal                  {push(BAL,0,NULL); }
vol                  {push(VOL,0,NULL); }

 /* overtone control */
 
freq2                {push(FREQ2,0,NULL); }
vol2                 {push(VOL2,0,NULL); }
freq3                {push(FREQ3,0,NULL); }
vol3                 {push(VOL3,0,NULL); }

 /* these keywords actually do stuff with output signal */

output               {push(OUTPUT,0,NULL); }
mix                  {push(MIX,0,NULL); }
sound                {push(SOUND,0,NULL); }
silence              {push(SILENCE,0,NULL); }

 /* after effects - things that modify an output signal */

boost                {push(BOOST,0,NULL); }
amod                 {push(BOOST,0,NULL); }  
reverb               {push(REVERB,0,NULL); }

 /* these commands modify the master time */

time                 {push(TIME,0,NULL); }
addtime              {push(ADDTIME,0,NULL); }
rewind               {push(REWIND,0,NULL); }

 /* simple settings keywords: each takes one or more constants or keywords */

automix              {push(AUTOMIX,0,NULL); }
manualmix            {push(MANUALMIX,0,NULL); }
fadein               {push(FADEIN,0,NULL); }
fadeout              {push(FADEOUT,0,NULL); }
duty                 {push(DUTY,0,NULL); }
shape                {push(SHAPE,0,NULL); }
form                 {push(FORM,0,NULL); }
circuit              {push(CIRCUIT,0,NULL); }
nocircuit            {push(NOCIRCUIT,0,NULL); }

 /* these keywords create NumberDrivers */

osc                  {push(OSC,0,NULL); }
randseq              {push(RANDSEQ,0,NULL); }
random               {push(RANDOM,0,NULL); }
ramp                 {push(RAMP,0,NULL); }
seq                  {push(SEQ,0,NULL); }
ramps                {push(RAMPS,0,NULL); }
cirp                 {push(CIRP,0,NULL); }
ciri                 {push(CIRI,0,NULL); }

 /* simple args */

sine                 {push(WF_SINE,0,NULL); }
square               {push(WF_SQUARE,0,NULL); }
saw                  {push(WF_SAW,0,NULL); }
tri                  {push(WF_TRI,0,NULL); }
tens                 {push(WF_TENS,0,NULL); }

 /* sound shapes */

tease1                 {push(SH_TEASE1,0,NULL); }
tease2                 {push(SH_TEASE2,0,NULL); }
tease3                 {push(SH_TEASE3,0,NULL); }
pulse1                 {push(SH_PULSE1,0,NULL); }
pulse2                 {push(SH_PULSE2,0,NULL); }
pulse3                 {push(SH_PULSE3,0,NULL); }
kick1                  {push(SH_KICK1,0,NULL); }
kick2                  {push(SH_KICK2,0,NULL); }
kick3                  {push(SH_KICK3,0,NULL); }
notch1                 {push(SH_NOTCH1,0,NULL); }
notch2                 {push(SH_NOTCH2,0,NULL); }
notch3                 {push(SH_NOTCH3,0,NULL); }
adsr1                  {push(SH_ADSR1,0,NULL); }
adsr2                  {push(SH_ADSR2,0,NULL); }
adsr3                  {push(SH_ADSR3,0,NULL); }
rev1                   {push(SH_REV1,0,NULL); }
rev2                   {push(SH_REV2,0,NULL); }
rev3                   {push(SH_REV3,0,NULL); }

 /* flow control */

repeat               {push(REPEAT,0,NULL); }
loop                 {push(LOOP,0,NULL); }

[\:]                 { }

"+"                  {push(PLUS,0,NULL); }
"-"                  {push(MINUS,0,NULL);}
"*"                  {push(MULT,0,NULL);}
"/"                  {push(DIV,0,NULL);}
[%][%]               {push(MODULUS,0,NULL);}

[=]                  {push(ASSIGNMENT,0,NULL); }

\"[a-zA-Z0-9_\+\-]+"."[a-zA-Z0-9_]+\" {push(FILENAME,0,(char*)doFilename(yytext)); }

[a-zA-Z][a-zA-Z0-9]* {push(STRING,0,strdup(yytext)); }  // note: must start with a letter

 /* note that negative and positive are handled above in plus/minus */

[0-9]*                  {push(NUMBER,atof(yytext),NULL); }  // without decimal...
[0-9]*[Hh][Zz]          {numberhz(yytext); }    // freq in Hertz
[0-9]*[Pp]              {numberPeriod(yytext); }    // period in sec
[0-9]*[Ss]              {numbers(yytext); }     // amount in seconds
[0-9]*%                 {numberpct(yytext); }  // percentage
[0-9]*[.][0-9]+         {push(NUMBER,atof(yytext),NULL); }  // with decimal...
[0-9]*[.][0-9]+[Hh][Zz] {numberhz(yytext); }  // freq in Hertz
[0-9]*[.][0-9]+[Ss]     {numbers(yytext); }  // amount in seconds
[0-9]*[.][0-9]+[Pp]     {numberPeriod(yytext); }  // period seconds
[0-9]*[.][0-9]+%        {numberpct(yytext); }  // percentage


.|\n                 {ECHO; }

%%
extern FILE * copyyyin;

int flag48=0;

int main(int argc, char * argv[]) {
  FILE *infile=NULL;
  int i;

  printf("EASY2: audio generator\n");

  if (argc>1) {
    for (i=1;i<argc;i++) {
      if (strcmp(argv[i],"-48")==0) {
        flag48=1;
      }
      else if (strcmp(argv[i],"-44")==0) {
        flag48=0;
      }
      else {
        // open a file handle to a particular file:

        infile = fopen(argv[i], "r");

        // make sure it's valid:

        if (!infile) {
          printf ("\nERROR: unable to open file: %s\n\n",argv[1]);
          return -1;
        }
      }
    }
  }

  if (infile==NULL) {
    printf("error: need to provide a script filename to process.\n");
    printf("\n\nusage: %s [scriptfile] [-48]\n",argv[0]);
    printf("           scriptfile is a .e2 set of commands\n");
    printf("           48 sets output to 48kHz format\n");
    exit(0);
  }


  // set lex to read from it instead of defaulting to STDIN:

  yyin = infile;
  copyyyin=yyin;
  
  init();             // setup
  yylex();            // process commands
  finish();           // write output
  exit(0);
}

void restart (void) {
  yy_flush_buffer(YY_CURRENT_BUFFER);
} 

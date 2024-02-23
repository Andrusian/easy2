
%{
#include "easy_code.h"
int endOfFile (void);

/* recognize the keywords */
%}

%%

"#".*                { /* DO NOTHING */; }
[\t ]+              /* ignore whitespace */

\n                  {lineNumber++;processCommands(); }

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
include              {push(INCLUDE,0,NULL); }
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
noise                {push(WF_NOISE,0,NULL); }


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
wedge1                 {push(SH_WEDGE1,0,NULL); }
wedge2                 {push(SH_WEDGE2,0,NULL); }
gap1                   {push(SH_GAP1,0,NULL); }
gap2                   {push(SH_GAP2,0,NULL); }

 /* flow control */

repeat                          {push(REPEAT,0,NULL); }
loop[ ]+[a-zA-Z][a-zA-Z0-9_]*   {push(LOOP,0,loopvar(strdup(yytext))); }
exit                            {push(EXIT,0,NULL); }

 /* time stamps */

[0-9]+[\:][0-9][0-9]\:[0-9][0-9]  {push(TIMESTAMP,0,yytext); }   // h:mm:ss
[0-9]+\:[0-9][0-9]                {push(TIMESTAMP,0,yytext); }   // m:ss
:[0-9]+                           {push(TIMESTAMP,0,(yytext)); }   // :ss

 /* ignore colon otherwise */

[\:]                 { }

 /* math operators */

"+"                  {push(PLUS,0,NULL); }
"-"                  {push(MINUS,0,NULL);}
"*"                  {push(MULT,0,NULL);}
"/"                  {push(DIV,0,NULL);}
[%][%]               {push(MODULUS,0,NULL);}


[=]                  {push(ASSIGNMENT,0,NULL); } 

\"[a-zA-Z0-9_\+\-]+"."[a-zA-Z0-9_]+\" {push(FILENAME,0,(char *) doFilename(yytext)); }

[a-zA-Z_][a-zA-Z0-9_]* {push(STRING,NO_NUMBER,strdup(yytext)); } 

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

<<EOF>>                 {endOfFile();}

%%
extern FILE * copyyyin;
extern char * copyinfile;

int flag48=1;

int main(int argc, char * argv[]) {
  FILE *infile=NULL;
  int i;

  printf("%sEASY2: audio generator\n%s",CYN,WHT);

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
          printf ("\n%sERROR: unable to open file: %s\n\n%s",RED,argv[1],WHT);
          return -1;
        }
        copyinfile=malloc(strlen((char *) argv[i])+3);  /* extra character */
        strcpy((char *)copyinfile,(char *) argv[i]);
        copyinfile[strlen((char *) argv[i])]='\0';
        printf("Processing input file: %s%s%s\n",CYN,argv[i],WHT);
      }
    }
  }

  if (infile==NULL) {
    printf("%serror: need to provide a script filename to process.\n",RED);
    printf("\n\nusage: %s [scriptfile] [-48]\n",argv[0]);
    printf("           scriptfile is a .e2 set of commands\n");
    printf("           48 sets output to 48kHz format\n%s",WHT);
    exit(0);
  }

  // set lex to read from it instead of defaulting to STDIN:

  yyin = infile;
  copyyyin=yyin;
  
  init();             // setup
  yypush_buffer_state(yy_create_buffer(yyin,YY_BUF_SIZE));
  
  yylex();            // process commands

  yypop_buffer_state();
  finish();           // write output
  exit(0);
}

void restart (void) {
  yyrestart(yyin);
  yy_flush_buffer(YY_CURRENT_BUFFER);
} 

void pushIncludeFile (FILE *f) {
  yypush_buffer_state(yy_create_buffer(f,YY_BUF_SIZE));
}

int endOfFile (void) {
  yypop_buffer_state();
  checkEndOfInclude();
  if (!YY_CURRENT_BUFFER) {
    printf("%sEnd of main input file\n%s",CYN,WHT);
    finish();
    exit(0);
  }
    
  return 1;
}

int yywrap() {   /* maybe unneeded I think */
  return 1;
}

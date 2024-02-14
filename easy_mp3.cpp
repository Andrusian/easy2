//----------------------------------------------------------------------
// convert the provided output file to MP3
//
// Confession: the first attempt at this I asked ChatGPT to write.
// It got most of the way there but the provided no hint of what
// parts of the sprawling ffmpeg package it used. And the code it
// wrote seemed to be out of date and used deprecated functions.
//
// So, fuck it. I'm just calling the external utility.
//
// This looks like:
//
// /usr/bin/lame --preset standard infile outfile 
//
// As a bonus, we can tag the file...
//
// /usr/bin/id3v2 --song "songname" --artist "artist" --comment "comment" outfile
//
// And maybe this is the better approach.
//
// assumes Linux (Ubuntu) but should be easily adaptable to Windows
// assumes lame and id3v2 are installed but should fail in a non-fatal way
extern "C" {

  #include <unistd.h>
  #include <sys/types.h>
  #include <wait.h>
  #include <string.h>
  #include <stdio.h>
  #include <stdlib.h>
  extern int flag48;
  #include "easy_code.h"
}

const char * lame="/usr/bin/lame";
const char * ffmpeg="/usr/bin/ffmpeg";
const char * id3v2="/usr/bin/id3v2";

//----------------------------------------------------------------------

void doMp3(const char * infile, const char * songname) {
  char * outfile;
  char * outfile2;
  int pid;
  int dc;
  
  // swap the ending on the file

  outfile=strdup(infile);
  outfile2=strdup(infile);
  
  char * location=strcasestr(outfile,".wav");
  if (location==NULL) {
    printf("Sorry - wav filename problem. Could not convert.\n");
    return;
  }
  location++;  *location='m';
  location++;  *location='p';
  location++;  *location='3';
  location++;  *location='\0';

  location=strcasestr(outfile2,".wav");
  if (location==NULL) {
    printf("Sorry - wav filename problem. Could not convert.\n");
    return;
  }
  location++;  *location='o';
  location++;  *location='g';
  location++;  *location='g';
  location++;  *location='\0';


  /* make the mp3 */
  
  pid=fork();
  if (pid ==0) {
    printf(CYN);
    // printf(BLINK);
    if (flag48) {
      printf("Converting %s to %s\n",infile,outfile2);
      execl(ffmpeg,ffmpeg,"-hide_banner","-loglevel","error","-y","-i",infile,"-q:a","9",outfile2,NULL);
    }
    else {
      /* mp3 supports 44.1kHz only */
      
      printf("Converting %s to %s\n",infile,outfile);
      execl(ffmpeg,ffmpeg,"-hide_banner","-loglevel","error","-y","-i",infile,"-codec:a","libmp3lame","-q:a","1",outfile,NULL);
      
    }
    printf(WHT);
    printf(RESET);

    exit(0);
    
  }
  waitpid(pid,&dc,0);

  printf("%sComplete\n%s",BLU,WHT);

  return;
}


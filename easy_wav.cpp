//----------------------------------------------------------------------
// easy_wav.cpp
//
// This file (and the corresponding header) deals with the creation
// of a .wav file. Why a .wav file? The reason is that wav files
// are reasonably well documented and possible to create without
// a specialized library. This is not to say that the wav format
// is simple: it has a lot of complexities added over time.
//
// This is not true of other formats which are proprietary and possibly
// require licensing. Try to work out how to write MPEG4 and you'll
// quickly see what I mean.
//
// It's important to get the sample rate correct for your intended
// use. 44.1k is common as it is the Redbook CD standard. 48k is
// needed for ultimate MPEG4 encoding. Conversion between 44.1k
// and 48k is lossy and typically damages phase information if using
// anything based on FFMPEG (like Audacity).
//
// In the earlier versions of this file, an attempt was made to
// write the 24-bit PCM / 48k format. This didn't work, nor did 32-bit
// on first attempt. What did work was simply changing the sample rate to 48k.
// There may be some code relics remaining from that.
//

#include <fstream>
#include <iostream>

extern "C" {
#include "stdint.h"
#include "math.h"
};

#include "easy_wav.hpp"

WaveWriter::WaveWriter(int32_t size, uint32_t sampleRate) {
    static_assert(sizeof(wav) == 44, "");
    this->channels=2;
    this->sr=sampleRate;
    this->size=size;
    
    if (sampleRate==44100) {   // 16 bit 44.1k
      wav.SamplesPerSec=sampleRate;
      wav.bytesPerSec=wav.SamplesPerSec*2*16/8;
      wav.bitsPerSample=16;
      data16L=(int16_t *)calloc (size,sizeof(int16_t));
      data16R=(int16_t *)calloc (size,sizeof(int16_t));
      scratch16L=(int16_t *)calloc (size,sizeof(int16_t));
      scratch16R=(int16_t *)calloc (size,sizeof(int16_t));
      MAXVAL=pow(2,15)-1;
    }
    else if (sampleRate==48000) { 
      wav.SamplesPerSec=sampleRate;
      wav.bytesPerSec=wav.SamplesPerSec*2*16/8;
      wav.bitsPerSample=16;
      data16L=(int16_t *)calloc (size,sizeof(int16_t));
      data16R=(int16_t *)calloc (size,sizeof(int16_t));
      scratch16L=(int16_t *)calloc (size,sizeof(int16_t));
      scratch16R=(int16_t *)calloc (size,sizeof(int16_t));

      MAXVAL=pow(2,15)-1;  
    }
    else {
      std::cout << "invalid sample rate / wave format \n";
    }
    
    wav.ChunkSize=size+sizeof(wav)-8;

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// write the output file
//

int WaveWriter::writeFile (char * filename) {
   std::ofstream out(filename, std::ios::binary);
    
    // wav.ChunkSize=size+sizeof(wav)-8;
    // wav.Subchunk2Size=size+sizeof(wav)-44;
    // wav.ChunkSize=2*maxPos+sizeof(wav)-8;
    // wav.Subchunk2Size=maxPos+sizeof(wav)-44;

    // There is a 4* multiplier here to fix a bug. This was arrived at
    // experimentally to reach the correct audio length.
    //
    // It is very likely the size of two 16-bit words but was not
    // documented in the original examples I worked from. It will probably
    // have to be sorted out for the HI-bit/48k audio.
    
    wav.ChunkSize=4*maxPos+sizeof(wav)-8;
    wav.Subchunk2Size=4*maxPos+sizeof(wav)-44;   

    out.write(reinterpret_cast<const char *>(&wav), sizeof(wav));

    printf("writing: %6.2fMB memory, %d samples, %d seconds or %f minutes\n",(maxPos*4/1024./1024.),maxPos,maxPos/sr,maxPos/sr/60.);

    for (uint32_t i = 0; i < maxPos; ++i) {
      out.write((const char *) &(data16L[i]), 2);
      out.write((const char *) &(data16R[i]), 2);
    }
    out.close();
    return 0;
}

//----------------------------------------------------------------------
// convert a time to a a position index the output
  
int32_t WaveWriter::findPosition(double targetTime) {
    int32_t position=floor(targetTime*sr);
    return position;
  }

//----------------------------------------------------------------------
// convert a position to a time
  
double WaveWriter::findTime(uint32_t position) {
    return floor(position/sr);
}

//----------------------------------------------------------------------
// if we run out of buffer, double the size

void WaveWriter::checkSize(uint32_t pos) {
  if (pos>size) {
    size=size*2;
    printf("WAVWRITER: reallocating buffer to %ld samples\n",size);
    data16L=(int16_t *)realloc ((void *) data16L,size*sizeof(int16_t));
    data16R=(int16_t *)realloc ((void *) data16R,size*sizeof(int16_t));
    scratch16L=(int16_t *)realloc ((void *) scratch16L,size*sizeof(int16_t));
    scratch16R=(int16_t *)realloc ((void *) scratch16R,size*sizeof(int16_t));
  }

  // keep tabs on last position written
  
  if (pos>maxPos) {    
    maxPos=pos;
  }
}

//----------------------------------------------------------------------

void WaveWriter::setValueL(uint32_t pos,int32_t value, bool scratch) {
  checkSize(pos);
  
  if (scratch) {
    scratch16L[pos]=value;

    if (pos>scratchPos) {
      scratchPos=pos;
    }
  }
  else {
    if (wav.bitsPerSample==16) {
      data16L[pos]=value;
    }
    if (pos>maxPos) {
      maxPos=pos;
    }
  }
}
  
void WaveWriter::setValueR(uint32_t pos,int32_t value,bool scratch) {
  if (scratch) {
    scratch16R[pos]=value;
    if (pos>scratchPos) {
      scratchPos=pos;
    }
  }
  else {
    data16R[pos]=value;
    if (pos>maxPos) {
      maxPos=pos;
    }
  }
}

//----------------------------------------------------------------------

int32_t WaveWriter::getValueL(uint32_t pos, bool scratch) {
  int16_t value;
  
  if (scratch) {
    value=scratch16L[pos];
    return value;
  }
  else {
    value=data16L[pos];
    return value;
  }
}
  
int32_t WaveWriter::getValueR(uint32_t pos,bool scratch) {
  int16_t value;
  
  if (scratch) {
    value=scratch16R[pos];
    return value;
  }
  else {
    value=data16R[pos];
    return value;
  }
}

//----------------------------------------------------------------------
// I don't expect the destructor function to ever be called.

WaveWriter::~WaveWriter() {
      free(data16L);
      free(data16R);
      free(scratch16L);
      free(scratch16R);
}

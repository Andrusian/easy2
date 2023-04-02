//----------------------------------------------------------------------
// Audio ouput WAV file format.
//
// We're going to support two audio output formats. One is 44.1k
// and the other is 48k. The first is widely used in CD "redbook" format,
// easily converted to MPEG1 Layer 3 (MP3) and in various video formats
// like .AVI files. Encoding in this format is easily achieved.
//
// The second audio format is 48k and is used in MPEG4 standard. Encoding
// in this standard is harder to come by as is still considered
// proprietary. Note that converting back and forth between the two
// can result in a loss of audio quality. In particular, depending on the
// converter used, phasors can be lost in the process.
//
// As such, it is important to generate the file in the format you
// need.
//
// This code is based on, and thanks to:
// author: fangjun kuang <csukuangfj at gmail dot com>
// date: Apr. 22, 2019

// refer to http://www.topherlee.com/software/pcm-tut-wavformat.html

#include <fstream>
#include <iostream>
#include <cstdint>
#include <cmath>

struct WAV_HEADER {
  /* RIFF Chunk Descriptor */
  uint8_t RIFF[4] = {'R', 'I', 'F', 'F'}; // RIFF Header Magic header
  uint32_t ChunkSize;                     // RIFF Chunk Size
  uint8_t WAVE[4] = {'W', 'A', 'V', 'E'}; // WAVE Header
  /* "fmt" sub-chunk */
  uint8_t fmt[4] = {'f', 'm', 't', ' '};  // FMT header
  uint32_t Subchunk1Size = 16;            // Size of the fmt chunk
  uint16_t AudioFormat = 1;               // Audio format 1=PCM,6=mulaw,7=alaw,     257=IBM
                                          // Mu-Law, 258=IBM A-Law, 259=ADPCM
  uint16_t NumOfChan = 2;                 // Number of channels 1=Mono 2=Stereo
  uint32_t SamplesPerSec;                 // Sampling Frequency in Hz
  uint32_t bytesPerSec;                   // bytes per second
  uint16_t blockAlign = 2;                // 2=16-bit mono, 4=16-bit stereo
  uint16_t bitsPerSample = 16;            // Number of bits per sample
  
              /* "data" sub-chunk */
  
  uint8_t Subchunk2ID[4] = {'d', 'a', 't', 'a'}; // "data"  string
  uint32_t Subchunk2Size;                        // Sampled data length
};

class WaveWriter {

public:

  WAV_HEADER wav;         // the wav header template
  uint16_t  channels;     // always 2 channels for now
  uint32_t  sr;           // sample rate, either 44100 or 48000
  uint16_t bitsPerSample; // either 16 or nothing: 24 bit stuff deleted

  long     size;          // allocated buffer size
  uint32_t MAXVAL;        // maximum sample value for resolution
  uint32_t maxPos;        // maximum sample location used
  uint32_t scratchPos;    // maximum scratch location used 

  // two output data buffers (L/R) for each supported format
  // 24 bit deleted: it's all 16 bit here
  
  int16_t   *data16L;
  int16_t   *data16R;

  // scratch buffers are used for mixing calculations
  
  int16_t   *scratch16L;
  int16_t   *scratch16R;
  
  WaveWriter(int32_t size, uint32_t sampleRate);
  ~WaveWriter();

  int32_t findPosition(double targetTime);
  double findTime(int32_t position);
  void setValueL(int32_t pos,int32_t value, bool scratch);
  void setValueR(int32_t pos,int32_t value, bool scratch);
  int32_t getValueL(int32_t pos, bool scratch);
  int32_t getValueR(int32_t pos, bool scratch);
  int writeFile (char * filename);
  void checkSize(int32_t pos);

  void DEBUG (uint32_t startX,uint32_t endX) {
    uint32_t i;
    
    for (i=startX; i<endX; i++) {
      std::cout << "index " << i << " L " << data16L[i] << " R " << data16R[i] << "\n";
    }
  }
  
};


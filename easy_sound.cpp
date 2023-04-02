//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// sound
//
// This source file handles any sort of writing of waveforms and
// after-effects into the output data.
//
// Easy v1 (in Python) is the basis for some of this, but that crap
// was complicated. A particular problem for this code is
// nomenclature; we're talking about numbers about numbers about
// numbers and we quickly run out works to describe the concepts.
//
// Inherently, this is the slowest part of the program to execute
// so a careful balance of features vs. usefulness should be
// considered here. At time of writing this comment, performance appears
// to be very good and should be even better with optimization enabled.
//
// A particular warning on debugging & errors: C/C++ behaviour around integer
// types often falls into the 'undefined' area. The onus is on the coder
// to use them properly. In particular, it is easy to make mistakes
// on int/uint and int divison.
//
//----------------------------------------------------------------------
//
//

#include "easy_code.hpp"
#include <cmath>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "easy.hpp"
#include "easy_wav.hpp"
#include "easy_node.hpp"

extern settings_struct_stacked settings;
extern WaveWriter * wavout;
extern long defaultFormat;
extern const char * defaultFileout;

extern uint32_t SR;

uint32_t periodX;
uint32_t dutyX;
double sawslope;
double trislope;
uint32_t waitX;
double freq;
uint32_t tensX;
double dutyThresh;

//----------------------------------------------------------------------
// This is a simple circuit model to mimic the effects of
// capacitance and inductance in the oscillator behaviour.
// 
// For an idealized signal, turn this off. (default)
//
// For more complex waveforms, with additional harmonics,
// provide a proportional and integral term. Cautious values
// will be provided as defaults in cirp and ciri settings.

class Circuit {

public:

  double propFactor;
  double intFactor;

  int32_t delta=0;    
  int32_t Factor=0;
  int32_t integral;

  int32_t last=0;
  int32_t lastDemanded=0;
  int32_t lag;
  long output;

  Circuit (void) {
    integral=0;
    last=0;
  }

  //----------------------------------------------------------------------
  // input is the current demanded value
  // output is the deviation due to simulated circuit imperfection
  // caused 

  int32_t effect(uint32_t x,int32_t demanded) {

    propFactor=settings.cirp->getValue(x);
    intFactor=settings.ciri->getValue(x);
    // printf("circuit: %d propFactor %f %f\n",x,propFactor,intFactor);

    delta=demanded-last;

    integral+=delta;

    int32_t term1=delta*propFactor;
    int32_t term2=intFactor*integral;

    
    output=term1+term2+last;

    // clipping limits are important for a few reasons
    // caution here: C/C++ compiler is blind to sign/non-signed integer
    // comparisons so this has to be coded oddly
    
    if (output>wavout->MAXVAL) {
      output=wavout->MAXVAL;
    }
    if (output<-float(wavout->MAXVAL)) {
      output=-int(float(wavout->MAXVAL));
    }
    
    last=output;
    lastDemanded=demanded;

    // printf("circuit: last %d demanded %5d change %5d  %5d+%5d+%5d=%5d\n",last,demanded,delta,term1,term2,last,output);
    
    return output;
  }

  void zero (void) {
    integral=0;
    last=0;
    lastDemanded=0;
  }

};

//----------------------------------------------------------------------
// updatePeriods recalcuates certain parameters that
// are used in the waveform generation based on frequency.
// Collected in one place for simplicity in mainline code.
//
  
void updatePeriods(void) {
  periodX=int(SR/freq);
  dutyX=int(SR/freq)*dutyThresh; 
  sawslope=2./dutyX;
  trislope=4./dutyX;
  waitX=(periodX-dutyX)/2;
}

// this need an explanation: ramp gets processed before the sound
// length is known. But we don't want to make a special interface
// for it, so we'll store the current length in a global. That
// way, IF a ramp is used, and IF the ramp adopts the parent
// sound length, it will be able to access it through
// the global.
//
// Note that this (at present) apply to after-effects.

extern uint32_t soundLengthX;

//----------------------------------------------------------------------
// Does fadein-effect
//
// This is enabled by default as it is desirable for most signals.

double calcFadein (double x) {
  // initialize
  static long fadecounter;
  static long fadeends;
  double fade1;

  if (x==0) {  // trigger at the start of each new sound
    fadecounter=0;
    fadeends=settings.fadein*SR;  // convert sec to frames
  }

  if (fadecounter<fadeends) {

    fade1=float(fadecounter)/float(fadeends);

    fadecounter++;
  }
  else {
    fade1=1.;
  }

  return fade1;
}

//--------------------------------------------------
// Does fadeout effect.
//
// Not enabled by default. It can smooth any signal
// discontinuities or finish off the end of
// a signal in a more natural way.

double calcFadeout (uint32_t x, uint32_t startX, uint32_t endX) {
  static long fadeout_starts=1;
  static long spanFade=1;
  
  double fade2;
  
  if (x==0) {  // trigger at the start of each new sound
    spanFade=settings.fadeout*SR;
    fadeout_starts=endX-startX-spanFade;
  }

  if (x>fadeout_starts) {
    fade2=1-(x-fadeout_starts)/float(spanFade);
  }
  else {
    fade2=1.;
  }

  return fade2;
}

//----------------------------------------
// And the vision that was planted in my brain
// still remains, within the sound of silence.

void doSilence (double length) {
  uint32_t startX=wavout->findPosition(masterTime);
  double endTime=masterTime+length;
  uint32_t endX=wavout->findPosition(endTime);
  uint32_t deltaX=endX-startX;

  if (wavout==NULL) {
      wavout=new WaveWriter(defaultFormat*60*10,defaultFormat);
  }

  std::cout << "Silence from " << masterTime << " to " << endTime << "\n";

  // a silence is just like a sound but with more zeros

  for (uint32_t x=0;x<deltaX;x++) {
    if (settings.left) {
      wavout->setValueL(startX+x,0,false);
    }
    if (settings.right) {
      wavout->setValueR(startX+x,0,false);
    }
  }
  
  if (endX>wavout->maxPos) {
    wavout->maxPos=endX;
  }
}

//----------------------------------------------------------------------
// Boost increases or decreases the volume for the time range.
// It is the simplest of the after effects.
//
// Boost allows clipping.
// It also allows inversion of signal... hmmmm. Might be useful.
//
void doBoost (double length, NumberDriver *nd) {
  uint32_t startX=wavout->findPosition(masterTime);
  double endTime=masterTime+length;
  uint32_t endX=wavout->findPosition(endTime);
  int32_t newval;
  
  if (endTime>wavout->maxPos) {  // limit our action to the current output range
    endX=wavout->maxPos;
  }

  nd->init(0);
  
  // left / right settings apply, as usual
  
  uint32_t countX=0;
  for (uint32_t x=startX;x<endX;x++) {
    double multiplier=nd->getValue(countX);           // multiplier can vary!
      
    if (settings.left) {
      // printf("val: %d\n",wavout->getValueL(x,false));
      newval=wavout->getValueL(x,false)*multiplier;  // get numbber and make it bigger
      // printf("newval: %d\n",newval);
      wavout->setValueL(x,newval,false);             // write it back same spot
    }
    if (settings.right) {
      newval=wavout->getValueR(x,false)*multiplier;
      wavout->setValueR(x,newval,false);
    }
    countX++;
  }
}


//----------------------------------------------------------------------
// Reverb does a reverb effect, or with a large delay, an echo.
//
// Both the amount and the delay are NumberDrivers so they
// can vary. The amount of reverb is controlled by the vol command.
//
// Reverb/echo is strongly influenced by volume and you can very
// easily saturate the signal. There is also reverb-on-the-reverb
// as it is done forwards through time.
//
void doReverb (double length, NumberDriver *amt, NumberDriver *del) {
  uint32_t startX=wavout->findPosition(masterTime);
  double endTime=masterTime+length;
  uint32_t endX=wavout->findPosition(endTime);
  int32_t val;
  
  if (endTime>wavout->maxPos) {  // limit our action to the current output range
    endX=wavout->maxPos;
  }

  // left / right settings apply, as usual
  
  uint32_t countX=0;
  for (uint32_t x=startX;x<endX;x++) {
    double amount=amt->getValue(countX);         // amount can vary and is vol parameter
    double delayS=del->getValue(countX);          // delay can vary and is seconds
    uint32_t delayX=delayS*SR;
    uint32_t laterX;
    long oldval;
    long newval;
    
    if (settings.left) {
      // printf("reverb. amt: %f del: %f \n",amount,delayS);
      val=wavout->getValueL(x,false)*amount;  // get current value
      laterX=x+delayX;
      oldval=wavout->getValueL(laterX,false);  // get old value at target
      newval=oldval+val;                              // add echo
      wavout->setValueL(laterX,newval,false);          // echo it later
    }
    if (settings.right) {
      // printf("val: %d\n",wavout->getValueR(x,false));
      val=int(wavout->getValueR(x,false)*amount);  // get current value
      // printf("newval: %d\n",newval);
      laterX=x+delayX;
      oldval=wavout->getValueR(laterX,false);  // get old value at target
      newval=oldval+val;                              // add echo
      wavout->setValueR(laterX,newval,false);          // echo it later
    }

    countX++;
  }
}

//======================================================================
// doSound
//
// Most of the sound generation is done here including
// all the waveform types. From experience, I know that this code
// can get VERY messy quickly.
//
// Some tips:
// - variables ending in X pertain to sample locations/times in integer format
// - be mindful of whether a sample value is in double (-1 to 1) or integer
//   format (e.g. -32768 to -32767).
// - coding style should be expansive (step by step) rather than condensed
//   to facilitate debugging. (Let compiler optimizations handle speed.)


void doSound (double length, bool scratch) {
  double endTime=masterTime+length;
  long startX;
  long endX;
  uint32_t mult=wavout->MAXVAL;
  Circuit circuit;
  double sineval2;
  double sineval3;

  // std::cout << "Sound from " << masterTime << " to " << endTime << "\n";

  // what if no output command was ever given?
  // initialize output buffers to default

  if (wavout==NULL) {
      wavout=new WaveWriter(defaultFormat*60*10,defaultFormat);
  }
  
  // std::cout << "Play a sound from " << masterTime << " to " << endTime << "\n";

  startX=wavout->findPosition(masterTime);
  endX=wavout->findPosition(endTime);
  soundLengthX=length*SR;     // needed for shape and ramp which can repeat

  // initialize shape if it is being used and not already set

  settings.shape->init(soundLengthX);
  
  // setup work...
  // NumberDrivers to strange stuff when you don't init them
  
  NumberDriver * freqFD=settings.freqStack.top();
  NumberDriver * phaseFD=settings.phaseStack.top();
  int form=settings.formStack.top();
  
  freqFD->init(0);                  
  phaseFD->init(0);                  
  settings.freq2->init(0);                  
  settings.freq3->init(0);                  
  settings.bal->init(0);
  settings.vol->init(0);
  settings.vol2->init(0);
  settings.vol3->init(0);
    
  freq=freqFD->getValue(0);        // get frequency setting
  dutyThresh=settings.duty->getValue(0);  // get duty setting
  uint32_t freqRefX=0;                    // starting point for calc freq
  circuit.zero();                        // clear any history in Circuit model

  // generation work...

  uint32_t deltaX=endX-startX;
  double lastval=0.0;
  double lastfreq=freq;
  int tridirection=1;

  updatePeriods();  
  
  // this counts the step in a cycle
  // we'll try and reuse it for various
  // types of waveforms.
  
  uint32_t countX=0;   
  uint32_t countXadj=0;  // adjusted for phase

  for (uint32_t x=0;x<deltaX;x++) {
    countX++;
    double sawval;

    // current phase percent and frames...
    // e.g. -.3*44100/1000 = -13 frames ... range would be -22 to +22
    //
    // easyV1 had some hellish code to do phasors at various speeds
    // and to limit adjustment speed. We're relying on phasors being
    // driven by oscillators to hopefully make this unnecessary.
    
    double phase=phaseFD->getValue(x);
    int32_t phaseX=phase*SR/freq;

    countXadj=countX+phaseX;  // adjust our counter for phase
      
    if (countXadj>periodX) {
      countXadj=0;
      countX=-phaseX;
    }

    // update settings driven by NumberDrivers and other settings

    double fadein=calcFadein(x);
    double fadeout=calcFadeout(x,startX,endX);

    // effective volume...

    double shapevol=settings.shape->getValue(x);
    double vol=abs(settings.vol->getValue(x));
    double volnet=fadein*fadeout*shapevol;
    double vol2=abs(settings.vol2->getValue(x));
    double vol3=abs(settings.vol3->getValue(x));
    double freq2=abs(settings.freq2->getValue(x));
    double freq3=abs(settings.freq3->getValue(x));
    
    // balance volume modifier... this is independent
    // of right/left enable. e.g. bal=1 with both enabled means signal entirely
    // left and right will be zeroed. bal=0 means signal goes equally to both
    // channels. This behavior is different from easy V1.
    //
    // Something to consider is this is non-linear but matches what a real
    // balance potentiometer works.
    
    double bal=settings.bal->getValue(x);
    double balR, balL;
    if (bal>=0) {
      balL=1.;
      balR=1.-bal;
    }
    if (bal<0) {
      balR=1.;
      balL=(1.+bal);
    }
    if (balL>1) balL=1.;
    if (balR>1) balR=1.;
    
    // update duty...

    dutyThresh=settings.duty->getValue(x);
    
    // printf("phase: %d %f %d\n",x,phase,phaseX);
    
    // generate current value: note use of potentially
    // shifted start pont for freq/sine calculation

    //  SS   I   N   N  EEE                                       
    // S     I   N N N  E     is done here 
    //  S    I   N  NN  EE                                      
    //   S   I   N   N  E                                      
    // S S   I   N   N  EEE                                       

    double radians=2*M_PI*freq*float((x-freqRefX)+phaseX)/SR;
    double sineval=sin(radians);

    //
    // OTHER WAVEFORM TYPES...
    //
    
    if (form==WF_SQUARE) {               // SQUARE
      if ((countXadj)>dutyX) {  
        sineval=0;
      }
      else if ((countXadj)>(dutyX/2)) {  
        sineval=1;
      }
      else {
        sineval=-1;
      }
    }
    
    else if (form==WF_TRI) {            // TRI

      // note: duty cycle gets strange on tri above values
      // around 66%, but I'll leave it be
      
      if (countXadj==0)  {
        tridirection=1;
      }  
      else if ((countXadj)==dutyX/4)  {
        tridirection=-1;
        sawval=1.0;
      }
      else if ((countXadj)==dutyX*3/4)  {
        tridirection=1;
        sawval=-1.0;
      }
      else if ((countXadj)==dutyX) {
        sawval=0.0;
        tridirection=1;
      }
      else if ((countXadj)>dutyX) {
        sawval=0.0;
        tridirection=0;
      }

      if (tridirection>0) {
        sawval+=trislope;
      }
      else if (tridirection<0) {
        sawval-=trislope;
      }
      // printf("tri: x %d direction %d trislope %f sawval %f \n", x, tridirection, trislope, sawval);
      sineval=sawval;
    }
    
    else if (form==WF_SAW) {            // SAW

      // note: duty cycle gets strange on saw above values
      // around 80%, but I'll leave it be
      
      if ((countXadj)>dutyX)  {
        sawval=0;
      }
      else if ((countXadj)==dutyX/2)  {
        sawval=-1.0;
      }  
      else {  
        sawval+=sawslope;
      }
      
      if (sawval>1.0) {
        sawval=-1.;
      }
      sineval=sawval;
      // std::cout << "saw: " << " " << sawval << " " << sawslope << " \n";
    }

    else if (form==WF_TENS) {            // TENS

      // TENS has constant volume
      // but instead makes wider pulses as volume
      // increases. Actual volume does not change.
      //
      // duty has no effect.
      // Phase has no effect (I seem to remember Easy V1 can do a sort of triphase TENS)
      //
      // Higher frequencies will deliver
      // more power as the ratio of 0 time
      // will be lower.
      //
      // rough calc:
      // 500Hz is 2000us period
      // aim for maybe 200us pulsewidth at 100% vol?
      // so conversion factor around 1% = 1usec?

      // printf("tens: x %d width %d\n", x, tensX);

      tensX=int(.2*volnet*vol*SR/freq);   

      if (countXadj<tensX) {
        sineval=.95;
      }
      else if (countXadj<(tensX*2)) {
        sineval=-.95;    // needs both polarities
      }
      else {
        sineval=0;
      }
      tensX++;
    }
    
    // update frequency... (for next iteration)
    //
    // my experience with easy version 1 was that changing
    // frequency mid-sound is very difficult. you have to
    // wait for a negative zero crossing and only update
    // frequency then.
    //
    // what you can't do is recalcuate the frequency
    // based on the same (reference) starting point as the original
    // sine wave. Doing so would shift the entire wave thus far... but
    // we've already written part of it... so the effect is bizzare
    // and unexpected slews of frequency if you do it that way.
    // To picture this: imagine a streched slinky passing
    // over a fixed point along it's length. As each coil passes,
    // you get a very different value. Instead, you need
    // to move the starting point of the slinky plus the tension on it.
    //
    // Another way to do this would to only calculate the current sine-wave cycle,
    // resetting the reference every time. But this is messy too.

    if ((lastval<=0.0) && (sineval>0.0)) {      // detect end of sign wave cycle?
      
      lastfreq=freq;
      freq=freqFD->getValue(x);  // use unmodified x reference

      updatePeriods();

      if (freq!=lastfreq) {                       // did frequency change?
        // printf("new freq at %d %f -> %f \n",x, lastfreq,freq);
        freqRefX=x-1;                             // new freq reference point for sine()
        // note: this will cause a tiny audio artifact
        // and if the frequency is ramped, this is going to be noticable
      }
    }
    
    lastval=sineval;

    if (form==WF_TENS) {            // for TENS signal
      if (settings.left) {
        wavout->setValueL(x+startX,sineval*mult,scratch);
      }
      if (settings.right) {
        wavout->setValueR(x+startX,sineval*mult,scratch);
      }
    }
    else {                                  // for other waveforms

      // harmonics are done here...

      if (vol2>0.) {
        sineval2=sin(2*M_PI*freq2*float(x)/SR);
      }
      else {
        sineval2=0.;
      }

      if (vol3>0.) {
        sineval3=sin(2*M_PI*freq3*float(x)/SR);
      }
      else {
        sineval3=0.;
      }

      int32_t waveval=volnet*mult*(vol*sineval+vol2*sineval2+vol3*sineval3);
      int32_t outval=waveval;  // applies if Circuit effect not enabled

      // circuit mimics the overshoot/undershoot in real circuitry
      // at full volume, this isn't useful but it creates more
      // complex and realistic waveforms at unsaturated volumes.
      // Note that the circle class
      // has an internal integration which is cleared at the start
      // of each sound.
      //
      // circuit has little effect on sine waveforms until
      // a critical value his hit.

      if (settings.circuit) {
        outval=circuit.effect(x,waveval);
      }

      // write the current sample to output!
      // 
      //  OOO   U   U   TTTT                
      // O   O  U   U     T                 
      // O   O  U   U     T                 
      //  OOO    UUU      T                  
    
      if (settings.left) {
        int32_t outvalL=outval*balL;     // applies if Circuit effect not enabled
        wavout->setValueL(x+startX,outvalL,scratch);
      }
      if (settings.right) {
        int32_t outvalR=outval*balR;     // applies if Circuit effect not enabled
        wavout->setValueR(x+startX,outvalR,scratch);
      }
    
      // std::cout << "x:" << x << " value: " << waveval << " f1:" << fadein << " f2:" << fadeout << " vol:" << settings.vol->getValue(x) << "\n";
    }

  }

  if (endX>wavout->maxPos) {
    // printf("maxX: %d\n",wavout->maxPos);
    wavout->maxPos=endX;
  }

  // take a look at our output for DEBUG
  
  // wavout->DEBUG(0,100);

}

//----------------------------------------------------------------------
// doMix
//
// Mix requires several stages. First, the sound is written to
// the scratch space. Next, the audio levels of both the new sound
// and the existing sound are assessed.
//
// The volume of each sound is adjusted according to the automix
// maximum and the proportion of the signals.
//
// Automix pre-scans the output signal to prevent the output
// signal to exceed some value. For example:
// - if automix is set to 80%, and the existing signal has max
//   value at 30%, and volume is set to 50%, no adjustment
//   is required.
//
// - if automix is set to 80%, and the existing signal has max
//   value at 75%, and new volume is set to 90%:
//    o effective volume is 43.6% (80*90/(75+90)
//
//    o existing signal is reduced to 36% max first
//    by multiplying by 45%  75%/(75%+90%)
//
// - if automix is set to 90% and the two signals are
//    30% and 50% maximum, then no adjustment is
//    required: the signals can be simply added.
//
//  TODO: it's possible that automix could be fed
//    from a NumberDriver. This might make some
//    interesting effects. It probably would not
//    work on a sample-by-sample basis tho.
//
//
//   Let's write this equation as follows:
//
//            newvol=AM*vol/(vol+exist)
//
//            reduced portion=AM*exist/(vol+exist)
//
//            multfactor=exist/(vol+exist)
//
void doMix(double length) {
  int32_t mult=wavout->MAXVAL;
  int32_t beforemax=0;
  int32_t newmax=0;
  int32_t value=0;
  double newmaxPct;
  double beforemaxPct;
  uint32_t startX=masterTime*SR;
  double endTime=masterTime+length;
  uint32_t endX=endTime*SR;
  double AM=settings.automix;

  printf("doMix exammining %d to %d   ... %d\n",startX,endX,mult);

  // first, sample the current audio

  for (uint32_t x=startX; x<endX; x++) {
    if (settings.left) {
      value=wavout->getValueL(x, false);
      if (abs(value)>beforemax) {
        beforemax=abs(value);
        printf("L beforemax: %d %d\n",x,beforemax);
      }
    }
    if (settings.right) {
      value=wavout->getValueR(x,false);
      if (abs(value)>beforemax) {
        beforemax=abs(value);
        printf("R beforemax: %d %d\n",x,beforemax);
      }
    }
  }

  beforemaxPct=beforemax/double(mult);
  
  //  next, sample the new audio
  
  for (uint32_t x=startX; x<endX; x++) {
    if (settings.left) {
      value=wavout->getValueL(x, true);
      if (abs(value)>newmax) {
        newmax=abs(value);
        printf("L newmax: %d %d\n",x,newmax);
      }
    }
    if (settings.right) {
      value=wavout->getValueR(x,true);
      if (abs(value)>newmax) {
        newmax=abs(value);
        printf("R newmax: %d %d\n",x,newmax);
      }
    }
  }
  
  newmaxPct=newmax/double(mult);

  printf("beforemax: %f  newmax: %f\n",beforemaxPct,newmaxPct);

  // so working out the 2 new ratios for mixing...
  
  double mixNew=AM*newmaxPct/(newmaxPct+beforemaxPct);
  double mixBefore=beforemaxPct/(newmaxPct+beforemaxPct);

  printf("new/old mix ratio: %f %f\n",mixNew,mixBefore);

  // do the mix, finally!

  for (uint32_t x=startX; x<endX; x++) {
    double value1;
    double value2;
    double value3;
    if (settings.left) {
      value1=mixNew*wavout->getValueL(x, true)/mult;
      value2=mixBefore*wavout->getValueL(x, false)/mult;
      value3=value1+value2;
      int32_t waveval=value3*mult;
      wavout->setValueL(x,waveval,false);
    }
    if (settings.right) {
      value1=mixNew*wavout->getValueR(x, true)/mult;
      value2=mixBefore*wavout->getValueR(x, false)/mult;
      value3=value1+value2;
      int32_t waveval=value3*mult;
      wavout->setValueR(x,waveval,false);
    }
  }
}




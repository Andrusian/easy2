extern "C" {
  #include <sys/types.h>
}

#include <iostream>
#include <map>
#include <queue>
#include <vector>
#include <stack>
#include <cmath>
#include <cassert>

#ifndef EASY_HPP
#define EASY_HPP 1

extern double masterTime;
extern uint32_t soundLengthX;
extern uint32_t SR;

//----------------------------------------------------------------------
//
// This class is produces a sequence of numbers that drive
// value that vary with audio time (provided by caller
// as the x index within the signal, as sample count.

class NumberDriver {         // virtual class
public:
  virtual NumberDriver& operator=(const NumberDriver& right) {
    return *this;
  }
  virtual double getValue(uint32_t x)=0;
  virtual void init(long)=0;
  virtual ~NumberDriver();
    
  // virtual double getValue();
};

//------------------------------

class Value: public NumberDriver {  // derived class for a constant
  public:
  double value;

  Value(double v) {
    value=v;
  }
  void setValue(double value) {
    this->value=value;
  }
  virtual Value& operator=(const Value& right) {
    value=right.value;
    return *this;
  }
  double getValue(uint32_t x) {
    return value;
  }
  void init(long) {

  }
};

//------------------------------


class Ramp: public NumberDriver {  // derived class for a ramp
  uint32_t reftime;   // base time for calculation in samples
  int32_t length;     // also in samples
  double targetValue;
  double startValue;
  
  public:
  Ramp(double start,double target,int32_t len) {
    reftime=0;   // this is x index: start of sound
    startValue=start;
    targetValue=target;
    length=len;

    // printf("RAMP: start %f target %f len %d\n",start,target,len);
  }
  void setValue(double value) {
    this->startValue=value;
  }
  virtual Ramp& operator=(const Ramp& right) {
    startValue=right.startValue;
    targetValue=right.targetValue;
    targetValue=right.startValue;
    return *this;
  }

  // calculate the current value on current time
    
  double getValue(uint32_t x) {
    uint32_t rem;
    double value;

    if (length==-1) {   // is length unknown?
      length=soundLengthX;
    }
    
    rem=(x-reftime)%length;  // using remainder implements repeating
    value=(targetValue-startValue)*((float)rem/length)+startValue;
    return value;
  }
  
  void init(long) {

  }

};

//------------------------------

class Osc: public NumberDriver {  // derived class for a ramp
public:
  double minValue;
  double maxValue;
  double midValue;
  double amplitude;
  double value;
  int form;
  uint32_t refX;
  double freq;
  NumberDriver * freqDriver;
  NumberDriver * phaseDriver;
  double lastfreq;
  double lastval;
  uint32_t countX;
  uint32_t periodX;
  int tridirection;
  double sawval;
  double trislope;
  double sawslope;
  double oscval;
  
  // OSCs and SOUNDs share two settings:
  // FREQ and FORM
  //
  // This is done for simplicity of syntax
  // but in some layered situations it
  // might be confusing.
  //
  // As each are used, the local setting
  // is 'consumed' and the setting
  // reverts to the default.
  //
  // Each number generator needs the minimum
  // needs passed in when it is created. And
  // this is the job of the creating function.
  
  Osc(double minV,double maxV,NumberDriver *fd,int inform, NumberDriver *phase) {

    if (minV>maxV) {
      minValue=maxV;
      maxValue=minV;
    }
    else {
      minValue=minV;
      maxValue=maxV;
    }
    midValue=(minValue+maxValue)/2.;
    amplitude=maxValue-midValue;


    freqDriver=fd;
    phaseDriver=phase;

    freqDriver->init(0);
    phaseDriver->init(0);

    form=inform;
    refX=0;
    
    // printf("osc: %f to %f form %d freq: %f phase:%f\n", minValue,maxValue,inform,freqDriver->getValue(0),phase->getValue(0));
  }
  void setValue(double value) {

  }
  virtual Osc& operator=(const Osc& right) {
    minValue=right.minValue;
    maxValue=right.maxValue;
    midValue=right.midValue;
    return *this;
  }

  // calculate the current value on current time
    
  double getValue(uint32_t x) {
    countX++;
    if (countX>periodX) {
      countX=0;
    }

    double phaseV=phaseDriver->getValue(x);
    int32_t phaseX=phaseV*periodX;
    
    // OSC supports SINE, SQUARE, TRI, and SAW
    // secondary functionality like duty isn't supported
    // code will be kept as compact as possible

    if (form==WF_SINE) {               // SINE
      oscval=sin(2.*M_PI*freq*( ((double)(x)-double(refX)+phaseX) / SR));
    }
    else if (form==WF_SQUARE) {               // SQUARE
      if (countX>periodX) oscval=0;
      else if (countX>(periodX/2)) oscval=1;
      else oscval=-1;
    }
    else if (form==WF_TRI) {            // TRI
      if (countX==0)  {
        tridirection=1;
      }  
      else if (countX==periodX/4)  {
        tridirection=-1;
        sawval=1.0;
      }
      else if (countX==periodX*3/4)  {
        tridirection=1;
        sawval=-1.0;
      }
      else if (countX==periodX) {
        sawval=0.0;
        tridirection=1;
      }
      else if (countX>periodX) {
        sawval=0.0;
        tridirection=0;
      }

      if (tridirection>0) sawval+=trislope;
      else if (tridirection<0) sawval-=trislope;
      oscval=sawval;
    }
    
    else if (form==WF_SAW) {            // SAW
      if (countX>periodX) sawval=0;
      else if (countX==periodX/2) sawval=-1.0;
      else sawval+=sawslope;
      if (sawval>1.0) sawval=-1.;
      oscval=sawval;
    }
    
    // do we need to update frequency?
    // just like in SOUND, we should only
    // update freq at completion of cycles

    if ((lastval<0.0) && (oscval>=0.0)) {         // end of sign wave cycle?
      freq=freqDriver->getValue(x);
      // printf ("osc: new  frequency %d %f\n",x,freq);

      if (freq!=lastfreq) {
        lastfreq=freq;
        refX=x;
        periodX=int(SR/freq); 
        sawslope=2./periodX;
        trislope=4./periodX;
        countX=0;
      }
    }
    else {
      // printf ("osc: not zero crossing %d %f %f\n",x,lastval,oscval);
    }
    lastval=oscval;

    value=oscval*amplitude+midValue;

    //if (form==WF_SQUARE) {               // SQUARE
    //  printf ("osc: x %d val %f =oscval %f x %f amplitude %f midValue \n",countX,value,oscval,amplitude,midValue);
    //}
    
    return value;
  }

  void init(long) {
    freqDriver->init(0);
    phaseDriver->init(0);
    freq=freqDriver->getValue(0);

    lastfreq=freq;
    refX=0;
    countX=0;
    periodX=int(SR/freq); 
    sawslope=2./periodX;
    trislope=4./periodX;
  }

};

//------------------------------

class Seq: public NumberDriver {    // sequencer
public:
  double value;
  int step=0;
  std::vector<double> entV;
  std::vector<uint32_t> entT;
  double curval;
  uint32_t curtime;
  uint32_t countX=0;
  uint32_t lastX=0;
  
  Seq() {
    step=0;
    countX=0;
  }

  void load(std::vector<double> values, std::vector<uint32_t> times) {
    entV=values;
    entT=times;
    curval=entV[0];
    curtime=entT[0];
  }

  double getValue(uint32_t x) {
    // countX++;    <- this is no good. We don't call getValue every sample

    uint32_t addX=x-lastX;  // difference... typically 1 period
    countX+=addX;
    lastX=x;
        
    if (countX>curtime) {          // time for next value??
      countX=0;
      step++;
      // printf("seq entries: %d\n",entV.size());
      if (step<=entV.size()) {   // is there another value?
        
        if (curtime==0) {          // loop around if last time is zero
          // loop when a zero time value seen

          step=0;
        }

        curval=entV[step];
        curtime=entT[step];
        // printf("seq entry: %d %f %d\n",step,curval,curtime);
      }
      else {
        // do nothing: we're done here
      }
    }
    return curval;
  }
    
  void setValue(double value) {
  }
  
  virtual Seq& operator=(const Seq& right) {
    value=right.value;
    return *this;
  }

  void init(long) {
    
  }

};

//------------------------------

class Ramps: public NumberDriver {    // ramps
public:
  int step=0;
  std::vector<double> entV;
  std::vector<uint32_t> entT;
  uint32_t tarTimeX;
  uint32_t countX=0;
  uint32_t lastX=0;

  double curval;
  double lastval;
  
  Ramps() {
    step=0;
    countX=0;
  }

  void load(std::vector<double> values, std::vector<uint32_t> times) {
    entV=values;
    entT=times;
    curval=entV[0];
    tarTimeX=entT[0];
  }

  // RAMPS is most similar to the SHAPE function
  // a different and simpler method is used to calculate the
  // slope of change of value
  
  double getValue(uint32_t x) {

    // difference... typically 1 period but if used on
    // a freq it will jump by the period of the waveform
    
    uint32_t addX=x-lastX;  
    countX+=addX;
    lastX=x;
        
    if (countX>=tarTimeX) {          // time for next value??
      step++;
      if (step==entV.size()) {      // are we beyond end of list?
        // always loop
        
        step=0;
        countX=0;                  // reset master counter;
        tarTimeX=entT[0];       // and set next target time
        //printf("ramps back to zero %d %d %f %d\n",x,countX,entV[step],entT[step]);
      }
      else {
        // do nothing
      }
      tarTimeX=entT[step];
      //printf("%d %d ramps next target %f @ %d\n",x,countX,entV[step],entT[step]);
    }

    if ((entT[step]-countX)==0) {  // protect against divide by zero
      curval=entT[step];
    }
    else {

      // move toward the target value by a small increment
      
      
      curval=(entV[step]-lastval)/(entT[step]-countX)+lastval;
    }
    //printf("step %d ramps %d  %d - target %f  %d curval %f\n",step,x,countX,entV[step],entT[step],curval);
    lastval=curval;
    
    return curval;
  }
    
  void setValue(double value) {
    curval=value;
  }
  
  virtual Ramps& operator=(const Ramps& right) {
    curval=right.curval;
    entV=right.entV;
    entT=right.entT;
    return *this;
  }

  void init(long) {
    lastval=entV[0];
    tarTimeX=entT[0];
  }

};



//------------------------------

class RandSeq: public NumberDriver {    // random sequence number
public:
  double value;
  double minValue;
  double maxValue;
  double countdown;
  double amplitude;
  uint32_t updateX;
  double updatePeriod;

  RandSeq(double minV, double maxV, double update) {
    if (minV>maxV) {
      minValue=maxV;
      maxValue=minV;
    }
    else {
      minValue=minV;
      maxValue=maxV;
    }
    amplitude=maxValue-minValue;
    updatePeriod=update;
    updateX=int(update*SR);

    //printf("new random sequence: %f %f %f\n",minV,maxV,update);

    value=getValue(0);  // get an initial value
  }
    
  void setValue(double value) {
  }
  
  virtual RandSeq& operator=(const RandSeq& right) {
    value=right.value;
    return *this;
  }
  
  double getValue(uint32_t x) {
    if (x>updateX) {
      updateX+=int(updatePeriod*SR);
      double randvalue=((float) rand()/(float) (RAND_MAX));
      value=randvalue*amplitude+minValue;               // update value
      //printf("new random value: %d %f\n",x,value);
    }
    else if (x==0) {
      double randvalue=((float) rand()/(float) (RAND_MAX));
      value=randvalue*amplitude+minValue;               // update value
    }
    
    return value;
  }

  void init(long) { }


};

//------------------------------
// Shape is a special number driver that is used with volume.
// It probably won't be applied to other things. Probably.
//
// Shapes are pre-defined, currently in opcode range of 2000-2017.
//
// Volumes are default 0-1 in the predefines but are also
// scaled the volume setting.
//
// Shape is a lot like Seq except it automatically ramps
// between values. [Todo If this works well, it might be
// better to make seq work this way.]
//

class Shape: public NumberDriver {    // sequencer
public:
  int preset;
  long length=-1;
  int step=0;
  int steps;
  std::vector<double> entV;
  std::vector<double> entT;
  std::vector<double> entTX;
  double curval;
  uint32_t curtime;
  uint32_t countX=0;   // counter for whole shape length
  uint32_t stepX=0;    // counter within one step
  uint32_t lenStepX;
  uint32_t ta;
  uint32_t tb;
  double va;
  double vb;
  uint32_t lenX;
  
  Shape() {
    step=1;
    countX=0;
    stepX=0;
  }

  Shape(int dtype, double len) {
    preset=dtype;
    length=len;   // if -1, this scales to size of sound - worked out at first getValue call
  }

  void loadTable(void);   // it's over in easy_code.cpp because tables are there
  

  void init(long len) {
    //printf("init: length is %d was %d\n",len,length);
    
    if (length==-1) {   // is length unknown? must do this first at start of sound generation
      length=len;
      lenX=len;
    }
    //printf("init: length is %d\n",length);
    
    loadTable();
  }

  // by the time we get to getValue, everything must be worked out and ready to calculate
  
  double getValue(uint32_t x) {
               // X is counter within whole sound
    countX++;  // countX is counter for whole shape
    stepX++;   // stepX is counter for this step

    // its a slope calculation of value between points
    
    curval=vb+(va-vb)*stepX/double(lenStepX);

//    printf("DEBUG shape: vb %f va %f  ratio: %f  stepX: %d  bottom: %d\n",vb,va,stepX/(double(lenStepX)),stepX,lenStepX);
    

    // printf("%d %d %d %d curval %f = vb %f + (va %f -vb %f )* x %d / (%d)\n",x,countX,stepX,lenStepX,curval,vb,va,vb, stepX,lenStepX);


    if (countX>=lenX) {
      step=1;
      countX=0;     // reset both counters
      stepX=0;

      // restart back at beginning of steps
      
      ta=entTX[step];      // time after (at end of step)
      tb=entTX[step-1];    // time before (at end of step)
      lenStepX=ta-tb;
    }

    // have we reached the end of this step?
    
    else if (stepX>lenStepX) {
      step++;
      stepX=0;   // reset counter

      if (step<entTX.size()) {

        // simply go to next step
        
        ta=entTX[step];      // time after (at end of step)
        tb=entTX[step-1];    // time before (at end of step)
        lenStepX=ta-tb;
        va=entV[step];      // value target (after step)
        vb=entV[step-1];    // value before (before step)
      }
    }
    else {
      // otherwise just freeze output value
    }
    return curval;
  }
    
  void setValue(double value) {
  }
  
  virtual Shape& operator=(const Shape& right) {
    length=right.length;
    return *this;
  }
  
};

//------------------------------------------------------------------------
// the setting structure contains the current or default
// settings that affect sound generation.
//
// current settings are reset as they are used
// default settings are persistent between sounds/oscillators

struct settings_struct {
  NumberDriver *freq;
  NumberDriver *freq2;
  NumberDriver *freq3;
  NumberDriver *vol;
  NumberDriver *vol2;
  NumberDriver *vol3;
  NumberDriver *bal;
  NumberDriver *phase;
  NumberDriver *duty;
  NumberDriver *shape;
  bool left;
  bool right;
  float automix;
  int form;
  float fadein;
  float fadeout;
  bool circuit;                // whether to enable circuit effects
  NumberDriver *cirp;          // proportional term for circuit model
  NumberDriver *ciri;          // integral term for circuit model
  
};

struct settings_struct_stacked {

  // these three settings overlap and need to be in a stack  

  std::stack<NumberDriver *> freqStack;
  std::stack<NumberDriver *> phaseStack;
  std::stack<int> formStack;

  // the rest of these settings merely fall back to defaults
  
  NumberDriver *freq2;
  NumberDriver *freq3;
  NumberDriver *vol;
  NumberDriver *vol2;
  NumberDriver *vol3;
  NumberDriver *bal;
  NumberDriver *duty;
  NumberDriver *shape;
  bool left;
  bool right;
  float automix;
  float fadein;
  float fadeout;
  bool circuit;                // whether to enable circuit effects
  NumberDriver *cirp;          // proportional term for circuit model
  NumberDriver *ciri;          // integral term for circuit model
  
};



#endif

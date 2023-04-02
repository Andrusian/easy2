#include "easy_code.hpp"

const char * debug_type (int dtype) {

  switch (dtype) {

    case WF_SINE:
      return "sine";
    case WF_SQUARE:
      return "square";
    case WF_SAW:
      return "saw";
    case WF_TRI:
      return "tri";
    case WF_TENS:
      return "tens";

    case PLUS:
      return " + ";
    case MINUS:
      return " - ";
    case MULT:
      return " * ";
    case DIV:
      return " \\ ";
    case MODULUS:
      return " %% ";
    case STRING:
      return " string ";
    case NUMBER:
      return " number ";
    case ASSIGNMENT:
      return " = ";
    case FILENAME:
      return " filename ";
    case AUTOMIX:
      return "automix";
    case OUTPUT:
      return "output";
    case FADEIN:
      return "fadein";
    case FADEOUT:
      return "fadeout";
    case FORM:
      return "form";
    case RIGHT:
      return "right";
    case LEFT:
      return "left";
    case FREQ:
      return "freq";
    case PHASE:
      return "phase";
    case VOL:
      return "vol";
    case BAL:
      return "bal";
    case DUTY:
      return "duty";
    case RANDOM:
      return "random";
    case RAMP:
      return "ramp";
    case SHAPE:
      return "shape";
    case OSC:
      return "osc";
    case SOUND:
      return "sound";
    case MIX:
      return "mix";
    case SILENCE:
      return "silence";
    case TIME:
      return "time";
    case REWIND:
      return "rewind";
    case AMOD:
      return "amod";
    case BOOST:
      return "boost";
    case REPEAT:
      return "repeat";
    case BOTH:
      return "both";
    case RANDSEQ:
      return "randseq";
    case NOOP:
      return " NOOP ";
    case ADDTIME:
      return "addtime";
    case SEQ:
      return "seq";
    case MANUALMIX:
      return "manualmix";
    case LOOP:
      return "loop";
    case REVERB:
      return "reverb";
    case CIRCUIT:
      return "circuit";
    case NOCIRCUIT:
      return "nocircuit";
    case CIRP:
      return "cirp";
    case CIRI:
      return "ciri";
    case TO:
      return "to";
    case COMMA:
      return "COMMA";

    case SH_TEASE1:
      return "tease1";
    case SH_TEASE2:
      return "tease2";
    case SH_TEASE3:
      return "tease3";
    case SH_PULSE1:
      return "pulse";
    case SH_PULSE2:
      return "pulse2";
    case SH_PULSE3:
      return "pulse3";
    case SH_KICK1:
      return "kick1";
    case SH_KICK2:
      return "kick2";
    case SH_KICK3:
      return "kick3";
    case SH_NOTCH1:
      return "notch1";
    case SH_NOTCH2:
      return "notch2";
    case SH_NOTCH3:
      return "notch3";
    case SH_ADSR1:
      return "adsr1";
    case SH_ADSR2:
      return "adsr2";
    case SH_ADSR3:
      return "adsr3";
    case SH_REV1:
      return "rev1";
    case SH_REV2:
      return "rev2";
    case SH_REV3:
      return "rev3";
  }
return "Unknown";
}
